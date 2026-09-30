# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Validation binaries and source-graph integration products."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")
load("@tetro_toolchain//:validation.bzl", "test")
load("//toolchain:tetrodotoxin.bzl", "ttx_source")

package(default_visibility = ["//visibility:public"])

filegroup(name = "sources", srcs = glob(["**"]))

cc_library(
    name = "test_headers",
    hdrs = glob(["**/*.hpp"]),
    includes = ["."],
    deps = ["@tetro_toolchain//source/validation:headers"],
)

ttx_source(
    name = "runtime_package",
    src = "data/ttx/products/runtime/package.ttx",
)

ttx_source(
    name = "foreign_package",
    src = "data/ttx/products/foreign/package.ttx",
)

ttx_source(
    name = "system_abi_package",
    src = "data/ttx/products/system_abi/package.ttx",
)

ttx_source(
    name = "native_dependency",
    src = "data/ttx/package_native/provider/package.ttx",
)

ttx_source(
    name = "package_native_consumer",
    src = "data/ttx/package_native/consumer/package.ttx",
    deps = [":native_dependency"],
)

ttx_source(
    name = "shader_package",
    src = "data/ttx/shader_artifact/package.ttx",
    deps = ["//packages/ttx:perimortem_math"],
)

test(
    name = "test",
    testonly = True,
    srcs = glob(
        [
            "**/*.cpp",
            "**/*.c",
            "**/*.h",
        ],
        exclude = [
            "data/**",
            "model/fixtures/foreign.c",
            "model/fixtures/cursor.c",
            "dialect/fixtures/library.cpp",
            "dialect/fixtures/nesting.c",
        ],
    ),
    data = glob(
        ["data/**/*.ttx"],
        exclude = ["data/ttx/shader_artifact/**"],
    ) + [
        "data/ttx/app_profiles/resources/icon.bin",
        "data/ttx/package_resources/resources/empty.bin",
        "data/ttx/package_resources/resources/table.bin",
        ":libcursor_foreign.so",
        ":liblibrary.so",
        ":libmodel_foreign.so",
        ":libnesting.so",
    ],
    includes = ["."],
    deps = [
        ":test_headers",
        "//source/puffer:position_encoding",
        "//source/puffer:publisher",
        "//source/tetrodotoxin:build",
        "//source/tetrodotoxin:diagnostics",
        "//source/tetrodotoxin:environment",
        "//source/tetrodotoxin:formatting",
        "//source/tetrodotoxin:library",
        "//source/tetrodotoxin:linker",
        "//source/tetrodotoxin:package",
        "//source/tetrodotoxin/app",
        "//source/tetrodotoxin/dialect/library",
        "//source/tetrodotoxin/graphics/runtime",
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/model",
        "//source/tetrodotoxin/render",
        "//source/tetrodotoxin/runtime:application",
        "//source/tetrodotoxin/scene",
        "//source/tetrodotoxin/shader",
        "//source/tetrodotoxin/source:lexical",
        "//source/tetrodotoxin/terminal:execution",
        "//source/tetrodotoxin/terminal:llvm",
        "//source/tetrodotoxin/terminal:spirv",
        "@perimortem",
        "@ttx",
    ],
)

cc_library(
    name = "fixture_headers",
    testonly = True,
    hdrs = glob(["**/fixtures/*.h"]),
    deps = [
        "//source/tetrodotoxin/model",
        "//source/tetrodotoxin/source",
    ],
)

cc_binary(
    name = "libmodel_foreign.so",
    testonly = True,
    srcs = ["model/fixtures/foreign.c"],
    copts = [
        "-std=c17",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pedantic-errors",
    ],
    linkshared = True,
    deps = [":fixture_headers"],
)

cc_binary(
    name = "libcursor_foreign.so",
    testonly = True,
    srcs = ["model/fixtures/cursor.c"],
    copts = [
        "-std=c17",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pedantic-errors",
    ],
    linkshared = True,
    deps = [":fixture_headers"],
)

cc_library(
    name = "library_fixture",
    testonly = True,
    srcs = ["dialect/fixtures/library.cpp"],
    deps = [
        ":fixture_headers",
        "//source/tetrodotoxin/dialect/library",
    ],
)

cc_library(
    name = "nesting_fixture",
    testonly = True,
    srcs = ["dialect/fixtures/nesting.c"],
    copts = [
        "-std=c17",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pedantic-errors",
    ],
    deps = [":fixture_headers"],
)

# Both providers import the shared allocator. Their unload checks must not
# accidentally retain a private allocator through its thread finalizer.
cc_shared_library(
    name = "liblibrary.so",
    testonly = True,
    shared_lib_name = "liblibrary.so",
    deps = [":library_fixture"],
)

cc_shared_library(
    name = "libnesting.so",
    testonly = True,
    shared_lib_name = "libnesting.so",
    deps = [":nesting_fixture"],
)
