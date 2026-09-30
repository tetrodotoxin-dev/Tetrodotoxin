# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Record the exact SDK archives produced by this build."""

import hashlib
from pathlib import Path
import sys

output = Path(sys.argv[1])
archives = sorted((Path(name) for name in sys.argv[2:]), key=lambda path: path.name)
output.write_text("".join(
    f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n"
    for path in archives
))
