# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""The App Dialect bundle."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "app",
    srcs = ["dialect.cpp"],
    hdrs = ["dialect.hpp"],
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/app/interpreter",
        "//tetrodotoxin/app/language",
        "//tetrodotoxin/language",
        "//ttx:concept",
        "//tetrodotoxin/source:lexical",
    ],
)
