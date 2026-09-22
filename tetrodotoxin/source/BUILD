# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# These headers describe the shared Cursor ABI without selecting a lexer.
lexical_contracts = [
    "lexical/code.hpp", "lexical/token.h", "lexical/token.hpp",
    "lexical/span.h", "lexical/span.hpp", "lexical/cursor.h", "lexical/cursor.hpp",
]
lexical_contract_sources = ["lexical/code.cpp", "lexical/cursor.cpp"]

# Source observations and provenance have no dependency on a tokenizer or
# language implementation. Providers negotiate these records through TTX.
cc_library(
    name = "source",
    srcs = glob(["**/*.cpp"], exclude = ["lexical/**"]) + lexical_contract_sources,
    hdrs = glob(["**/*.hpp", "**/*.h"], exclude = ["lexical/**"]) + lexical_contracts,
    deps = ["//ttx:concept", "//ttx:semantic", "//perimortem:headless"],
)

# The concrete TTX lexer owns tokenization and native cursor storage. Consumers
# can use the Source contracts without selecting this implementation.
cc_library(
    name = "lexical",
    srcs = glob(["lexical/**/*.cpp"], exclude = lexical_contract_sources),
    hdrs = glob(["lexical/**/*.h", "lexical/**/*.hpp"], exclude = lexical_contracts),
    deps = [":source", "//perimortem:headless", "//ttx:concept"],
)
