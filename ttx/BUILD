# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""
TTX language core.
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "data",
    srcs = glob(["data/**/*.cpp"]),
    hdrs = glob([
        "data/**/*.h",
        "data/**/*.hpp",
    ]),
    deps = [
        "//perimortem:headless",
    ],
)

cc_library(
    name = "semantic",
    srcs = glob(["semantic/**/*.cpp"]),
    hdrs = glob([
        "semantic/**/*.h",
        "semantic/**/*.hpp",
    ]),
    deps = [
        ":data",
        "//perimortem:headless",
    ],
)

cc_library(
    name = "concept",
    srcs = glob([
        "concept/**/*.cpp",
    ]),
    hdrs = glob([
        "concept/**/*.h",
        "concept/**/*.hpp",
    ]),
    deps = [
        ":semantic",
        "//perimortem:headless",
    ],
)

cc_library(
    name = "ttx",
    deps = [
        ":concept",
        ":data",
        ":semantic",
        "//perimortem:headless",
    ],
)
