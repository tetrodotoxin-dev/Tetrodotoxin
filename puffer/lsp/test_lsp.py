#!/usr/bin/env python3
"""
# Tetrodotoxin
Copyright (c) 2023-present Matt Kaes and contributors

Local LSP server test harness.

Creates a Unix socket server to test the TTX language server with a few basic
commands without having to do a full .visx build + restart of VS Code.

Usage:
    python3 puffer/lsp/test_lsp.py
"""

import json
import os
import socket
import subprocess
import sys
import tempfile
import threading
import time

SOCKET_PATH = "/tmp/ttx_lsp_test.sock"
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
BINARY = os.environ.get(
    "PUFFER_BINARY",
    os.path.join(REPO_ROOT, ".bin/bin/puffer/puffer"))
PACKAGE_REPOSITORY = os.environ.get("PUFFER_PACKAGE_REPOSITORY", "")
PACKAGE_IDENTITIES = (
    "Perimortem.Memory",
    "Perimortem.Math",
    "Perimortem.System",
    "Perimortem.Graphics",
)
POSITION_ENCODING = "utf-16" if "--utf16" in sys.argv else "utf-8"
SCENE_ONLY = "--scene-only" in sys.argv
RESPONSE_BUFFERS = {}


def prepare_package_repository():
    if PACKAGE_REPOSITORY:
        return PACKAGE_REPOSITORY, None

    temporary = tempfile.TemporaryDirectory(prefix="puffer-lsp-products-")
    for identity in PACKAGE_IDENTITIES:
        source = os.path.join(
            REPO_ROOT, "packages", "ttx", identity, "package.ttx")
        completed = subprocess.run(
            [BINARY, source, f"-terminal_repository={temporary.name}"],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
        )
        if completed.returncode:
            raise RuntimeError(
                f"could not materialize {identity}: {completed.stderr}")
    return temporary.name, temporary


def lsp_frame(obj):
    body = json.dumps(obj)
    return f"Content-Length: {len(body)}\r\n\r\n{body}".encode()


def drain_output(proc, label):
    for line in proc.stderr:
        print(f"[{label}] {line}", end="", flush=True)


def read_lsp_response(conn, timeout=5.0):
    """Read one complete LSP response (header + body). Returns the parsed JSON
    body dict, or None on timeout."""
    conn.settimeout(timeout)
    key = conn.fileno()
    buf = RESPONSE_BUFFERS.pop(key, b"")
    try:
        while True:
            chunk = conn.recv(4096)
            if not chunk:
                return None
            buf += chunk
            if b"\r\n\r\n" in buf:
                header, _, rest = buf.partition(b"\r\n\r\n")
                content_length = 0
                for part in header.split(b"\r\n"):
                    if part.lower().startswith(b"content-length:"):
                        content_length = int(part.split(b":")[1].strip())
                while len(rest) < content_length:
                    chunk = conn.recv(4096)
                    if not chunk:
                        break
                    rest += chunk
                RESPONSE_BUFFERS[key] = rest[content_length:]
                return json.loads(rest[:content_length].decode())
    except socket.timeout:
        return None


def read_request_response(conn, request_id, timeout=10.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        response = read_lsp_response(
            conn, timeout=max(0.1, deadline - time.monotonic()))
        if response is None:
            return None
        if response.get("id") == request_id:
            return response
    return None


def send_format_edit(conn, source_text, name):
    """Open and format one document through the standard LSP request."""
    uri = f"file:///{name}"
    send_did_open(conn, uri, source_text)
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": 10,
        "method": "textDocument/formatting",
        "params": {
            "textDocument": {"uri": uri},
            "options": {"tabSize": 2, "insertSpaces": True},
        },
    }))
    resp = read_lsp_response(conn, timeout=10.0)
    if resp is None:
        print(f"  ERROR: no response for {name}")
        return None
    if "error" in resp:
        print(f"  ERROR from server for {name}: {resp['error']}")
        return None
    edits = resp.get("result", [])
    if len(edits) != 1 or "newText" not in edits[0]:
        print(f"  ERROR: response for {name} has no complete document edit")
        return None
    return edits[0]


def send_format(conn, source_text, name):
    edit = send_format_edit(conn, source_text, name)
    return edit.get("newText") if edit else None


def send_did_open(conn, uri, source_text):
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "textDocument/didOpen",
        "params": {
            "textDocument": {
                "uri": uri,
                "languageId": "tetrodotoxin",
                "version": 1,
                "text": source_text,
            },
        },
    }))
    return read_lsp_response(conn, timeout=10.0)


def send_did_change(conn, uri, source_text, version):
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "textDocument/didChange",
        "params": {
            "textDocument": {"uri": uri, "version": version},
            "contentChanges": [{"text": source_text}],
        },
    }))
    return read_lsp_response(conn, timeout=10.0)


def send_hover(conn, uri, source_text, needle, request_id, start=0):
    offset = source_text.index(needle, start)
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": request_id,
        "method": "textDocument/hover",
        "params": {
            "textDocument": {"uri": uri},
            "position": source_position(source_text, offset),
        },
    }))
    return read_request_response(conn, request_id)


def send_completion(conn, uri, source_text, offset, request_id):
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": request_id,
        "method": "textDocument/completion",
        "params": {
            "textDocument": {"uri": uri},
            "position": source_position(source_text, offset),
        },
    }))
    return read_request_response(conn, request_id)


def source_position(source_text, offset):
    line = source_text.count("\n", 0, offset)
    line_start = source_text.rfind("\n", 0, offset) + 1
    prefix = source_text[line_start:offset]
    character = (len(prefix.encode("utf-8")) if POSITION_ENCODING == "utf-8"
                 else len(prefix.encode("utf-16-le")) // 2)
    return {"line": line, "character": character}


def send_definition(conn, uri, source_text, needle, request_id, start=0):
    offset = source_text.index(needle, start)
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": request_id,
        "method": "textDocument/definition",
        "params": {
            "textDocument": {"uri": uri},
            "position": source_position(source_text, offset),
        },
    }))
    return read_request_response(conn, request_id)


def matches_location(response, uri, source_text, needle, start=0):
    location = response.get("result") if response else None
    if not location or location.get("uri") != uri:
        return False
    offset = source_text.index(needle, start)
    expected_start = source_position(source_text, offset)
    expected_end = source_position(source_text, offset + len(needle))
    target_range = location.get("range", {})
    return (target_range.get("start") == expected_start and
            target_range.get("end") == expected_end)


def matches_file_start(response, uri):
    location = response.get("result") if response else None
    if not location or location.get("uri") != uri:
        return False
    origin = {"line": 0, "character": 0}
    target_range = location.get("range", {})
    return (target_range.get("start") == origin and
            target_range.get("end") == origin)


def send_semantic_tokens(conn, uri, request_id):
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": request_id,
        "method": "textDocument/semanticTokens/full",
        "params": {"textDocument": {"uri": uri}},
    }))
    return read_request_response(conn, request_id)


def send_inlay_hints(conn, uri, source_text, request_id):
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": request_id,
        "method": "textDocument/inlayHint",
        "params": {
            "textDocument": {"uri": uri},
            "range": {
                "start": {"line": 0, "character": 0},
                "end": source_position(source_text, len(source_text)),
            },
        },
    }))
    return read_request_response(conn, request_id)


def semantic_token_texts(source_text, data):
    lines = source_text.splitlines(keepends=True)
    line = 0
    column = 0
    tokens = []

    for i in range(0, len(data), 5):
        delta_line, delta_start, length, token_type, modifiers = data[i:i + 5]
        line += delta_line
        if delta_line:
            column = delta_start
        else:
            column += delta_start

        if line < len(lines):
            encoded = lines[line].encode(
                "utf-8" if POSITION_ENCODING == "utf-8" else "utf-16-le")
            unit = 1 if POSITION_ENCODING == "utf-8" else 2
            text = encoded[column * unit:(column + length) * unit].decode(
                "utf-8" if POSITION_ENCODING == "utf-8" else "utf-16-le")
            tokens.append((text, token_type))

    return tokens


