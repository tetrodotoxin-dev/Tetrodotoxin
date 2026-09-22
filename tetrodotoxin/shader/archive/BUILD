# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Durable Shader query surfaces."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "archive",
    srcs = glob(["**/*.cpp"]),
    hdrs = glob(["**/*.hpp"]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin:library",
        "//tetrodotoxin/language",
        "//tetrodotoxin/render/language",
        "//tetrodotoxin/shader/language",
        "//ttx:concept",
        "//tetrodotoxin/source:lexical",
        "//tetrodotoxin/source",
    ],
)
