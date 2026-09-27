# Native ABI Terminal

The ABI Terminal is Tetrodotoxin's common native representation. It walks a
completed Library graph and projects public Types, Callables, layouts, symbols,
and Package routes into one C compatible agreement.

C is the practical meeting point because existing languages and toolchains
already know how to consume it. The ABI Terminal emits the raw C declarations
needed for that boundary and can derive friendlier language surfaces from the
same facts. The C++ producer preserves established Perimortem names such as
`Core::View::Bytes` and canonical include paths such as
`perimortem/memory/dynamic/bytes.hpp`.

LLVM consumes this representation when it creates module Types and calling
conventions. It does not rediscover publication, symbols, or cross language
carriers. Another native producer can consume the same ABI without depending on
LLVM or reproducing its decisions.

The folder keeps those responsibilities visible:

* `representation` owns graph derived native Type and naming facts
* `c` emits the exact C declaration surface
* `cpp` emits the C++ header and forwarding implementation
* `compiler` coordinates one complete ABI Terminal transaction

## Bootstrapping the compiler

Puffer uses the same generated Packages that it can compile for everyone else.
A reviewed release Puffer therefore begins the next build, compiles the TTX
runtime needed by the source compiler, and then gives that source compiler the
completed graph and ABI products for the rest of the toolchain.

Each supported host needs its own reviewed bootstrap release because Puffer is
a native executable. The generated ABI remains shared, while Linux and Windows
prove their host executables independently before the portable C++ seed can
retire. This keeps self hosting reproducible without treating a developer's
local binary as part of the build.
