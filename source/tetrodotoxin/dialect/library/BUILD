load("@tetro_toolchain//:package.bzl", "source_files")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    strip_include_prefix = "/source",
    name = "library",
    srcs = [
        "dialect.cpp",
        "function.cpp",
        "monograph.cpp",
    ],
    hdrs = [
        "dialect.hpp",
        "function.hpp",
        "monograph.hpp",
        "type_reference.hpp",
    ],
    deps = [
        "//source/tetrodotoxin/model",
        "//source/tetrodotoxin/source",
        "@perimortem",
        "@ttx",
    ],
)

source_files()
