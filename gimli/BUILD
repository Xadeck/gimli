load("@grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")
load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

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
    name = "gimli_service_impl",
    srcs = ["gimli_service_impl.cc"],
    hdrs = ["gimli_service_impl.h"],
    implementation_deps = ["@grpc//:grpc++"],
    visibility = ["//visibility:public"],
    deps = [
        ":gimli_cc_grpc",  # keep
        ":gimli_cc_proto",
        "@grpc//:grpc++",
    ],
)

cc_library(
    name = "publish_build_event_callback_service_impl",
    srcs = ["publish_build_event_callback_service_impl.cc"],
    hdrs = ["publish_build_event_callback_service_impl.h"],
    implementation_deps = [
        ":recording_cc_proto",
        "@abseil-cpp//absl/cleanup",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/strings",
        "@bazel//src/main/java/com/google/devtools/build/lib/buildeventstream/proto:build_event_stream_cc_proto",  # keep
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@googleapis//google/devtools/build/v1:build_cc_grpc",  # keep
        "@googleapis//google/devtools/build/v1:build_cc_proto",  # keep
    ],
)

cc_test(
    name = "publish_build_event_callback_service_impl_test",
    size = "small",
    srcs = ["publish_build_event_callback_service_impl_test.cc"],
    data = ["//gimli/testdata"],
    deps = [
        ":publish_build_event_callback_service_impl",
        ":recording_cc_proto",
        "@abseil-cpp//absl/base:log_severity",
        "@abseil-cpp//absl/log:globals",
        "@abseil-cpp//absl/log:initialize",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",  # keep
        "@grpc//:grpc++",
    ],
)

cc_binary(
    name = "gimli_server",
    srcs = ["gimli_server.cc"],
    deps = [
        ":gimli_service_impl",
        ":publish_build_event_callback_service_impl",
        "@abseil-cpp//absl/base:log_severity",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:globals",
        "@abseil-cpp//absl/log:initialize",
        "@abseil-cpp//absl/strings",
        "@grpc//:grpc++",
        "@grpc//:grpc++_reflection",
    ],
)

cc_binary(
    name = "gimli_client",
    srcs = ["gimli_client.cc"],
    deps = [
        ":gimli_cc_grpc",  # keep
        ":gimli_cc_proto",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@grpc//:grpc++",
    ],
)

cc_proto_library(
    name = "recording_cc_proto",
    visibility = ["//visibility:public"],
    deps = [":recording_proto"],
)

proto_library(
    name = "gimli_proto",
    srcs = ["gimli.proto"],
    visibility = ["//visibility:public"],
)

proto_library(
    name = "recording_proto",
    srcs = ["recording.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "@bazel//src/main/java/com/google/devtools/build/lib/buildeventstream/proto:build_event_stream_proto",  # keep
        "@googleapis//google/devtools/build/v1:build_proto",  # keep
    ],
)
