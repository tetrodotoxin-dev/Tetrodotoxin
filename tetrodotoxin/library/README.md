# Library

Library is the reusable execution language inside a larger Tetrodotoxin
project. It is where code, data structures, algorithms, native interfaces, and
the behavior hosted by Apps, Scenes, or Shaders come together.

You can write scalar expressions, Functions, Structs, shared Objects,
Enumerations, and Generic containers without leaving the semantic world used by
the rest of the platform. A Package refers to the real Library Type, the editor
navigates to it, and an LLVM or Vulkan Terminal can consume it from the context
that owns its execution. There is no translated Library shaped IR between those
experiences.

Library keeps its own rules for values, access, receivers, construction, and
Generic materialization. It shares exact identities, Layouts, Packs,
Addressables, and Callables through TTX wherever another language or tool needs
to participate.

Three cooperating parts keep that meaning reusable. The Language model owns
the graph people and tools query. The Interpreter reads Library source into
that model. The Archive producer walks a completed Monograph and records the
facts needed to reconstruct a fresh one later. Embedding Library in Scene or
Shader therefore reuses its Types, Functions, and value flow without importing
its source reader or choosing its persistent format.

Canonical grammar reference: [Library.g4](grammar/Library.g4).

```ttx
// A reusable Library source.
dialect : Library;

public twice : func = [.value : U64] -> U64 {
  return value * 2;
}
```

The guide starts with the expressions and Types you meet while reading Library
source, then grows outward into declarations, control flow, Packages,
persistence, and native compilation. If you are looking for one feature, the
headings are designed to work as a reference. If you are meeting the language
for the first time, following them in order builds the model gradually.

## Names and access

Every Library access evaluates the one Expression on its left. An Expression
exposes its exact semantic result separately from its output Type. Ordinary
value operations use the output Type, which always proves the Library Type
protocol. An Expression whose result is a semantic Type retains that identity
for the next access but has no value output Type and cannot enter Pack flow.

Library uses punctuation to select separate semantic domains:

| Syntax                               | Meaning                                                       |
| ------------------------------------ | ------------------------------------------------------------- |
| `expression.name`                    | select one named value from the receiver Pack's Layout        |
| `expression::Type`                   | produce one exact Type result with no value output            |
| `receiver -> callable(arguments...)` | fit one argument Pack and invoke one Callable                  |
| `value.[names...]`                   | select and reorder named Pack values                           |
| `access[index]`                      | produce one writable indexed address with the element Type    |
| `value:[index]`                      | return an element value or its default                        |
| `value:[start, count]`               | return a Ranged Pack whose size is known during linking        |
| `option!`                            | return the payload or a fresh element default                  |
| `option?`                            | continue with the payload or return empty flow                 |

These domains never fall through to one another. A Field, Callable, and nested
Type may share a spelling because the operator already states which category is
being requested.

A declaration that requires a Type retains a type route with no identity. It
does not retain an access Expression. The route may carry one optional Generic
argument Layout, and each Type entry may recursively contain another route.
Without an argument Layout, the terminal route must select a Library Type. With
one, it must select a Generic formula that creates a Library Type from those
arguments. Intermediate Package, Monograph, Alias, source, and other Abstract
contexts need not pretend to be Types. The route can stay unresolved until
linking without pretending to be a runtime value or postfix access expression.

### Address access

`.` evaluates its receiver and selects one exact TTX Addressable from that
identity. A Library Addressable forwards the explicit query to its exact
Library Type as Self, while an exact Library Type makes the corresponding
Static query. These are Library protocol rules, not behavior inherited from TTX
Type or Addressable. An Addressable receiver can select state Fields and const
Fields owned by its Type. An exact Type receiver can select ordinary Static
Fields and const Fields. An exact Source receiver can select its ordinary
Static Fields and const Fields. Arbitrary computed values do not provide
mutable member access. The operation creates no group Type.

```ttx
packet.width
self.progress
foreign.external_counter
```

The selected mutable Addressable identifies one semantic address and its Type.
A compiler may realize it as a stack location, an offset from an inline Struct,
or an offset from an Object reference. A const Field instead identifies one
value completed during linking. Selection through its Type, an Addressable
instance, or Source returns that same foldable declaration value. It never
creates storage relative to the receiver.

Source cannot be instantiated and rejects state Fields. Its ordinary Fields
are Static values with global construction and lifetime. Structure and Object
Type results expose their ordinary Static and const Field categories. State
Fields require one exact Addressable receiver.

The caller has private authority for every Composite in its Definition host
chain. That chain authorizes members selected from an explicit receiver. It
does not supply an implicit receiver or create another lookup path. A hosted
Function still writes `self.field` or selects the Field through another
explicit value. A Static Function cannot read a host Field as a bare
identifier.

### Type access

Postfix `::` advances a contextual Type route:

```ttx
Graphics::Image
System::Terminal
Option[Graphics::Image]
```

It evaluates its receiver and asks that exact semantic result for the next
context. Package, Monograph, and namespace results may continue through another
`::`. A Static invocation requires the terminal result to be one Library Type.
A Package Source Alias continues to retain its real Monograph. When that source
publishes a root Type matching the authored Source route, expression Type access
selects that Type without changing what Package and `using` queries observe.
The access produces no runtime value. An ordinary value cannot use `::`.

Contextual declaration routes through Alias, Package, Monograph, Library
source, and Type objects remain references with no identity. They do not become
Expressions.

Qualification preserves the original caller authority across every segment.
An Alias is opaque to access and declaration operations: they may only ask it
to resolve. Resolution may reveal another identity, but it does not add that
identity to the caller's host chain or transfer its private authority.

### Callable access

`->` is the Callable access and invocation operator:

```ttx
Packet -> create(width, height)
packet -> resize(width, height)
System::Terminal -> write_line(message)
```

An invocation evaluates one receiver Expression and retains one parenthesized
argument Pack. An exact Library Type result selects the Static Callable
registered on that Composite. A typed value receiver asks its output Library
Type for the registered Self Callable. The caller's Definition host chain
remains unchanged while making that selection: it admits the receiver's private
surface only when that exact Composite is already in the chain. Resolving an
Alias never transfers private authority.

Static and Self are properties of each Callable's parameter Layout. A Callable
is Self exactly when parameter entry zero is the reserved `self` Addressable
with the receiver's exact Type. Otherwise it is Static. A Composite admits at
most one Callable for each spelling and receiver role, rejecting a duplicate
during registration. Static and Self Callables may share a spelling. The
invocation therefore selects one registered Callable and only then fits its
argument Pack against the remaining parameter entries. It never constructs an
overload set or reports ambiguity during a call.

