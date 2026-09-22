#!/usr/bin/env python3
"""
# Tetrodotoxin
Copyright (c) 2023-present Matt Kaes and contributors

Bounded Puffer LSP stress harness.

This starts a fresh Puffer process for each session and sends a mix of
formatting, synchronization, semantic-token, Package import, and replacement
traffic. The goal is not a language conformance suite; it is a bounded crash,
framing, and responsiveness probe for the server boundary that VSCode exercises.

Usage:
    python3 puffer/lsp/stress_lsp.py
    python3 puffer/lsp/stress_lsp.py --sessions=10 --requests=50 --package-edits=25
"""

import argparse
import os
import socket
import subprocess
import sys
import threading
import time

from test_lsp import (
    BINARY,
    REPO_ROOT,
    lsp_frame,
    read_lsp_response,
    send_did_change,
    send_did_open,
    send_format,
    send_semantic_tokens,
)


def parse_args():
    parser = argparse.ArgumentParser(description="Stress Puffer LSP mode.")
    parser.add_argument("--sessions", type=int, default=5)
    parser.add_argument("--requests", type=int, default=25)
    parser.add_argument("--package-edits", type=int, default=10)
    parser.add_argument("--max-request-seconds", type=float, default=5.0)
    return parser.parse_args()


def source_for(iteration):
    if iteration % 3 == 0:
        return (
            "// Stress Library.\n"
            "dialect : Library;\n"
            "public run : func = [] -> U64 {\n"
            "  if (true) {\n"
            "    return 1;\n"
            "  }\n"
            "  return 0;\n"
            "}\n"
        )
    if iteration % 3 == 1:
        return (
            "// Stress Pipeline.\n"
            "dialect : Pipeline;\n"
        )
    return (
        "// Stress Scene.\n"
        "dialect : Scene;\n"
        "public Item : struct {\n"
        "  public state value : U64;\n"
        "}\n"
    )


def launch_server(socket_path):
    if os.path.exists(socket_path):
        os.unlink(socket_path)

    server_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server_socket.bind(socket_path)
    server_socket.listen(1)
    server_socket.settimeout(5)

    proc = subprocess.Popen(
        [BINARY, f"-lsp={socket_path}"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True)
    output_tail = []
    stdout_thread = threading.Thread(
        target=drain_output, args=(proc.stdout, output_tail), daemon=True)
    stderr_thread = threading.Thread(
        target=drain_output, args=(proc.stderr, output_tail), daemon=True)
    stdout_thread.start()
    stderr_thread.start()

    try:
        conn, _ = server_socket.accept()
    except socket.timeout:
        proc.terminate()
        proc.wait(timeout=3)
        raise RuntimeError("server did not connect to the LSP socket")

    return server_socket, conn, proc, stdout_thread, stderr_thread, output_tail


def drain_output(stream, output_tail):
    for line in stream:
        output_tail.append(line.rstrip())
        if len(output_tail) > 50:
            output_tail.pop(0)


def stop_server(
        socket_path,
        server_socket,
        conn,
        proc,
        stdout_thread,
        stderr_thread):
    try:
        conn.close()
    except OSError:
        pass

    try:
        server_socket.close()
    except OSError:
        pass

    if proc.poll() is None:
        proc.terminate()
        proc.wait(timeout=3)

    stdout_thread.join(timeout=2)
    stderr_thread.join(timeout=2)

    if os.path.exists(socket_path):
        os.unlink(socket_path)


def initialize(conn):
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
            "processId": os.getpid(),
            "clientInfo": {"name": "puffer-stress"},
            "capabilities": {},
        },
    }))
    response = read_lsp_response(conn, timeout=10.0)
    if response is None or "result" not in response:
        raise RuntimeError("initialize did not return a result")

    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "initialized",
        "params": {},
    }))


def exercise_package_imports(conn, session, edit_count, maximum_seconds):
    package_root = os.path.join(
        REPO_ROOT, "validation", "data", "ttx", "package_session")
    paths = {
        name: os.path.join(package_root, name + ".ttx")
        for name in ("package", "helper", "main")
    }
    sources = {}
    for name, path in paths.items():
        with open(path, "r", encoding="utf-8") as source_file:
            sources[name] = source_file.read()

    uris = {name: "file://" + path for name, path in paths.items()}
    for name in ("package", "helper", "main"):
        if send_did_open(conn, uris[name], sources[name]) is None:
            raise RuntimeError(f"Package {name} did not publish diagnostics")

    maximum = 0.0
    for iteration in range(edit_count):
        replacement = "Reply: " if iteration % 2 else "Echo: "
        helper = sources["helper"].replace("Echo: ", replacement)
        started = time.monotonic()
        if send_did_change(
                conn, uris["helper"], helper, iteration + 2) is None:
            raise RuntimeError("Package replacement did not publish diagnostics")
        if send_semantic_tokens(
                conn, uris["main"], 50000 + session * 1000 + iteration) is None:
            raise RuntimeError("Package semantic request timed out")
        elapsed = time.monotonic() - started
        maximum = max(maximum, elapsed)
        if elapsed > maximum_seconds:
            raise RuntimeError(
                f"Package rebuild took {elapsed:.3f}s, above "
                f"{maximum_seconds:.3f}s")

    print(
        f"  session {session}: {edit_count} Package rebuilds, "
        f"slowest {maximum:.3f}s",
        flush=True)


def exercise_session(
        session,
        request_count,
        package_edit_count,
        maximum_request_seconds):
    socket_path = f"/tmp/puffer_lsp_stress_{os.getpid()}_{session}.sock"
    (
        server_socket,
        conn,
        proc,
        stdout_thread,
        stderr_thread,
        output_tail,
    ) = launch_server(socket_path)

    try:
        initialize(conn)

        for i in range(request_count):
            source = source_for(i)
            uri = f"file:///puffer-stress-{session}-{i}.ttx"
            send_did_open(conn, uri, source)

            if send_semantic_tokens(conn, uri, 1000 + i) is None:
                raise RuntimeError(f"semantic tokens timed out in session {session}")

            if send_format(conn, source, uri) is None:
                raise RuntimeError(f"format timed out in session {session}")

            conn.sendall(lsp_frame({
                "jsonrpc": "2.0",
                "method": "$/cancelRequest",
                "params": {"id": 1000 + i},
            }))

            exit_code = proc.poll()
            if exit_code is not None:
                raise RuntimeError(
                    f"server exited early in session {session}: {exit_code}")

            if i % 10 == 0:
                print(
                    f"  session {session}: {i + 1}/{request_count}",
                    flush=True)

        exercise_package_imports(
            conn, session, package_edit_count, maximum_request_seconds)
    except RuntimeError as error:
        if output_tail:
            tail = "\n".join(output_tail)
            raise RuntimeError(f"{error}\nserver output tail:\n{tail}")
        raise
    finally:
        stop_server(
            socket_path,
            server_socket,
            conn,
            proc,
            stdout_thread,
            stderr_thread)


def main():
    args = parse_args()
    start = time.monotonic()

    print(
        f"Stress testing {BINARY} "
        f"({args.sessions} sessions x {args.requests} requests)")

    for session in range(args.sessions):
        exercise_session(
            session,
            args.requests,
            args.package_edits,
            args.max_request_seconds)

    elapsed = time.monotonic() - start
    print(f"OK: completed in {elapsed:.2f}s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
