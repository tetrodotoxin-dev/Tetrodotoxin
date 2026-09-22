# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""
TTX language core.
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

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

# Native hosts and independently loaded C++ providers can share these existing
# TTX and Perimortem owners without linking a particular application's models.
# Providers still control their publications and release them through their
# own operations. The shared runtime is a C++ deployment choice, not a TTX
# requirement that foreign implementations use the same allocator.
cc_shared_library(
    name = "runtime",
    shared_lib_name = "libttx_runtime.so",
    # Link roots retain each compiled library's public symbols, including
    # operations used only by an external provider rather than TTX itself.
    deps = [
        ":ttx",
        ":concept",
        ":semantic",
        ":data",
        "//perimortem:abi",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:system",
        "//perimortem:serialization",
        "//perimortem:compression",
        "//perimortem:graphics",
    ],
    exports_filter = ["//ttx:__pkg__", "//perimortem:__pkg__"],
    user_link_flags = ["-Wl,-z,defs"],
)
