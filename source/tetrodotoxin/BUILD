load("@tetro_toolchain//:package.bzl", "source_files")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""
Tetrodotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    strip_include_prefix = "/source",
    name = "build",
    srcs = glob(["build/**/*.cpp"]),
    hdrs = glob(["build/**/*.hpp"]),
    deps = [
        "//source/tetrodotoxin/language",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "package",
    srcs = glob([
        "package/**/*.cpp",
    ]),
    hdrs = glob([
        "package/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":linker",
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/library",
        "//source/tetrodotoxin/source",
        "//source/tetrodotoxin/source:lexical",
        "@perimortem",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "linker",
    srcs = glob([
        "linker/**/*.cpp",
    ]),
    hdrs = glob([
        "linker/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "@perimortem",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "formatting",
    srcs = glob([
        "formatting/**/*.cpp",
    ]),
    hdrs = glob([
        "formatting/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/source:lexical",
        "@perimortem",
    ],
)

alias(
    name = "library",
    actual = "//source/tetrodotoxin/library",
)

cc_library(
    strip_include_prefix = "/source",
    name = "diagnostics",
    srcs = glob([
        "diagnostics/**/*.cpp",
    ]),
    hdrs = glob([
        "diagnostics/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "@perimortem",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "environment",
    srcs = glob([
        "environment/**/*.cpp",
    ]),
    hdrs = glob([
        "environment/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        ":package",
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/source",
        "@perimortem",
        "@ttx",
    ],
)

source_files()
