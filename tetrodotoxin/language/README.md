# Tetrodotoxin Language

This is where a new language becomes part of Tetrodotoxin instead of another
tool beside it. A language that joins here can share source handling,
diagnostics, Packages, editor sessions, cross language navigation, and Terminal
production while keeping the grammar and meaning that motivated it in the
first place.

Tetrodotoxin calls that language a **Dialect**. For each source, the Dialect
turns authored Tokens into a **Monograph**, the lasting semantic result that
other languages and tools can inspect. Related Monographs live together in one
Workspace and can refer directly to one another.

Package, Library, App, Scene, Pipeline, and Shader all use this lifecycle. Their
objects expose the TTX Types, Layouts, and relationships useful across the
platform, while language aware tools remain free to explore their richer domain
models. No common syntax tree has to stand in for the real program.

Dialects are the input side of Toolchain composition. They determine which
meanings a Workspace can construct. Terminal producers form the complementary
output side after completion, which lets a new Dialect participate in several
products without carrying Terminal policy in its language model.

## When to implement a Dialect

A Dialect is appropriate when a source body has its own grammar, semantic
invariants, and completion work. A spelling variation over an existing language
usually belongs in that language instead. A grammar rule can be shared by
several Dialects when the complete construct and returned contract are genuinely
the same.

Adding a Dialect means owning the complete source language contract. The Dialect
defines how source is read, how names are resolved, which errors are reported,
how its result is completed, and what an Archive must store. General tools still
use the common TTX surface, while richer tooling uses the concrete Dialect.

Every top level Dialect provided by this repository publishes a canonical G4
grammar reference for authored language shape and parse order. These references
describe valid input. The toolchain does not generate or run its parsers from
them. A custom Dialect owns its grammar but does not have to express it in G4.

The shared grammar uses `Definition` for the common prefix of a declaration.
It contains Documentation, Attributes, Visibility, evaluation modifiers, and a
name followed by `:`. The concrete language reads the qualifier that follows
and decides what kind of declaration it creates. Definition records how that
object was introduced, but it is not a second declaration object or a universal
syntax tree node.

Descriptor Layouts share a second small grammar block. Language reads optional
brackets, commas, Attributes, and `.name :` prefixes, then lends each entry to
the active Dialect. Library can create parameter and result edges with Generic
Type routes, while Pipeline can create contract slots with simpler Type routes.
The punctuation is shared without turning either semantic Layout into the
other.

Every Definition also remembers the language object that hosts it. The host
records where the declaration was admitted and which private access it may use.
It is not a universal parent link. Authored Definitions gain their source Anchor
only after the complete declaration parses successfully. A language can also
create a generated Definition with a truthful Anchor, but generated declarations
never pretend that source Tokens were authored for them.

An Attribute is an ordered key with at most one scalar value. A Definition can
keep any number of Attributes, including repeated keys. Definition and the
concrete declaration preserve those facts without predicting which later system
will use them. A compiler, embedding language, tool, or other consumer decides
the meaning and validity of only the keys it actually consumes. The shared
parser only preserves the authored data.

## Dialect

A Dialect interprets one kind of source body. Environment Toolchain installs
each concrete Dialect under the exact name accepted by the source envelope:

```ttx
// Reusable source.
dialect : Library;
```

Environment consumes the envelope with one source transaction Cursor and calls
the selected Dialect directly with that Cursor, the source backed
Documentation, its Anchor, and the semantic context. The Dialect constructs one
Monograph in the Cursor's Arena and returns an optional reference. Presence
means the Dialect established a real semantic root, even if it also reported
source errors. Absence means no Monograph could be established. Environment
retains a returned Monograph for tooling and attempts to link every fact its
current graph can answer. Existing errors still keep it from finalization and
publication.

An installed Dialect is itself an ordinary TTX Abstract context. Its exact live
identity selects Monograph layers, its installed name answers source dispatch,
and its contextual resolution exposes immutable language vocabulary. A Dialect
is stateless after Toolchain construction and can serve every Workspace that
borrows that Toolchain. The operation local Cursor traverses the source and
publishes textual reports, while Workspace's local Arena handle carries the
produced root until the Workspace retains or releases it.

The context local to a source during interpretation is an ordinary TTX
Abstract. A direct source may receive the Workspace, while every source in a
Package graph receives that Package root. A Monograph is itself a contextual
Type with an empty value Layout. Its reachable Type graph includes common
Import Types, which Environment acquires before linking. The concrete Dialect
decides which contextual queries each acquired root supports.

