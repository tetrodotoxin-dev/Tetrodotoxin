# Source Parse capability migration

## Active checkpoint

The approved Source rework keeps the restored tokenizer and eight-byte token
profile. Stream publishes described contiguous token storage and a separate
lexical identity through the released TTX negotiation contracts. Cursor retains
only traversal state. Errors owns report construction and exposes Diagnostics.
Parse supplies a C operation table and native facade for independently implemented
Dialects, with explicit admission, callback and retention rules.

The Source module now invokes the canonical Tokenizer. Its earlier generic
scanner and diagnostic enum are removed. Import retains bytes and tokens through
Borrow. Build, Puffer, the independent C consumer and terminal fixtures are active
again. No production Library grammar or encoding promotion is part of this change.

The followup byte ownership decision is implemented at Import. Input supplies
an Abstract byte owner. Source negotiates Borrow and reads from the acquired
answer, retaining it without copying its payload. Unknown or Rejected at binding
or acquisition copies the original observation. Snapshot describes the borrowed
path and byte views published by Stream and passed to Diagnostics. Tests check
pointer identity, an acquired answer with different storage, final release,
release on tokenization refusal, and both fallback statuses at both steps.

The 58 restored lexical, formatter, association and rendering cases remain. Eight
additional cases exercise independent C providers, vocabulary and policy refusal,
nested cursor advancement, retained diagnostic evidence and Parse results,
compact bounds, and described storage. Formatter reports unsupported input as a
typed failure. Recovery boundaries now belong to the calling grammar.

Oversized source or tokens reject before narrowing. Scanner movement repairs keep an escaped final byte and an empty trailing
attribute within the real source end, avoiding a token or Terminal beyond the
input. Existing lexical classifications and formatting
oracles remain unchanged.

## Validation

Linux debug and release tests execute the complete suite, including the C
consumer, retained Build workspace and Puffer module integration. The Source
suite contains 66 cases. SDK archives package the active public contracts and
both loadable providers. Dependency pins remain Toolchain 0.2.1, Perimortem 0.1.0
and TTX 0.2.0, with no dependency overrides.

Passed on the completed migration:

```sh
bazel test //tests:all --config=debug --config=linux --test_output=errors
bazel test //tests:all --config=release --config=linux --test_output=errors
bazel build //:sdk --config=release --config=linux
bazel build //:sdk //tests:all --config=release --config=windows --repo_env=TETRO_ACCEPT_WINDOWS_SDK_LICENSE=1
git diff --check
```

Both Linux configurations executed all four test targets. Windows compiled the
SDK, modules, Puffer and tests. Native Windows execution, deployment relocation,
and rebuilding this migration from its source archive were not performed.
The C module consumer links only public headers and the released dependencies,
then loads Source independently. Packaging is not evidence of an external SDK
consumer build.

A local release probe compares the captured restored baseline with this change,
using a 30,000-byte repeated declaration fixture producing 5,401 tokens. Across
seven alternating runs, median tokenization increased from 22.613 to 25.225 us
per input, about 12 percent. Median Cursor traversal changed from 4.163 to
3.959 us. Checksums matched. These are narrow local measurements, not a general
throughput guarantee. Tokenization now checks the supported length and rejects
unsupported input instead of truncating it.

The guarded before/after patch, measurements and test details are recorded under
`/home/matt/tetrodev/.scratch/agents/source-parse-implementation-2026-09-29/`.
Comment review checked 548 raw Clang comment tokens in 37 owned C/C++ files.
Exact-path formatting used `.vscode/format.sh`. Whitespace checks include new
untracked files. The original dirty rewrite remains uncommitted.

Earlier evidence describes its own checkpoint and does not establish results
for this migration.

# Earlier canonical Source restoration


## Restoration checkpoint

The canonical lexical implementation is preserved under
`source/dialects/source/` with namespace `Tetrodotoxin::Dialects::Source`.
The user corrected the earlier replacement approach and selected a lexical
library and legacy test checkpoint before reconnecting the dynamic module.
Cursor API trimming is the user's next pass.

The restored Tokenizer, Lexicon, Code, Formatter, Errors, Token, Span, Anchor,
Stream and Cursor match the archived implementation apart from namespace and
include paths. `stream.hpp`, which the restored Tokenizer and Cursor require,
was recovered from the same archive. Token widths, lexical classifications,
formatter behavior and Cursor operations were preserved.

Associations consumes the released TTX 0.2 Abstract view instead of the retired
`Reference<Abstract>` object model. Entries copy borrowed views and lookup uses
`Abstract::get_identity()` within the supplying publication. The user explicitly
approved returning `Option<Abstract>` from `find_at` and `Abstract` from
`Entry::get_semantic`, so growing the entry vector cannot invalidate an answer.
The source owner still keeps provider state and code alive. Copies acquire no
retention and no compatibility type was introduced.

