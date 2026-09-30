# Tetrodotoxin

Tetrodotoxin builds languages from independently composable source and execution
systems. Each language keeps its own meaning while exposing the interfaces
another language, editor or compiler needs through [TTX](https://github.com/tetrodotoxin-dev/TTX).

[Source](source/dialects/source/README.md) turns text into a retained lexical
stream with source locations. Dialects expose Parse to interpret tokens and
receive diagnostics through a separate capability. Consumers can inspect that stream
through its C or C++ contract, traverse it with a Cursor, or retain it across
calls through TTX Borrow. Source leaves declarations and executable meaning to
the language consuming it.

[Build](source/dialects/build/README.md) loads named providers, imports sources
into a retained Workspace, discovers terminal capabilities and exports that
workspace through each terminal. Puffer supplies the native request and presents
diagnostics. Neither runtime invokes a build system or shell command runner.

Each native provider exposes `ttx_query` and can be loaded independently through
TTX's module loader. Its runtime comes from the published Perimortem and TTX
SDKs. No sibling checkout is a build input.

## Development

Install Python 3 and the Bazel version in `.bazelversion`.

```sh
bazel build //:build //source/dialects/build:module //source/puffer:puffer
bazel test //tests:all
bazel build --config=release //:build
```

Toolchain acquires the pinned compiler and target SDKs. `--config=linux` and
`--config=windows` select native target platforms. The Source library alone
can also target `--config=web`, while Puffer and Build require native loading.
Windows SDK acquisition requires the explicit license acceptance described by
[Toolchain](https://github.com/tetrodotoxin-dev/Toolchain/blob/v0.2.1/windows.bzl).
Cross-compiling a library does not execute it on the target system.

See [contribution guidance](CONTRIBUTING.md) for ownership and validation,
[philosophy](PHILOSOPHY.md) for the composition model, and
[SDK usage](SDK.md) for independent consumption and deployment.
See [development evidence](DEVELOPMENT.md) for the active implementation checkpoint.
