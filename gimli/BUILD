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
  name = "gtest_logging",
  srcs = ["gtest_logging.cc"],
  testonly = True,
  alwayslink = 1,
  deps = [
        "@googletest//:gtest",
        "@abseil-cpp//absl/log:initialize",
        "@abseil-cpp//absl/log",
  ],
)

cc_library(
    name = "report",
    hdrs = ["report.h"],
    visibility = ["//visibility:public"],
    deps = ["@abseil-cpp//absl/time"],
)

cc_library(
    name = "reporter",
    srcs = ["reporter.cc"],
    hdrs = ["reporter.h"],
    deps = [
        ":report",
    ],
)

cc_library(
    name = "stderr_processor",
    srcs = ["stderr_processor.cc"],
    hdrs = ["stderr_processor.h"],
    deps = [
        ":report",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/strings",
    ],
)

cc_test(
    name = "stderr_processor_test",
    size = "small",
    srcs = ["stderr_processor_test.cc"],
    deps = [
        ":stderr_processor",
        "@googletest//:gtest",
        "@googletest//:gtest_main",  # keep
        ":gtest_logging",  # keep
    ],
)

cc_test(
    name = "reporter_test",
    srcs = ["reporter_test.cc"],
    deps = [
        ":reporter",
        "@googletest//:gtest",
        "@googletest//:gtest_main",  # keep
    ],
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
        "@abseil-cpp//absl/cleanup",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/strings",
        "@bazel//src/main/java/com/google/devtools/build/lib/buildeventstream/proto:build_event_stream_cc_proto",  # keep
    ],
    visibility = ["//visibility:public"],
    deps = [
        # Next one is technically in implementation_deps but clang-tidy doesn't
        # see it then. Maybe https://github.com/erenon/bazel_clang_tidy/issues/30?
        ":recording_cc_proto",
        ":reporter",
        ":stderr_processor",
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
        ":gtest_logging",  # keep
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
        ":reporter",
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