A Callable is not an Addressable and never appears in a value Layout. Callable,
Addressable, and Type registration are independent spaces, so sharing a
spelling across those categories creates no collision or fallback. The
invocation is a Pack whose output follows the selected Callable's complete
result Layout, including an empty Layout or one with several entries. A scalar
consumer can use it only when that Pack proves one exact result Type. `->`
introduces neither
an implicit receiver nor a universal member resolver: `.`, `::`, and `->`
continue to ask their distinct semantic questions.

## Packs and Layouts

A Pack carries produced value flow. It retains the real producer identities and
exposes one output Layout for directional fitting. A Layout is a descriptor
with no identity. It promises the ordered shape accepted or exposed by a Type,
declaration, Function, or Pack. Producing several values therefore remains
fluid Pack flow rather than materializing an anonymous aggregate Type.

Parentheses group Packs and brackets describe Layouts:

```ttx
()                              // empty Pack
(value)                         // the same Pack as value
(left, right)                   // positional Pack
(.x = left, .y = right)         // named Pack

[]                              // empty Layout
[U64, Bool]             // positional Layout
[.x : U64, .y : Bool]   // named Layout
```

A Function always has an empty or Named parameter Layout. Its result may use
any empty, scalar, positional, or Named Layout:

```ttx
public pair : func = [
  .left : U64,
  .right : Bool,
] -> [U64, Bool]

public classify : func = [.value : U64] -> [
  .accepted : Bool,
  .adjusted : U64,
] {
  return (.accepted = value > 0, .adjusted = value + 1);
}
```

The leading `.accepted` and `.adjusted` spellings are not postfix Address
access because they have no receiver. `.accepted : Bool` names a descriptor
slot, while `.accepted = expression` names supplied Pack flow. A slot name need
not be the semantic name of its producer, and fitting still returns that exact
producer without a renamed value or Alias. Keeping `:` for descriptors and `=`
for Packs also reserves `.name : Type = expression` for an explicitly typed
default and `.name := expression` for an inferred one.

Both forms share rules for empty forms, separators, trailing commas, positional
and named entries, and duplicate names. They do not share one semantic owner. A
Generic application accepts a Layout of Type references and literal Constants. A
Function signature accepts descriptor Types and parameter names. Parenthesized
value flow accepts Packs. A Call requires those parentheses. Another context
may omit them when its grammar remains unambiguous.

A receiving declaration or operation fits the Pack's complete output Layout
directionally against the descriptor it requires. `()` fits `[]` without
becoming a Type or sharing Pack identity.

Swizzle selects named Addressables and returns their values as one reordered
positional Pack:

```ttx
state dimensions : Fixed[U64, 2] = packet.[width, height];
```

The result is a Pack over the real selected producers. It becomes
`Fixed[U64, 2]` only because the receiving declaration chooses that
Type. The swizzle itself creates no aggregate Type.

Plain brackets are reference access on `Access[T]`. They never substitute a
default address:

```ttx
access[index]
access[start, count]
```

This form does not introduce a Library `Option` Type. A scalar selection is an
Expression whose Pack produces one writable address with the exact element
Type. Runtime bounds determine whether that address is engaged. Assignment
writes through an engaged address and leaves the receiver unchanged otherwise.
A ranged selection produces one writable Ranged target with exactly `count`
element slots. Its bounds decision is atomic: either the complete interval is
engaged and assignment writes every supplied value in order, or no value is
written. The receiving target owns complete Pack compatibility, so Assignment
does not inspect Index, Slice, View, Fixed, or another concrete carrier.
`+=` and `-=` remain scalar operations. Use `:[...]` when missing elements
should instead produce defaults.

Colon bracket value access selects values. A missing scalar element yields its
Type default. A ranged selection requires its count to fold during linking to
one supported nonnegative integer and returns a Ranged Pack with exactly that
many element values. Each position performs the same bounds decision as scalar
selection and produces the exact element Type default when missing. This is
equivalent to lowering `View::Bytes::operator[]` for each selected index rather
than clipping the requested interval. It does not materialize `Fixed`, `View`,
or an anonymous aggregate Type merely to carry the range. Neither form
preserves writable `Access` in its result:

```ttx
bytes:[4]
bytes:[4, 16]
```

The operands must still have integer Types. A scalar index that cannot represent
a valid position selects the same safe default. A range count that does not
fold to a constant or cannot represent a supported nonnegative count is a semantic
error, as is another operand Type.

## Built in Types

Library provides these scalar families:

* `Bool`
* `S8`, `S16`, `S32`, and `S64`
* `U8`, `U16`, `U32`, and `U64`
* `R32` and `R64`

These are Library refinements of its one Type protocol. `Bool` proves the
Library Flag contract, and the integer and real families prove Library Signed,
Unsigned, and Real contracts. TTX does not define those scalar categories or
make another Dialect's Type participate in Library operations.

A Flag Type interprets the first completed value in a Pack as active or
inactive. `Bool` supplies Library's standard Flag Layout and Constant
representation, but control flow and logical operations query the Flag
protocol. They do not select `Bool` or inspect its storage.

Library has no zero value Type. An authored `[]` is the empty result Layout and
an empty Composite is a Static namespace rather than an instantiable value.

Scalar operations require the exact resolved Type identity expected by that
operation. Library does not silently widen, narrow, retag, or reinterpret a
Constant to make an operation legal.

Use `new[Target](value)` when a program intends to cross between Signed,
Unsigned, and Real scalar Types:

```ttx
state width := new[S64](image_size.width);
state sample := new[R32](pixel.red);
```

This conversion is total. Integer results saturate at the target bounds. Real
to integer conversion truncates toward zero and maps NaN to zero. Integer to
real and real to real conversion use the destination precision. Keeping this
operation explicit lets ordinary arithmetic continue to require exact Types.

Generic formulas describe reusable Type families. A formula is not itself a
Type. Applying its ordered arguments materializes one exact Library Type. Type
arguments must themselves resolve to Library Types and may recursively apply
another formula:

```ttx
Fixed[U8, 64]
View[U8]
View[Fixed[U8, 4]]
Access[U8]
Implementation[Graphics::Pipeline::TexturedQuad2D]
Range[U64]
Option[View[U8]]
Result[View[U8], ParseError]
```

