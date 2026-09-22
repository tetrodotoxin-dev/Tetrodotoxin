# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""App source interpretation."""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "interpreter",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    deps = [
        "//perimortem:headless",
        "//tetrodotoxin/app/language",
        "//tetrodotoxin/language",
        "//ttx:concept",
        "//tetrodotoxin/source:lexical",
    ],
)
