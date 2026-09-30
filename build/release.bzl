# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Package the selected provider platform and the complete rebuildable tree."""

load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_files", "strip_prefix")
load("@rules_pkg//pkg:zip.bzl", "pkg_zip")
load("@tetro_toolchain//:library.bzl", "LINUX", "WINDOWS")

def sdk_release(name, headers, libraries, sources):
    stem = native.module_name() + "-" + native.module_version()
    platform = select({LINUX: "linux-x86_64-v3", WINDOWS: "windows-x86_64-msvc"})
    pkg_files(name = name + "_headers", srcs = [headers], prefix = "include/tetrodotoxin", strip_prefix = strip_prefix.from_root("source"))
    pkg_files(name = name + "_library", srcs = libraries, prefix = "lib", strip_prefix = strip_prefix.files_only())
    pkg_files(
        name = name + "_interface",
        srcs = select({WINDOWS: ["//:interface", "//source/dialects/build:interface"], "//conditions:default": []}),
        prefix = "lib",
        renames = select({WINDOWS: {"//:interface": "tetrodotoxin.lib", "//source/dialects/build:interface": "tetrodotoxin_build.lib"}, "//conditions:default": {}}),
    )
    pkg_files(name = name + "_sources", srcs = [sources], strip_prefix = strip_prefix.from_root())
    pkg_files(name = name + "_scripts", srcs = ["//:.vscode/format.sh"], strip_prefix = strip_prefix.from_root(), attributes = pkg_attributes(mode = "0755"))
    pkg_zip(name = name + "_headers_archive", srcs = [":" + name + "_headers", "LICENSE"], out = stem + "-headers.zip", package_dir = "/")
    pkg_zip(name = name + "_binary_archive", srcs = [":" + name + "_library", ":" + name + "_interface", "LICENSE"], out = stem + "-binary.zip", package_file_name = stem + "-" + platform + ".zip", package_dir = "/")
    pkg_zip(name = name + "_source_archive", srcs = [":" + name + "_sources", ":" + name + "_scripts"], out = stem + "-source.zip", package_dir = "/")
    pkg_files(
        name = name + "_runtime",
        srcs = libraries + ["@ttx//:build", "@perimortem//:build"],
        strip_prefix = strip_prefix.files_only(),
    )
    pkg_files(name = name + "_host", srcs = ["//source/puffer:puffer"], strip_prefix = strip_prefix.files_only(), attributes = pkg_attributes(mode = "0755"))
    pkg_files(
        name = name + "_licenses",
        srcs = ["LICENSE", "//build:licenses/TTX-LICENSE", "//build:licenses/Perimortem-LICENSE"],
        renames = {"LICENSE": "Tetrodotoxin-LICENSE", "//build:licenses/TTX-LICENSE": "TTX-LICENSE", "//build:licenses/Perimortem-LICENSE": "Perimortem-LICENSE"},
        prefix = "licenses",
    )
    pkg_zip(name = name + "_runtime_archive", srcs = [":" + name + "_runtime", ":" + name + "_host", ":" + name + "_licenses"], out = stem + "-runtime.zip", package_file_name = stem + "-" + platform + "-runtime.zip", package_dir = "/")
    archives = [":" + name + "_" + suffix + "_archive" for suffix in ["headers", "binary", "source"]]
    native.filegroup(name = name, srcs = archives + [":" + name + "_runtime_archive"])
