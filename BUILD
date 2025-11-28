# gazelle:cc_group unit
# gazelle:proto file

load("@gazelle//:def.bzl", "gazelle", "gazelle_binary")

filegroup(
    name = "clang_tidy_config",
    srcs = [".clang-tidy"],
    visibility = ["//visibility:public"],
)

# Define a gazelle binary with a list of enabled extensions
gazelle_binary(
    name = "gazelle_cc",
    languages = [
        "@gazelle//language/proto",  # Optional, should be defined before cc
        "@gazelle_cc//language/cc",
    ],
)

# `gazelle` rule can be used to provide additional arguments, eg. for CI integration
gazelle(
    name = "gazelle",
    gazelle = ":gazelle_cc",
)
