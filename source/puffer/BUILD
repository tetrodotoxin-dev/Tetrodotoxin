load("@tetro_toolchain//:package.bzl", "source_files")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Puffer - Tetrodotoxin language-server host."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    strip_include_prefix = "/source",
    name = "position_encoding",
    srcs = ["lsp/position_encoding.cpp"],
    hdrs = ["lsp/position_encoding.hpp"],
    deps = ["@perimortem"],
)

cc_library(
    strip_include_prefix = "/source",
    name = "publisher",
    srcs = ["publisher.cpp"],
    hdrs = ["publisher.hpp"],
    deps = [
        "//source/tetrodotoxin/language",
        "@perimortem",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "dependencies",
    srcs = ["dependencies.cpp"],
    hdrs = ["dependencies.hpp"],
    deps = [
        "//source/tetrodotoxin:environment",
        "//source/tetrodotoxin:package",
        "//source/tetrodotoxin/language",
        "@perimortem",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "lsp",
    srcs = glob(
        [
            "lsp/**/*.cpp",
        ],
        exclude = ["lsp/position_encoding.cpp"],
    ),
    hdrs = glob(
        [
            "lsp/**/*.hpp",
        ],
        exclude = ["lsp/position_encoding.hpp"],
    ),
    deps = [
        ":dependencies",
        ":position_encoding",
        "//source/tetrodotoxin:environment",
        "//source/tetrodotoxin:formatting",
        "//source/tetrodotoxin:library",
        "//source/tetrodotoxin:package",
        "//source/tetrodotoxin/app",
        "//source/tetrodotoxin/render",
        "//source/tetrodotoxin/scene",
        "//source/tetrodotoxin/shader",
        "//source/tetrodotoxin/terminal:graph_text",
        "@perimortem",
        "@ttx",
    ],
)

cc_binary(
    name = "puffer",
    srcs = [
        "main.cpp",
    ],
    linkopts = ["-Wl,-rpath,$ORIGIN"],
    deps = [
        ":lsp",
        "//source/tetrodotoxin:build",
        "//source/tetrodotoxin:environment",
        "@perimortem",
    ],
)


source_files()
