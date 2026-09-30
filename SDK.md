# Native Tetrodotoxin SDK

The headers archive contains the Source and Build contracts under
`include/tetrodotoxin/dialects/`. A platform archive contains the independently
loadable Source and Build providers: `libtetrodotoxin.so` and
`libtetrodotoxin_build.so`, or their Windows DLLs and MSVC import libraries.
Perimortem and TTX remain separately imported SDKs.

The native runtime archive contains Puffer, both providers, their pinned TTX
and Perimortem libraries, and the three license notices in one deployable
directory. Platform C and C++ runtimes remain system requirements.

`MODULE.bazel` pins the compiler, target SDKs, shared headers and foundation
binaries. Product packaging lives in `build/release.bzl`, keeping the selected
platform and complete source inventory explicit while using Toolchain's compiler
and Windows export generation.

```sh
bazel build --config=release //:sdk
bazel build --config=release //source/puffer:puffer
```

Use `--config=windows` to select the Windows SDK platform. Puffer and Build
require native library loading. The Source library can be built separately for
Wasm through `//:build --config=web`.

The SDK source archive includes the active provider, Puffer, tests, build
extensions and dependency declarations. It requires Python 3 and Bazel but no
reference archive or sibling source checkout. Unpack it and run:

```sh
bazel test //tests:all
bazel build //source/puffer:puffer
```

After collecting the selected platform archives into a release directory,
`build/checksums.py` records their SHA-256 values. Pass the output manifest path
followed by the exact archive paths. Invoke it with the machine's Python 3.

## Loading

A consumer links the published TTX and Perimortem libraries and loads the Source
provider through `Ttx::Semantic::Negotiation::Library`. The loader finds
`ttx_query`, whose callback supplies an Import interface. Import accepts the
Input representation from the public Source contract and returns an Abstract
supporting Stream and Borrow. Input carries its source bytes through an Abstract
owner. Source acquires Borrow when available and otherwise copies the observed
bytes. The acquired byte owner stays alive until the Stream is released.

The C contract exports representation functions for Input, Token, Stream,
Parse and Diagnostics. A C++
consumer can derive those descriptions from the public record declarations
without linking the Source provider. `tests/consumer.c` and `consumer.cpp`
exercise this independent loading boundary.

Puffer bootstraps the Build provider with an explicit module inventory and source
selection. Build discovers Import and Export capabilities in those modules:

```sh
puffer ./libtetrodotoxin_build.so \
  --module source ./libtetrodotoxin.so \
  --module terminal ./my_terminal.so \
  --source main example.ttx source \
  --output ./products
```

The output directory must be suitable for the selected terminals. Every source
selects one named importer, and every discovered terminal receives the same
workspace after the complete import group succeeds. There is no hard-coded
terminal class list and no external build-system invocation.

For a native deployment, place Puffer and its matching TTX, Perimortem, Source
and Build libraries together. Linux binaries use an origin-relative library
search. Windows uses the corresponding DLLs. Any additional provider carries its
own declared runtime dependencies. Release retained workspace and source
observations before unloading their supplying code.