`Fixed[T, extent]` requires its `extent` to be a positive exact `U64`
value known during linking. Every generated container Type requires an element
Type with a nonempty Layout. `View` is a borrowed contiguous view.
`Access` additionally carries the language's writable contiguous capability.
`Implementation[Requirement]` accepts a real Library Object when that Object's
semantic owner satisfies the exact requirement through a higher order TTX
Interface relation. It retains that Object through one explicit erased value.
Native Terminals derive an immutable Projection for the accepted Object and
requirement, while code that keeps the concrete Object uses direct lowering.
The empty Implementation value is the ordinary unconfigured state and adds no
allocation.
`Range` describes a lazy ascending integer sequence. `Option[T]` represents a
value that may be absent in an otherwise nonnullable language. The Option Type
always has a nonempty Layout. Its state either carries one exact `T` or carries
no payload. Native Library targets normally use the Perimortem value carrier:
one inline payload slot followed by its selected state. An Option over one
nonnull authored Object uses the invalid null handle as its absent state and
therefore remains one word. Empty capable `Object[T]` retains the ordinary tag.
The payload is live only when selected, and Option adds no allocation,
reference count, or shared identity.
`Result[T, E]` stores exactly one live value or error alternative in an inline
union followed by Bool state. `T` and `E` must be distinct nonempty Types so raw
received flow selects exactly one alternative. Result adds no allocation or
shared identity.
An explicit empty list applies a formula with no arguments.
Omitting the list instead requires the route to name a Type. Applying the same
formula to the same semantic arguments returns the same Type identity.

Generated contiguous Types own their intrinsic Self Callables as part of the
same semantic graph. Every Library Type publishes authored and generated
Callables through one callable surface. A Generic installs its required
Callables when it materializes the exact Type, so lookup, reflection, and
completion enumerate the same identities regardless of their origin.
`view -> get_size()` and `access -> get_size()` return the runtime element count
as exact `U64`. `is_empty()` reports whether that count is zero and
folds for immutable Views. `fixed -> get_view()` borrows the complete Fixed
storage without changing its read only authority. Byte literals remain Fixed
values and therefore use this explicit conversion when a View is required. A
writable Fixed Addressable may additionally produce `Access[T]` with
`fixed -> get_access()`. Neither operation copies the Fixed, and every returned
borrow is valid only while its backing storage remains alive.

`view -> slice(start, count)` and `access -> slice(start, count)` return one
read only `View[T]`. When `start` is within the receiver, its size is the lesser
of `count` and the available suffix. A start at or beyond the receiver size
returns the empty View. Access deliberately loses write authority through this
operation. This borrowed subview is distinct from `:[start, count]`, which
produces exactly `count` independent values and supplies defaults outside the
receiver.

`Object[T]` is the empty capable worker local managed buffer formula. Its
one word handle retains one Bibliotheca allocation while capacity remains
recoverable from that allocation. `object -> get_capacity()` returns the
allocated element count. `get_view()` borrows every allocated element, while
`get_access()` borrows the same writable buffer observed by every alias.
`reserve(count)` is a no operation when the current capacity is sufficient. Otherwise
it replaces that receiver handle with a larger copied buffer and returns Access
covering the new capacity. Other aliases retain the old Object. `is_shared()`
reports whether another owned handle retains the current buffer, and `clone()`
explicitly replaces a writable receiver with an independent buffer copy. Empty
Object storage returns zero capacity and empty bounds without exposing its
internal null representation. Buffer element Types may be scalar or Structures
whose recursive Fields own no Objects. This restriction belongs to buffer wide
destruction. Authored nonnull Objects still destroy their contained Object
Fields recursively.

`Dynamic::Bytes` is the worker local copy on write byte value. Its native
carrier is `Object[U8]` plus one logical size. Copying the value retains
the Object, while a writable operation detaches shared storage before exposing
it. Capacity remains owned by Bibliotheca rather than duplicated in Bytes.
`bytes -> get_view()` borrows the complete contents and
`bytes -> slice(start, count)` borrows the clipped suffix using the same rules
as View. Bytes is an inline value whose Self Callables receive its address.
Transformations update that receiver and return the same reference with the
scalar result `self`, enabling chains without copying the Bytes carrier. Before
writing, Bytes reserves the required capacity. Growth already produces a
private Object. When existing capacity is sufficient, `is_shared()` selects
`clone()` before writable Access escapes. Copy on write policy therefore
belongs to Bytes rather than Object.
The Memory Package owns `copy`, append, concat, resize, shrink, clear, and
reserve behavior directly over Object. It exposes no Access that could bypass
the logical size. Static `Dynamic::Bytes -> concat(left, right)` and receiver
`bytes -> concat(view)` may share one spelling because they have distinct
receiver roles. Byte Views and Dynamic::Bytes are standard interchange
carriers, so receiving those exact contracts across Library source roots does
not depend on the roots sharing one Generic materialization cache.

Structure ownership is derived recursively from its exact Fields. Object Fields
retain and release their Core handles, so a runtime backed value needs no
native lifecycle Attribute or compiler specific hook.

Each Library root Generic owns its canonical materialized identities in that
root's source transaction Arena. The Monograph reaches them through its root
vocabulary and releases the whole graph with that Arena. Materialization
therefore owns no source, parser, diagnostic, or general interpretation
context, and the installed Dialect retains no semantic identity cache.

Generic rejection is a typed, source free result owned by the formula. The
authored TypeReference maps that result to the exact retained argument Anchor
and reports it through the operation Cursor. Silent resolution is reserved for
repeated Alias probing. Every committed consumer uses the reporting path.

Option construction belongs to target fitting:

```ttx
state absent : Option[Result] = ();
state present : Option[Result] = result;
```

The empty Pack creates the state with no payload. A Pack accepted by `T` creates
the state that carries its value. Option has no `some` or `empty` construction
Callables.

The receiving Library Type owns both admission and any value construction that
admission requires. Pack asks that protocol and never inspects Option or another
concrete target. Ordinary Types retain exact Layout fitting, while Option extends
the same query with its absent and present states. Adding another target
conversion therefore changes only the Type that defines it.

Result uses the same receiving protocol. A Pack accepted only by `T` constructs
its value state, while a Pack accepted only by `E` constructs its error state.
Flow accepted by both alternatives is ambiguous and rejected. Result has no
named construction Callables.

This is Pack fitting rather than Layout fitting. `[]` does not fit
`Option[T]`, and Option never acquires an empty Layout. Its absent state can
produce `()` only through the flow control owned by postfix `?`.

Option and Result are built in Library Generic Types rather than standard
Packages. Option represents recoverable absence without making Objects nullable.
Result represents one handled error Type that cannot be discarded by
propagation. A user defined fallible operation is a Static factory returning the
appropriate sum. Object initialization itself never publishes a partly
initialized value.

### Default values

Every completed Library Type admitted to value flow owns one total default
construction operation. The exact Type implements that operation. No central
kind switch, visitor, semantic Default object, Monograph path, or copied Type
inventory decides on its behalf. The caller supplies only the transaction Arena
where that Type creates its Pack.

