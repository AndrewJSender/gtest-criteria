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

The performance test sleeps for one second per iteration; the integrity and
stability checks are quick. The source is
[`example/main.cpp`](example/main.cpp).

## What the example checks

- **Performance (`TestExampleFixture.Performance`):** measures a test that
  sleeps for one second. In `TearDown`, the fixture compares its duration with
  the mean and standard deviation of up to the 100 most recent successful runs
  of that test. It allows a three-standard-deviation difference and ignores
  differences under 10 milliseconds. No comparison is made until a successful
  duration is available. This is a simple example threshold, not a robust
  performance benchmark.
- **Integrity (`TestExampleFixture.Integrity`):** generates a sample from a
  normal distribution with mean 0 and standard deviation 1. When earlier
  successful samples are available, it calculates their arithmetic mean and
  population standard deviation, then expects the new sample to be within three
  historical standard deviations of that mean. With no prior samples, it logs
  the result without making this comparison. This is a historical consistency
  check, not a comparison against an independently specified expected value;
  with very little history, the estimated mean and standard deviation can be
  unstable.
- **Stability (`TestExampleFixture.Stability`):** intentionally has a 5%
  chance of failing a random check on each run. If that check fails,
  `expect_stable(0.05)` looks up the failure rate for this test and calls
  `GTEST_SKIP()` when that rate is below 5%. The SQL query calculates failed
  test rows divided by all recorded rows with the same suite and test name; it
  does not limit the calculation to a fixed number of recent runs or filter
  out non-failed statuses from the denominator. The current run's row has
  already been inserted when this check runs.

This stability behavior does not stop the random check from failing, predict
future failures, or independently determine whether a test is flaky. It applies
a specific policy to a failing run based on the recorded failure fraction. The
query includes the current in-progress row in its denominator, so the result
should be interpreted as a demonstration of the policy, not as a reliable
historical failure-rate estimate.

For integrity, only successful logged result parts are returned as historical
values. A value logged by a failed assertion is not included in later
comparisons.

## Stability scope

The demo computes an observed failure fraction for one named test and applies
the skip policy described above; it is not a general stability score, a
flakiness classifier, or a predictor of future failures. The result is
particularly sensitive to small sample sizes and to which runs are present in
the database.

## Three dimensions of test quality

The project uses integrity, stability, and performance as a way to think about
test quality:

![Diagram showing integrity, stability, and performance as the three pillars of quality](docs/images/pillars.png)

- **Integrity:** does a test's result still meet its expected behavior? The
  example uses prior logged values as inputs to GoogleTest matchers.
- **Stability:** does a test behave consistently across repeated runs? The
  `Stability` example applies a skip policy to a failed random check using the
  recorded failure fraction. It is not a flakiness detector or a reliable
  failure-rate estimate.
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

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for
details.
