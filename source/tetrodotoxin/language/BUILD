load("@tetro_toolchain//:package.bzl", "source_files")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Shared Tetrodotoxin language contracts and TTX parser fragments."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    strip_include_prefix = "/source",
    name = "language",
    srcs = glob(["**/*.cpp"]),
    hdrs = glob(["**/*.hpp"]),
    deps = [
        "//source/tetrodotoxin/source",
        "//source/tetrodotoxin/source:lexical",
        "@perimortem",
        "@ttx",
    ],
)

source_files()
