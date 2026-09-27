# LLVM Terminal

The LLVM Terminal carries completed Library code from semantic meaning to a CPU
product. It follows the real Types, Callables, values, and control flow already
linked in the Workspace and consumes the native representation selected by
`Terminal::Abi`. One request can produce readable LLVM IR, an object module,
and debug information without becoming the owner of the C interface.

This is the point where raising hands the program to lowering. Library can stay
focused on what the program means because LLVM owns the instruction choices that
come next. The semantic graph remains useful to editors and other Terminals, and
the generated LLVM objects leave with the completed product.

LLVM IR is Terminal relative to the Workspace once it can be consumed without
the live semantic graph. It can still continue through LLVM optimization and
machine lowering, so the Terminal boundary does not claim that every external
pipeline has finished.

## Follow one compilation

A request arrives with a completed Library Monograph, its source facts, the
chosen CPU target, and a home for diagnostics and products. From there the
Terminal:

* selects physical carriers for the exact Library Types
* prepares native Callables and Static storage
* walks declarations and executable bodies in semantic order
* verifies the finished LLVM module
* publishes reviewable IR and object bytes against the selected ABI

Along the way, physical maps use original semantic identities as temporary
keys. A debug Type, native Function, or storage address can always be traced
back to the meaning that requested it without creating a second Type, Callable,
or control flow model. Those maps and native handles finish with the request.

## Find your way through the source

The source tree follows the journey above. The root namespace welcomes a
compilation request and returns its products. Three internal namespaces make the
middle of that journey easier to follow:

* `Lowering` walks completed Library meaning
* `Module` owns the LLVM module and its request local facts
* `Emission` creates instructions for individual operations

Their folders use the same names in lowercase, so an include path and a
qualified owner point to the same architectural place.

## Meaning and representation

Library decides that `U32` is an unsigned integer with 32 bit arithmetic
semantics. ABI maps that meaning to the common C representation. LLVM then
realizes the selected carrier in its module and preserves the same observable
behavior. The division remains explicit:

* Library owns Type identity, value flow, fitting, mutation, and control
* ABI owns native carriers, public symbols, C declarations, and C++ projection
* LLVM owns LLVM Types, registers, addresses, instructions, and debug encoding
* Linker owns the final symbol set and native program products

The configured target currently covers 64 bit x86 Linux with the ELF System V
ABI. Target selection is explicit rather than inferred from the build host.

## Native interfaces

The ABI Terminal follows every fully public semantic route and gives LLVM the
exact symbols and carrier shapes it emits. LLVM validates that its finished
module agrees with those facts. It neither regenerates the headers nor derives a
second publication list.

Object values preserve Library's nonnull shared identity and reference counted
lifetime. Option, Result, View, Access, Fixed, Structure, and scalar values keep
their Library behavior while receiving target specific storage and calling
conventions here.

## The parallel GPU path

`Terminal::Spirv` follows the same architectural boundary. A SPIR V producer
walks the Library Functions and Flow in Shader's real child together with their
exact Pipeline contract and Bridge edges. It does not need a translated Library
IR or a second body language.

The CPU and GPU Terminals can choose very different instruction representations.
They meet at completed meaning, while native cross language products additionally
share the C ABI where that agreement applies.
