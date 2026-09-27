load("@tetro_toolchain//:package.bzl", "source_files")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Scene source interpretation over the reusable Library parser."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    strip_include_prefix = "/source",
    name = "interpreter",
    srcs = glob(
        ["*.cpp"],
        allow_empty = True,
    ),
    hdrs = glob(
        ["*.hpp"],
        allow_empty = True,
    ),
    deps = [
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/library:interpreter",
        "//source/tetrodotoxin/library:language",
        "//source/tetrodotoxin/scene/language",
        "//source/tetrodotoxin/source",
        "//source/tetrodotoxin/source:lexical",
        "@perimortem",
        "@ttx",
    ],
)

source_files()
