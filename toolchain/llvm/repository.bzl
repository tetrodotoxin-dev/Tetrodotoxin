# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"Repository rule for the immutable LLVM and LLD Terminal SDK."

_LLVM_VERSION = "22.1.8"
_PACKAGE_VERSION = "1:22.1.8~++20260613092327+e80beda6e255-1~exp1~20260613092437.81"
_PACKAGE_ROOT = "https://apt.llvm.org/jammy/"

_PACKAGES = {
    "llvm": struct(
        name = "llvm-22-dev",
        filename = "pool/main/l/llvm-toolchain-22/llvm-22-dev_22.1.8~++20260613092327+e80beda6e255-1~exp1~20260613092437.81_amd64.deb",
        sha256 = "d3aa93146a17065a12428a0db4f94eaf77475e25030784235396dc67b841cd01",
    ),
    "lld": struct(
        name = "liblld-22",
        filename = "pool/main/l/llvm-toolchain-22/liblld-22_22.1.8~++20260613092327+e80beda6e255-1~exp1~20260613092437.81_amd64.deb",
        sha256 = "006d5dab6de3fda52ee1e1379d39e8a4d49a650a43f8812d1cc5870a89694118",
    ),
    "lld_headers": struct(
        name = "liblld-22-dev",
        filename = "pool/main/l/llvm-toolchain-22/liblld-22-dev_22.1.8~++20260613092327+e80beda6e255-1~exp1~20260613092437.81_amd64.deb",
        sha256 = "d4c0e4728208eba7d4068250bc46dd3aa2380d8c918d19810463b22d4a0b37ed",
    ),
    "polly": struct(
        name = "libpolly-22-dev",
        filename = "pool/main/l/llvm-toolchain-22/libpolly-22-dev_22.1.8~++20260613092327+e80beda6e255-1~exp1~20260613092437.81_amd64.deb",
        sha256 = "fc32d319eb3113898d3ad9246d46a0605ce1fdabfa4993294464640649cbd9e9",
    ),
}

def _require_file(repository_ctx, relative_path):
    path = repository_ctx.path(relative_path)
    if not path.exists:
        fail("pinned LLVM SDK is missing expected file: {}".format(relative_path))
    return path

def _execute(repository_ctx, arguments, description):
    result = repository_ctx.execute(arguments)
    if result.return_code:
        fail(
            "{} failed with exit code {}\n{}\n{}".format(
                description,
                result.return_code,
                result.stdout,
                result.stderr,
            ),
        )

def _find_inner_archive(repository_ctx, root, name):
    for extension in ["zst", "xz", "gz"]:
        relative_path = "{}/{}.tar.{}".format(root, name, extension)
        if repository_ctx.path(relative_path).exists:
            return repository_ctx.path(relative_path)
    fail("{} does not contain a supported {} archive".format(root, name))

def _extract_debian_package(repository_ctx, bsdtar, root, package):
    archive = root + ".deb"
    repository_ctx.download(
        url = _PACKAGE_ROOT + package.filename,
        output = archive,
        sha256 = package.sha256,
    )

    # Debian packages contain a second compressed archive. Keeping each package
    # in its own root preserves its metadata and makes the SDK inputs easy to
    # inspect when a pin changes.
    repository_ctx.file(root + "/.keep", "")
    _execute(
        repository_ctx,
        [
            str(bsdtar),
            "-xf",
            str(repository_ctx.path(archive)),
            "-C",
            str(repository_ctx.path(root)),
        ],
        "extracting {}".format(package.name),
    )
    data_archive = _find_inner_archive(repository_ctx, root, "data")
    _execute(
        repository_ctx,
        [str(bsdtar), "-xf", str(data_archive), "-C", str(repository_ctx.path(root))],
        "extracting the {} payload".format(package.name),
    )

    repository_ctx.file(root + "/metadata/.keep", "")
    control_archive = _find_inner_archive(repository_ctx, root, "control")
    _execute(
        repository_ctx,
        [
            str(bsdtar),
            "-xf",
            str(control_archive),
            "-C",
            str(repository_ctx.path(root + "/metadata")),
        ],
        "extracting the {} metadata".format(package.name),
    )
    metadata = repository_ctx.read(
        _require_file(repository_ctx, root + "/metadata/control"),
    )
    if "Package: {}\n".format(package.name) not in metadata:
        fail("unexpected package name in {}".format(archive))
    if "Version: {}\n".format(_PACKAGE_VERSION) not in metadata:
        fail("unexpected package version in {}".format(archive))

def _llvm_sdk_repository_impl(repository_ctx):
    bsdtar = repository_ctx.which("bsdtar")
    if not bsdtar:
        fail(
            "the pinned LLVM SDK currently needs bsdtar to unpack its Debian archives",
        )

    for root, package in _PACKAGES.items():
        _extract_debian_package(repository_ctx, bsdtar, root, package)

    config = repository_ctx.read(
        _require_file(
            repository_ctx,
            "llvm/usr/lib/llvm-22/include/llvm/Config/llvm-config.h",
        ),
    )
    expected_define = '#define LLVM_VERSION_STRING "{}"'.format(_LLVM_VERSION)
    if expected_define not in config:
        fail("LLVM headers do not report the pinned version {}".format(_LLVM_VERSION))

    _require_file(repository_ctx, "llvm/usr/lib/llvm-22/lib/libLLVMCore.a")
    _require_file(repository_ctx, "llvm/usr/lib/llvm-22/lib/libLLVMX86CodeGen.a")
    _require_file(repository_ctx, "lld/usr/lib/llvm-22/lib/liblldELF.a")
    _require_file(repository_ctx, "lld/usr/lib/llvm-22/lib/liblldCOFF.a")
    _require_file(repository_ctx, "lld_headers/usr/lib/llvm-22/include/lld/Common/Driver.h")
    _require_file(repository_ctx, "polly/usr/lib/llvm-22/lib/libPolly.a")
    repository_ctx.symlink(repository_ctx.attr.build_file, "BUILD.bazel")

llvm_sdk_repository = repository_rule(
    implementation = _llvm_sdk_repository_impl,
    attrs = {
        "build_file": attr.label(
            default = "//toolchain/llvm:archive.BUILD.bazel",
            allow_single_file = True,
        ),
    },
)
