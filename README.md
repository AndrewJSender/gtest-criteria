# Google Test Criteria

**Add history-aware checks to GoogleTest suites.** Google Test Criteria is a
C++ test-fixture example that stores test results and logged values in SQL, then
uses prior runs to check expected behavior and flag significant changes in test
duration.

It is intended for C++ teams that want to investigate how test behavior changes
over time, rather than relying only on the pass/fail result from the latest
run.

## Try the demo

Build the example after initializing the submodules:

```sh
git submodule update --init --recursive
cmake -S example -B build
cmake --build build
```

Run five iterations and save the results in a SQLite database:

```sh
./build/example --gtest_output=sqlite:demo.db --gtest_repeat=5
```

Use a new database filename for a clean demo. If `demo.db` already exists, its
successful results also become part of the history. With a fresh database, the
first iteration stores values and durations without comparing them; later
iterations compare against successful results already written during the run.
The database is created by the listener if it does not already exist.

If you have the `sqlite3` command-line tool, inspect the result of every
iteration, including any failed assertions:

```sh
sqlite3 -header -column demo.db \
  "SELECT suite.name, test.name, test.result,
          test.end_timestamp - test.start_timestamp AS duration_ms
   FROM test JOIN suite ON test.suite_id = suite.id
   ORDER BY test.id;"
```

The example takes about two seconds per iteration because each test includes a
one-second sleep. Its source is [`example/main.cpp`](example/main.cpp).

## What the example checks

- **Result integrity (`Test1`):** the test logs an integer result and, when a
  previous successful logged value exists, expects the current result to equal
  the first such value.
- **Bounded variation (`Test2`):** the test logs a random sample and, when
  previous successful samples exist, expects the new sample to be within
  `3 * stdev` of their arithmetic mean. In this example, `stdev` is the
  configured input standard deviation (1.0). It is **not** calculated from the
  historical samples. The example calculates a historical standard deviation
  but does not currently use it in the assertion.
- **Duration regression check:** for each test, the fixture compares the
  current duration with the mean and standard deviation of up to the 100 most
  recent successful runs. It uses a three-standard-deviation tolerance and
  ignores differences smaller than 10 milliseconds. It needs prior successful
  runs before it can check a duration.

The database query for logged values only selects successful result parts.
Values from a run where the logged assertion fails do not become the next
baseline.

## What stability means here

The example does **not** calculate a general stability score, estimate a
flakiness rate, or automatically identify flaky tests. `Test2` demonstrates a
specific, user-defined consistency check: individual samples should remain
within a configured range around prior successful samples' mean. It can flag
out-of-range samples, but it does not establish why they occurred or measure
how likely a test is to fail over time.

Repeated output is available in the SQL history for teams to query and analyze.
To assess stability for a particular test, define an assertion that matches
that test's expected behavior and the evidence you want to evaluate.

## Three dimensions of test quality

The project uses integrity, stability, and performance as a way to think about
test quality:

![Diagram showing integrity, stability, and performance as the three pillars of quality](docs/images/pillars.png)

- **Integrity:** does a test's result still meet its expected behavior? The
  example uses prior logged values as inputs to GoogleTest matchers.
- **Stability:** does a test behave consistently across repeated runs? The
  listener records results and logged values; `Test2` demonstrates one
  configured bounded-variation assertion. There is no automatic flakiness
  detector or general stability metric.
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

To rebuild the example later, configure and build it from the repository root:

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

## Run with SQL output

The demo command above writes results to SQLite. To run the example against a
PostgreSQL database instead, provide a libpq connection URI:

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
    -> std::optional<testing::Matcher<int>> {
    auto previous = GetValueList<int>(name);
    if (!previous.has_value()) {
        return std::nullopt;  // No baseline yet.
    }
    return testing::Eq(previous->front());
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
./build/example --criteria_disable_expect_stable
```

These options can be combined with SQL output and other GoogleTest arguments.
The stability dimension is not a separate automatic check in this example, so
there is no `--criteria_disable_stability` option.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for
details.
