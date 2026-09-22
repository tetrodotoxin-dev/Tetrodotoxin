# TTX fixture data

This directory contains authored TTX inputs, binary resources, and exact process
observations used by validation tests. Each test chooses the files relevant to
its own contract. The directory as a whole is a source corpus rather than one
program.

## Directory map

| Path | Contents |
| --- | --- |
| [`library/`](library/) | Library source examples, focused rejection inputs, Foreign declarations, and a native C harness |
| [`package_resources/`](package_resources/) | Canonical source and Resource path reuse across one Package graph |
| [`products/`](products/) | Complete Package fixtures for native Library and Foreign integration |
| [`oracles/`](oracles/) | Exact Scene lifecycle observations |
| [`shader_artifact/`](shader_artifact/) | One Pipeline and Shader implementation compiled into an embedded GPU module |

## Access syntax in fixtures

The source files use three independent access domains:

| Syntax | Meaning |
| --- | --- |
| `value.name` | select one Addressable from an applicable named Layout |
| `context::Type` | select a Type through contextual resolution |
| `receiver -> callable(arguments)` | select and invoke one Callable |

For example, `self.elapsed` selects state, `Scene::Flow` selects a Type, and
`Scene::Flow -> stay()` invokes a Callable. A Callable is never selected with
`.`. Named entries such as `.color` inside a parameter or result Layout have no
receiver and are not postfix Address access.

## Package fixtures

Common source imports keep semantic identity separate from path:

```ttx
public Splash : alias = source("scenes/splash.ttx");
```

`Splash` is the local Alias granted by this source. The quoted path is resolved
relative to that source, canonicalized inside the Package root, and never
derives semantic identity.

## Resource fixtures

[`package_resources/package.ttx`](package_resources/package.ttx) imports the
same source through two equivalent paths and proves both Aliases bind one
identity. A nested third source reads `../resources/./table.bin`, while root
sources read `resources/table.bin`; all routes select one cached Resource.
`SharedA` also reads the zero byte `resources/empty.bin`.

The first 64 bytes of `table.bin` are:

```text
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/
```

These files distinguish several resource facts:

* equivalent normalized routes can share one retained input snapshot
* separate Library declarations retain separate Constant identities
* empty content is a successful Resource
* indexed value access selects reachable bytes without changing the Package
  path.

## Process observations

Unit validation runs a reserved self process fixture to prove exact standard
input, standard output, standard error, exit status, and timeout observation.
That fixture validates the process observer itself. It is not evidence for a
generated TTX Terminal.

[`scene_lifetime.golden`](oracles/scene_lifetime.golden) records deterministic
Scene clock steps, hosted state and submission order, signals, releases,
transition construction, and final process exit. Stable instance ordinals
distinguish fresh Scene construction without using process addresses. It does
not make managed Object reclamation observable.

## Shader source sample

[`shader_artifact/shader.ttx`](shader_artifact/shader.ttx) demonstrates named
Stage Layouts, portable U32 parameter transport, optional R64 execution, and
`Formats::Simple` contextual Type access. Its companion
[`render.ttx`](shader_artifact/render.ttx) owns the matching target neutral
contract. Package validation compiles both Programs through the Vulkan Terminal,
links the resulting words as read only native data, and validates the linked
bytes independently. The canonical Pipeline model is documented in
[Tetrodotoxin Pipeline](../../../tetrodotoxin/render/README.md).

See the [Library fixture reference](library/README.md),
[Package language](../../../tetrodotoxin/package/README.md), and
[TTX semantics](../../../ttx/ttx_semantics.md) for the corresponding contracts.
