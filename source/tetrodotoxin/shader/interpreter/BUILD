load("@tetro_toolchain//:package.bzl", "source_files")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Shader source interpretation over the reusable Library parser."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    strip_include_prefix = "/source",
    name = "interpreter",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    deps = [
        "//source/tetrodotoxin/language",
        "//source/tetrodotoxin/library:interpreter",
        "//source/tetrodotoxin/library:language",
        "//source/tetrodotoxin/render/language",
        "//source/tetrodotoxin/shader/language",
        "//source/tetrodotoxin/source",
        "//source/tetrodotoxin/source:lexical",
        "@perimortem",
        "@ttx",
    ],
)

source_files()
