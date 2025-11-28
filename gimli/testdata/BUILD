load("@rules_cc//cc:defs.bzl", "cc_binary")

filegroup(
    name = "testdata",
    srcs = glob(["*.textproto"]),
    visibility = ["//gimli:__pkg__"],
)

# Targets all have the "manual" tag,  see README.md in this directory.

cc_binary(
    name = "non_fatal_error",
    srcs = ["non_fatal_error.cc"],
    tags = ["manual"],
)
