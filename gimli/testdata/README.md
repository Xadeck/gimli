This directory contains example of Bazel C++ targets that do **not** compile.
They are used to record the build events when compiling them.

In one terminal, run the command:

```shell
$ bazel run //gimli:gimli_server -- --record
```

In another terminal, run the commands:

```shell
$ bazel build --bes_backend=grpc://127.0.0.1:8080 \
    //gimli/external:non_fatal_error
```

This will update the file `non_fatal_error.textproto` in this directory.

NOTE: the targets all have the `"manual"` tag, so they are excluded from
`bazel build //...`, which would fail otherwise.
