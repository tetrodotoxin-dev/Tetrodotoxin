# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""
Tetrodotoxin - A saltless engine language (.ttx)
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "build",
    srcs = glob(["build/**/*.cpp"]),
    hdrs = glob(["build/**/*.hpp"]),
    deps = ["//tetrodotoxin/language", "//ttx"],
)

cc_library(
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
        "//perimortem:headless",
        "//tetrodotoxin/library",
        "//tetrodotoxin/language",
        "//ttx:concept",
        "//tetrodotoxin/source:lexical",
        "//tetrodotoxin/source",
    ],
)

cc_library(
    name = "linker",
    srcs = glob([
        "linker/**/*.cpp",
    ]),
    hdrs = glob([
        "linker/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem:headless",
    ],
)

cc_library(
    name = "formatting",
    srcs = glob([
        "formatting/**/*.cpp",
    ]),
    hdrs = glob([
        "formatting/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//tetrodotoxin/source:lexical",
    ],
)

alias(
    name = "library",
    actual = "//tetrodotoxin/library",
)

cc_library(
    name = "diagnostics",
    srcs = glob([
        "diagnostics/**/*.cpp",
    ]),
    hdrs = glob([
        "diagnostics/**/*.hpp",
    ]),
    includes = ["."],
    deps = [
        "//perimortem:headless",
    ],
)

cc_library(
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
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//ttx:concept",
        "//tetrodotoxin/source",
    ],
)