`new[T]` with no argument list asks that exact Type for the same default used by
an omitted Field value, a missing scalar slice element, and postfix `option!`:

```ttx
state count := new[U32];
state block := new[Fixed[U8, 8]];
state session := new[Session];
```

The selected Type must be source admissible and have a nonempty Layout. Bare
`new` is invalid. An explicit empty argument list is also invalid, so `()` does
not become a second spelling for default construction. The language defines
each default independently of the storage chosen by a compiler:

* `Bool` is false and numeric Types use zero.
* An Enumeration uses its underlying zero value even when no case names
  zero.
* `View[T]` and `Access[T]` use empty read only and writable views respectively.
* `Implementation[Requirement]` contains no selected Object.
* `Range[T]` uses the empty range.
* `Option[T]` uses the state with no payload and does not construct `T`.
* `Result[T, E]` uses the value state containing the default of `T`.
* `Fixed[T, count]` contains `count` default `T` values.
* A Structure initializes state Fields in source order from each Field's
  authored initializer when present and otherwise from that Field Type's
  default.
* An Object default is one new nonnull Object initialized by the same Field
  rules.
* A route ending in an Alias resolves it first and asks the represented Library
  Type. Alias itself owns no default behavior.

Type selection remains outside value flow. `Descriptor` is consequently an
ordinary source name rather than a reserved internal Type. Before construction,
Library asks each completed value Layout whether its real Type and Addressable
edges reach terminal leaves. Target storage must also remain finite. `Option[T]`
and `Result[T, E]` store inline alternative slots, so neither can make a
Structure recursively contain itself by value. Option can break an Object
construction cycle because an Object payload is a finite reference carrier and
the absent state does not construct that referenced identity.

Cleared memory may make initialization faster, but it does not define these
defaults. Every initializer required by the Type still runs. An empty
`View[U8]` is still one View value rather than a Pack with no values.

A missing scalar `value:[index]` returns the element Type's default. Every slot
of `value:[start, count]` applies that same rule independently, so the result
always contains exactly `count` values even when the requested interval crosses
either bound. Slice operates only on Types that provide contiguous storage.
Postfix `option!` returns its payload when present and asks the same Type
protocol for a default `T` otherwise. Applying it to an Option of an Object Type
more than once can therefore create a different Object each time. The Option
itself does not change.

### Integer ranges

`start...end` constructs `Range[T]` when both endpoints have the same exact
Library Signed or Unsigned integer Type `T`. The sequence is half open and
advances by one, so it contains `start` and stops before `end`. It is empty when
`start` is not less than `end`.

A Range is lazy value flow rather than contiguous storage. It does not become a
`View`, an `Access`, or an anonymous aggregate Type. Library does not widen the
endpoints or use a general iterable registry. A `for` statement accepts either
one Range or one contiguous value and fits its read only entry binding against
the input's exact element Type. Iterating a View or Access visits its current
runtime count in ascending index order without changing the input's authority.

## Source and Composite Types

Each Library Monograph owns one generated Source Type with an empty instance
Layout. Top level declarations enter its Static surface. Instance Fields
cannot. The exact `source` route returns that Source, while ordinary Monograph
lookup forwards only its externally visible Static entries.

Every Composite and Enumeration is a defined Type and retains exactly one
Definition. Source, Structure, and Object follow that same rule, while Fields,
Functions, and authored Aliases retain Definitions without changing their TTX
categories. A Definition contributes source facts and host authority to the real
semantic identity and never becomes a competing declaration identity or graph.
Composite owns member categories, Layout completion, and lifecycle barriers
without becoming another declaration model.

Those barriers query the real Library Type, Addressable, and Callable retained
in each category. Enumeration, nested Composite, Field, and Function own their
phase work, while immediate or generated identities answer with no delayed
work. Composite therefore preserves one closure order without enumerating every
concrete declaration kind. Instance Layout contribution and recursive Layout
termination follow the same owner rule.

Each Monograph creates and retains its Source with one generated Definition.
That Definition uses the reserved name `<source>`, which cannot be emitted. It
also retains the exact opening Documentation and truthful source envelope
Anchor supplied by Environment. It
fabricates no authored Tokens. Its
host is the owning Monograph and its Visibility is public.

Top level mutable Field declarations are Static Addressables owned by that
Source. They retain the ordinary Field visibility, mutation policy, Type, and
initializer contracts. They have global construction and lifetime and never
enter the empty Source instance Layout. Top level const Fields retain one value
completed during linking and use no Source instance storage. A constant domain
may still retain the memory needed to represent that value. Root Functions may
resolve those exact identities as bare source names. Private Fields remain
limited to their owning source context.

Type aliases use the exact declaration
`public|private TypeName : alias = TypeRoute;` in the generated Source or an
authored Structure. The receiving Composite retains one exact TTX Alias in its
Type category and authored order. Its target is the Type selected through that
Composite's private local and enclosing source context, so an Alias may
name a private Type without making that target independently public. Public
Type lookup exposes the same Alias identity while private aliases remain local
to their containing Composite.

Authored alias documentation leads the target documentation. An alias without
local prose borrows the target documentation directly, avoiding an empty
wrapper while preserving the visible documentation chain.

Source, Structure, and Object bodies use the same declaration language. Each
Field, Function, Struct, Object, Enumeration, or Alias becomes its exact
semantic identity, and the receiving Composite routes that identity by its TTX
category. The Monograph reaches those declarations only through the Source, so
there is no parallel declaration tree.

Completion follows the relationships in that tree. Every reachable declaration
Type, including Enumeration storage, settles first. Every reachable Callable
signature then settles before any Field or initializer expression. An inferred
Field adopts the exact completed Type of its initializer, while an explicit
Field fits its initializer against its declared Type. All Fields and
initializers settle before Function bodies.

Library owns the grammar that applies to a complete source. `using` resolves one
authored route during linking and retains the exact selected object as a
borrowed Source fallback context. It creates no Alias, Definition, copied
declaration, binding inventory, or provider closure.

Source parses the possible leading Comment once before selecting each root
declaration form. The selected Import, Definition, or Foreign owner retains that
same Documentation rather than probing and reparsing the prefix. Source also
owns the one Foreign context for its transaction. Repeated same ABI blocks merge
atomically into that identity. Exact repeated State or Callable declarations
keep their first identity, while a changed declaration fails the later block.
State and Callable names remain separate query categories, and neither category
is copied into Monograph vocabulary.

The Source retains the exact Documentation that opens the Library source.
A Package member Alias can therefore route through `source` to one documented
root Type without copying the prose or becoming a Type itself.

