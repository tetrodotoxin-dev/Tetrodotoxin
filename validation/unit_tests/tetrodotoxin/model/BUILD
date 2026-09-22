# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

exports_files(["image.cpp", "image.hpp"], visibility = ["//validation/unit_tests/tetrodotoxin/dialect:__pkg__"])

cc_binary(
    name = "type_tests",
    testonly = True,
    srcs = [
        "type/scalar.cpp", "type/structure.cpp", "type/policies/conversion.cpp",
        "execution/assignment.cpp", "//validation:unit_test.cpp",
        "//validation:unit_tests/process/child.cpp",
    ],
    deps = ["//tetrodotoxin/model", "//validation:process_fixture", "//validation:test_headers"],
)

cc_library(
    name = "foreign_fixture",
    testonly = True,
    hdrs = ["fixtures/foreign.h"],
    deps = ["//tetrodotoxin/model"],
)

cc_binary(
    name = "libmodel_foreign.so",
    testonly = True,
    srcs = ["fixtures/foreign.c"],
    copts = ["-std=c17", "-Werror", "-Wall", "-Wextra", "-pedantic-errors"],
    linkshared = True,
    deps = [":foreign_fixture"],
)

cc_library(
    name = "cursor_fixture",
    testonly = True,
    hdrs = ["fixtures/cursor.h"],
    deps = ["//tetrodotoxin/source"],
)

cc_binary(
    name = "libcursor_foreign.so",
    testonly = True,
    srcs = ["fixtures/cursor.c"],
    copts = ["-std=c17", "-Werror", "-Wall", "-Wextra", "-pedantic-errors"],
    linkshared = True,
    deps = [":cursor_fixture"],
)

cc_binary(
    name = "model_tests",
    testonly = True,
    srcs = [
        "execution.cpp",
        "execution/carriers.cpp",
        "source.cpp",
        "dialect.cpp",
        "foreign.cpp",
        "image.cpp",
        "image.hpp",
        "//validation:unit_test.cpp",
        "//validation:unit_tests/process/child.cpp",
    ],
    data = [":libmodel_foreign.so", ":libcursor_foreign.so"],
    deps = [
        ":foreign_fixture",
        ":cursor_fixture",
        "//tetrodotoxin/model",
        "//tetrodotoxin/dialect/library",
        "//tetrodotoxin/source:lexical",
        "//tetrodotoxin/terminal:execution",
        "//validation:process_fixture",
        "//validation:test_headers",
    ],
)
