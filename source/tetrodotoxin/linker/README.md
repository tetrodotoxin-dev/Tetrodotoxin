# Linker

Compilation gives us native pieces. Linker turns those pieces into something a
host can load and run. By the time work arrives here, semantic meaning has
already produced object inputs and durable symbol agreements. Linker can focus
on relocations, imports, target encoding, and the final ELF or PE product.

Keeping the CPU target separate from the operating system host makes that path
easier to extend. The target chooses instructions, data layout, calling
convention, and object carriers. The host chooses process entry, loader
behavior, runtime providers, dependencies, and executable format.

## ABI manifest

Every Package native artifact has one ABI Manifest beside its native bytes.
The Manifest records:

* Package identity and version
* logical artifact and target identifiers
* one ABI fingerprint
* every selected native import and its logical provider

The fingerprint covers the selected CPU and GPU target profile, generated C
carrier surface, published native signatures and data locators, calling
conventions, and unresolved imported signatures. It detects stale or mismatched
build products. It is not a security or content integrity digest.

Package Archives retain the same artifact agreement. Repository and
source free application selection compare the Archive and Manifest before a
native path can be accepted. Moving an artifact changes neither semantic nor
ABI identity because filesystem paths remain build declarations outside both
formats.

## Imports and providers

A compiler publishes every unresolved Foreign State and Function as an Import.
The Import retains its ABI, native symbol, and category. It does not select a
filesystem object or platform implementation.

The build request supplies target specific Provider records. Provider selection
matches target, ABI, category, and symbol together and requires exactly one
answer. Package compilation records that logical provider in the artifact
agreement. Missing and ambiguous providers fail before an Archive or native
artifact is published.

This lets Linux and Windows providers implement one stable TTX facing C surface
without making Foreign declarations conditional. A platform package may still
declare distinct low level imports when the native APIs genuinely have
different signatures.

## Generated native interfaces

Public TTX declarations become host interfaces without repeating ABI metadata
in source. C++ consumers receive the authored Package, Type, and Callable
routes. The generated C++ implementation then forwards through the exact C
carrier surface used by the object module, keeping target spellings and layout
details out of normal application code.

The raw C header remains available for C consumers and platform integration.

Generated aggregate names begin with lowercase `ttx_`, followed by a lowercase
Package coordinate and the semantic member and Type route. For example:

```c
ttx_perimortem_memory_Dynamic_Bytes
```

Package and semantic separators remain readable underscores. Source underscores
and other nonalphanumeric bytes use hexadecimal escapes, so two distinct routes
cannot collapse merely because punctuation was removed. A generated header
includes dependency owner headers and never redeclares an imported carrier.
Per carrier guards let several generated headers share the same canonical owner
definition safely.

## Native production

Linker gives every native byte range a normal object format home. CPU Terminals
can supply complete ELF or COFF objects, while products such as SPIR V modules
arrive as named read only data that Linker places in an object section. Either
route leaves the next build step with ordinary symbols and files rather than a
special deployment protocol.

Final composition resolves declared symbols and imports, performs archive
extraction, applies relocations, and emits ELF or PE products for the selected
host. Linker works from source independent object contracts rather than
depending on a particular Terminal or a copied language graph.
