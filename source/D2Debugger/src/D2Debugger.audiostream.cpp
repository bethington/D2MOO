// D2Debugger.audiostream.cpp -- ship the game's audio to a remote client as a
// single lossless stream.
//
// WHAT THIS IS. The second consumer of the mixer's ring (the first is local
// WASAPI playback in D2Debugger.audiocap.cpp). It reads the one interleaved
// 44.1k/16-bit stereo mix, encodes it as FLAC, and broadcasts the encoded bytes
// over WebSocket. That is the audio leg of remote play, alongside the frame
// capture and the virtual-input layer.
//
// WHY THE RING MATTERS. The producer publishes into the ring on its own clock
// and touches no audio device, so this path works identically on a machine with
// no endpoint at all -- which is the whole point of the headless container
// story (see subprojects/dsound-headless). Local playback stuttering, being
// muted, or having no device to render to cannot affect what a remote listener
// receives.
//
// WHY LOSSLESS. Raw PCM here is ~1.4 Mbit/s, which is noise next to the video
// stream remote play also needs, so compression buys bandwidth rather than
// feasibility. FLAC halves it at zero quality cost and its frames are
// independently decodable, which is what makes the mid-stream join below work.
//
// ONE ENCODER, MANY CLIENTS. Encoding per-client would multiply CPU inside the
// GAME's process for identical output. Instead there is a single encoder whose
// STREAMINFO header is cached; a client joining mid-stream is sent that header
// and then the live frame broadcast. FLAC audio frames carry their own sample
// numbers and are decodable against STREAMINFO alone, so the join is clean.
//
// IDLE COSTS NOTHING. With no client connected the encoder is not fed and the
// ring is not read. This runs inside a game; a streaming feature nobody is
// using must not cost frames.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "FLAC/stream_encoder.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")

extern "C" void D2AudioCap_StreamFormat(int* rate, int* channels);
extern "C" int  D2AudioCap_ReadRing(unsigned long long* cursor, short* out,
                                    int maxFrames, int backlogFrames,
                                    unsigned long* dropped);

namespace
{
	constexpr int  kPort = 8791;
	constexpr int  kMaxClients = 4;
	// 1024 samples = 23 ms per FLAC frame. Small blocks cost a little ratio and
	// buy latency, which is the right trade for interactive remote play; the
	// encoder still has a whole frame of lookahead either way.
	constexpr int  kBlockSize = 1024;
	// How far behind live a fresh stream starts. Enough to absorb producer
	// jitter without adding audible lag on top of the network's own.
	constexpr int  kStartBacklog = 2205;          // 50 ms
	constexpr int  kReadChunk = 4096;             // frames per ring read

	SOCKET g_listen = INVALID_SOCKET;
	std::mutex g_mx;                              // guards clients + header
	SOCKET g_clients[kMaxClients];
	int    g_clientCount = 0;
	std::vector<unsigned char> g_header;          // STREAMINFO etc, replayed on join
	bool   g_headerDone = false;

	FLAC__StreamEncoder* g_enc = nullptr;
	std::atomic<bool> g_run{ false };
	std::atomic<bool> g_enabled{ true };
	HANDLE g_thread = nullptr;
	HANDLE g_acceptThread = nullptr;

	std::atomic<unsigned long long> g_framesEncoded{ 0 };
	std::atomic<unsigned long long> g_bytesSent{ 0 };
	std::atomic<unsigned long> g_dropped{ 0 };
	std::atomic<unsigned long> g_joins{ 0 };
	std::atomic<int> g_encoderOk{ 0 };

	void Log(const char* m)
	{
		char b[256];
		_snprintf_s(b, sizeof(b), _TRUNCATE, "[audiostream] %s\n", m);
		OutputDebugStringA(b);
	}

	// ---- WebSocket ------------------------------------------------------

