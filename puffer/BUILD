# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Puffer - Tetrodotoxin language-server host."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "position_encoding",
    srcs = ["lsp/position_encoding.cpp"],
    hdrs = ["lsp/position_encoding.hpp"],
    deps = ["//perimortem:headless"],
)

cc_library(
    name = "publisher",
    srcs = ["publisher.cpp"],
    hdrs = ["publisher.hpp"],
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//ttx:concept",
    ],
)

cc_library(
    name = "dependencies",
    srcs = ["dependencies.cpp"],
    hdrs = ["dependencies.hpp"],
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:package",
        "//tetrodotoxin/language",
        "//ttx",
    ],
)

cc_library(
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
        "//perimortem:headless",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:formatting",
        "//tetrodotoxin:library",
        "//tetrodotoxin:package",
        "//tetrodotoxin/app",
        "//tetrodotoxin/render",
        "//tetrodotoxin/scene",
        "//tetrodotoxin/shader",
        "//tetrodotoxin/terminal:graph_text",
        "//ttx",
    ],
)

cc_binary(
    name = "puffer",
    srcs = [
        "main.cpp",
    ],
    deps = [
        ":lsp",
        "//perimortem:headless",
        "//tetrodotoxin:environment",
        "//tetrodotoxin:build",
    ],
)
