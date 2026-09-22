# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""The Scene Dialect bundle."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "scene",
    srcs = ["dialect.cpp"],
    hdrs = ["dialect.hpp"],
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//tetrodotoxin/library",
        "//tetrodotoxin/scene/archive",
        "//tetrodotoxin/scene/interpreter",
        "//tetrodotoxin/scene/language",
        "//ttx:concept",
        "//tetrodotoxin/source:lexical",
    ],
)
