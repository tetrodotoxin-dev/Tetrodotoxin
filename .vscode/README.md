# VS Code builds and debugging

Open the repository root as the workspace. The default build task builds the
active Source lexical suite with debug information. The Tasks menu also offers
debug and release test runs and a release build.

The canonical tokenizer restoration currently enables **Source tests** only.
The earlier module consumers and Puffer launchers are preserved but hidden until
the canonical Source module representation is connected. Their Bazel targets
are explicitly incompatible during this checkpoint. See
[the development record](../DEVELOPMENT.md) for that boundary.

Select a test in Run and Debug, then launch it with F5. Each configuration runs
the debug build first, supplies the executable's module and source arguments,
and uses a separate writable directory under `.bin/debug/`. Normal test runs
continue to use `bazel test`, including Bazel's test environment and reporting.

Linux launches use [CodeLLDB](https://github.com/vadimcn/codelldb/blob/master/MANUAL.md).
Windows launches use the Visual Studio debugger supplied by the
[C/C++ extension](https://code.visualstudio.com/docs/cpp/launch-json-reference).
The Extensions view recommends both. Windows debug tasks request PDB generation.
The launchers use the workspace's current native platform and appear only on
that platform.

The **Puffer tests** launcher debugs the integration-test process, which starts
Puffer as a child. Select **Puffer** to debug the application itself while it
loads Build, imports the example sources, and invokes both terminal fixtures.
This lets breakpoints in Build and Source stop in the same debugged process.
The sample products go under `.bin/debug/pipeline/`.

Python 3 must be available as `python3` on Linux or `python` on Windows for the
output-directory preparation task. Bazel must also be available on PATH.
After accepting the Microsoft SDK licenses, Windows SDK acquisition can be
enabled in the ignored `.bazelrc.local` file:

```text
build:windows --repo_env=TETRO_ACCEPT_WINDOWS_SDK_LICENSE=1
```

The debugger uses absolute paths to the declared test inputs, so it does not
depend on a Windows runfiles symlink tree. Linux source mappings translate
Bazel's compilation paths back to the checkout and its generated headers.
Configuration fields follow the
[VS Code task](https://code.visualstudio.com/docs/reference/tasks-appendix) and
[debug launch](https://code.visualstudio.com/docs/debugtest/debugging-configuration)
schemas.
