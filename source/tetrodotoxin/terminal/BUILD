load("@tetro_toolchain//:package.bzl", "source_files")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Terminal producers for completed Tetrodotoxin graphs."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    strip_include_prefix = "/source",
    name = "graph_text",
    srcs = ["graph_text.cpp"],
    hdrs = ["graph_text.hpp"],
    deps = [
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/source",
        "@perimortem",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "application",
    srcs = glob(["application/**/*.cpp"]),
    hdrs = glob(["application/**/*.hpp"]),
    deps = [
        ":abi",
        ":graphics",
        ":vulkan",
        "//source/tetrodotoxin/app/language",
        "//source/tetrodotoxin/scene/language",
        "//source/tetrodotoxin/source",
        "@perimortem",
        "@photophore",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "abi",
    srcs = glob([
        "abi/**/*.cpp",
    ]),
    hdrs = glob([
        "abi/**/*.hpp",
    ]),
    deps = [
        "//source/tetrodotoxin:library",
        "//source/tetrodotoxin:linker",
        "//source/tetrodotoxin:package",
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/shader/language",
        "//source/tetrodotoxin/source",
        "@perimortem",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "llvm",
    srcs = glob([
        "llvm/**/*.cpp",
    ]),
    hdrs = glob([
        "llvm/**/*.hpp",
    ]),
    deps = [
        ":abi",
        ":graphics",
        "//source/tetrodotoxin:library",
        "//source/tetrodotoxin:linker",
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/scene/language",
        "//source/tetrodotoxin/source",
        "//source/tetrodotoxin/source:lexical",
        "//toolchain/llvm:lld",
        "//toolchain/llvm:sdk",
        "@perimortem",
        "@photophore",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "graphics",
    srcs = glob([
        "graphics/**/*.cpp",
    ]),
    hdrs = glob([
        "graphics/**/*.hpp",
    ]),
    deps = [
        "//source/tetrodotoxin/library:language",
        "//source/tetrodotoxin/scene/language",
        "//source/tetrodotoxin/source",
        "@perimortem",
        "@photophore",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "spirv",
    srcs = glob([
        "spirv/**/*.cpp",
    ]),
    hdrs = glob([
        "spirv/**/*.hpp",
    ]),
    deps = [
        "//source/tetrodotoxin:library",
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/render/language",
        "//source/tetrodotoxin/shader/language",
        "//source/tetrodotoxin/source",
        "//source/tetrodotoxin/source:lexical",
        "@perimortem",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "vulkan",
    srcs = glob([
        "vulkan/**/*.cpp",
    ]),
    hdrs = glob([
        "vulkan/**/*.hpp",
    ]),
    deps = [
        ":spirv",
        "//source/tetrodotoxin/library:language",
        "//source/tetrodotoxin/render/language",
        "//source/tetrodotoxin/shader/language",
        "//source/tetrodotoxin/source",
        "@perimortem",
        "@photophore",
        "@ttx",
    ],
)

# The extracted execution terminal consumes model capabilities alone. Legacy
# language lowering remains separate until its remaining operations migrate.
cc_library(
    strip_include_prefix = "/source",
    name = "execution",
    srcs = ["llvm/execution.cpp"],
    hdrs = ["llvm/execution.hpp"],
    deps = [
        "//source/tetrodotoxin/model",
        "//toolchain/llvm:sdk",
        "@perimortem",
        "@ttx",
    ],
)

source_files()