def semantic_token_records(data):
    line = 0
    column = 0
    records = []
    for i in range(0, len(data), 5):
        delta_line, delta_start, length, token_type, modifiers = data[i:i + 5]
        line += delta_line
        column = delta_start if delta_line else column + delta_start
        records.append((line, column, length, token_type, modifiers))
    return records


def run_test():
    failures = []

    try:
        package_repository, temporary_repository = (
            prepare_package_repository())
    except RuntimeError as error:
        print(f"ERROR: {error}")
        return 1

    def check(condition, message):
        if condition:
            print(f"  [OK] {message}")
        else:
            print(f"  [FAIL] {message}")
            failures.append(message)

    if os.path.exists(SOCKET_PATH):
        os.unlink(SOCKET_PATH)

    server_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind(SOCKET_PATH)
    server_sock.listen(1)
    server_sock.settimeout(5)
    print(f"Socket ready at {SOCKET_PATH}")

    env = os.environ.copy()
    if "--asan" in sys.argv:
        asan_path = subprocess.run(
            ["clang", "-print-file-name=libasan.so"],
            capture_output=True, text=True).stdout.strip()
        if asan_path and os.path.exists(asan_path):
            env["LD_PRELOAD"] = asan_path
            print(f"ASAN: {asan_path}")

    server_arguments = [
        BINARY,
        f"-lsp={SOCKET_PATH}",
        f"-package_repository={package_repository}",
    ]
    proc = subprocess.Popen(
        server_arguments,
        stderr=subprocess.PIPE,
        text=True,
        env=env,
    )
    print(f"Server launched (pid={proc.pid})")

    drain_thread = threading.Thread(
        target=drain_output, args=(proc, "server"), daemon=True)
    drain_thread.start()

    try:
        conn, _ = server_sock.accept()
        print("Server connected to socket.")
    except socket.timeout:
        print("ERROR: server did not connect within 5s")
        proc.terminate()
        drain_thread.join(timeout=2)
        return 1

    print("\n--- Sending initialize ---")
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
            "processId": os.getpid(),
            "clientInfo": {"name": "ttx-test"},
            "capabilities": {
                "general": {
                    "positionEncodings": (
                        ["utf-16"] if POSITION_ENCODING == "utf-16"
                        else ["utf-8", "utf-16"]),
                },
            },
        },
    }))

    init_resp = read_lsp_response(conn)
    caps = {}
    generic_token = None
    raw_comment_token = None
    if init_resp:
        caps = init_resp.get("result", {}).get("capabilities", {})
        info = init_resp.get("result", {}).get("serverInfo", {})
        print(f"  Server: {info.get('name')} v{info.get('version')}")
        print(f"  Capabilities: {list(caps.keys())}")
        check(caps.get("positionEncoding") == POSITION_ENCODING,
              f"server negotiates {POSITION_ENCODING} document positions")
        check("semanticTokensProvider" in caps,
              "server advertises semantic tokens")
        semantic_provider = caps.get("semanticTokensProvider", {})
        legend = semantic_provider.get("legend", {})
        check("keyword" in legend.get("tokenTypes", []),
              "semantic token legend includes keyword")
        generic_types = legend.get("tokenTypes", [])
        generic_token = (generic_types.index("generic")
                         if "generic" in generic_types else None)
        raw_comment_token = (generic_types.index("rawComment")
                             if "rawComment" in generic_types else None)
        check(generic_token is not None,
              "semantic token legend includes Generic formulas")
        check(raw_comment_token is not None,
              "semantic token legend includes raw comments")
        check(bool(semantic_provider.get("full")),
              "server supports full semantic token requests")
        check(bool(caps.get("hoverProvider")),
              "server advertises semantic hover")
        check(bool(caps.get("inlayHintProvider")),
              "server advertises parameter inlay hints")
        check(bool(caps.get("completionProvider")),
              "server advertises access completion")
        check(bool(caps.get("definitionProvider")),
              "server advertises go to definition")
        check(bool(caps.get("documentFormattingProvider")),
              "server advertises document formatting")
    else:
        print("  ERROR: no initialize response")
        failures.append("initialize response")

    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "initialized",
        "params": {},
    }))
    time.sleep(0.1)

    print("\n--- Nested Package member: Scene source session ---")
    splash_path = os.path.join(
        REPO_ROOT, "apps", "ttx", "scene_lifetime", "scenes", "splash.ttx")
    with open(splash_path, "r", encoding="utf-8") as f:
        splash_source = f.read()

    splash_uri = "file://" + splash_path
    splash_diagnostics = send_did_open(conn, splash_uri, splash_source)
    splash_messages = (splash_diagnostics or {}).get(
        "params", {}).get("diagnostics", [])
    if splash_messages:
        print("  Scene Package diagnostics:")
        for diagnostic in splash_messages:
            print("   ", diagnostic.get("message"))
    check(splash_diagnostics is not None and not splash_messages,
          "Scene member resolves its Package Resource and Graphics dependency")

    title_path = os.path.join(
        REPO_ROOT, "apps", "ttx", "scene_lifetime", "scenes", "title.ttx")
    blend_path = os.path.join(
        REPO_ROOT, "apps", "ttx", "scene_lifetime", "shaders", "blend.ttx")
    with open(title_path, "r", encoding="utf-8") as f:
        title_source = f.read()
    with open(blend_path, "r", encoding="utf-8") as f:
        blend_source = f.read()
    title_uri = "file://" + title_path
    blend_uri = "file://" + blend_path
    title_diagnostics = send_did_open(conn, title_uri, title_source)
    title_messages = (title_diagnostics or {}).get(
        "params", {}).get("diagnostics", [])
    check(title_diagnostics is not None and not title_messages,
          "Title Scene resolves its qualified Shader and Graphics Types")

    graphics_route = title_source.index("Graphics::Sprite")
    graphics_hover = send_hover(
        conn, title_uri, title_source, "Graphics", 101, graphics_route)
    graphics_result = graphics_hover.get("result") if graphics_hover else None
    graphics_markdown = (
        graphics_result.get("contents", {}).get("value", "")
        if graphics_result else "")
    graphics_hover_matches = (
        "alias Graphics = Perimortem.Graphics" in graphics_markdown and
        "Type Sprite" not in graphics_markdown)
    if not graphics_hover_matches:
        print("  Graphics hover:", graphics_hover)
    check(graphics_hover_matches,
          "qualified hover preserves the Graphics Package alias")
    graphics_definition = send_definition(
        conn, title_uri, title_source, "Graphics", 102, graphics_route)
    graphics_declaration = title_source.index("public Graphics")
    check(matches_location(
        graphics_definition, title_uri, title_source, "Graphics",
        graphics_declaration),
        "qualified definition selects the Graphics Package alias")

    shader_route = title_source.index("Blend::Material")

    blend_hover = send_hover(
        conn, title_uri, title_source, "Blend", 103, shader_route)
    blend_result = blend_hover.get("result") if blend_hover else None
    blend_markdown = (
        blend_result.get("contents", {}).get("value", "")
        if blend_result else "")
    blend_hover_matches = (
        "alias Blend" in blend_markdown and
        "Type Material" not in blend_markdown)
    if not blend_hover_matches:
        print("  Blend hover:", blend_hover)
    check(blend_hover_matches,
          "qualified hover preserves the Blend Package alias")
    blend_definition = send_definition(
        conn, title_uri, title_source, "Blend", 104, shader_route)
    blend_declaration = title_source.index("private Blend")
    check(matches_location(
        blend_definition, title_uri, title_source, "Blend",
        blend_declaration),
        "qualified definition selects the Blend Package alias")

    source_definition = send_definition(
        conn, title_uri, title_source, "source", 107)
    check(matches_file_start(source_definition, blend_uri),
          "source locator definition opens the imported file")

    graphics_package_root = os.path.join(
        REPO_ROOT, "packages", "ttx", "Perimortem.Graphics")
    graphics_package_uri = "file://" + os.path.join(
        graphics_package_root, "package.ttx")
    package_definition = send_definition(
        conn, title_uri, title_source, "package", 108)
    package_definition_matches = matches_location(
        package_definition, title_uri, title_source, "Graphics",
        graphics_declaration)
    if not package_definition_matches:
        print("  Package locator definition:", package_definition)
    check(package_definition_matches,
          "package locator definition selects its authored Import alias")

    with open(os.path.join(graphics_package_root, "package.ttx"),
              "r", encoding="utf-8") as f:
        graphics_package_source = f.read()
    graphics_package_diagnostics = send_did_open(
        conn, graphics_package_uri, graphics_package_source)
    check(graphics_package_diagnostics is not None and
          not graphics_package_diagnostics.get(
              "params", {}).get("diagnostics", []),
          "Graphics Package direct Import Types publish without diagnostics")
    pixel_expression = graphics_package_source.index(
        'source("pixel.ttx")::Pixel')
    pixel_segment = graphics_package_source.index(
        "Pixel", pixel_expression + len('source("pixel.ttx")::'))
    pixel_definition = send_definition(
        conn, graphics_package_uri, graphics_package_source, "Pixel", 109,
        pixel_segment)
    pixel_hover = send_hover(
        conn, graphics_package_uri, graphics_package_source, "Pixel", 111,
        pixel_segment)
    pixel_hover_result = pixel_hover.get("result") if pixel_hover else None
    pixel_markdown = (
        pixel_hover_result.get("contents", {}).get("value", "")
        if pixel_hover_result else "")
    check("Type Pixel" in pixel_markdown,
          "chained Import hover selects the external Type")
    pixel_uri = "file://" + os.path.join(graphics_package_root, "pixel.ttx")
    with open(os.path.join(graphics_package_root, "pixel.ttx"),
              "r", encoding="utf-8") as f:
        pixel_source = f.read()
    pixel_declaration = pixel_source.index("public Pixel")
    pixel_matches = matches_location(
        pixel_definition, pixel_uri, pixel_source, "Pixel",
        pixel_declaration)
    if not pixel_matches:
        print("  Chained Pixel definition:", pixel_definition)
        print("  Chained Pixel hover:", pixel_hover)
    check(pixel_matches,
        "chained Import definition selects the external Type")

    icon_uri = "file://" + os.path.join(
        REPO_ROOT, "apps", "ttx", "scene_lifetime", "resources",
        "icon.png")
    icon_definition = send_definition(
        conn, splash_uri, splash_source, "icon.png", 110)
    check(matches_file_start(icon_definition, icon_uri),
          "embedded Resource definition opens the acquired file")

    material_hover = send_hover(
        conn, title_uri, title_source, "Material", 105, shader_route)
    material_result = material_hover.get("result") if material_hover else None
    material_markdown = (
        material_result.get("contents", {}).get("value", "")
        if material_result else "")
    check("Type Material" in material_markdown,
          "qualified hover selects the app-owned Shader Material")
    material_definition = send_definition(
        conn, title_uri, title_source, "Material", 106, shader_route)
    material_declaration = blend_source.index("implements")
    material_matches = matches_location(
        material_definition, blend_uri, blend_source, "implements",
        material_declaration)
    if not material_matches:
        print("  Material definition:", material_definition)
    check(material_matches,
          "generated Material definition opens its owning Shader source")
    if SCENE_ONLY:
        conn.close()
        proc.terminate()
        proc.wait(timeout=3)
        drain_thread.join(timeout=2)
        server_sock.close()
        if os.path.exists(SOCKET_PATH):
            os.unlink(SOCKET_PATH)
        if failures:
            print("\nFailures:")
            for failure in failures:
                print(f"  - {failure}")
            return 1
        return 0

    print("\n--- Access completion: progressive Library graph ---")
    dot_source = (
        "// Progressive address completion.\n"
        "dialect : Library;\n"
        "public Item : struct {\n"
        "  public state count : U64;\n"
        "}\n"
        "public explore : func = [.value : Item] -> [] {\n"
        "  value.")
    dot_uri = "file:///completion-address.ttx"
    send_did_open(conn, dot_uri, dot_source)
    dot_completion = send_completion(
        conn, dot_uri, dot_source, len(dot_source), 60)
    dot_labels = {
        item.get("label")
        for item in (dot_completion or {}).get("result", [])
    }
    if "count" not in dot_labels:
        print("  Address completion:", dot_completion)
    check("count" in dot_labels,
          "address completion survives an unfinished access expression")

    type_source = (
        "// Progressive Type completion.\n"
        "dialect : Library;\n"
        "public Item : struct {\n"
        "  public Nested : struct {}\n"
        "}\n"
        "public explore : func = [] -> [] {\n"
        "  Item::")
    type_uri = "file:///completion-type.ttx"
    send_did_open(conn, type_uri, type_source)
    type_completion = send_completion(
        conn, type_uri, type_source, len(type_source), 61)
    type_labels = {
        item.get("label")
        for item in (type_completion or {}).get("result", [])
    }
    if "Nested" not in type_labels:
        print("  Type completion:", type_completion)
    check("Nested" in type_labels,
          "Type completion uses the retained nested Type graph")

    call_source = (
        "// Progressive Callable completion.\n"
        "dialect : Library;\n"
        "public Item : struct {\n"
        "  public size : func = [self] -> U64 : return 0;\n"
        "}\n"
        "public item : Item;\n"
        "public explore : func = [] -> [] {\n"
        "  item ->")
    call_uri = "file:///completion-call.ttx"
    send_did_open(conn, call_uri, call_source)
    call_completion = send_completion(
        conn, call_uri, call_source, len(call_source), 62)
    call_labels = {
        item.get("label")
        for item in (call_completion or {}).get("result", [])
    }
    if "size" not in call_labels:
        print("  Callable completion:", call_completion)
    check("size" in call_labels,
          "Callable completion preserves the Self receiver role")

    print("\n--- Package session: cross-source and System ABI ---")
    package_root = os.path.join(
        REPO_ROOT, "validation", "data", "ttx", "package_session")
    helper_path = os.path.join(package_root, "helper.ttx")
    main_path = os.path.join(package_root, "main.ttx")
    package_path = os.path.join(package_root, "package.ttx")
    with open(helper_path, "r", encoding="utf-8") as f:
        helper_source = f.read()
    with open(main_path, "r", encoding="utf-8") as f:
        main_source = f.read()
    with open(package_path, "r", encoding="utf-8") as f:
        package_source = f.read()
    helper_uri = "file://" + helper_path
    main_uri = "file://" + main_path
    package_uri = "file://" + package_path
    system_path = os.path.join(
        REPO_ROOT, "packages", "ttx", "Perimortem.System", "terminal.ttx")
    system_package_path = os.path.join(
        REPO_ROOT, "packages", "ttx", "Perimortem.System", "package.ttx")
    with open(system_path, "r", encoding="utf-8") as f:
        system_source = f.read()
    with open(system_package_path, "r", encoding="utf-8") as f:
        system_package_source = f.read()
    system_uri = "file://" + system_path
    system_package_uri = "file://" + system_package_path
    system_package_diagnostics = send_did_open(
        conn, system_package_uri, system_package_source)
    system_diagnostics = send_did_open(conn, system_uri, system_source)
    package_diagnostics = send_did_open(conn, package_uri, package_source)
    helper_diagnostics = send_did_open(conn, helper_uri, helper_source)
    main_diagnostics = send_did_open(conn, main_uri, main_source)
    check(helper_diagnostics is not None and not helper_diagnostics.get(
        "params", {}).get("diagnostics", []),
        "Package helper publishes without diagnostics")
    check(system_diagnostics is not None and not system_diagnostics.get(
        "params", {}).get("diagnostics", []),
        "Perimortem.System source Package publishes without diagnostics")
    check(system_package_diagnostics is not None and
          not system_package_diagnostics.get(
              "params", {}).get("diagnostics", []),
          "Perimortem.System Package root publishes without diagnostics")
    package_messages = (package_diagnostics or {}).get(
        "params", {}).get("diagnostics", [])
    if package_messages:
        print("  Consumer Package diagnostics:")
        for diagnostic in package_messages:
            print("   ", diagnostic.get("message"))
    check(package_diagnostics is not None and not package_diagnostics.get(
        "params", {}).get("diagnostics", []),
        "consumer Package root publishes without diagnostics")
    main_messages = (main_diagnostics or {}).get(
        "params", {}).get("diagnostics", [])
    if main_messages:
        print("  Consumer member diagnostics:")
        for diagnostic in main_messages:
            print("   ", diagnostic.get("message"))
    check(main_diagnostics is not None and not main_diagnostics.get(
        "params", {}).get("diagnostics", []),
        "Package source resolves its sibling and Perimortem.System")
    prefix_use = main_source.index("Dynamic::Bytes -> concat")
    system_hover = send_hover(
        conn, main_uri, main_source, "line_prefix", 11, prefix_use)
    system_result = system_hover.get("result") if system_hover else None
    system_markdown = (
        system_result.get("contents", {}).get("value", "")
        if system_result else "")
    if "line_prefix : View[U8]" not in system_markdown:
        print("  line_prefix hover:", system_hover)
    check("line_prefix : View[U8]" in system_markdown,
          "Package hover uses the shared cross-source analysis snapshot")
    helper_call = main_source.index("Helper -> prefix")
    prefix_definition = send_definition(
        conn, main_uri, main_source, "prefix", 41, helper_call)
    helper_declaration = helper_source.index("public prefix")
    check(matches_location(
        prefix_definition, helper_uri, helper_source, "prefix",
        helper_declaration),
        "definition resolves a sibling Package member")

    bytes_hover = send_hover(
        conn, main_uri, main_source, "Bytes", 16, prefix_use)
    bytes_result = bytes_hover.get("result") if bytes_hover else None
    bytes_markdown = (
        bytes_result.get("contents", {}).get("value", "")
        if bytes_result else "")
    check("Type Bytes" in bytes_markdown and
          "copy on write container of U8 values" in bytes_markdown,
          "Package hover preserves exported Bytes Type documentation")
    bytes_definition = send_definition(
        conn, main_uri, main_source, "Bytes", 42, prefix_use)
    if not matches_location(
            bytes_definition, main_uri, main_source, "Bytes", prefix_use):
        print("  Archived Bytes definition:", bytes_definition)
    check(matches_location(
        bytes_definition, main_uri, main_source, "Bytes", prefix_use),
        "source-free Package definition retains the exact local Type use")

    graphics_root = os.path.join(
        REPO_ROOT, "packages", "ttx", "Perimortem.Graphics")
    transform_path = os.path.join(graphics_root, "transform2d.ttx")
    with open(transform_path, "r", encoding="utf-8") as f:
        transform_source = f.read()
    transform_uri = "file://" + transform_path
    transform_diagnostics = send_did_open(
        conn, transform_uri, transform_source)
    check(transform_diagnostics is not None and not transform_diagnostics.get(
        "params", {}).get("diagnostics", []),
        "Graphics Transform2D publishes without diagnostics")
    point_use = transform_source.index("state translation : Point2D")
    point_definition = send_definition(
        conn, transform_uri, transform_source, "Point2D", 68, point_use)
    point_declaration = transform_source.index("public Point2D : alias")
    check(matches_location(
        point_definition, transform_uri, transform_source, "Point2D",
        point_declaration),
        "definition preserves the source-local Point2D Alias")

    concat_hover = send_hover(
        conn, main_uri, main_source, "concat", 17, prefix_use)
    concat_result = concat_hover.get("result") if concat_hover else None
    concat_markdown = (
        concat_result.get("contents", {}).get("value", "")
        if concat_result else "")
    check("func concat" in concat_markdown and
          "every element of left followed by" in concat_markdown,
          "Package hover preserves exported Bytes Callable documentation")
    concat_definition = send_definition(
        conn, main_uri, main_source, "concat", 43, prefix_use)
    if not matches_location(
            concat_definition, main_uri, main_source, "concat", prefix_use):
        print("  Archived concat definition:", concat_definition)
    check(matches_location(
        concat_definition, main_uri, main_source, "concat", prefix_use),
        "source-free Package definition retains the exact Callable use")

    get_view_use = main_source.index("line -> get_view")
    get_view_hover = send_hover(
        conn, main_uri, main_source, "get_view", 18, get_view_use)
    get_view_result = (
        get_view_hover.get("result") if get_view_hover else None)
    get_view_markdown = (
        get_view_result.get("contents", {}).get("value", "")
        if get_view_result else "")
    check("func get_view" in get_view_markdown and
          "exactly the logical bytes" in get_view_markdown,
          "Package hover preserves receiver Callable documentation")

    renamed_helper = helper_source.replace("public prefix", "public renamed")
    send_did_change(conn, helper_uri, renamed_helper, 2)
    invalidated_hover = send_hover(
        conn, main_uri, main_source, "line_prefix", 12, prefix_use)
    invalidated_result = (
        invalidated_hover.get("result") if invalidated_hover else None)
    invalidated_markdown = (
        invalidated_result.get("contents", {}).get("value", "")
        if invalidated_result else "")
    check("line_prefix : Unknown" in invalidated_markdown,
          "editing one member exposes an unresolved progressive hover")
    send_did_change(conn, helper_uri, helper_source, 3)
    restored_hover = send_hover(
        conn, main_uri, main_source, "line_prefix", 13, prefix_use)
    restored_result = restored_hover.get("result") if restored_hover else None
    restored_markdown = (
        restored_result.get("contents", {}).get("value", "")
        if restored_result else "")
    check("line_prefix : View[U8]" in restored_markdown,
          "restoring an overlay rebuilds one complete Package snapshot")

    renamed_system = system_source.replace(
        "public read_line", "public renamed_line")
    send_did_change(conn, system_uri, renamed_system, 2)
    dependency_hover = send_hover(
        conn, main_uri, main_source, "line_prefix", 14, prefix_use)
    dependency_result = (
        dependency_hover.get("result") if dependency_hover else None)
    dependency_markdown = (
        dependency_result.get("contents", {}).get("value", "")
        if dependency_result else "")
    check("line_prefix : View[U8]" in dependency_markdown,
          "editing a dependency retains progressive consumer hover")
    send_did_change(conn, system_uri, system_source, 3)
    dependency_restored_hover = send_hover(
        conn, main_uri, main_source, "line_prefix", 15, prefix_use)
    dependency_restored_result = (
        dependency_restored_hover.get("result")
        if dependency_restored_hover else None)
    dependency_restored_markdown = (
        dependency_restored_result.get("contents", {}).get("value", "")
        if dependency_restored_result else "")
    check("line_prefix : View[U8]" in
          dependency_restored_markdown,
          "restoring a dependency overlay rebuilds its consumers")

    context_main_source = main_source.replace(
        "  System::Terminal -> write_line(Dynamic::Bytes -> "
        "concat(line_prefix, view))?;\n",
        "  state output := Dynamic::\n")
    send_did_change(conn, main_uri, context_main_source, 2)
    context_completion = send_completion(
        conn, main_uri, context_main_source,
        context_main_source.index("Dynamic::") + len("Dynamic::"), 66)
    context_labels = {
        item.get("label")
        for item in (context_completion or {}).get("result", [])
    }
    check("Bytes" in context_labels,
          "Library Monograph context offers its retained Types")

    static_main_source = main_source.replace(
        "  System::Terminal -> write_line(Dynamic::Bytes -> "
        "concat(line_prefix, view))?;\n",
        "  state output := Dynamic::Bytes -> \n")
    send_did_change(conn, main_uri, static_main_source, 3)
    static_completion = send_completion(
        conn, main_uri, static_main_source,
        static_main_source.index("Dynamic::Bytes -> ") +
        len("Dynamic::Bytes -> "), 67)
    static_labels = {
        item.get("label")
        for item in (static_completion or {}).get("result", [])
    }
    if not {"copy", "concat"}.issubset(static_labels):
        print("  Static completion:", static_completion)
    check("copy" in static_labels and "concat" in static_labels,
          "Static Type completion survives preferred call spacing")

    partial_main_source = main_source.replace(
        "  System::Terminal -> write_line(Dynamic::Bytes -> "
        "concat(line_prefix, view))?;\n",
        "  while true {\n"
        "    state output := Dynamic::Bytes -> copy(line_prefix);\n"
        "    output->\n"
        "  }\n")
    partial_diagnostics = send_did_change(
        conn, main_uri, partial_main_source, 4)
    partial_messages = (partial_diagnostics or {}).get(
        "params", {}).get("diagnostics", [])
    partial_hover = send_hover(
        conn, main_uri, partial_main_source, "output", 63,
        partial_main_source.index("output->"))
    partial_result = partial_hover.get("result") if partial_hover else None
    partial_markdown = (
        partial_result.get("contents", {}).get("value", "")
        if partial_result else "")
    if "output : Bytes" not in partial_markdown:
        print("  Inferred Local hover:", partial_hover)
    inferred_completion = send_completion(
        conn, main_uri, partial_main_source,
        partial_main_source.index("output->") + len("output->"), 64)
    partial_definition = send_definition(
        conn, main_uri, partial_main_source, "output", 65,
        partial_main_source.index("output->"))
    inferred_labels = {
        item.get("label")
        for item in (inferred_completion or {}).get("result", [])
    }
    check(bool(partial_messages),
          "unfinished inferred access remains diagnostic")
    check("output : Bytes" in partial_markdown,
          "retained Package source preserves an inferred Local Type")
    check("concat" in inferred_labels,
          "inferred Local completion survives unfinished invocation")
    check(matches_location(
        partial_definition, main_uri, partial_main_source, "output",
        partial_main_source.index("state output")),
        "unfinished invocation preserves inferred Local definition")
    send_did_change(conn, main_uri, main_source, 5)

    print("\n--- Semantic tokens: Library/default dialect ---")
    library_source = (
        "dialect : Library;\n"
        "@first @second\n"
        "public func run[] -> Count {\n"
        "  pair.left;\n"
        "  pair -> sum();\n"
        "  source -> helper();\n"
        "  foreign -> external();\n"
        "  while (true) {\n"
        "    continue;\n"
        "  }\n"
        "  return 0;\n"
        "}\n"
    )
    library_uri = "file:///semantic-library.ttx"
    send_did_open(conn, library_uri, library_source)
    library_resp = send_semantic_tokens(conn, library_uri, 20)
    library_data = library_resp.get("result", {}).get("data", []) if library_resp else []
    library_tokens = semantic_token_texts(library_source, library_data)
    library_texts = [text for text, _ in library_tokens]
    check(len(library_data) > 0, "Library document returns semantic tokens")
    check(("@first", 12) in library_tokens and
          ("@second", 12) in library_tokens and
          ("public", 7) in library_tokens,
          "attributes preserve complete ranges and following columns")
    check("while" in library_texts and "continue" in library_texts,
          "Library document highlights loop-control keywords")
    check(("source", 7) in library_tokens,
          "Library document highlights the Source routing keyword")
    check(("foreign", 7) in library_tokens,
          "Library document highlights the Foreign routing keyword")
    check(("left", 5) in library_tokens,
          "address access highlights the selected property")
    check(("sum", 6) in library_tokens,
          "receiver invocation highlights the selected function")

    no_dialect_source = (
        "public func draft[] -> Count {\n"
        "  state label : Text = \"Icon \\\"Preview\\\"\";\n"
        "  if (true) {\n"
        "    continue;\n"
        "  }\n"
        "  return 0;\n"
        "}\n"
    )
    no_dialect_uri = "file:///semantic-draft.ttx"
    send_did_open(conn, no_dialect_uri, no_dialect_source)
    no_dialect_resp = send_semantic_tokens(conn, no_dialect_uri, 21)
    no_dialect_data = no_dialect_resp.get("result", {}).get("data", []) if no_dialect_resp else []
    no_dialect_texts = [text for text, _ in semantic_token_texts(
        no_dialect_source, no_dialect_data)]
    check(len(no_dialect_data) > 0, "no-dialect document returns semantic tokens")
    check("if" in no_dialect_texts and "continue" in no_dialect_texts,
          "missing dialect defaults to Library highlighting")
    check("\"Icon \\\"Preview\\\"\"" in no_dialect_texts,
          "document sync decodes escaped string text")

    print("\n--- Semantic tokens: Shader Library execution ---")
    shader_source = (
        "dialect : Shader;\n"
        "implements source(\"pipeline.ttx\");\n"
        "Shader fragment[] -> [] {\n"
        "  while true {\n"
        "    if true {\n"
        "      continue;\n"
        "    }\n"
        "  }\n"
        "  return;\n"
        "}\n"
    )
    shader_uri = "file:///semantic-shader.ttx"
    send_did_open(conn, shader_uri, shader_source)
    shader_resp = send_semantic_tokens(conn, shader_uri, 22)
    shader_data = shader_resp.get("result", {}).get("data", []) if shader_resp else []
    shader_texts = [text for text, _ in semantic_token_texts(
        shader_source, shader_data)]
    check(len(shader_data) > 0, "Shader document returns semantic tokens")
    check("return" in shader_texts, "Shader document keeps shared control keywords")
    check("if" in shader_texts and "continue" in shader_texts,
          "Shader document keeps Library control keywords")

    print("\n--- Semantic tokens: App and Scene policy ---")
    app_path = os.path.join(
        REPO_ROOT, "apps", "ttx", "scene_lifetime", "main.ttx")
    with open(app_path, "r", encoding="utf-8") as f:
        app_source = f.read()
    app_uri = "file://" + app_path
    send_did_open(conn, app_uri, app_source)
    app_resp = send_semantic_tokens(conn, app_uri, 68)
    app_data = app_resp.get("result", {}).get("data", []) if app_resp else []
    app_tokens = semantic_token_texts(app_source, app_data)
    for spelling in [
            "runtime", "lifecycle", "initial", "on", "replace", "exit"]:
        check((spelling, 7) in app_tokens,
              f"App policy highlights {spelling} from its semantic owner")

    scene_path = os.path.join(
        REPO_ROOT, "apps", "ttx", "scene_lifetime", "scenes", "title.ttx")
    with open(scene_path, "r", encoding="utf-8") as f:
        scene_source = f.read()
    scene_uri = "file://" + scene_path
    send_did_open(conn, scene_uri, scene_source)
    scene_resp = send_semantic_tokens(conn, scene_uri, 69)
    scene_data = (
        scene_resp.get("result", {}).get("data", []) if scene_resp else [])
    scene_tokens = semantic_token_texts(scene_source, scene_data)
    check(("/// Tetrodotoxin", raw_comment_token) in scene_tokens,
          "Scene gives raw comments their subdued semantic token")
    check(("signal", 7) in scene_tokens,
          "Scene highlights the Signal declaration keyword")
    check(("space_pressed", 5) in scene_tokens,
          "Scene highlights the real Signal identity as a property")

    print("\n--- Parameter inlay hints: retained Call fitting ---")
    hint_source = (
        "// Inlay hint source.\n"
        "dialect : Library;\n"
        "public Pair : struct {\n"
        "  public state left  : U64;\n"
        "  public state right : U64;\n"
        "}\n"
        "public combine : func = [.left : U64, .right : U64] -> U64 : "
        "return left + right;\n"
        "public consume : func = [.pair : Pair] -> U64 : "
        "return pair.left + pair.right;\n"
        "public hints : func = [] -> U64 {\n"
        "  state direct := source -> combine(1, 2);\n"
        "  state named := source -> combine(.left = 3, .right = 4);\n"
        "  return direct + named + source -> consume(5, 6);\n"
        "}\n"
    )
    hint_uri = "file:///semantic-inlay-hints.ttx"
    hint_diagnostics = send_did_open(conn, hint_uri, hint_source)
    check(hint_diagnostics is not None and not hint_diagnostics.get(
        "params", {}).get("diagnostics", []),
        "inlay hint source completes its semantic Call mappings")
    hint_resp = send_inlay_hints(conn, hint_uri, hint_source, 23)
    hints = hint_resp.get("result", []) if hint_resp else []
    direct_start = hint_source.index("combine(1, 2)")
    composed_start = hint_source.index("consume(5, 6)")
    expected_hints = [
        (".left =", source_position(
            hint_source, hint_source.index("1", direct_start))),
        (".right =", source_position(
            hint_source, hint_source.index("2", direct_start))),
        (".pair =", source_position(
            hint_source, hint_source.index("5", composed_start))),
    ]
    actual_hints = [(hint.get("label"), hint.get("position"))
                    for hint in hints]
    if actual_hints != expected_hints:
        print("  Inlay hints:", hint_resp)
    check(actual_hints == expected_hints,
          "positional and composed Packs use exact fitted parameter hints")
    check(all(hint.get("kind") == 2 and hint.get("paddingRight") is True
              for hint in hints),
          "inlay hints publish standard parameter presentation")

    print(f"\n--- {POSITION_ENCODING} protocol position boundary ---")
    unicode_source = (
        "// Unicode protocol source.\n"
        "dialect : Library;\n"
        "private const prefix := \"😀\" -> get_view(); public echo : func = "
        "[.value : U64] -> U64 : return value;\n"
        "public run : func = [] -> U64 { const local := \"😀\" -> get_view(); "
        "return source -> echo(7); }"
    )
    unicode_uri = "file:///semantic-unicode.ttx"
    unicode_diagnostics = send_did_open(conn, unicode_uri, unicode_source)
    check(unicode_diagnostics is not None and not unicode_diagnostics.get(
        "params", {}).get("diagnostics", []),
        "Unicode protocol source completes without diagnostics")
    unicode_use = unicode_source.index("source -> echo")
    unicode_hover = send_hover(
        conn, unicode_uri, unicode_source, "echo", 25, unicode_use)
    unicode_markdown = (
        (unicode_hover.get("result") or {}).get("contents", {}).get("value", "")
        if unicode_hover else "")
    if "func echo" not in unicode_markdown:
        print("  Unicode hover:", unicode_hover)
    check("func echo[.value : U64] -> [U64]" in unicode_markdown,
          f"hover maps {POSITION_ENCODING} positions after an astral character")
    unicode_definition = send_definition(
        conn, unicode_uri, unicode_source, "echo", 26, unicode_use)
    unicode_declaration = unicode_source.index("public echo")
    check(matches_location(
        unicode_definition, unicode_uri, unicode_source, "echo",
        unicode_declaration),
        "definition projects an Anchor after an astral character")
    unicode_inlays = send_inlay_hints(
        conn, unicode_uri, unicode_source, 27)
    unicode_hint_values = (
        unicode_inlays.get("result", []) if unicode_inlays else [])
    argument = unicode_source.index("7", unicode_use)
    if not unicode_hint_values:
        print("  Unicode inlays:", unicode_inlays)
    check(unicode_hint_values == [{
        "position": source_position(unicode_source, argument),
        "label": ".value =",
        "kind": 2,
        "paddingRight": True,
    }], f"inlay positions use the negotiated {POSITION_ENCODING} encoding")
    unicode_semantic = send_semantic_tokens(conn, unicode_uri, 28)
    unicode_data = (
        unicode_semantic.get("result", {}).get("data", [])
        if unicode_semantic else [])
    first_string = unicode_source.index('"😀"')
    expected_string_position = source_position(unicode_source, first_string)
    expected_string_length = (
        len('"😀"'.encode("utf-8")) if POSITION_ENCODING == "utf-8"
        else len('"😀"'.encode("utf-16-le")) // 2)
    check(any(
        line == expected_string_position["line"] and
        character == expected_string_position["character"] and
        length == expected_string_length and token_type == 9
        for line, character, length, token_type, _ in
        semantic_token_records(unicode_data)),
        f"semantic token range counts negotiated {POSITION_ENCODING} units")
    unicode_edit = send_format_edit(conn, unicode_source, "unicode.ttx")
    check(unicode_edit is not None and
          unicode_edit.get("range", {}).get("end") ==
          source_position(unicode_source, len(unicode_source)),
          f"formatting range ends at the negotiated {POSITION_ENCODING} position")

    unicode_invalid = (
        "// Unicode diagnostic source.\n"
        "dialect : Library;\n"
        "private broken : func = [] -> [] { \"😀\"; $ }\n"
    )
    invalid_uri = "file:///semantic-unicode-invalid.ttx"
    invalid_diagnostics = send_did_open(
        conn, invalid_uri, unicode_invalid)
    invalid_entries = (
        invalid_diagnostics.get("params", {}).get("diagnostics", [])
        if invalid_diagnostics else [])
    invalid_offset = unicode_invalid.index("$")
    invalid_start = source_position(unicode_invalid, invalid_offset)
    invalid_end = source_position(unicode_invalid, invalid_offset + 1)
    check(any(
        diagnostic.get("range", {}).get("start") == invalid_start and
        diagnostic.get("range", {}).get("end") == invalid_end
        for diagnostic in invalid_entries),
        "diagnostic range projects the focused Token at the protocol boundary")

    print("\n--- Semantic hover: completed Library graph ---")
    hover_path = os.path.join(
        REPO_ROOT, "validation", "data", "ttx", "products", "runtime",
        "runtime.ttx")
    with open(hover_path, "r", encoding="utf-8") as f:
        hover_source = f.read()
    hover_uri = "file:///llvm_nonobject-hover.ttx"
    hover_diagnostics = send_did_open(conn, hover_uri, hover_source)
    if (hover_diagnostics or {}).get("params", {}).get("diagnostics", []):
        print("  Hover source diagnostics:", hover_diagnostics)
    check(hover_diagnostics is not None and
          hover_diagnostics.get("method") ==
          "textDocument/publishDiagnostics" and
          not hover_diagnostics.get("params", {}).get("diagnostics", []),
          "valid attributed documentation publishes no diagnostics")
    hover_semantic_resp = send_semantic_tokens(conn, hover_uri, 24)
    hover_semantic_data = (
        hover_semantic_resp.get("result", {}).get("data", [])
        if hover_semantic_resp else [])
    hover_semantic_tokens = semantic_token_texts(
        hover_source, hover_semantic_data)
    check(generic_token is not None and
          ("Option", generic_token) in hover_semantic_tokens and
          ("Object", generic_token) in hover_semantic_tokens,
          "completed Generic formula identities receive their own token type")
    use_start = hover_source.index("total += OptionOps -> forward")
    present_resp = send_hover(
        conn, hover_uri, hover_source, "present", 30, use_start)
    absent_resp = send_hover(
        conn, hover_uri, hover_source, "absent", 31, use_start)
    present_markdown = (
        present_resp.get("result", {}).get("contents", {}).get("value", "")
        if present_resp else "")
    absent_markdown = (
        absent_resp.get("result", {}).get("contents", {}).get("value", "")
        if absent_resp else "")
    check("present : Option[U64]" in present_markdown,
          "hover resolves present to its exact Field and Type")
    check("some(5)" not in present_markdown,
          "Field hover stays separate from its folded Option payload")
    check("absent : Option[U64]" in absent_markdown,
          "hover resolves absent to its exact Field and Type")
    check("= absent" not in absent_markdown,
          "Field hover stays separate from its folded Option state")

    frozen_use = hover_source.index("total += frozen_dense")
    frozen_resp = send_hover(
        conn, hover_uri, hover_source, "frozen_dense", 32, frozen_use)
    frozen_markdown = (
        frozen_resp.get("result", {}).get("contents", {}).get("value", "")
        if frozen_resp else "")
    check("frozen_dense : Fixed[U64,4]" in frozen_markdown,
          "hover resolves the const Fixed stack Local and exact Type")
    check("(5, 6, 7, 8)" not in frozen_markdown,
          "Local hover stays separate from its folded values")

    bytes_use = hover_source.index("Dynamic::Bytes -> concat(left, right)")
    bytes_resp = send_hover(
        conn, hover_uri, hover_source, "left", 33, bytes_use)
    bytes_markdown = (
        bytes_resp.get("result", {}).get("contents", {}).get("value", "")
        if bytes_resp else "")
    check("left : View[U8]" in bytes_markdown,
          "hover resolves the const byte View and exact Type")
    check('"Hi"' not in bytes_markdown,
          "Addressable hover stays separate from its byte Constant")

    execute_start = hover_source.index("public execute : func")
    dense_use = hover_source.index("total += dense", execute_start)
    dense_resp = send_hover(
        conn, hover_uri, hover_source, "dense", 38, dense_use)
    dense_markdown = (
        (dense_resp.get("result") or {}).get("contents", {}).get("value", "")
        if dense_resp else "")
    check("Test documentation string for variable" in dense_markdown,
          "Local hover delegates to its Statement documentation")
    dense_definition = send_definition(
        conn, hover_uri, hover_source, "dense", 44, dense_use)
    dense_declaration = hover_source.index("state dense", execute_start)
    check(matches_location(
        dense_definition, hover_uri, hover_source, "dense",
        dense_declaration),
        "definition resolves a same-source Local")

    function_resp = send_hover(
        conn, hover_uri, hover_source, "execute", 39, execute_start)
    function_markdown = (
        (function_resp.get("result") or {}).get("contents", {}).get("value", "")
        if function_resp else "")
    check("func execute[] -> [U64]" in function_markdown and
          "Test documentation string for function" in function_markdown,
          "Function hover includes its complete signature and documentation")

    changed_hover_source = hover_source.replace(
        "private const present : Maybe = 5;",
        "private const present : Maybe = 7;")
    send_did_change(conn, hover_uri, changed_hover_source, 2)
    changed_present_resp = send_hover(
        conn, hover_uri, changed_hover_source, "present", 33, use_start)
    changed_present_markdown = (
        changed_present_resp.get("result", {})
        .get("contents", {}).get("value", "")
        if changed_present_resp else "")
    check("present : Option[U64]" in changed_present_markdown and
          "some(7)" not in changed_present_markdown,
          "document edits rebuild the exact semantic hover identity")

    detail_source = (
        "// Semantic hover details.\n"
        "dialect : Library;\n"
        "// Storage Type documentation.\n"
        "public Bucket : struct {\n"
        "  // Current value documentation.\n"
        "  public state value : U64 = 1;\n"
        "}\n"
        "// Alias documentation.\n"
        "public BucketAlias : alias = Bucket;\n"
        "private inspect : func = [] -> U64 {\n"
        "  const escaped : View[U8] = "
        "0x[09 0A 0D 22 5C 60 41 FF] -> get_view();\n"
        "  state bucket : BucketAlias = (.value = 2);\n"
        "  return bucket.value;\n"
        "}\n"
    )
    detail_uri = "file:///semantic-hover-details.ttx"
    send_did_open(conn, detail_uri, detail_source)

    field_start = detail_source.index("public state value")
    field_resp = send_hover(
        conn, detail_uri, detail_source, "value", 34, field_start)
    field_markdown = (
        field_resp.get("result", {}).get("contents", {}).get("value", "")
        if field_resp else "")
    check("value : U64" in field_markdown,
          "hover resolves a state Field declaration and exact Type")
    check("```tetrodotoxin\nvalue : U64\n```" in
          field_markdown,
          "state Field hover uses one theme-highlighted declaration")
    check("Current value documentation." in field_markdown and
          "**Kind:**" not in field_markdown and
          "**Documentation**" not in field_markdown,
          "state Field hover presents attached documentation without labels")

    type_start = detail_source.index("public Bucket : struct")
    type_resp = send_hover(
        conn, detail_uri, detail_source, "Bucket", 35, type_start)
    type_markdown = (
        (type_resp.get("result") or {}).get("contents", {}).get("value", "")
        if type_resp else "")
    check("```tetrodotoxin\nType Bucket\n```" in type_markdown,
          "hover resolves an authored Type in a highlighted declaration")
    check("Storage Type documentation." in type_markdown,
          "Type hover includes attached documentation")

    local_start = detail_source.index("state bucket")
    local_resp = send_hover(
        conn, detail_uri, detail_source, "bucket", 36, local_start)
    local_markdown = (
        (local_resp.get("result") or {}).get("contents", {}).get("value", "")
        if local_resp else "")
    check("bucket : Bucket" in local_markdown and
          "```tetrodotoxin" in local_markdown,
          "hover resolves a state Local and its Type in one declaration")

    escaped_start = detail_source.index("const escaped")
    escaped_resp = send_hover(
        conn, detail_uri, detail_source, "escaped", 37, escaped_start)
    escaped_markdown = (
        (escaped_resp.get("result") or {}).get("contents", {}).get("value", "")
        if escaped_resp else "")
    check("escaped : View[U8]" in escaped_markdown,
          "byte View hover preserves its exact Addressable and Type")

    alias_resp = send_hover(
        conn, detail_uri, detail_source, "BucketAlias", 38, local_start)
    alias_markdown = (
        (alias_resp.get("result") or {}).get("contents", {}).get("value", "")
        if alias_resp else "")
    check("alias BucketAlias = Bucket" in alias_markdown and
          "```tetrodotoxin" in alias_markdown,
          "hover preserves the authored Alias and target in one declaration")
    check(alias_markdown.index("Alias documentation.") <
          alias_markdown.index("Storage Type documentation.")
          if "Alias documentation." in alias_markdown and
          "Storage Type documentation." in alias_markdown else False,
          "Alias hover propagates local then target documentation")
    alias_definition = send_definition(
        conn, detail_uri, detail_source, "BucketAlias", 45, local_start)
    alias_declaration = detail_source.index("public BucketAlias")
    check(matches_location(
        alias_definition, detail_uri, detail_source, "BucketAlias",
        alias_declaration),
        "definition preserves the authored Alias identity")

    foreign_parameter_start = hover_source.index(
        ".value : Object[U8]")
    foreign_parameter_resp = send_hover(
        conn, hover_uri, hover_source, "value", 47,
        foreign_parameter_start)
    foreign_parameter_markdown = (
        (foreign_parameter_resp.get("result") or {})
        .get("contents", {}).get("value", "")
        if foreign_parameter_resp else "")
    check(
        "```tetrodotoxin\nvalue : Object[U8]\n```" in
        foreign_parameter_markdown,
        "Layout-owned parameter hover uses its exact Addressable Type")

    foreign_call_start = hover_source.index(
        "foreign -> llvm_object_identity")
    foreign_call_resp = send_hover(
        conn, hover_uri, hover_source, "llvm_object_identity", 48,
        foreign_call_start)
    foreign_call_markdown = (
        (foreign_call_resp.get("result") or {})
        .get("contents", {}).get("value", "")
        if foreign_call_resp else "")
    check(
        "```tetrodotoxin\nfunc llvm_object_identity"
        "[.value : Object[U8]] -> [U64]\n```" in
        foreign_call_markdown,
        "Foreign Callable hover shows its complete signature")

    dense_call_start = hover_source.index("foreign -> llvm_dense_access")
    dense_foreign_definition = send_definition(
        conn, hover_uri, hover_source, "llvm_dense_access", 49,
        dense_call_start)
    dense_foreign_declaration = hover_source.index(
        "public func llvm_dense_access")
    check(matches_location(
        dense_foreign_definition, hover_uri, hover_source,
        "llvm_dense_access", dense_foreign_declaration),
        "definition resolves a Foreign Callable declaration")

    literal_start = detail_source.index("0x[09")
    literal_definition = send_definition(
        conn, detail_uri, detail_source, "09", 46, literal_start)
    literal_spelling = "0x[09 0A 0D 22 5C 60 41 FF]"
    if not matches_location(
            literal_definition, detail_uri, detail_source, literal_spelling,
            literal_start):
        print("  Literal definition:", literal_definition)
    check(matches_location(
        literal_definition, detail_uri, detail_source, literal_spelling,
        literal_start),
        "definition preserves an authored Constant identity")

    diagnostic_source = (
        "// Invalid hover source.\n"
        "dialect : Library;\n"
        "private broken : func = [] -> [];\n"
    )
    diagnostic_uri = "file:///semantic-diagnostic.ttx"
    diagnostic_resp = send_did_open(
        conn, diagnostic_uri, diagnostic_source)
    diagnostics = (
        diagnostic_resp.get("params", {}).get("diagnostics", [])
        if diagnostic_resp else [])
    check(diagnostic_resp is not None and
          diagnostic_resp.get("method") ==
          "textDocument/publishDiagnostics" and diagnostics and
          "Library Blocks require" in diagnostics[0].get("message", ""),
          "semantic failures publish editor diagnostics")

    long_lines = "".join(
        f"// Hover documentation line {index:03d} carries retained text.\n"
        for index in range(100))
    long_hover_source = (
        "// Long hover source.\n"
        "dialect : Library;\n" + long_lines +
        "public Documented : struct {}\n"
    )
    long_hover_uri = "file:///semantic-long-hover.ttx"
    long_diagnostics = send_did_open(
        conn, long_hover_uri, long_hover_source)
    long_hover_resp = send_hover(
        conn, long_hover_uri, long_hover_source, "Documented", 40)
    long_markdown = (
        (long_hover_resp.get("result") or {})
        .get("contents", {}).get("value", "")
        if long_hover_resp else "")
    check(long_diagnostics is not None and len(long_markdown) > 4096 and
          "Hover documentation line 099" in long_markdown,
          "Arena-backed hover output preserves documentation beyond 4 KiB")

    print("\n--- Ignored notifications ---")
    conn.sendall(lsp_frame({
        "jsonrpc": "2.0",
        "method": "$/cancelRequest",
        "params": {"id": 999},
    }))
    notification_probe_resp = send_semantic_tokens(conn, shader_uri, 23)
    check(
        notification_probe_resp is not None
        and "result" in notification_probe_resp,
        "$ notification keeps server responsive")

    print("\n--- Formatting: Markdown heading boundary ---")
    heading_source = (
        "// # Tetrodotoxin\n"
        "// Copyright (c) 2023-present Matt Kaes and contributors\n"
        "//\n"
        "// Describes one source while leaving its legal notice untouched.\n"
        "//\n"
        "dialect : Library;\n"
        "public value : U64;\n")
    heading_header = (
        "// # Tetrodotoxin\n"
        "// Copyright (c) 2023-present Matt Kaes and contributors\n"
        "//\n")
    heading_formatted = send_format(conn, heading_source, "heading.ttx")
    check(heading_formatted is not None and
          heading_formatted.startswith(heading_header),
          "formatting preserves the canonical Markdown header")

    print("\n--- Round-trip: apps/ttx/scene_lifetime/scenes/splash.ttx ---")
    splash_path = os.path.join(
        REPO_ROOT, "apps", "ttx", "scene_lifetime", "scenes", "splash.ttx")
    with open(splash_path, "r", encoding="utf-8") as f:
        splash_source = f.read()

    splash_formatted = send_format(conn, splash_source, "splash.ttx")
    if splash_formatted is not None:
        if splash_formatted == splash_source:
            print("  [OK] splash.ttx is unchanged after formatting")
        else:
            print("  [DIFF] splash.ttx changed after formatting:")
            src_lines = splash_source.splitlines()
            fmt_lines = splash_formatted.splitlines()
            for i, (a, b) in enumerate(zip(src_lines, fmt_lines), 1):
                if a != b:
                    print(f"    line {i}:")
                    print(f"      before: {repr(a)}")
                    print(f"      after:  {repr(b)}")
            if len(src_lines) != len(fmt_lines):
                print(f"  line count: {len(src_lines)} → {len(fmt_lines)}")
        check(splash_formatted == splash_source,
              "tracked TTX source is already canonical")
        splash_second = send_format(conn, splash_formatted, "splash.ttx")
        check(splash_second == splash_formatted,
              "document formatting is byte-idempotent")

    print("\n--- Round-trip: invalid source ---")
    invalid_source = "dialect : Library;\n\n$\n"
    invalid_formatted = send_format(conn, invalid_source, "invalid.ttx")
    if invalid_formatted is not None:
        if invalid_formatted == invalid_source:
            print("  [OK] invalid source is unchanged after formatting")
        else:
            print("  [DIFF] invalid source changed after formatting:")
            print(f"      before: {repr(invalid_source)}")
            print(f"      after:  {repr(invalid_formatted)}")
        check("$" in invalid_formatted,
              "formatting preserves malformed authored content")
        check(invalid_formatted.startswith(
            "//\n// Place holder source documentation.\n//\n"),
            "formatting supplies missing source documentation")

    exit_code = proc.poll()
    if exit_code is None:
        print("\nServer still running. Shutting down.")
        conn.close()
        proc.terminate()
        proc.wait(timeout=3)
    else:
        print(f"\nServer exited with code {exit_code}")

    drain_thread.join(timeout=2)

    try:
        conn.close()
    except Exception:
        pass
    server_sock.close()
    if os.path.exists(SOCKET_PATH):
        os.unlink(SOCKET_PATH)

    if failures:
        print("\nFailures:")
        for failure in failures:
            print(f"  - {failure}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(run_test())
