# Google Test Criteria

This repository is a mock-up for evaluating stability, integration, and
performance criteria in a GoogleTest-based test suite.

## Dependencies

The SQL output listener is included as the `gtest_output-sql` submodule. Its
nested submodules provide GoogleTest and SQLite:

```sh
git submodule update --init --recursive
```

The build also requires:

- CMake 3.16 or newer
- A C++17-capable compiler
- `make` or `gmake`
- The PostgreSQL client library and headers (`libpq`)

There is no need to add a separate GoogleTest submodule at the top level.
GoogleTest is initialized from
`submodules/gtest_output-sql/submodules/googletest`.

## Build

Configure and build the example from the repository root:

```sh
cmake -S example -B build
cmake --build build
```

This builds the `example` executable from the sources in `src/`, `include/`,
`example/`, and `submodules/gtest_output-sql/`.

### Generate an Xcode project

To generate an Xcode project instead of using the default build system:

```sh
cmake -S example -B build-xcode -G Xcode
cmake --build build-xcode --config Debug
open build-xcode/gtest_criteria_example.xcodeproj
```

## Run

Run the tests normally:

```sh
./build/example
```

To write test results to a SQLite database:

```sh
./build/example --gtest_output=sqlite:test_results.db
```

The SQL schema used by the listener is stored in
`submodules/gtest_output-sql/sql/`.
