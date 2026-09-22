# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "model",
    deps = [
        ":execution",
        ":type",
    ],
)

# Plugins select model vocabularies independently of source and dialect code.
cc_library(
    name = "type",
    srcs = glob(["type/**/*.cpp"]),
    hdrs = glob([
        "type/**/*.h",
        "type/**/*.hpp",
    ]),
    deps = [
        "//perimortem:headless",
        "//ttx:concept",
        "//ttx:semantic",
    ],
)

cc_library(
    name = "execution",
    srcs = glob(["execution/**/*.cpp"]),
    hdrs = glob([
        "execution/**/*.h",
        "execution/**/*.hpp",
    ]),
    deps = [
        ":type",
        "//perimortem:headless",
        "//ttx:concept",
        "//ttx:semantic",
    ],
)