Package can be installed in a Tetrodotoxin Toolchain without becoming an
implicit context for every source. A standalone Toolchain may omit the Package
Dialect. Package participates when the request composes a Package, acquires its
resources, or restores an Archive.

## Source transaction

Environment owns one local Arena handle and constructs the retained source
bytes, Tokenizer, and operation local Cursor in that Arena. It passes the
Cursor, opening Documentation, source Anchor, and source semantic context
directly to the selected installed Dialect. The Dialect uses
`Cursor::get_arena()` for every source backed semantic fact and returns one
optional Monograph reference from that same Arena.

Comments, Attributes, Tokens, and semantic objects may therefore retain direct
source backed views without proxying them into another domain. Workspace keeps
the Arena handle once the Dialect returns a Monograph. A result accompanied by
source errors remains useful to editor queries, and linking may still establish
independent or earlier semantic edges. Finalization and Terminal production
continue to require the complete error free island. An embedded layer uses the
same Cursor, Arena, and semantic context with its exact child language identity.

Archive reconstruction does not introduce a parallel Restoration context. A
persistent Dialect receives its destination Arena, opaque payload, and exact
Package context directly and returns one optional Monograph reference through
the same ownership contract. A fixed child receives its own payload section
with that same Package context. Source free validation and toolchain failures are written to
Perimortem Diagnostics instead of manufacturing a source Cursor.

## Dialect dependencies

Some languages build on the work of another language. Scene authors Library
state and functions in one owned child. Shader also owns a Library child for its
executable Program and Stage meaning, while selecting Pipeline contracts through
its Workspace context. In both cases, Toolchain installs each dependency once
and every source observes the same Dialect identity.

Dependencies only point from a higher level language to a lower level one.
Library does not depend on Scene or Shader. Pipeline does not depend on Shader,
and Shader does not depend on Vulkan. This rule also applies to build targets.
If two language targets need each other, the shared contract belongs in a
lower level owner.

When a required language is missing, Tetrodotoxin reports the problem before it
tries to finish the source. A dependency loop is always an invalid Workspace.

## Contextual resolution

`resolve()` follows represented identity. `resolve_concept(name)` asks the
receiving Abstract to interpret one borrowed, unqualified concept in its own
domain. A concrete grammar operator owns punctuation, resolves a selected Alias,
and issues the next segment as another query. No Abstract accepts `A::B` as one
lookup key. The consumer then proves the category required by its grammar.

Concrete languages compose shared concept questions instead of adding operator
modes to Abstract:

1. An Addressable receiver asks its exact Type for `instance`, then asks that
   authority for the authored name.
2. A Type receiver asks itself for `static`, then asks that authority for the
   authored name.
3. The consuming language proves Addressable, Type, or Callable only after the
   concept has resolved.

The resulting questions stay nested and factual. They do not flatten member
names, invocation, or receiver policy into a shared routing table.

## Monograph

A Monograph is the retained result of reading one source with one Dialect. It
provides:

* stable TTX identity
* opening Documentation
* the source transaction Arena that owns source bytes and its semantic graph
* name resolution defined by its Dialect
* link and finalize lifecycle stages

A Monograph may expose no Types, one global Type, several independent Types,
package members, entry policy, or another semantic context. Its role is the
retained root of one source, not a promise that every language has the same
shape.

A Monograph may contain a small, fixed set of child layers when the outer source
actually authors meaning owned by that child. A Scene contains one Library
layer. Shader also contains one Library layer because its Stage bodies directly
author Library execution meaning. The selected Pipeline contract remains a
neighboring Workspace identity rather than a child.

This lookup is intentionally narrow. It does not search by name, follow Aliases,
or create a wrapper around the child. A top level Monograph answers with itself.
Scene and Shader answer with their real Library child. Any other request has no
result.

Workspace owns each outer member Monograph and moves those handles through
linking and finalization with their exact source Cursors. Package owns no member
table; its restricted Library source exports Types and common Alias imports name
the graph. A fixed child layer remains owned by its outer Monograph and does not
become a separate Archive member or copied view of the same declarations.

The Monograph remains queryable for the lifetime of its Workspace. It retains
semantic facts rather than parser positions or source traversal state.

The concrete Dialect creates its Types, Addressables, Callables, lifecycle facts,
or Package members directly. An object keeps the Definition that introduced it,
while the Dialect decides which other facts remain part of the completed
language model. The Monograph exposes those real objects. Environment does not
wrap them in generic declarations or copy them into a shared member list.

Shared grammar rules return the complete semantic result requested by the
concrete Dialect. Definition preserves only its common authored prefix and is
retained directly instead of becoming an intermediate declaration model.

