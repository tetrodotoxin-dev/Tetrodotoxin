# Library fixture reference

These source files provide Library acceptance inputs and focused semantic
rejections. They are fixtures rather than applications. A test that opens one
file defines the exact production behavior it observes.

## Source files

| File | Purpose |
| --- | --- |
| [`source_acceptance.ttx`](source_acceptance.ttx) | focused production Workspace acceptance for completed Definition identities and native Function requests |
| [`value_acceptance.ttx`](value_acceptance.ttx) | focused production Workspace acceptance for defaults, scalar operations, value access, invocation, Address chaining, and Swizzle fitting |
| [`propagation.ttx`](propagation.ttx) | focused production Workspace acceptance for Option and Result fitting, defaults, typed propagation, Bool propagation, unwrap, and Match elimination |
| [`broad.ttx`](broad.ttx) | broad Library source corpus covering declarations, Layouts, access chains, expressions, control flow, Foreign, Struct, Object, and Enumeration syntax |
| [`foreign.ttx`](../products/foreign/foreign.ttx) | consolidated Foreign semantics and repeated state C interoperability |
| [`runtime.ttx`](../products/runtime/runtime.ttx) | executable Library and LLVM matrix for Locals, calls, control flow, safe access, Option and Result ownership, Enumeration iteration, and logging owned by TTX |
| [`foreign_harness.c`](../llvm/foreign_harness.c) | generated C header interoperability provider used by the validation binary |
| [`runtime_harness.c`](../llvm/runtime_harness.c) | generated C header runtime observer used by the validation binary |

`broad.ttx` intentionally contains more language surface than any one narrow
test needs. It enters acceptance only when a production Workspace can complete
the represented capabilities. Tokenization alone is not acceptance evidence.

## Executable acceptance

[`runtime.ttx`](../products/runtime/runtime.ttx) keeps the completed
Library statement owner in one Function body. Its semantic test opens the
source through the production Workspace, completes every lifecycle barrier,
then inspects the real statement identities and authored order. The ordinary
validation binary also links its generated archive through a real C observer.

## Access model

The fixtures keep Addressable, Type, and Callable access separate:

```ttx
packet.width                    // Addressable from Packet's named Layout
Graphics::Image                 // Type through contextual resolution
Packet -> identity(limit)       // Static Callable invocation
packet -> identity()            // Self Callable invocation
```

`.` selects one Addressable from an applicable named Layout. The Addressable may
later lower to stack storage, an offset relative to a Struct, an offset relative to an Object,
offset, or a folded value.

`::` traverses Abstract contexts to a Type. Intermediate Package, Monograph,
Alias, source, and Type contexts remain their real identities.

`->` owns the complete Callable invocation: receiver role, registered Callable
selection, argument Pack fitting, and result Pack flow are one source construct.

`Packet` deliberately declares both Static and Self Callables named `identity`.
Static has no implicit Self parameter. Self reserves parameter entry zero for
the selected receiver reference. A scalar `self` result returns that same
reference for chaining. `[self]` is its explicit one entry Layout form.

## Layouts and Packs

Parameters and results are Layout descriptors, while returns and swizzles
produce Packs whose output Layouts are fitted by their receivers:

```ttx
public classify : func = [.value : U64] -> [
  .accepted : Bool,
  .adjusted : U64,
]

return (.adjusted = value + 1, .accepted = value > 0);
```

The `:` spellings name descriptor slots. The `=` spellings name produced Pack
slots. Neither is postfix Address access. Positional flow remains positional,
and `packet.[width, height]` selects named values for repacking.

`access[index]` is the optional reference request for `Access[T]`. It does not
create a Library Option Type. Indexed assignment writes through an engaged
reference and leaves the receiver unchanged when it is absent.
`value:[index]` and `value:[start, count]` are safe value forms. Scalar access
supplies the element Type's default when selection misses. Range count must
fold to a supported nonnegative value and produces exactly that many Pack
entries. It has no implicit empty View fallback.

## Library declarations

`broad.ttx` contains these representative declarations:

* `Packet` is an inline Struct with public Fields, private state, and Static and
  Self Callables.
* `Mode` is an `U8` Enumeration with ordered named cases.
* `Session` is an Object whose aliases share nonnull reference identity.
* `PacketAlias` retains an Alias to the real `Packet` Type.
* `PrivateOps` is a private Struct selected as a Type context for Static calls.

Fields keep exposure separate from writability. `expose state progress` permits
external reads of the same Field identity while restricting writes to hosted
code.

## Foreign declarations

[`foreign.ttx`](../products/foreign/foreign.ttx) declares the three external categories:

1. `library_foreign_bias` is an exposed external State that cannot be written.
2. `library_foreign_state` is a writable external Addressable.
3. `library_foreign_add` is a bodyless external Callable.

The fixture selects data with `foreign.name` and invokes the Callable with
`foreign -> library_foreign_add(...)`. The `"C"` selector identifies the ABI.
Native provider selection remains outside source lookup.

## Native ABI fixture

[`foreign.ttx`](../products/foreign/foreign.ttx) and
[`foreign_harness.c`](../llvm/foreign_harness.c) describe an interaction with two calls. The
Library source increments private state by 20, copies it to imported state, adds
an imported bias of 2, and returns the result.

The corresponding C observation is:

```text
22 42
```

After both calls, the imported state is 40. `library_native` is the exported TTX
entry. `PrivateOps`, local state, and helper calls remain local to the source, while
the Foreign names are supplied by the C file through the shared ABI
header.

`library_cross_artifact` also passes one aggregate occupying 24 bytes to `llvm_large_sum`,
which is defined by the separately generated Runtime object. The two sources use
distinct semantic Structure identities, so the final native link and result
prove their shared System V memory carrier rather than relying on one LLVM
module.

The C source is compiled separately and linked into the ordinary validation
binary with the emitted TTX archive. Its unit wrapper invokes the complete
round trip rather than treating the harness build alone as execution evidence.
The Runtime integration additionally covers inline Option carriers, const View
headers, reference counted Object aliasing and destruction, and the clean
rejection of recursive inline storage.

## Focused rejection inputs

Each remaining TTX file isolates one source condition:

| File | Condition represented by the source |
| --- | --- |
| [`public_parameter_private_type.ttx`](public_parameter_private_type.ttx) | a public parameter exposes the private `Hidden` Type |
| [`public_result_private_type.ttx`](public_result_private_type.ttx) | a public result exposes the private `Hidden` Type |
| [`ordinary_bodyless.ttx`](ordinary_bodyless.ttx) | an ordinary Library Function ends as a bodyless declaration |
| [`duplicate_name.ttx`](duplicate_name.ttx) | two root declarations use the same name in one category |
| [`bare_new.ttx`](bare_new.ttx) | bare `new` omits its required Object Type |
| [`bare_return.ttx`](bare_return.ttx) | bare `return;` appears with an empty result Layout |
| [`dialect_led_callable.ttx`](dialect_led_callable.ttx) | a Library source uses a Scene lifecycle role declaration |

Foreign rejection coverage uses compact sources constructed in memory in the Foreign unit
suite so mutually exclusive failures do not require one physical file each.
The remaining files preserve reusable authored programs rather than acting as
a filesystem error taxonomy.

See the
[Library language reference](../../../../tetrodotoxin/library/README.md),
[Foreign reference](../../../../tetrodotoxin/foreign/README.md), and
[TTX semantics](../../../../ttx/ttx_semantics.md).
