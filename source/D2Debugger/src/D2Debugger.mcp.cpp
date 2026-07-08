// D2Debugger.mcp.cpp -- WS-5 in-process HTTP control server.
//
// A minimal localhost-ONLY HTTP/1.1 server (raw Winsock) that exposes the MCP
// control surface routed by D2Mcp_HandleRequest (D2Debugger.LiveDispatch.cpp).
// This lets an external agent (the d2debugger-mcp bridge, GRADUATED_CONFORMANCE
// _PIPELINE_PLAN.md WS-5) read dispatcher/profiler state and drive shadow-proving
// without a human clicking the ImGui panel -- the keystone for autonomous proving.
//
// SECURITY: binds 127.0.0.1 only (never a routable interface). A non-elevated
// client (curl / the MCP bridge) can still reach this loopback socket even
// though the game process is elevated -- TCP loopback isn't gated by elevation.
//
// winsock2 MUST precede windows.h (else winsock.h v1 gets pulled in and clashes).
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <string>
#include <cstdio>
#include <cctype>
#pragma comment(lib, "ws2_32.lib")

// Router, defined in D2Debugger.LiveDispatch.cpp (same DLL/CRT).
std::string D2Mcp_HandleRequest(const std::string& method, const std::string& path, const std::string& body);

namespace
{
	const unsigned short kPort = 8790; // 127.0.0.1:8790 -- documented in the plan.

	// Read one HTTP request. Fills method/path/body. Returns "" on success or a
	// short error tag. Handles Content-Length bodies; one request per connection.
	const char* ReadRequest(SOCKET s, std::string& method, std::string& path, std::string& body)
	{
		std::string data;
		char buf[4096];
		size_t headerEnd = std::string::npos;
		while ((headerEnd = data.find("\r\n\r\n")) == std::string::npos)
		{
			int r = recv(s, buf, sizeof(buf), 0);
			if (r <= 0) return "recv-fail";
			data.append(buf, r);
			if (data.size() > (1u << 20)) return "headers-too-large";
		}

		size_t sp1 = data.find(' ');
		size_t sp2 = (sp1 == std::string::npos) ? std::string::npos : data.find(' ', sp1 + 1);
		if (sp1 == std::string::npos || sp2 == std::string::npos) return "bad-request-line";
		method = data.substr(0, sp1);
		path   = data.substr(sp1 + 1, sp2 - sp1 - 1);

		// Content-Length (case-insensitive), only within the header region.
		size_t cl = 0;
		{
			std::string lower = data.substr(0, headerEnd);
			for (char& ch : lower) ch = (char)tolower((unsigned char)ch);
			size_t p = lower.find("content-length:");
			if (p != std::string::npos)
				cl = strtoul(data.c_str() + p + 15, nullptr, 10);
		}

		body = data.substr(headerEnd + 4);
		while (body.size() < cl)
		{
			int r = recv(s, buf, sizeof(buf), 0);
			if (r <= 0) break;
			body.append(buf, r);
			if (body.size() > (1u << 20)) return "body-too-large";
		}
		return "";
	}

	void SendResponse(SOCKET s, const std::string& body, int code = 200)
	{
		const char* status = code == 200 ? "200 OK" : (code == 404 ? "404 Not Found" : "400 Bad Request");
		char hdr[320];
		_snprintf_s(hdr, sizeof(hdr), _TRUNCATE,
			"HTTP/1.1 %s\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: %zu\r\n"
			"Connection: close\r\n"
			"Access-Control-Allow-Origin: *\r\n"
			"Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
			"Access-Control-Allow-Headers: Content-Type\r\n\r\n",
			status, body.size());
		send(s, hdr, (int)strlen(hdr), 0);
		if (!body.empty()) send(s, body.c_str(), (int)body.size(), 0);
	}

	// SEH-guarded call into the router: a faulting handler (e.g. a bad
	// direct-call oracle later) must not take down the game. Kept in its own
	// function with NO C++ objects requiring unwinding (C2712) -- it only
	// touches the global g_respTmp via a plain helper.
	std::string g_respTmp;
	void DoHandle(const std::string* m, const std::string* p, const std::string* b)
	{
		g_respTmp = D2Mcp_HandleRequest(*m, *p, *b);
	}
	bool SafeHandle(const std::string* m, const std::string* p, const std::string* b)
	{
		__try { DoHandle(m, p, b); return true; }
		__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	}

	// SEH-guarded ReadRequest: previously the socket-read/parse path ran totally
	// unprotected -- a fault or runaway allocation there would kill this whole
	// per-connection loop, taking the listening socket down PERMANENTLY (no
	// retry, no restart short of relaunching the game). Found the hard way,
	// 2026-07-07: the server stopped accepting connections entirely mid-session
	// with no crash dialog/Application-Error event (ruling out a classic
	// unhandled process-wide fault) -- this was the one unguarded gap in the
	// per-connection path. Same "no C++ objects needing unwinding in the __try
	// function" (C2712) pattern as DoHandle/SafeHandle above: method/path/body
	// live in the CALLER's frame (ServerThread), only pointers cross here.
	const char* g_readErrTmp = nullptr;
	void DoReadRequest(SOCKET cli, std::string* method, std::string* path, std::string* body)
	{
		g_readErrTmp = ReadRequest(cli, *method, *path, *body);
	}
	bool SafeReadRequest(SOCKET cli, std::string* method, std::string* path, std::string* body, const char** errOut)
	{
		__try { DoReadRequest(cli, method, path, body); *errOut = g_readErrTmp; return true; }
		__except (EXCEPTION_EXECUTE_HANDLER) { *errOut = "read-exception"; return false; }
	}

	DWORD WINAPI ServerThread(LPVOID)
	{
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

		SOCKET srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (srv == INVALID_SOCKET) return 1;
		BOOL yes = TRUE;
		setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(kPort);
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); // loopback ONLY
		if (bind(srv, (sockaddr*)&addr, sizeof(addr)) != 0) { closesocket(srv); return 1; }
		if (listen(srv, 16) != 0) { closesocket(srv); return 1; }

		for (;;)
		{
			SOCKET cli = accept(srv, nullptr, nullptr);
			if (cli == INVALID_SOCKET) continue;

			std::string method, path, body;
			const char* e = nullptr;
			if (!SafeReadRequest(cli, &method, &path, &body, &e))
			{
				// A fault during read/parse -- close THIS connection only, keep
				// accepting (this is exactly the gap that used to take the whole
				// listening socket down permanently).
				closesocket(cli);
				continue;
			}
			if (e && e[0])
			{
				SendResponse(cli, std::string("{\"ok\":false,\"error\":\"") + e + "\"}", 400);
				closesocket(cli);
				continue;
			}
			if (method == "OPTIONS") // CORS preflight
			{
				SendResponse(cli, "{}", 200);
				closesocket(cli);
				continue;
			}

			std::string resp = SafeHandle(&method, &path, &body)
				? g_respTmp
				: std::string("{\"ok\":false,\"error\":\"handler-exception\"}");
			SendResponse(cli, resp);
			closesocket(cli);
		}
	}
}

// Start the control server once (idempotent). Called from D2Debugger startup.
void D2Mcp_StartServer()
{
	static bool started = false;
	if (started) return;
	started = true;
	CreateThread(nullptr, 0, ServerThread, nullptr, 0, nullptr);
}
