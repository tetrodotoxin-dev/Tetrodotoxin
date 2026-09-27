# Package

Package gives a connected source graph one public Type surface, identity,
version, confined resource domain, and durable Archive. It does not own a
second table of source or dependency meaning. Sources name the graph edges they
use, Workspace owns the resulting Monographs, and Terminal generation decides
how that graph becomes a product.

Canonical grammar reference: [Package.g4](grammar/Package.g4).

## Source imports

Imports belong to the common source envelope and are available to every
Dialect:

```ttx
private LocalName : alias = source("./local/path.ttx");
public OtherName : alias =
    package(.name = "External.Package", .version = "1.0");
public PublicType : alias = source("./implementation.ttx")::Api::PublicType;
```

Each declaration creates one contextual Import Type in the importing
Monograph. Its locator is an external acquisition capability, while every
following `::` is an ordinary Type-context query. Chaining can continue through
as many published Types or namespaces as the selected interface provides.
Consumers then use the same context operations:

```ttx
OtherName::PublicType
LocalName -> static_callable()
```

`private` keeps the imported Type available only to the source's lexical
context. `public` republishes the selected interface under its local name, which
is useful when a Package wants to forward a complete source or Package root.
The same external root may therefore be imported under different local names
without giving the Monograph an intrinsic authored name.

Library `using` remains a separate forwarding choice:

```ttx
public System : alias =
    package(.name = "Perimortem.System", .version = "1.0");

using System::Key;
```

`using` forwards the selected context's public names into the current Library
source. It does not copy declarations or create more Aliases. Imports are
established before Library members link, so formatting may place `using` after
the source's import declarations without changing meaning.

## Package source

A Package source is a restricted Library source. Common imports come first;
its body then contains only Type definitions, Aliases, and namespaces used to
manufacture the public joint surface:

```ttx
/// Example package.
dialect : Package;

public Vector : alias = source("vector.ttx")::Vector;

private VectorSource : alias = source("vector.ttx");

public Dynamic : namespace {
  public Bytes : alias = VectorSource::Bytes;
}
```

Package accepts no runtime Fields, Functions, or executable statements. Those
belong to ordinary Library, Scene, Shader, App, Pipeline, or another concrete
source. An empty Package body is valid when its imported root is itself the
complete product surface, as in an application Package.

Package identity and version are product coordinates supplied by the terminal
request. A source imports that coordinate explicitly with `package(...)`; no
filesystem path or ambient repository name creates it.

## Workspace graph

Workspace starts with the Package source and walks the Monograph's reachable
Types. An Import Type identifies an external acquisition edge; ordinary Types
remain with their concrete language owner.
Every imported file receives its own source transaction Arena, Token stream,
Associations, diagnostics, and concrete Monograph. New source imports extend
the same graph; a `package(...)` import terminates the local walk at one exact
Package identity and version that the terminal has already supplied.

All local paths are resolved relative to the source that authored them. The
shared Path owner canonicalizes `.` and `..` before Storage lookup. Equivalent
spellings therefore reuse one cached file and one semantic source identity:

```ttx
public First  : alias = source("./shared.ttx");
public Second : alias = source("folder/../shared.ttx");
```

Rooted paths and paths that escape the opened Package root are rejected. Source
cycles are rejected before linking. When the graph is acyclic, Workspace links
dependencies before importers and finalizes only after every retained source
has linked without errors. Incomplete graphs remain available to editor
tooling but cannot enter a Terminal producer.

## Embedded resources

Embedded paths are source-relative for the same reason source imports are:
each file is self-contained when it moves with its neighboring assets.

```ttx
$[resources/icon.png]
$[../resources/noise.png]
$[../resources/./table.bin]:[0, 64]
```

The Cursor carries the source's canonical logical path. Embedded parsing asks
the same Path owner used by source imports to resolve and confine the request,
then Package Storage caches the canonical route. Equivalent paths return the
same retained Resource, including when different sources reach it through
different relative spellings. Empty content is a successful Resource. Library
and other Dialects assign meaning to the returned bytes; Package does not.

## Repository products

A completed Package is selected by exact identity and version. Its repository
directory has one stable semantic product:

```text
<repository>/<identity>/<major>.<minor>/
  package.ttxp
  api.h       # optional C ABI product
  api.hpp     # optional C++ wrapper
  api.cpp     # optional C++ wrapper
```

Puffer receives one authored source. Source imports decide which neighboring
language files join its graph, while embedded operands decide which images,
tables, generated data, or other files become retained Resources. Package
confinement applies to that source transaction; it is not an installed
sidecar inventory.

Repository receives the coordinate from a real Package Import and selects the
exact versioned product. The authored `package(...)` declaration remains the
only authority for the identity and version; directory names merely locate the
already materialized product.

## Archive

The current Package envelope records the completed graph rather than recreating
a manifest table. It contains:

1. Package identity and version.
2. One restricted Library payload for the Package export surface.
3. One opaque payload for each source Monograph, keyed by its deterministic
   first route from the Package root rather than an intrinsic source name.
4. Every source and Package Import Type with importer, local name, Visibility,
   chained Type route, target, and exact Package version when applicable.
5. The canonical Resource closure.
Package edges exist exclusively in the Import graph, so restoration constructs
no parallel dependency scope. Workspace restores every member, reacquires each
recorded external Type, orders the source graph, and applies the same
composition, linking, finalization, and publication barriers as authored
source.

Package arranges the envelope but never interprets a member's opaque complete
payload. Compiled CPU objects and SPIR-V modules remain independent Terminal
products rather than semantic Package state.
