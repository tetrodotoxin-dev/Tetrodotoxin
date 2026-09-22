# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")

cc_library(
    name = "fixture",
    testonly = True,
    hdrs = ["fixtures/lifetime.h"],
    deps = ["//tetrodotoxin/source"],
)

# Only this provider links Library's implementation. Its consumer discovers
# Source::Dialect through the standard TTX module entry.
cc_library(
    name = "library_fixture",
    testonly = True,
    srcs = ["fixtures/library.cpp"],
    deps = [":fixture", "//tetrodotoxin/dialect/library"],
)

cc_library(
    name = "nesting_fixture",
    testonly = True,
    srcs = ["fixtures/nesting.c"],
    copts = ["-std=c17", "-Werror", "-Wall", "-Wextra", "-pedantic-errors"],
    deps = [":fixture"],
)

# A private Perimortem runtime would give the dialect its own allocator TLS
# finalizers and pin its code until thread exit. Share the existing runtime so
# result lifetime, rather than that accidental deployment detail, governs unload.
cc_shared_library(
    name = "liblibrary.so",
    testonly = True,
    shared_lib_name = "liblibrary.so",
    dynamic_deps = ["//ttx:runtime"],
    deps = [":library_fixture"],
)

cc_shared_library(
    name = "libnesting.so",
    testonly = True,
    shared_lib_name = "libnesting.so",
    dynamic_deps = ["//ttx:runtime"],
    deps = [":nesting_fixture"],
)

cc_binary(
    name = "monograph_tests",
    testonly = True,
    srcs = [
        "monograph.cpp",
        "//validation/unit_tests/tetrodotoxin/model:image.cpp",
        "//validation/unit_tests/tetrodotoxin/model:image.hpp",
        "//validation:unit_test.cpp",
        "//validation:unit_tests/process/child.cpp",
    ],
    data = [":liblibrary.so", ":libnesting.so"],
    dynamic_deps = ["//ttx:runtime"],
    deps = [
        ":fixture",
        "//tetrodotoxin/source:lexical",
        "//tetrodotoxin/model",
        "//tetrodotoxin/terminal:execution",
        "//validation:test_headers",
        "//validation:process_fixture",
    ],
)
