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
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:utility",
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
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:system",
        "//perimortem:utility",
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
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:system",
        "//perimortem:utility",
    ],
)

cc_library(
    name = "ttx",
    deps = [
        ":concept",
        ":data",
        ":semantic",
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:system",
        "//perimortem:utility",
    ],
)

# Native hosts and independently loaded C++ providers can share these existing
# TTX and Perimortem owners without linking a particular application's models.
# Providers still control their publications and release them through their
# own operations. The shared runtime is a C++ deployment choice, not a TTX
# requirement that foreign implementations use the same allocator.
config_setting(
    name = "wasm32",
    constraint_values = ["@platforms//cpu:wasm32"],
)

cc_shared_library(
    name = "runtime",
    exports_filter = [
        "//ttx:__pkg__",
        "//perimortem:__pkg__",
    ],
    shared_lib_name = select({
        ":wasm32": "libttx_runtime.wasm",
        "//conditions:default": "libttx_runtime.so",
    }),
    user_link_flags = select({
        ":wasm32": [
            "--oformat=wasm",
            "-sSIDE_MODULE=1",
        ],
        "//conditions:default": ["-Wl,-z,defs"],
    }),
    # Link roots retain each compiled library's public symbols, including
    # operations used only by an external provider rather than TTX itself.
    deps = [
        ":concept",
        ":data",
        ":semantic",
        ":ttx",
        "//perimortem:abi",
        "//perimortem:compression",
        "//perimortem:core",
        "//perimortem:graphics",
        "//perimortem:memory",
        "//perimortem:serialization",
        "//perimortem:system",
    ],
)