## Restored coverage

The four archived lexical test files were restored under
`tests/dialects/source/`: `tokenizer.cpp`, `formatter.cpp`, `anchor.cpp` and
`associations.cpp`. Their 58 cases run through Toolchain 0.2's validation
harness. Existing authored inputs and formatter output oracles are retained.
Each case documents its purpose. Names are at most 24 characters, fixed test
tables use `Static::Vector`, and object construction uses parentheses.

The association fixture uses independent TTX 0.2 subjects and temporary borrowed
views rather than retired Alias and Invalid implementations. Anchor assertions
compare actual coordinates and ranges instead of allowing implicit boolean
conversion to reduce Token and Span equality to validity alone.

## Build boundary

`//:build` builds the canonical lexical library. `//tests:test` is the active
suite, and `//tests:all` runs it while explicitly skipping these pending tests:

- `//tests:consumer`
- `//tests:workspace`
- `//tests:puffer`

The terminal fixtures `//tests:export_impl` and `//tests:second_export_impl`,
and `//source/dialects/build:implementation`, are also marked incompatible until
module integration resumes. Dependent module and runtime packaging targets
inherit that boundary. The earlier provider source and integration fixtures
remain in place as pending work, not part of the lexical library or its active
header bundle. In particular, Source's `dialect.cpp`, `module.cpp`,
`representation.cpp`, `dialect.hpp`, `input.hpp` and `diagnostic.h`, plus
`tests/dialects/source/stream.cpp`, are outside the active lexical targets.
Reconnection must use the canonical tokenizer and token model.

VS Code's existing build and test tasks continue to use `//tests:all`. Only the
Source test launchers are visible during this checkpoint. The pending launch
configurations are preserved and hidden rather than offered as working targets.

## Validation

Passed on the restored lexical tree:

```sh
bazel test //tests:all --config=debug --config=linux --test_output=errors
bazel build //tests:test --config=debug --config=windows --features=generate_pdb_file --repo_env=TETRO_ACCEPT_WINDOWS_SDK_LICENSE=1
git diff --check
```

Linux executed 58 cases with zero failures and zero skipped cases inside the
active suite. Bazel separately skipped the three pending integration targets.
Windows compilation produced `test.exe` and PDB symbols using the previously
accepted SDK licenses. Native Windows execution and the dynamic Source module
are not established by this checkpoint. No dependency overrides were used.

The 17 canonical source files and four restored test files were checked for
old lexical namespace and include references. A raw Clang token scan checked
571 comment tokens without forbidden punctuation, excluding the prescribed
copyright date. Namespace and include normalization against the archive proved
that the 15 source files outside Associations retained their implementation.
The test files and Associations were formatted through `.vscode/format.sh`.
The comment review retained the canonical causal voice described by the
Bibliotheca and textual stream exemplars.

The active public header inventory is Anchor, Associations, Code, Cursor,
Errors, Formatter, Lexicon, Span, Stream, Token and Tokenizer. Associations::Entry
is its index record, and Errors::Report and its private Error record retain
their restored ownership. This pass did not split or trim those APIs.

# Earlier minimal provider checkpoint

The record below predates the canonical Source restoration. Its build, runtime,
SDK and relocation evidence describes that earlier tree, not the active
checkpoint above. Its Source representation and minimal scanner are superseded.

## Accepted scope

Rebuild Tetrodotoxin from first principles. The old implementation is reference
material only. Source owns the initial lexical system. Puffer bootstraps a native
C++ Build module. Build loads explicitly named providers, uses Import to acquire
real source roots, retains them in a Workspace, then discovers and invokes every
Export provider. Neither Puffer nor Build invokes Bazel or a shell command runner.

The native request supplies the module inventory and source-to-importer routing.
This checkpoint defines no authored Build grammar, execution language, dependency
scheduler, cross-source linker or restored Library compatibility surface.
Bazel is used only to bootstrap and validate this native implementation.

## Reference preservation

The clean starting revision was `70cc134acc8bfefd0ff6dd2097639a1da196d992`.
The old implementation, fixtures, documentation, editor files and local context
were copied to `/home/matt/tetrodev/.scratch/tetrodotoxin-reference/70cc134a/`.
All 1,948 copied files or symlinks were verified before the old active trees were
removed, and the archive was rechecked after implementation. `manifest.json`
records checksums and `committed.tar` retains the complete tracked baseline.

The archive is neither an architectural authority nor a build dependency.
Git history also retains the committed source. No repository history was changed.

## Ownership and public headers

Source's public headers own Input, Token, Diagnostic, Stream, Cursor and Dialect.
Stream's C record and operation table share one header because they describe one
interface. Build's public headers own module and source declarations, Input and
Workspace. Workspace's record and table have the same justified colocation.
C++ representation specializations describe those same C records, not another
semantic model. Source storage, loaded module ownership and retained workspace
members remain private to their implementation files.

