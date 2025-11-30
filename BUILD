load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "bazel_deps_checker_lib",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "src/**/*.h",
    ]),
    includes = [
        "src",
        "src/common",
        "src/core",
        "src/runtime",
        "src/output",
    ],
    copts = [
        "-std=c++17",
    ],
    visibility = ["//visibility:public"],
    deps = [],
)


cc_binary(
    name = "bazel-deps-analyzer",
    srcs = [
        "src/main.cpp",
    ],
    deps = [
        ":bazel_deps_checker_lib"
    ],
    copts = [
        "-std=c++17",
    ],
)
