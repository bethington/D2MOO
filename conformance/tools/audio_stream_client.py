#!/usr/bin/env python3
"""Pull the game's audio off the D2Debugger stream and write it to a .flac file.

The verification tool for the remote-play audio leg. Speaks the WebSocket
handshake and framing with raw sockets rather than a library, deliberately: the
point is to prove OUR server implements RFC 6455 correctly, and a library that
tolerates a sloppy handshake would hide exactly the bug worth finding.

    python audio_stream_client.py --seconds 10 --out C:/tmp/stream.flac

Then decode it to prove real audio survived the round trip:

    uv run --with soundfile python -c "import soundfile,numpy;d,r=soundfile.read('C:/tmp/stream.flac');print(r,d.shape,(d**2).mean()**0.5)"

A non-zero RMS there is the end-to-end result: the game's DirectSound writes
reached the mixer's ring, were encoded losslessly, crossed a socket, and decoded
back into audio.
"""
import argparse
import base64
import os
import socket
import struct
import sys
import time

GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


def handshake(sock, host, port):
    """Send the upgrade request and verify the server's accept key."""
    key = base64.b64encode(os.urandom(16)).decode()
    req = (
        f"GET / HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Upgrade: websocket\r\n"
        f"Connection: Upgrade\r\n"
        f"Sec-WebSocket-Key: {key}\r\n"
        f"Sec-WebSocket-Version: 13\r\n\r\n"
    )
    sock.sendall(req.encode())

    buf = b""
    while b"\r\n\r\n" not in buf:
        chunk = sock.recv(4096)
        if not chunk:
            raise RuntimeError("server closed during handshake")
        buf += chunk
    head, _, rest = buf.partition(b"\r\n\r\n")
    text = head.decode(errors="replace")
    if "101" not in text.split("\r\n")[0]:
        raise RuntimeError(f"expected 101, got: {text.splitlines()[0]}")

    # Verify the accept key rather than trusting the 101 -- a server that
    # upgrades without computing it is broken in a way only this check catches.
    import hashlib
    expect = base64.b64encode(hashlib.sha1((key + GUID).encode()).digest()).decode()
    got = ""
    for line in text.split("\r\n")[1:]:
        if line.lower().startswith("sec-websocket-accept:"):
            got = line.split(":", 1)[1].strip()
    if got != expect:
        raise RuntimeError(f"bad accept key: got {got!r} want {expect!r}")
    return rest


def frames(sock, prebuffered=b""):
    """Yield payloads of binary/text frames. Server->client frames are unmasked."""
    buf = bytearray(prebuffered)

    def need(n):
        while len(buf) < n:
            chunk = sock.recv(65536)
            if not chunk:
                raise ConnectionError("closed")
            buf.extend(chunk)

    while True:
        need(2)
        b0, b1 = buf[0], buf[1]
        opcode = b0 & 0x0F
        masked = b1 & 0x80
        length = b1 & 0x7F
        offset = 2
        if length == 126:
            need(4)
            length = struct.unpack(">H", bytes(buf[2:4]))[0]
            offset = 4
        elif length == 127:
            need(10)
            length = struct.unpack(">Q", bytes(buf[2:10]))[0]
            offset = 10
        if masked:
            need(offset + 4)
            mask = bytes(buf[offset:offset + 4])
            offset += 4
        need(offset + length)
        payload = bytes(buf[offset:offset + length])
        del buf[:offset + length]
        if masked:
            payload = bytes(b ^ mask[i % 4] for i, b in enumerate(payload))
        if opcode == 0x8:               # close
            return
        if opcode in (0x1, 0x2):
            yield payload


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=8791)
    ap.add_argument("--seconds", type=float, default=10.0)
    ap.add_argument("--out", default="stream.flac")
    args = ap.parse_args()

    sock = socket.create_connection((args.host, args.port), timeout=10)
    rest = handshake(sock, args.host, args.port)
    print(f"connected to ws://{args.host}:{args.port}, accept key verified")

    total = 0
    first = None
    deadline = time.time() + args.seconds
    with open(args.out, "wb") as f:
        try:
            for payload in frames(sock, rest):
                if first is None:
                    first = payload[:4]
                    print(f"first bytes: {first!r} "
                          f"({'fLaC marker OK' if first == b'fLaC' else 'NOT a FLAC stream'})")
                f.write(payload)
                total += len(payload)
                if time.time() > deadline:
                    break
        except (ConnectionError, socket.timeout) as e:
            print(f"stream ended: {e}")
    sock.close()

    secs = max(args.seconds, 0.001)
    print(f"wrote {total} bytes to {args.out} in {secs:.1f}s "
          f"({total * 8 / secs / 1000:.0f} kbit/s)")
    print(f"raw PCM for the same span would be "
          f"{int(44100 * 4 * secs)} bytes "
          f"({total / max(44100 * 4 * secs, 1) * 100:.0f}% of raw)")
    return 0 if total else 1


if __name__ == "__main__":
    sys.exit(main())
