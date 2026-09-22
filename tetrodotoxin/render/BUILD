# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""The Render Dialect bundle."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "render",
    srcs = ["dialect.cpp"],
    hdrs = ["dialect.hpp"],
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//tetrodotoxin/render/archive",
        "//tetrodotoxin/render/interpreter",
        "//tetrodotoxin/render/language",
        "//ttx:concept",
        "//tetrodotoxin/source:lexical",
    ],
)
