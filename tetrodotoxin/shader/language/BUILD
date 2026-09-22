# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Shader relationships over Library execution and Render contracts."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "language",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//tetrodotoxin/library:language",
        "//tetrodotoxin/render/language",
        "//ttx:concept",
        "//tetrodotoxin/source:lexical",
        "//tetrodotoxin/source",
    ],
)
