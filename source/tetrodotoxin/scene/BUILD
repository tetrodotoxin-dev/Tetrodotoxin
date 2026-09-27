load("@tetro_toolchain//:package.bzl", "source_files")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""The Scene Dialect bundle."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    strip_include_prefix = "/source",
    name = "scene",
    srcs = ["dialect.cpp"],
    hdrs = ["dialect.hpp"],
    deps = [
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/library",
        "//source/tetrodotoxin/scene/archive",
        "//source/tetrodotoxin/scene/interpreter",
        "//source/tetrodotoxin/scene/language",
        "//source/tetrodotoxin/source:lexical",
        "@perimortem",
        "@ttx",
    ],
)

source_files()
