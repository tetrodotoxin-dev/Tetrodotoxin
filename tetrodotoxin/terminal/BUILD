# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Terminal producers for completed Tetrodotoxin graphs."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "graph_text",
    srcs = ["graph_text.cpp"],
    hdrs = ["graph_text.hpp"],
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//ttx:concept",
        "//tetrodotoxin/source",
    ],
)

cc_library(
    name = "application",
    srcs = glob(["application/**/*.cpp"]),
    hdrs = glob(["application/**/*.hpp"]),
    deps = [
        ":abi",
        ":graphics",
        ":vulkan",
        "//perimortem:headless",
        "//tetrodotoxin/app/language",
        "//tetrodotoxin/scene/language",
        "//ttx:concept",
        "//tetrodotoxin/source",
    ],
)

cc_library(
    name = "abi",
    srcs = glob([
        "abi/**/*.cpp",
    ]),
    hdrs = glob([
        "abi/**/*.hpp",
    ]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin:library",
        "//tetrodotoxin:linker",
        "//tetrodotoxin:package",
        "//tetrodotoxin/shader/language",
        "//tetrodotoxin/language",
        "//ttx:concept",
        "//tetrodotoxin/source",
    ],
)

cc_library(
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
        "//perimortem:headless",
        "//tetrodotoxin:library",
        "//tetrodotoxin:linker",
        "//tetrodotoxin/language",
        "//tetrodotoxin/scene/language",
        "//toolchain/llvm:sdk",
        "//toolchain/llvm:lld",
        "//ttx:concept",
        "//tetrodotoxin/source:lexical",
        "//tetrodotoxin/source",
    ],
)

cc_library(
    name = "graphics",
    srcs = glob([
        "graphics/**/*.cpp",
    ]),
    hdrs = glob([
        "graphics/**/*.hpp",
    ]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/library:language",
        "//tetrodotoxin/scene/language",
        "//ttx:concept",
        "//tetrodotoxin/source",
    ],
)

cc_library(
    name = "spirv",
    srcs = glob([
        "spirv/**/*.cpp",
    ]),
    hdrs = glob([
        "spirv/**/*.hpp",
    ]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin:library",
        "//tetrodotoxin/language",
        "//tetrodotoxin/render/language",
        "//tetrodotoxin/shader/language",
        "//ttx:concept",
        "//tetrodotoxin/source:lexical",
        "//tetrodotoxin/source",
    ],
)

cc_library(
    name = "vulkan",
    srcs = glob([
        "vulkan/**/*.cpp",
    ]),
    hdrs = glob([
        "vulkan/**/*.hpp",
    ]),
    deps = [
        ":spirv",
        "//perimortem:headless",
        "//tetrodotoxin/library:language",
        "//tetrodotoxin/render/language",
        "//tetrodotoxin/shader/language",
        "//ttx:concept",
        "//tetrodotoxin/source",
    ],
)

# The extracted execution terminal consumes model capabilities alone. Legacy
# language lowering remains separate until its remaining operations migrate.
cc_library(
    name = "execution",
    srcs = ["llvm/execution.cpp"],
    hdrs = ["llvm/execution.hpp"],
    deps = [
        "//tetrodotoxin/model",
        "//toolchain/llvm:sdk",
        "//ttx:concept",
        "//ttx:semantic",
        "//perimortem:headless",
    ],
)
