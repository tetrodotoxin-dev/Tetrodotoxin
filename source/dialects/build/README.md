# Build

Build composes a workspace from independently loaded providers and submits its
real source roots to independently loaded terminals. It uses TTX Import for
source interpretation, Borrow for retained access, and Export for production.
Its orchestration has no dependency on Bazel or a shell command runner.

## Native request

The module exposes Import for `tetrodotoxin_build_input`. That request contains
named native modules, named sources, an output directory and an optional
diagnostic callback. Each source selects its importer by module name and supplies
a physical input path. Names are exact byte keys, unique within their inventory.
Puffer lends this request through the import call without constructing a second
workspace model.

Build validates the entire routing description before loading modules. It then
loads each declared module through TTX's native loader and reads each source.
The selected module must bind Import for Source's described path and text input.
The callback must supply a graph supporting Borrow, and acquisition must succeed.
The workspace retains the returned Abstract itself rather than copying its
declarations or lowering it into a universal representation.

Sources may carry incomplete domain evidence. Import success establishes the
provider's completed observation, not universal semantic validity. Each terminal
negotiates the contracts it needs and owns admission to its product domain.

## Terminal discovery

After every source has been imported and retained, Build observes every loaded
module again and attempts to bind Export. It retains native library owners, never
Query views beyond their supplying callbacks. A provider can therefore expose
capabilities from invocation-local state.

A module without Export contributes no terminal. A module advertising Export
but unable to bind its representation is an error. Every discovered exporter
receives the same workspace. Unknown or Rejected from an export fails the build,
as does discovering no terminal at all.

The module inventory chooses the providers available to this build. Capability
negotiation discovers their roles without a hard-coded list of dialect or
terminal classes. Module discovery does not scan unrelated files on the machine.

No terminal runs until all imports succeed. Terminal calls follow the declared
module order and stop at the first failure. Each terminal owns its artifact
publication policy. Build does not promise to undo effects already published by
an earlier successful terminal.

## Workspace

Workspace supports the public Build Workspace interface and ordinary Abstract
navigation. Each declared source name resolves directly to its retained root.
Enumeration lends those same roots and has no ordering guarantee. The Workspace
interface provides the selected output directory for terminals that write files.
The caller supplies a directory suitable for those terminals.

The successful Build import callback borrows the complete workspace. Borrowing
that workspace acquires its source roots and module lifetimes together. A caller
retaining a child independently also retains the workspace and releases child
acquisitions before releasing the workspace. Destruction releases every imported
graph before unloading any provider code. Access remains on the owning worker.

An unsuccessful build reports its failure through the optional request callback
and does not invoke the result receiver. Reporter state and messages remain
scoped to the build invocation.