	// RFC 6455: the accept key is base64(SHA1(client-key + magic GUID)). Both
	// primitives come from Windows -- bcrypt for SHA-1, crypt32 for base64 --
	// rather than vendoring a hash for eleven bytes of protocol.
	std::string AcceptKey(const std::string& clientKey)
	{
		static const char kMagic[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
		const std::string in = clientKey + kMagic;

		BCRYPT_ALG_HANDLE alg = nullptr;
		BCRYPT_HASH_HANDLE hash = nullptr;
		unsigned char digest[20] = {};
		std::string out;

		if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, nullptr, 0) == 0)
		{
			if (BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) == 0)
			{
				BCryptHashData(hash, (PUCHAR)in.data(), (ULONG)in.size(), 0);
				BCryptFinishHash(hash, digest, sizeof(digest), 0);
				BCryptDestroyHash(hash);
			}
			BCryptCloseAlgorithmProvider(alg, 0);
		}

		DWORD n = 0;
		if (CryptBinaryToStringA(digest, sizeof(digest),
		                         CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &n) && n)
		{
			out.resize(n);
			if (CryptBinaryToStringA(digest, sizeof(digest),
			                         CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &out[0], &n))
				out.resize(strlen(out.c_str()));
			else
				out.clear();
		}
		return out;
	}

	bool SendAll(SOCKET s, const unsigned char* p, size_t n)
	{
		while (n)
		{
			const int sent = send(s, (const char*)p, (int)n, 0);
			if (sent <= 0)
				return false;
			p += sent;
			n -= (size_t)sent;
		}
		return true;
	}

	// A single unfragmented BINARY frame. Server-to-client frames are never
	// masked (RFC 6455 5.1), which also keeps this loop free of per-byte work.
	bool SendFrame(SOCKET s, const unsigned char* payload, size_t len)
	{
		unsigned char hdr[10];
		size_t h = 0;
		hdr[h++] = 0x82;                          // FIN + opcode 2 (binary)
		if (len < 126)
		{
			hdr[h++] = (unsigned char)len;
		}
		else if (len <= 0xFFFF)
		{
			hdr[h++] = 126;
			hdr[h++] = (unsigned char)((len >> 8) & 0xFF);
			hdr[h++] = (unsigned char)(len & 0xFF);
		}
		else
		{
			hdr[h++] = 127;
			for (int i = 7; i >= 0; --i)
				hdr[h++] = (unsigned char)((((unsigned long long)len) >> (i * 8)) & 0xFF);
		}
		if (!SendAll(s, hdr, h))
			return false;
		return SendAll(s, payload, len);
	}

	void DropClient(SOCKET s)
	{
		for (int i = 0; i < g_clientCount; ++i)
		{
			if (g_clients[i] != s)
				continue;
			closesocket(s);
			g_clients[i] = g_clients[g_clientCount - 1];
			--g_clientCount;
			return;
		}
	}

	void Broadcast(const unsigned char* p, size_t n)
	{
		std::lock_guard<std::mutex> lk(g_mx);
		for (int i = g_clientCount - 1; i >= 0; --i)
		{
			if (!SendFrame(g_clients[i], p, n))
			{
				closesocket(g_clients[i]);
				g_clients[i] = g_clients[g_clientCount - 1];
				--g_clientCount;
				continue;
			}
		}
		if (g_clientCount)
			g_bytesSent.fetch_add(n, std::memory_order_relaxed);
	}

	// ---- FLAC -----------------------------------------------------------

	// `samples == 0` is metadata (the STREAMINFO block and friends emitted at
	// init, plus the rewrite at finish); anything else is an audio frame. That
	// distinction is exactly what lets a late joiner be caught up with a replay
	// of the header and nothing else.
	FLAC__StreamEncoderWriteStatus WriteCb(const FLAC__StreamEncoder*,
	                                       const FLAC__byte buffer[], size_t bytes,
	                                       uint32_t /*samples*/, uint32_t /*current_frame*/,
	                                       void* /*client_data*/)
	{
		{
			// Scoped: Broadcast takes the same (non-recursive) mutex.
			std::lock_guard<std::mutex> lk(g_mx);
			if (!g_headerDone)
				g_header.insert(g_header.end(), buffer, buffer + bytes);
		}
		// Header and audio alike go out live -- the cached copy above exists
		// only to catch up clients who join later.
		Broadcast(buffer, bytes);
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	}

	bool StartEncoder(int rate, int channels)
	{
		g_enc = FLAC__stream_encoder_new();
		if (!g_enc)
			return false;
		FLAC__stream_encoder_set_verify(g_enc, false);
		FLAC__stream_encoder_set_compression_level(g_enc, 5);
		FLAC__stream_encoder_set_channels(g_enc, (uint32_t)channels);
		FLAC__stream_encoder_set_bits_per_sample(g_enc, 16);
		FLAC__stream_encoder_set_sample_rate(g_enc, (uint32_t)rate);
		FLAC__stream_encoder_set_blocksize(g_enc, kBlockSize);
		// A live stream has no known length and nobody can seek it.
		FLAC__stream_encoder_set_total_samples_estimate(g_enc, 0);

		const FLAC__StreamEncoderInitStatus st =
			FLAC__stream_encoder_init_stream(g_enc, WriteCb, nullptr, nullptr, nullptr, nullptr);
		if (st != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
		{
			FLAC__stream_encoder_delete(g_enc);
			g_enc = nullptr;
			return false;
		}
		{
			std::lock_guard<std::mutex> lk(g_mx);
			g_headerDone = true;               // everything written from here is audio
		}
		g_encoderOk.store(1, std::memory_order_relaxed);
		return true;
	}

	void StopEncoder()
	{
		if (!g_enc)
			return;
		FLAC__stream_encoder_finish(g_enc);
		FLAC__stream_encoder_delete(g_enc);
		g_enc = nullptr;
		g_encoderOk.store(0, std::memory_order_relaxed);
		std::lock_guard<std::mutex> lk(g_mx);
		g_header.clear();
		g_headerDone = false;
	}

	// ---- threads --------------------------------------------------------

	DWORD WINAPI AcceptThread(LPVOID)
	{
		while (g_run.load(std::memory_order_relaxed))
		{
			sockaddr_in addr{};
			int len = sizeof(addr);
			const SOCKET c = accept(g_listen, (sockaddr*)&addr, &len);
			if (c == INVALID_SOCKET)
			{
				if (!g_run.load(std::memory_order_relaxed))
					break;
				Sleep(50);
				continue;
			}

			// Read the handshake. One recv is enough in practice for a browser's
			// opening request; a short deadline stops a silent peer pinning a
			// thread inside the game process.
			DWORD tv = 3000;
			setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
			char req[2048];
			const int got = recv(c, req, sizeof(req) - 1, 0);
			if (got <= 0) { closesocket(c); continue; }
			req[got] = 0;

			// Sec-WebSocket-Key, case-insensitively.
			std::string reqs(req), key;
			{
				std::string low = reqs;
				for (char& ch : low) ch = (char)tolower((unsigned char)ch);
				const size_t k = low.find("sec-websocket-key:");
				if (k != std::string::npos)
				{
					size_t b = reqs.find_first_not_of(" \t", k + 18);
					const size_t e = reqs.find_first_of("\r\n", b);
					if (b != std::string::npos && e != std::string::npos)
						key = reqs.substr(b, e - b);
				}
			}
			if (key.empty())
			{
				static const char kBad[] =
					"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
				send(c, kBad, (int)strlen(kBad), 0);
				closesocket(c);
				continue;
			}

			// NOT named `accept` -- that is the Winsock call used above.
			const std::string acceptKey = AcceptKey(key);
			char resp[256];
			const int n = _snprintf_s(resp, sizeof(resp), _TRUNCATE,
				"HTTP/1.1 101 Switching Protocols\r\n"
				"Upgrade: websocket\r\nConnection: Upgrade\r\n"
				"Sec-WebSocket-Accept: %s\r\n\r\n", acceptKey.c_str());
			if (send(c, resp, n, 0) != n) { closesocket(c); continue; }

			// Blocking sends from here; the broadcast drops a client that wedges.
			tv = 5000;
			setsockopt(c, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

			std::lock_guard<std::mutex> lk(g_mx);
			if (g_clientCount >= kMaxClients) { closesocket(c); continue; }
			// Catch the newcomer up with the stream header before it sees any
			// audio frame, or its decoder has no STREAMINFO to decode against.
			if (!g_header.empty() && !SendFrame(c, g_header.data(), g_header.size()))
			{
				closesocket(c);
				continue;
			}
			g_clients[g_clientCount++] = c;
			g_joins.fetch_add(1, std::memory_order_relaxed);
			Log("client joined");
		}
		return 0;
	}

	DWORD WINAPI StreamThread(LPVOID)
	{
		int rate = 44100, ch = 2;
		D2AudioCap_StreamFormat(&rate, &ch);

		std::vector<short> pcm((size_t)kReadChunk * 2);
		std::vector<FLAC__int32> conv((size_t)kReadChunk * 2);
		unsigned long long cursor = 0;

		while (g_run.load(std::memory_order_relaxed))
		{
			int clients = 0;
			{
				std::lock_guard<std::mutex> lk(g_mx);
				clients = g_clientCount;
			}
			// Nobody listening: tear the encoder down so the next joiner gets a
			// fresh stream starting at sample 0, and stop reading the ring
			// entirely. A feature nobody is using must not cost the game frames.
			if (!clients || !g_enabled.load(std::memory_order_relaxed))
			{
				if (g_enc)
					StopEncoder();
				cursor = 0;
				Sleep(100);
				continue;
			}
			if (!g_enc && !StartEncoder(rate, ch))
			{
				Log("encoder init failed");
				Sleep(1000);
				continue;
			}

			unsigned long dropped = 0;
			const int frames = D2AudioCap_ReadRing(&cursor, pcm.data(), kReadChunk,
			                                       kStartBacklog, &dropped);
			if (dropped)
				g_dropped.fetch_add(dropped, std::memory_order_relaxed);
			if (frames <= 0)
			{
				Sleep(5);                      // caught up with the producer
				continue;
			}
			// libFLAC takes one int32 per sample regardless of bit depth.
			for (int i = 0; i < frames * 2; ++i)
				conv[i] = (FLAC__int32)pcm[i];
			if (!FLAC__stream_encoder_process_interleaved(g_enc, conv.data(), (uint32_t)frames))
			{
				Log("encoder process failed; restarting stream");
				StopEncoder();
				continue;
			}
			g_framesEncoded.fetch_add((unsigned long long)frames, std::memory_order_relaxed);
		}

		StopEncoder();
		return 0;
	}
}

// ---------------------------------------------------------------- public ----

extern "C" void D2AudioStream_Start()
{
	static bool done = false;
	if (done)
		return;
	done = true;

	WSADATA wsa{};
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		Log("WSAStartup failed");
		return;
	}
	g_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_listen == INVALID_SOCKET)
	{
		Log("socket failed");
		return;
	}
	BOOL yes = TRUE;
	setsockopt(g_listen, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(kPort);
	// LOOPBACK ONLY. This ships the game's audio; it is not going on the LAN
	// until there is an authentication story. Tunnel it deliberately.
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	if (bind(g_listen, (sockaddr*)&addr, sizeof(addr)) != 0 ||
	    listen(g_listen, 4) != 0)
	{
		Log("bind/listen failed");
		closesocket(g_listen);
		g_listen = INVALID_SOCKET;
		return;
	}

	g_run.store(true, std::memory_order_relaxed);
	g_acceptThread = CreateThread(nullptr, 0, AcceptThread, nullptr, 0, nullptr);
	g_thread = CreateThread(nullptr, 0, StreamThread, nullptr, 0, nullptr);
	Log("listening on 127.0.0.1:8791");
}

extern "C" void D2AudioStream_SetEnabled(int on)
{
	g_enabled.store(on != 0, std::memory_order_relaxed);
}

extern "C" void D2AudioStream_Stats(int* port, int* clients, int* enabled,
                                    int* encoderOk, unsigned long* framesEncoded,
                                    unsigned long* bytesSent, unsigned long* dropped,
                                    unsigned long* joins)
{
	if (port)      *port = kPort;
	if (enabled)   *enabled = g_enabled.load(std::memory_order_relaxed) ? 1 : 0;
	if (encoderOk) *encoderOk = g_encoderOk.load(std::memory_order_relaxed);
	if (framesEncoded) *framesEncoded = (unsigned long)g_framesEncoded.load(std::memory_order_relaxed);
	if (bytesSent) *bytesSent = (unsigned long)g_bytesSent.load(std::memory_order_relaxed);
	if (dropped)   *dropped = g_dropped.load(std::memory_order_relaxed);
	if (joins)     *joins = g_joins.load(std::memory_order_relaxed);
	if (clients)
	{
		std::lock_guard<std::mutex> lk(g_mx);
		*clients = g_clientCount;
	}
}
