# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "library",
    srcs = ["function.cpp", "dialect.cpp", "monograph.cpp"],
    hdrs = ["function.hpp", "type_reference.hpp", "dialect.hpp", "monograph.hpp"],
    deps = [
        "//tetrodotoxin/source",
        "//tetrodotoxin/model",
        "//ttx:concept",
        "//ttx:semantic",
        "//perimortem:headless",
    ],
)
