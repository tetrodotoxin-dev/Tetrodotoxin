load("@tetro_toolchain//:package.bzl", "source_files")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Runtime realization for completed App products."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    strip_include_prefix = "/source",
    name = "application",
    srcs = glob(["**/*.cpp"]),
    hdrs = glob(["**/*.hpp"]),
    deps = [
        "//source/tetrodotoxin/graphics/runtime",
        "@perimortem",
        "@photophore",
    ],
)

source_files()
