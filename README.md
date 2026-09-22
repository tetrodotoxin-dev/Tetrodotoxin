<p align="center">
  <img src="extension/media/logo.png" alt="Tetrodotoxin Toolchain" width="100%">
</p>

A system for building reusable extensible toolchains and extensions while also leveraging
any existing toolchain without having to bootstrap from scratch.


## Build

The current development environment targets x86_64 Linux with Clang and Bazel.
Windowed applications are supported using Wayland, while the Vulkan renderer
uses the installed Vulkan loader and driver.

Build the repository with:

```sh
bazel build //...
```

Run the complete unit suite with:

```sh
bazel run //validation:unit_tests --config=debug
```

Build the Visual Studio Code extension with:

```sh
./extension/package.sh
```

Adding `--install` installs the generated VSIX after packaging it.

## License

Tetrodotoxin is available under the [MIT License](LICENSE).
