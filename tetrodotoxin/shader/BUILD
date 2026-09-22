# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""The Shader Dialect bundle."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "shader",
    srcs = ["dialect.cpp"],
    hdrs = ["dialect.hpp"],
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/language",
        "//tetrodotoxin/library",
        "//tetrodotoxin/render",
        "//tetrodotoxin/shader/archive",
        "//tetrodotoxin/shader/interpreter",
        "//tetrodotoxin/shader/language",
        "//ttx:concept",
        "//tetrodotoxin/source:lexical",
    ],
)