The Library Monograph exposes its exact Source and installed Library Dialect
directly, with no category scan or shadow source edge. A root Function's
Definition host is the Source, which already reaches the Monograph that owns
intrinsic vocabulary. Source owns imports and completion and publishes textual
errors through its transaction Cursor. The Function retains no duplicate
source, host, or parent edge.

### Embedded Library layers

A top level Library source is already a Library layer. Scene can also contain a
Library child because Scene source directly authors Library state and behavior.
Shader contains a Library child for the same reason: its Stages directly author
Library Functions, expressions, Blocks, and Flow. Reusing the installed language
keeps Generic Types such as `Option[T]`, `Fixed[T, count]`, and `View[T]`
consistent everywhere they appear.

Each child has its own Source context and intrinsic vocabulary. Scene places its
Object, state Fields, helpers, and lifecycle Functions there. Shader places its
executable Program Types and Stage Functions there. Only the outer Dialect
appears as a Package member, while Library tools and Terminals can inspect the
real child directly. Shader Bridges retain exact Library Type edges without
copying them.

Scene and Shader remain responsible for the parts of their languages that are
not Library code. Scene owns `emit`, while Shader owns Pipeline contracts, storage
roles, and Bridges. Expressions and ordinary statements still follow Library
rules without making Library depend on either outer Dialect.

## Definitions

Every ordinary Library member begins with one shared Definition:

```ttx
@tooling("entry") public twice : func = [
  .value : U64,
] -> U64 {
  return value * 2;
}
```

The Definition greedily retains Documentation, every Attribute, one exact
Visibility, ordered evaluation modifiers, the name, and the qualifier after
`:`. That qualifier selects the declaration category, and the containing
Composite retains the resulting identity. A Type route or `=` begins a Field.
`alias`, `enum`, `struct`, and `object` begin their Type forms, while `func` begins
a Callable. Each resulting Field, Structure, Object, Enumeration, or Function
retains that same Definition while its concrete language form owns the
remaining grammar and validation.

Definition remembers the host that admits the declaration. Ordinary members use
their containing Composite, while Source uses its Monograph. That host explains
where the declaration came from and which private members it may access. It is
not a universal parent link or an implicit receiver.

Defined Types retain their Definition as part of the Type identity. Fields,
Functions, and authored Aliases retain the same declaration facts while
remaining solely Addressable, Callable, and Alias identities. Once its grammar
is complete, its concrete Library owner exposes the complete Definition.
Consumers do not recover a lossy declaration projection through Abstract.

Definition owns Library Visibility and the authored Tokens. Source uses a
generated Definition and does not pretend to have authored declaration text.
Imports create no forwarding identities or declarations.

Attributes do not choose the definition category and are not rejected because
of that category. A consumer may interpret selected keys and leave all
others as authored facts. Repetition is likewise consumer policy rather than a
shared parser error.

## Fields

A Field is a TTX Addressable owned by one Composite. `state` is the sole
authored discriminator for instance storage, so only state Structure and Object
Fields enter the instance Layout. An ordinary mutable Field is Static even when
hosted by a Structure or Object. Source rejects state Fields and retains only
ordinary Static or const Fields. A const Field never enters the receiver's
instance Layout. Its initializer must fold before the Abstract DAG is complete.
Address access through its declaring Type, an Addressable instance, or Source
selects the same immutable declaration value. Visibility and evaluation policy
remain independent.

A constant domain may retain memory for its completed representation. A
default constructed Object is a valid const value only when linking can produce
its complete immutable representation. The declaration
`const object : SomeObject = new[SomeObject];` is legal only when construction
provides that proof during linking.

A Field Type must expose at least one Layout entry because an Addressable names
real value flow. `Fixed[T, 0]` and a Generic application with an empty element
Type are invalid. An empty Composite remains a valid contextual Type but cannot
become a Field, named parameter, Function result entry, or `self`.
An empty Composite can still own Static Functions and nested Types, which makes
it a natural namespace without manufacturing a value for compatibility.

```ttx
public width : U64 = 0;
private checksum : U64 = 0;
public const signature : U64 = 1;
private state updates : U64 = 0;
expose state progress : U64 = 0;
```

Visibility controls selection:

* `private` is visible only when the caller's Definition host chain contains the
  declaring Type.
* `public` is visible outside the containing Type.
* `expose state` makes state readable externally while retaining internal write
  authority.

Evaluation policy has three states:

* an ordinary Field owns Static storage and is fully writable by callers that
  can select it
* `state` owns instance storage. `public state` is writable by external callers,
  while `private state` and `expose state` require authority from the declaring
  Type's Definition host chain
* `const` is never writable and must resolve completely at compile time

Every view exposes the same Field identity. Visibility does not create a public
copy, and evaluation policy does not change the underlying TTX Addressable.

A present initializer links through the Field in its containing Type's private
context and must fit the declared Field Type. A const initializer must also
fold completely during linking. It remains one exact Pack that produces one
value rather than a separate initializer inventory.

A declaration written as `name := expression` has no declared Type to fit. The
Field retains the exact completed Type of that initializer without widening or
retagging it. `new[T]` carries its exact result Type, so an inferred declaration
may use any source admissible Library default.

## Structs

`struct` declares an inline value Type:

```ttx
public Packet : struct {
  public state width : U64 = 0;
  public state height : U64 = 0;
  private state checksum : U64 = 0;

  public area : func = [self] -> U64 {
    return self.width * self.height;
  }
}
```

The Struct's instance Layout is a named Layout over its exact state Fields in
authored order. Ordinary Static and const Fields remain on the same authored
inventory but do not enter that Layout. Copying a Struct value copies its inline
value semantics. Target offsets and padding are derived later by the compiler.

The containing Type supplies complete access to its hosted Functions and an
external view to other callers. Nested Types, Callables, and Fields remain
separate query domains.

## Objects

`object` uses the same declaration and access model as `struct` while changing
value identity and lifetime:

```ttx
public Session : object {
  expose state progress : U64 = 0;
  private state token : U64 = 7;

  public advance : func = [self, .amount : U64] -> U64 {
    self.progress = self.progress + amount;
    return self.progress;
  }
}
```

An Object value is a nonnull managed reference. Assignment asks its completed
target Expression for a writable Type instead of classifying syntax or storage
owners. Ordinary targets delegate that authority to their real Addressable,
while Index answers for its explicit writable address result. Parameter passing
and return preserve the Object reference, so every alias sees the same mutations.
Object uses the same Fields, Functions, Layout, Visibility, and Writability as a
Structure instead of defining a second member system.

Library defines Object as a nonnull reference counted identity. Copying,
passing, or returning an Object preserves that identity and retains its
allocation. Assignment and value lifetime completion release the previous
reference. The final release runs the Object payload's generated destruction,
including release of its contained Object Fields, before returning the storage.
Source code has no finalizer, weak reference, explicit release, or observable
reclamation callback.

