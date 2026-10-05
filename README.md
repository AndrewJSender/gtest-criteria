# Google Test Criteria

**Add history-aware checks to GoogleTest suites.** Google Test Criteria is a
C++ test-fixture example that stores test results and logged values in SQL, then
uses prior runs to check expected behavior and flag significant changes in test
duration.

It is intended for C++ teams that want to investigate how test behavior changes
over time, rather than relying only on the pass/fail result from the latest
run.

## What the example demonstrates

The example in [`example/main.cpp`](example/main.cpp) has two tests:

- **Compare a result with its history.** `Test1` logs an integer result and,
  when an earlier value is available, checks that the new result equals the
  first previously logged value for that test.
- **Check a variable result against historical behavior.** `Test2` logs a
  random sample and, when earlier samples are available, checks that the new
  value is within three configured standard deviations of their historical
  average.
- **Track test duration.** The fixture checks each test's duration against the
  mean and standard deviation of its most recent 100 successful runs. The
  current implementation uses a three-standard-deviation threshold and ignores
  differences smaller than 10 milliseconds.

The first run establishes history: without prior values, the example skips its
history-based value assertions, and without prior durations, there is no
performance baseline to compare. Run it again with the same database to compare
against earlier results.

These are examples of history-aware assertions, not an automatic flaky-test
classifier or a complete quality score. The project does not currently compute
a general stability metric; teams define the assertions appropriate to their
own tests.

## Three dimensions of test quality

The project uses integrity, stability, and performance as a way to think about
test quality:

![Diagram showing integrity, stability, and performance as the three pillars of quality](docs/images/pillars.png)

- **Integrity:** does a test's result still meet its expected behavior? The
  example uses prior logged values as inputs to GoogleTest matchers.
- **Stability:** does a test behave consistently across repeated runs? The SQL
  history can support investigation, and custom assertions can check recorded
  values, but the example does not automatically detect flakiness.
- **Performance:** has a test's duration changed significantly compared with
  its recent successful runs? The fixture includes a basic historical duration
  check.

![Diagram illustrating how integrity, stability, and performance contribute to quality](docs/images/function_diagram.png)

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

## Run and build history

Write test results to a SQLite database:

```sh
./build/example --gtest_output=sqlite:test_results.db
```

Run the command again with the same database to exercise the history-based
assertions against earlier results. GoogleTest arguments can be combined with
SQL output, for example:

```sh
./build/example --gtest_repeat=5 --gtest_output=sqlite:test_results.db
```

To write results to PostgreSQL, provide a libpq connection URI:

```sh
./build/example --gtest_output=postgresql://DB_HOST:5432/DB_NAME
```

The listener stores test results, logged values, and execution durations in
SQL. The schema used by the listener is in
[`submodules/gtest_output-sql/sql/`](submodules/gtest_output-sql/sql/).

Do not commit connection URIs or credentials to source control.

## Add history-aware assertions

Derive a fixture from `GTestCriteriaFixture` and wrap a result and matcher with
`EXPECT_THAT_AND_LOG`. The macro checks the matcher when it is available and
logs the result for future runs:

```cpp
auto matcher = [this](std::string name)
    -> std::optional<::testing::Matcher<int>> {
    auto previous = GetValueList<int>(name);
    if (!previous.has_value()) {
        return std::nullopt;  // No baseline yet.
    }
    return ::testing::Eq(previous->front());
};
EXPECT_THAT_AND_LOG(result, matcher);
```

`GetValueList<T>` returns `std::optional<std::vector<T>>` containing values
logged under that name, in logging order. It returns `std::nullopt` if no
database is configured, no matching value is available, or a value cannot be
parsed as `T`. An empty optional matcher skips the integrity assertion while
still logging the current result.

To disable a criterion check for a run:

```sh
./build/example --criteria_disable_expect_performant
./build/example --criteria_disable_expect_integrity
```

These options can be combined with SQL output and other GoogleTest arguments.
The stability dimension is not a separate automatic check in this example, so
there is no `--criteria_disable_stability` option.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for
details.
