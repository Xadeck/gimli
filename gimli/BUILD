load("@grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")
load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary")

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

cc_proto_library(
    name = "build_event_stream_cc_proto",
    deps = [
        "@bazel//src/main/java/com/google/devtools/build/lib/buildeventstream/proto:build_event_stream_proto",
    ],
)

cc_binary(
    name = "gimli_server",
    srcs = ["gimli_server.cc"],
    deps = [
        ":build_event_stream_cc_proto",
        ":gimli_cc_grpc",
        ":gimli_cc_proto",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:initialize",
        "@googleapis//google/devtools/build/v1:build_cc_grpc",
        "@googleapis//google/devtools/build/v1:build_cc_proto",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
    ],
)

cc_binary(
    name = "gimli_client",
    srcs = ["gimli_client.cc"],
    deps = [
        ":gimli_cc_grpc",
        ":gimli_cc_proto",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@grpc//:grpc++",
    ],
)