The native carrier remains one payload pointer. LLVM emits one immutable
descriptor per Object Type containing its payload size, alignment, and generated
finalizer. Perimortem Core stores only a pointer to that descriptor beside the
allocation. Bibliotheca owns the reservation count and allocation bucket but
never becomes an Object Type or runtime vtable.

Object references remain on their owning worker. An interface between workers
may borrow read only data through a View for the duration of one completed call.
The receiving worker copies anything it retains and constructs a new Object
identity when it needs persistent managed state. Writable Access does not cross
workers, and wrapping storage in a View does not make contained Object handles
transferable. Reference cycles are not reclaimed automatically, so Object
graphs avoid owning cycles or make an owning edge explicitly breakable.

### Object initialization arguments

Inline Struct values use positional or named values and are checked against the
declaration that receives them. An Object uses the common `new[T]` default when
its argument list is omitted and additionally admits one nonempty named Pack to
replace selected state Field defaults:

```ttx
state session : Session = new[Session];
state configured := new[Session](.progress = 4);
```

The selected Object Type creates one private Object, initializes its Fields in
source order, and returns one completed Pack to the receiving declaration. The
nonnull reference becomes visible only when initialization is complete. The
explicit Type keeps inferred declarations unambiguous. Generic Initializer and
default code never inspect Composite Fields. Object alone owns the named
argument specialization through the common Library Type construction query.

The arguments to `new[ObjectType]` can name public and exposed state Fields.
Code hosted by the Object Type can also name its private state Fields. Unknown,
repeated, or inaccessible names are errors, as are Static or const Fields. A
state Field not supplied by `new[ObjectType]` uses its own initializer when
present and otherwise its Type's default. Static Fields are initialized
separately and are never inputs to construction. Omitting the argument list
requests the same Object default described above.

Arguments are evaluated in source order. The Object then initializes each state
Field once in its declared order. It uses the supplied value first, then the
Field's initializer, and finally the Field Type's default. Initialization has no
recoverable failure path, so it needs no rollback behavior. Running out of
memory is a fatal diagnostic. Cleared memory may speed up allocation, but the
language defaults still determine the finished values.

A chain of Structure or Object defaults must eventually end. Library rejects a
cycle while completing the program instead of discovering it during runtime
initialization. An `Option[ObjectType]` Field breaks an Object construction
cycle because its default has no payload. It does not make recursive inline
Structure storage finite. Construction that can reject input belongs in a
Static factory returning `Option[T]`, not in `new`.

## Enumerations

An Enumeration selects an exact Library Signed or Unsigned storage Type and
declares named integer cases:

```ttx
public Mode : enum[U8] {
  idle = 0;
  running = 1;
  stopped = 2;
}
```

Each case has its own Alias and exact Enumeration Constant identity. Two case
names may carry the same integer value without becoming the same semantic
identity.
The Enumeration default is the exact Enumeration value whose underlying
integer is zero. That representable value remains valid even when no case Alias
names it.

Every Enumeration publishes one Static const `size` Addressable and one Self
`get_name()` Callable. `Mode.size` is the exact compile time `U64` case
count. `mode -> get_name()` returns the authored case name as
`View[U8]`, or an empty View when no case names that representable
value. When several cases share that value, it returns the first authored name.
Case name bytes are immutable program data.

An Enumeration Type is itself iterable in authored case order. A value Layout
binds each exact Enumeration value:

```ttx
for [.value : Mode] in Mode {
  value -> consume();
}
```

A name Layout instead binds the exact storage value and its authored name:

```ttx
for [.value : U8, .name : View[U8]] in Mode {
  name -> consume();
}
```

The first Type must be the Enumeration's exact storage Type. The second Type is
exactly `View[U8]`. The reserved `value` and `name` binding names make
the two iteration contracts unambiguous.

## Functions and invocation roles

A Function declares one Named parameter Layout and one arbitrary result Layout
followed by a body. `[]` is the empty parameter Layout. Every ordinary parameter
uses `.name : Type`. Only the reserved `self` entry may appear first without
that spelling. A scalar Type is shorthand for a result Layout with one entry:

```ttx
public add : func = [
  .left : U64,
  .right : U64,
] -> U64 {
  return left + right;
}
```

A Function without `self` is Static. Static means there is no implicit Self
value. Source still selects it through a Type or source context:

```ttx
Math -> add(2, 3)
```

A Callable derives type binding from its signature's parameter Layout: it is
type bound exactly when entry zero is the reserved `self` Addressable. A
Function with that shape is Self. That entry has the selected receiver's exact
Type, is always passed by reference, and every following parameter is named.
The Function is selected through an addressable value:

```ttx
packet -> area()
```

The reserved scalar result `self` returns that same reference. It is the
canonical shorthand for the explicit one entry `[self]` Layout and enables
effectful chaining without copying the receiver:

```ttx
public clear : func = [self] -> self {
  self.size = 0;
}

packet -> clear() -> reset();
```

Reaching the end of a `self`-returning Function returns that reference
implicitly. `return self;` remains the explicit early exit form.

Static and Self Callables may share a name because their receiver roles
distinguish the invocation. A Composite rejects a second Callable with the same
name and role during registration, before any Call can observe the name. Both
remain Callables reached only through `->`. The parameter Layout carries the
role without a second Callable category.

## Expressions and Constants

Library expressions retain authored value dependencies and expose both their
exact semantic result and output Type. The result preserves the identity
selected by an access. The output Type states which value operations apply, and
a selected Type has no value output. Every Expression implements the Pack
support contract, so its Layout remains safe to inspect. Pack has no identity
or resolution behavior; its Layout points directly at the real semantic
producers, and an empty Layout is completed zero value flow. Scalar
expression consumers require one exact produced value and output Type, while
calls, swizzles, and slices may preserve empty output or output with several
values without inventing a group Type.
`Pack::get_value_type(index)` derives each output Type from the real producer at
that position. Calls and composed Packs map the index through their retained
producer structure. A raw Type identity remains a descriptor or contextual
query result and never proves that a value was produced.
Constants cover Bytes, Bool, signed integers, unsigned integers, and real
values.

Arithmetic and comparison operate on exact compatible scalar Types. The
keyword forms `and` and `or` alone own short circuit Flag semantics and require
matching exact Flag Types. The host neutral `&` and `|` Tokens remain reserved
for future bitwise operators and are not alternate spellings of those Library
Operations. Prefix `!value` accepts a Flag and preserves its exact Type. Postfix
`option!` accepts `Option[T]` and produces exact `T`, using the Type default when
the Option has no payload. Unary `-` accepts signed integer and real domains.
Integer overflow and division by zero are semantic failures in their owning
operation. Safe
`:[...]` selection uses a default value instead of publishing a bounds failure.

