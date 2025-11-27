load("@grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")
load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_proto_library(
    name = "build_event_stream_cc_proto",
    deps = [
        "@bazel//src/main/java/com/google/devtools/build/lib/buildeventstream/proto:build_event_stream_proto",
    ],
)

cc_proto_library(
    name = "gimli_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":gimli_proto"],
)

cc_grpc_library(
    name = "gimli_cc_grpc",
    srcs = [":gimli_proto"],
    grpc_only = True,
    deps = [":gimli_cc_proto"],
)

cc_library(
    name = "publish_build_event_callback_service_impl",
    srcs = ["publish_build_event_callback_service_impl.cc"],
    hdrs = ["publish_build_event_callback_service_impl.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":build_event_stream_cc_proto",
        ":gimli_cc_proto",
        "@abseil-cpp//absl/cleanup",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/strings",
        "@googleapis//google/devtools/build/v1:build_cc_grpc",
        "@googleapis//google/devtools/build/v1:build_cc_proto",
    ],
)

cc_binary(
    name = "gimli_server",
    srcs = ["gimli_server.cc"],
    deps = [
        ":gimli_cc_grpc",
        ":gimli_cc_proto",
        ":publish_build_event_callback_service_impl",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:globals",
        "@abseil-cpp//absl/log:initialize",
        "@abseil-cpp//absl/strings",
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

proto_library(
    name = "gimli_proto",
    srcs = ["gimli.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "@bazel//src/main/java/com/google/devtools/build/lib/buildeventstream/proto:build_event_stream_proto",
    ],
)
