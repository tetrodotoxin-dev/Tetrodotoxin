load("@tetro_toolchain//:package.bzl", "source_files")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Independent graphics runtime Interfaces and frame submission."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    strip_include_prefix = "/source",
    name = "runtime",
    srcs = glob(["**/*.cpp"]),
    hdrs = glob(["**/*.hpp"]),
    deps = [
        "@perimortem",
        "@photophore",
    ],
)

source_files()