Postfix `?` makes a chain of fallible operations concise. Its exact receiver Type
owns both the continuation and escape flow:

* `Option[T]` continues with `T` when present and otherwise escapes with empty
  flow.
* An active Flag such as Bool continues with that exact Flag value, while an
  inactive value escapes with empty flow.
* `Result[T, E]` continues with `T` for its value state and escapes with exact
  `E` for its error state.

The enclosing Function must receive the complete escape Pack. Empty flow fits
`[]` or one `Option[R]`. A Result error fits exact `E` or a receiving
`Result[R, E]`. It cannot disappear into `[]` or Option. Different error Types
do not convert:

```ttx
state parsed := Parser -> parse(source)?;
return parsed -> finish();
```

The same operator works with Bool as a concise success check in a Function with
result `[]`. Propagation into a Function with several result entries is reserved
for future language support.

Binary `+` accepts exact signed, unsigned, or real operands and returns that
same Type. It does not concatenate Bytes or Views. An output owner that accepts
several byte spans exposes that operation as a Callable instead of changing the
numeric operator.

Constant evaluation may cache a result, but it never replaces the authored
expression or its exact edges. A compiler may fold a complete expression,
address selection, or indexed byte value while the authored graph remains
available to tools.

## Statements and control flow

A Function body is a Library semantic object, not a lowered control flow graph.
Each Block retains an ordered sequence of identity free Statement records. A
Statement keeps one borrowed root together with its one leading source backed
Documentation and the fixed lifecycle operations selected by grammar. For an
expression Statement, that root is the complete outermost Pack returned by the
Expression parser. Otherwise it is the exact Local, control owner, or nested
Block selected by grammar. Statement never becomes another Abstract, copies
the root's facts, or requires Block to inspect every concrete statement
category. Lowering derives target blocks and branches only after the body is
complete.

Block owns both authored body spellings. `{ ... }` contains an empty or ordered
multi Statement body. `:` contains exactly one following Statement, so the
same Block model supports compact Functions and control flow without a wrapper
or caller specific parse path:

```ttx
private classify : func = [.value : U64] -> U64 : return value;

if ready : total += 1; else : total = 0;
```

The formatter selects `:` for one directly retained non control Statement and
braces for several Statements or a nested control Statement whose braces
preserve unambiguous `else` binding. An empty result Function omits redundant
trailing bare returns. If no other Statement remains, its canonical body is
`: return;`. This is a mechanical presentation rule: nested returns and text
following an earlier return remain authored content. An empty body for any
other Block remains `{}`.

One paragraph contains Definitions, ordinary Statements, zero or more
compressed `:` Blocks, then at most one braced Block. A later item from an
earlier stage starts a new paragraph. Consecutive compressed Blocks therefore
stay together, while an ordinary Statement following them receives a blank
line.

Statement processing preserves the Library transaction stages. Parsing chooses
and retains the exact owner. Linking visits those owners in source order, makes
only preceding Local declarations visible, and validates reachability.
Finalization visits the same records in the same order without rediscovering
their concrete kinds. `Flow::Scope` carries only the enclosing Function result
Layout, member access Type, and nearest loop fact needed across those owners.
it does not shadow Block state or become another semantic graph.

A local `state` declaration creates one mutable Addressable. A local `const`
declaration requires an initializer that folds completely during linking. It
never creates mutable local storage or an assignment target. An explicit Type
receives and fits the initializer. An inferred local retains the initializer's
exact completed Type under the same rules as an inferred Field. A local becomes
visible after its declaration. Every Local, `for` entry, and match payload name
must be absent from its complete reachable lexical context. A nested Block
therefore cannot shadow a preceding Local, Function parameter, loop entry,
match payload, or another enclosing binding. A standalone `{ ... }` is itself
one Statement and retains that exact nested Block rather than fabricating a
control flow owner.

Diagnostic recovery does not change that transaction boundary. When an
explicit Local Type has settled but its initializer fails, later Statements in
the same rejected transaction may still query that Local's name, Type, and
member surface. Its value remains incomplete, the Local continues to answer
Unknown, and the failed Monograph is never published. An inferred Local has no
such recovery binding until its initializer establishes one exact Type.

`=`, `+=`, and `-=` are distinct lowest precedence Library Expression
operators selected by unambiguous TTX Tokens. They parse right associatively
after the complete tighter expression on their left and ask that exact
Expression for explicit write authority over the complete right Pack.
AddAssignment and SubtractAssignment each own their exact scalar
read modify write rule rather than being modes of Assignment or hidden nested
arithmetic Expressions. Indexed writes occur only when the selected optional
reference is engaged. No write falls through from Address access to Type or
Callable lookup.

Each write operator produces completed empty flow because it records an effect,
not a new value. It can therefore occupy an ordinary expression Statement
without a special Block parse path, but it cannot feed another operator. For
example, `a = b = value` has the conventional right associated parse and is
rejected because the inner Assignment supplies no value to the outer one.

`return` retains one Pack and fits its complete output Layout against the
Function result Layout. `return;` and `return ();` supply empty flow.
`return value;` supplies one value. Positional and named parenthesized forms may
supply several. Ordinary fallthrough is legal for an empty result Layout and
for the reserved scalar result `self`, which implicitly returns the receiver
reference. `()` fits `[]` directly. It also fits a single `Option[T]` result by
creating the state with no payload. Every other nonempty result must return on
every reachable path.

`if` and `while` consume a Pack and use its first produced value for the control
decision. That value's Type must satisfy the Library Flag contract, which alone
interprets the completed value as active or inactive. Parentheses may be omitted
when the Pack is otherwise unambiguous, and additional produced values do not
change which entry controls the branch. `for` consumes one `Range[T]` or
contiguous `Fixed[T, extent]`, `View[T]`, or `Access[T]` and fits its read only
loop binding against exact `T`. Enumeration iteration uses either of the named
Layouts defined in the Enumeration section. The exact input Type owns its
iteration contracts, so a future Type can add iteration without extending one
closed loop category list. `break` and `continue` target the nearest enclosing
loop and are illegal outside one. An `else if` retains its exact Branch as a
Statement, including its leading Documentation, rather than acquiring
synthetic braces or a second alternate representation.

`match` evaluates its input once and compares cases in source order. An ordinary
case must fold to a Constant with the input's exact Type. The first equal case
runs and there is no fallthrough. `_` is the final default case. It may be
omitted only when Library can prove that the preceding cases cover the complete
input domain.