## Resource and Error

Two Abstract contracts shared across Dialects let a semantic context answer
requests without sharing its private policy.

### Resource

`Language::Resource` exposes stable retained bytes acquired by another owner.
It does not assign those bytes a Type or interpretation. Empty bytes are a
successful Resource.

For example, Package can resolve `$[resources/icon.png]` to a Resource while
Library constructs a Bytes Constant and Shader constructs a fact defined by its
own language from the same result.

### Error

`Language::Error` represents a contextual request that was recognized but
failed in the receiving domain. The concrete owner retains the cause. The
source consumer supplies the authored location and presentation.

An unrecognized semantic name still resolves to TTX `Unknown`. Resource and
Error therefore distinguish successful data, recognized failure, and ordinary
absence without introducing a universal error enum.

## Failure reporting

The Cursor owns all textual TTX contents and the ordered reports produced while
that source is parsed, linked, and finalized. The outer Monograph and its fixed
child layers receive that operation local Cursor explicitly, so lexical and
semantic failures point into the authored text without a retained Language
Diagnostic collection. A Monograph never keeps a Cursor after the operation.

Puffer, an editor, or another source evaluation caller presents the textual
reports written through that Cursor directly to the end user. Binary Archive
validation and other source free system or toolchain failures use Perimortem
Diagnostics, whose severity and persistence policy belongs to the host. They do
not invent an authored Token or a second Tetrodotoxin diagnostic model.

## Semantic lifecycle

One source participates in four stages:

1. The selected Dialect constructs one optional Monograph in the source
   transaction Arena and writes any source reports through the Cursor.
2. Workspace retains every Monograph and its lexical evidence.
3. Linking attempts every route the retained graph can currently answer and
   reports its own failures to the Cursor.
4. Finalization performs language work only after interpretation and linking
   complete without errors across the island.

Workspace performs these stages in one direct source call or across one Package
source graph. Retained incomplete meaning remains available to tooling, while
only completed meaning can enter a Terminal producer. Workspace interprets all
reachable source imports first and finalizes none of them until the island
links.

## Persistence

A language that supports Archives defines the data needed to rebuild one of its
Monographs without the original source. Languages that are always read from
source do not need an Archive format. Package stores each language's data under
the corresponding member and leaves its contents to that language.

Each persistent payload keeps the complete public and private facts promised by
its Dialect. It contains no executable bodies. Native objects, SPIR-V, and
other compiled implementations remain separate Terminal products.

Child layers belong to the same complete graph. The outer language stores an
opaque section for each child, and only the child's language reads and checks
that section. Payloads store no parser state, temporary caches, generated IR,
live runtime handles, or process addresses. Debug symbols and source mapping
belong to a separate output.

Archive reconstruction creates a fresh graph with equivalent observable
semantic facts and identity relations. It applies the same link and finalize
lifecycle as authored source. Package remains independent of the payload schema.

The payload is part of a Terminal product and carries reconstruction facts
rather than live graph identities. Equivalence means that a fresh Workspace
exposes the same observable names, categories, represented identity relations,
semantic edges, order, Layout behavior, completion, and language facts. The
internal graph shape and process addresses may differ.

A payload may be much smaller than a memory image because it records only the
owner facts needed for those observations. Compactness is a format benefit. It
does not define whether a Dialect is persistent.

When reconstructing a Package, Workspace creates its restricted export
Monograph, restores every member, and then reacquires the archived external Type
graph. Resources are reconstructed before a payload that refers to them. Scene
and Shader pass the surrounding context to their child layers. Exact Package
dependencies still come from the Workspace. If a child rejects its data, the
outer Monograph also fails.

The Dialect validates its complete bounded payload before returning an optional
Monograph reference from its reconstruction Arena. Workspace holds those Arena
handles, links every member, and then finalizes every member before it publishes
the completed root.
A target
representation such as LLVM IR cannot substitute for this payload because it
has already lost owner facts that were meaningful in the source language.

## Shared source envelope

The shared envelope contains required opening Documentation and one Dialect
declaration:

```ttx
// Package source documentation.
dialect : Package;
```

An explicit empty comment represents intentionally empty Documentation.
Absence is a malformed source envelope. Environment passes the exact
source backed Documentation directly to the selected Dialect, and the resulting
Monograph retains it.

Concrete body grammar starts immediately afterward. A grammar rule belongs to
the shared Language layer only when multiple concrete Dialects use its source
shape and its returned semantic contract.

See [Environment](../environment/README.md) for Workspace lifetime and
[TTX semantics](../../ttx/ttx_semantics.md) for the Abstract query model.
