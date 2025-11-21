load("@grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

proto_library(
    name = "gimli_proto",
    srcs = ["gimli.proto"],
)

cc_proto_library(
    name = "gimli_cc_proto",
    deps = [":gimli_proto"],
)

cc_grpc_library(
    name = "gimli_cc_grpc",
    srcs = [":gimli_proto"],
    grpc_only = True,
    deps = [":gimli_cc_proto"],
)

cc_binary(
    name = "gimli_server",
    srcs = ["gimli_server.cc"],
    deps = [
        ":gimli_cc_grpc",
        ":gimli_cc_proto",
        "@grpc//:grpc++",
    ],
)

cc_binary(
    name = "gimli_client",
    srcs = ["gimli_client.cc"],
    deps = [
        ":gimli_cc_grpc",
        ":gimli_cc_proto",
        "@grpc//:grpc++",
    ],
)