An `Option[T]` input instead admits one value binding and one final discard
case:

```ttx
match value {
  case item : item -> consume();
  case _ {}
}
```

The first case makes the stored `T` available only inside that branch. The
discard case observes the state with no payload and introduces no binding.
Option element Types always have nonempty Layouts. General runtime Type patterns
require a real sum or dynamic Type domain. They are not meaningful for ordinary
values that already have one known static Type.

Any complete Pack or Expression followed by `;` is a Statement. Block
membership discards that output after preserving its semantic identity and
source order, so Calls, pure expressions, and assignment use one path instead
of requiring an invocation only statement category.

## Imports and resources

`using` adds the selected object as a contextual fallback for the current
source:

```ttx
using Core;
using Graphics::Utilities;
```

The route follows ordinary `::` concept access one name at a time. Alias
selection resolves to its target before the next query. Once the route is
complete, unanswered `resolve_concept` queries fall through to that exact
borrowed context. Library never enumerates or copies its declarations,
constructs forwarding Aliases, or requires the selected object to be a Package
or Library Monograph.

Linking asks every admitted fallback about each locally declared name and
rejects a second answer. If multiple fallback contexts answer some other later
query with different identities, that query resolves to Unknown. Language
extensions therefore compose as localized query contexts without a shared
registry or imported declaration table.

An embedded operand asks the exact source Package for retained bytes:

```ttx
public const signature : Fixed[U8, 4] = 0x[54 54 58 31];
public const table := $[resources/table.bin];
public const header := $[resources/table.bin]:[0, 64];
```

Library interprets a successful Resource as a Bytes Constant. Package retains
path confinement and acquisition policy. Library never opens Package storage
directly.

A Library Monograph completes only its own source. Package and Environment
barriers ensure that a context selected by `using` is already complete. Using
never discovers, links, finalizes, or owns a provider closure.

## Attributes and native publication

Public Library declarations already say which parts of a completed graph may
be used by another source. A native Terminal can follow those same semantic
routes to create its host interface. When a Function and each Type that hosts
it are public or exposed, the ABI Terminal publishes it automatically. Ordinary
TTX code therefore needs no ABI Attribute and no authored native symbol.

The generated C++ API keeps the Package, Type, and Function names that were
authored in TTX. Its implementation crosses a generated C boundary privately,
which gives C and C++ consumers one carrier agreement without making encoded C
symbols part of the friendly interface.

Definition still keeps every authored Attribute as ordered source data and
assigns no Attribute a Library wide meaning. An embedding boundary can consume
an Attribute when it genuinely needs to meet an existing platform contract.
For example, a legacy C host may require one exact symbol:

```ttx
@abi("C") @symbol("library_native")
public library_native : func = [] -> U64 {
  return 42;
}
```

That override adapts the generated Terminal to a name owned outside the Package.
It is not required for publication and should not be repeated for ordinary TTX
APIs.

The consumer decides whether `abi`, `symbol`, `retain`, or `release` is relevant,
which values and repetitions it supports, and whether the selected declaration
and target representation satisfy that request. The semantic owner does not
preconfirm those choices because another compiler or an embedding Dialect may
assign the same authored facts different policy. Unknown keys remain available
to future consumers without changing the declaration.

The native ABI Terminal checks that every published parameter and result Type
has a valid representation for the selected target. Internal TTX calls may use
a different aggregate convention from the generated host boundary. Linker
checks the completed symbol set before it emits native bytes. Function decides
neither target representation nor complete program symbol policy.

Private Callables stay internal unless an embedding contract explicitly names
one. [Linker](../linker/README.md) owns the resulting Symbol records and native
bytes.

## Persistence

Library can be stored in a Package product and reconstructed without its source
file. The complete payload keeps its public and private Types, Fields, Function
signatures, folded constants, Foreign declarations, and Attributes. It retains
foreign symbol requirements but no native provider or artifact locations,
Function bodies, expressions, control flow, or access operations.

The restored Library answers the Type, Addressable, Callable, and Constant
queries required for composition. A later build tool links ordinary native
libraries for the retained foreign symbols. A source build or live Workspace
performs another lowering.

The payload stores no parser state, process addresses, compiler caches, LLVM
IR, native bytes, live Object references, or source level debugging data.

Archive is a Terminal producer rather than a capability injected into Library
objects. Its writer walks completed semantic identities and owns Format 2 tag
selection. Its reader validates bounded records, then calls the same
Cursor independent Language factories available to any trusted producer. The
Language model contains no Archive reader, writer, tag, or persistence callback.

Restoring an Archive creates new Library objects and completes them through the
same rules used for source. The result preserves all names, categories,
relationships, ordering, Layout behavior, and other visible facts promised by
the product. Its in memory arrangement does not need to match the old process.
A Library child inside Scene or Shader belongs to the same complete graph as
its parent.

## Compilation boundary

Library ends with completed meaning. It exposes its real Sources, Types,
Callables, Blocks, Expressions, Packs, and retained semantic edges without
knowing which product will consume them. There is no compiler transaction,
lowering callback, native handle, or Terminal capability in the Dialect.

A Terminal begins from that completed graph. It walks the concrete Library
owners it supports and derives physical facts for one configured target. Those
facts may use original Abstract identities as request local keys, but they never
become another semantic model and never flow back into Library.

The LLVM Terminal currently produces CPU objects and reviewable LLVM IR from a
top level Library or a real Library child selected by another Dialect. The
Vulkan Terminal walks the same Library execution graph from Shader while using
the outer Pipeline contracts, storage roles, and Bridges to choose GPU operations
and their matching CPU bindings.

The ABI Terminal owns the shared C representation, exported symbols, and native
publication surface. LLVM consumes that agreement while owning instruction
choice, target data layout, calling convention realization, and debug
representation. Puffer supplies the target configuration and coordinates those
sibling products. Library keeps evaluation order, control flow, fitting, and
graph identity as semantic facts.

The CPU target chooses the instruction set, data layout, and calling convention.
x86-64 System V and x86-64 Win64 are separate targets. The LLVM Terminal produces
an object module for Linker without owning Package locations, operating system
startup, or linking rules.

Linux and Windows hosts provide process entry, runtime and System services,
loader inputs, and the executable format around the CPU code. Linker owns ELF,
COFF, PE, symbols, relocations, and final native files. Package owns the Archive
that stores language payloads. Runtime allocation and execution happen after
both compilation and restoration.

See [TTX semantics](../../ttx/ttx_semantics.md) for the shared contracts and
[Package](../package/README.md) for `using` and resource contexts. The
[standard packages](../../packages/ttx/README.md) apply these contracts to the
provided Memory, Math, System, and Graphics surfaces.
