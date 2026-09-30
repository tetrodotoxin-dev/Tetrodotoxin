# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Provide separate writable output directories for debugger launches."""

from pathlib import Path

repository = Path(__file__).resolve().parents[1]
for name in ("test", "consumer", "workspace", "puffer", "pipeline"):
    (repository / ".bin" / "debug" / name).mkdir(parents=True, exist_ok=True)
