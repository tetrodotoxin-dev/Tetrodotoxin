#!/bin/bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors
#
# Builds Puffer and synchronizes the VSCode extension LSP server. The default
# mode packages a VSIX after synchronization.
# Run from anywhere inside the repository.
#
# Usage:
#   ./extension/package.sh [--sync | --install]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
VSIX_DIR="$REPO_ROOT/.vscode"
SERVER_BIN="$REPO_ROOT/.bin/bin/puffer/puffer"
PACKAGE_SERVER="$SCRIPT_DIR/puffer"
PACKAGE_GRAPHICS_ROOT="$SCRIPT_DIR/packages/Perimortem.Graphics"
PACKAGE_MATH_ROOT="$SCRIPT_DIR/packages/Perimortem.Math"
PACKAGE_MEMORY_ROOT="$SCRIPT_DIR/packages/Perimortem.Memory"
PACKAGE_SYSTEM_ROOT="$SCRIPT_DIR/packages/Perimortem.System"
PACKAGE_GRAPHICS="$PACKAGE_GRAPHICS_ROOT/1.0"
PACKAGE_MATH="$PACKAGE_MATH_ROOT/1.0"
PACKAGE_MEMORY="$PACKAGE_MEMORY_ROOT/1.0"
PACKAGE_SYSTEM="$PACKAGE_SYSTEM_ROOT/1.0"

INSTALL=0
SYNC=0
for arg in "$@"; do
  case "$arg" in
    --install) INSTALL=1 ;;
    --sync) SYNC=1 ;;
    *) echo "Unknown argument: $arg" >&2; exit 1 ;;
  esac
done

if [ "$INSTALL" -eq 1 ] && [ "$SYNC" -eq 1 ]; then
  echo "--sync and --install cannot be combined" >&2
  exit 1
fi

echo "==> Building the Puffer SDK and standard Packages (release)..."
cd "$REPO_ROOT"
bazel build --config=release \
  //puffer:puffer \
  //packages/ttx:perimortem_graphics \
  //packages/ttx:perimortem_math \
  //packages/ttx:perimortem_memory \
  //packages/ttx:perimortem_system

if [ ! -x "$SERVER_BIN" ]; then
  echo "Expected server binary was not created: $SERVER_BIN" >&2
  exit 1
fi

echo "==> Copying latest language server into extension package..."
rm -f "$PACKAGE_SERVER" "$SCRIPT_DIR/ttx-lang-server"
cp -L "$SERVER_BIN" "$PACKAGE_SERVER"
chmod 755 "$PACKAGE_SERVER"

echo "==> Copying versioned standard Package sources, resources, and products..."
rm -rf \
  "$PACKAGE_GRAPHICS_ROOT" \
  "$PACKAGE_MATH_ROOT" \
  "$PACKAGE_MEMORY_ROOT" \
  "$PACKAGE_SYSTEM_ROOT"
mkdir -p \
  "$PACKAGE_GRAPHICS" \
  "$PACKAGE_MATH" \
  "$PACKAGE_MEMORY" \
  "$PACKAGE_SYSTEM"
cp -RL "$REPO_ROOT/packages/ttx/Perimortem.Graphics/." "$PACKAGE_GRAPHICS/"
cp -RL "$REPO_ROOT/packages/ttx/Perimortem.Math/." "$PACKAGE_MATH/"
cp -RL "$REPO_ROOT/packages/ttx/Perimortem.Memory/." "$PACKAGE_MEMORY/"
cp -RL "$REPO_ROOT/packages/ttx/Perimortem.System/." "$PACKAGE_SYSTEM/"
cp -RL "$REPO_ROOT/.bin/bin/packages/ttx/Perimortem.Graphics/1.0/." "$PACKAGE_GRAPHICS/"
cp -RL "$REPO_ROOT/.bin/bin/packages/ttx/Perimortem.Math/1.0/." "$PACKAGE_MATH/"
cp -RL "$REPO_ROOT/.bin/bin/packages/ttx/Perimortem.Memory/1.0/." "$PACKAGE_MEMORY/"
cp -RL "$REPO_ROOT/.bin/bin/packages/ttx/Perimortem.System/1.0/." "$PACKAGE_SYSTEM/"
mkdir -p \
  "$PACKAGE_GRAPHICS/native/x86_64-sysv-linux" \
  "$PACKAGE_MATH/native/x86_64-sysv-linux" \
  "$PACKAGE_MEMORY/native/x86_64-sysv-linux" \
  "$PACKAGE_SYSTEM/native/x86_64-sysv-linux"
cp -L "$REPO_ROOT/.bin/bin/packages/ttx/libperimortem_graphics.a" \
  "$PACKAGE_GRAPHICS/native/x86_64-sysv-linux/package.a"
cp -L "$REPO_ROOT/.bin/bin/packages/ttx/libperimortem_math.a" \
  "$PACKAGE_MATH/native/x86_64-sysv-linux/package.a"
cp -L "$REPO_ROOT/.bin/bin/packages/ttx/libperimortem_memory.a" \
  "$PACKAGE_MEMORY/native/x86_64-sysv-linux/package.a"
cp -L "$REPO_ROOT/.bin/bin/packages/ttx/libperimortem_system.a" \
  "$PACKAGE_SYSTEM/native/x86_64-sysv-linux/package.a"

if [ -L "$PACKAGE_SERVER" ]; then
  echo "Packaged server must be a real file, not a symlink: $PACKAGE_SERVER" >&2
  exit 1
fi

echo "==> Installing npm dependencies..."
cd "$SCRIPT_DIR"
npm install --silent

echo "==> Compiling TypeScript..."
npm run compile

if [ "$SYNC" -eq 1 ]; then
  echo "==> Development extension synchronized."
  exit 0
fi

echo "==> Reading extension manifest..."
PACKAGE_NAME="$(node -p "require('$SCRIPT_DIR/package.json').name")"
PACKAGE_PUBLISHER="$(node -p "require('$SCRIPT_DIR/package.json').publisher")"
PACKAGE_VERSION="$(node -p "require('$SCRIPT_DIR/package.json').version")"
VSIX_NAME="${PACKAGE_NAME}-${PACKAGE_VERSION}.vsix"
VSIX="$VSIX_DIR/$VSIX_NAME"

echo "==> Removing an existing output for this version..."
rm -f "$VSIX"

echo "==> Packaging extension..."
npm run package -- --out "$VSIX" --no-rewrite-relative-links

if [ ! -f "$VSIX" ]; then
  echo "Expected VSIX was not created: $VSIX" >&2
  exit 1
fi

echo "==> Packaged: $VSIX"

if [ "$INSTALL" -eq 1 ]; then
  echo "==> Installing extension into VS Code..."
  code --uninstall-extension "$PACKAGE_PUBLISHER.$PACKAGE_NAME" || true
  code --install-extension "$VSIX" --force
  echo "==> Done. Reload VS Code to activate the new version."
fi