Puffer owns argument collection, initial Build loading and diagnostic presentation.
Build owns source acquisition, exact importer routing, module lifetime, the import
barrier and terminal dispatch. Workspace retains real acquired roots. Its fields
release those roots before their supplying modules. Terminals own domain admission
and their artifacts. Failure does not roll back earlier terminal effects.

Source preserves malformed lexical evidence. Build does not impose Source's
validity rules on another dialect: each discovered terminal accepts or rejects
the contracts and observations relevant to its product.

## Validation

Dependency pins are Toolchain 0.2.1, Perimortem 0.1.0 and TTX 0.2.0. Published
archives were verified against their published SHA-256 files. No sibling sources,
SDK overlays or dependency patches were used. The SDK importer does not expose
license files as Bazel targets, so runtime packaging includes byte-identical
notices from the pinned foundation header archives under `build/licenses/`.

The following commands passed:

```sh
bazel query @ttx//:ttx
bazel test //tests:all --test_output=errors
bazel test //tests:all --config=release --test_output=errors
bazel build //:sdk //source/puffer:puffer --config=release
bazel build //:sdk //source/puffer:puffer //tests:all --config=release --config=windows --repo_env=TETRO_ACCEPT_WINDOWS_SDK_LICENSE=1
```

The complete active suite has four test executables. Seven Source cases cover
locations, Unicode spelling, quoting, large input, malformed input, refusal and
independent retention. A C consumer loads Source without linking its implementation
and observes retained data after the query callback. Puffer's integration imports
two roots and invokes two independently loaded terminal modules, checking both
output files. It also covers malformed source refusal, failed source acquisition,
duplicate names, missing importers, absent terminals, missing source retention and
a non-Workspace result. A separate consumer retains the whole Workspace beyond
Build's query and then reads a source whose provider remains loaded through that
reservation.

Windows SDK licenses were explicitly accepted for these builds. The Windows
modules, Puffer and tests cross-compile. Native Windows execution is unverified.
Source alone retains a Wasm target. Build and Puffer explicitly require native
module loading. No Wasm runtime claim is made.

## Review and proof limits

All owned C and C++ files were reviewed against the public contracts and formatted
with `.vscode/format.sh` using exact paths. Comment review follows the reference
Bibliotheca explanation of allocation ordering and the textual stream explanation
of buffer growth costs. The released TTX Import, Borrow and Borrowed headers are
the lifetime authority. A raw comment-token scan of the 27 owned C/C++ files found
no forbidden punctuation, excluding the prescribed copyright date. `git diff
--check` passed. Runtime source scans found no Bazel or process-launch calls.

Build/test and cross-compilation evidence is separate from native execution,
independent SDK consumption, source-archive rebuilding and relocation. The archive and independent deployment checks below passed on identical source,
test, build-rule and dependency files.

## Independent archive and deployment proof

Verification artifacts are under `/tmp/tetrodotoxin-checkpoint-akezv12t/`.
The source archive was unpacked into `rebuild/`. A fresh Bazel output directory
rebuilt and executed all four tests successfully:

```sh
bazel --output_user_root=/tmp/tetrodotoxin-checkpoint-akezv12t/cache-source test //tests:all --config=release --test_output=errors
```

A separate `consumer/` project compiled the C/C++ consumer using only the produced
SDK headers and binary plus the declared published TTX and Perimortem SDKs. It
loaded Source through TTX and completed successfully:

```sh
bazel --output_user_root=/tmp/tetrodotoxin-checkpoint-akezv12t/cache-consumer run //:consumer --config=release -- provider/lib/libtetrodotoxin.so
```

The Linux runtime archive was unpacked into `runtime/`, beside two independent
terminal fixtures and an authored source. Puffer completed the Build request and
both terminals wrote `main` and `support` entries with seven tokens each. The
same deployment passed with `LD_LIBRARY_PATH` and `LD_PRELOAD` unset and `PATH`
set to `/nonexistent`. No Bazel runfiles or reference tree were required.

The runtime archives retain executable permissions for Puffer. The source
archive retains the formatting script's executable permission. Both SDK binary
archives contain independently loadable Source and Build libraries. Windows
archives include both MSVC import libraries. Export inspection found `ttx_query`
and the public representation functions in the actual Windows DLLs.

The highest observed proof is the complete active suite rebuilt from a source
archive, supplemented by an external SDK consumer and relocated native execution.
Windows evidence remains cross-compilation and export inspection only. Terminal
fixtures prove orchestration and output publication, not a production compiler.
All source, test, build-rule and dependency files in the tested source archive
were compared byte-for-byte with the final working tree. Subsequent changes to
this record only document that evidence.
