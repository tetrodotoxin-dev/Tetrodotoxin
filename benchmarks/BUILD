load("@tetro_toolchain//:validation.bzl", "benchmark")
package(default_visibility = ["//visibility:public"])
filegroup(name = "sources", srcs = glob(["**"]))
benchmark(name = "run", srcs = glob(["**/*.cpp"]), deps = ["//source/tetrodotoxin/source:lexical"])
