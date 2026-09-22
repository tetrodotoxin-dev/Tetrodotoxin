# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Shader source interpretation over the reusable Library parser."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "interpreter",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//tetrodotoxin/library:interpreter",
        "//tetrodotoxin/library:language",
        "//tetrodotoxin/render/language",
        "//tetrodotoxin/shader/language",
        "//ttx:concept",
        "//tetrodotoxin/source:lexical",
        "//tetrodotoxin/source",
    ],
)
