# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

# Source checks also join the full language suite. The focused runners prove
# these services without requiring the remaining language consumers to migrate.
filegroup(
    name = "sources",
    srcs = glob(["**/*.cpp", "**/*.c"]),
    visibility = ["//validation:__pkg__"],
)

# The C publication and native Source owner must agree without linking the
# unfinished language model or acquiring an interpreter Cursor.
cc_binary(
    name = "binding_tests",
    testonly = True,
    srcs = [
        "declaration.cpp",
        "declaration.c",
        "declaration.h",
        "content.cpp",
        "content.c",
        "content.h",
        "tokenization.cpp",
        "errors.cpp",
        "//validation:unit_test.cpp",
        "//validation:unit_tests/process/child.cpp",
    ],
    deps = [
        "//tetrodotoxin/source",
        "//validation:process_fixture",
        "//validation:test_headers",
    ],
)

cc_binary(
    name = "source_tests",
    testonly = True,
    srcs = [
        "abstract.cpp",
        "documentation.cpp",
        "lexical/anchor.cpp",
        "lexical/associations.cpp",
        "lexical/formatter.cpp",
        "lexical/tokenizer.cpp",
        "lexical/cursor.cpp",
        "lexical/publication.cpp",
        "lexical/publication.c",
        "lexical/publication.h",
        "content.c",
        "content.h",
        "//validation:unit_test.cpp",
        "//validation:unit_tests/process/child.cpp",
    ],
    deps = [
        "//tetrodotoxin/source",
        "//tetrodotoxin/source:lexical",
        "//validation:process_fixture",
        "//validation:test_headers",
    ],
)
