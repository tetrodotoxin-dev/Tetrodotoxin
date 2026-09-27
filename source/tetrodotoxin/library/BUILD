load("@tetro_toolchain//:package.bzl", "source_files")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""
The Library Dialect bundle
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    strip_include_prefix = "/source",
    name = "language",
    srcs = glob([
        "builtin/**/*.cpp",
        "language/**/*.cpp",
    ]),
    hdrs = glob([
        "builtin/**/*.hpp",
        "language/**/*.hpp",
    ]),
    deps = [
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/source",
        "//source/tetrodotoxin/source:lexical",
        "@perimortem",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "interpreter",
    srcs = glob([
        "interpreter/**/*.cpp",
    ]),
    hdrs = glob([
        "interpreter/**/*.hpp",
    ]),
    deps = [
        ":language",
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/source",
        "//source/tetrodotoxin/source:lexical",
        "@perimortem",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "simulacra",
    srcs = ["simulacra.cpp"],
    hdrs = ["simulacra.hpp"],
    deps = [
        ":language",
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/source",
        "//source/tetrodotoxin/source:lexical",
        "@perimortem",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "library",
    srcs = ["dialect.cpp"],
    hdrs = ["dialect.hpp"],
    deps = [
        ":interpreter",
        ":language",
        ":simulacra",
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/source",
        "//source/tetrodotoxin/source:lexical",
        "@perimortem",
        "@ttx",
    ],
)

source_files()
