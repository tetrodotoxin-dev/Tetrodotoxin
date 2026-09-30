# Contributing to Tetrodotoxin

Begin with the semantic question and its owner. Source, Execution and Library
have distinct responsibilities, while TTX owns their shared negotiation model.
Keep new behavior with the system that demonstrates its need.

Use the versions declared in `MODULE.bazel`. Perimortem, TTX and Toolchain are
independent repositories. A missing dependency contract is a design question for
that owner, not a reason to add a header overlay, local override or compatibility
implementation here.

Public contracts describe behavior, representation agreement and lifetime.
Comments explain why those boundaries exist, including failure and ABI limits.
Keep one primary public contract per header and avoid forwarding models that
only reconcile old interfaces.

Tests belong under `tests/` and use Toolchain's validation harness. Test distinct
Tetrodotoxin behaviors and retain an independent consumer across each public
module boundary. Upstream library and validation-harness coverage stays upstream.

```sh
bazel build //:build
bazel test //tests:all
```

VS Code includes build and test tasks plus debugger launchers for the tests and
Puffer. See the [editor setup](.vscode/README.md) for Linux and Windows usage.

Report compilation, execution, independent consumption, relocation, rebuilds
from packaged source and target-platform execution separately. A cross build
alone does not establish native Windows behavior.

Format only the files being changed with `.vscode/format.sh`, then run
`git diff --check`. Preserve unrelated dirty work and describe the concrete
behavior, owner and validation evidence in a contribution.
