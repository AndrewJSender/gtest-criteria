# Contributing

## Development setup

Initialize dependencies and configure a debug build:

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Enable PostgreSQL compilation with:

```sh
cmake -S . -B build-postgresql \
  -DCMAKE_BUILD_TYPE=Debug \
  -DGTEST_CRITERIA_WITH_POSTGRESQL=ON
```

## Pull requests

- Add deterministic tests for behavioral changes.
- Keep SQLite-only builds independent of PostgreSQL.
- Run the smallest relevant build and CTest suite before submitting.
- Update the README and changelog for user-visible changes.
- Do not commit generated build directories, databases, or credentials.

## Code style

Use C++17 and follow the formatting and naming of the surrounding code. Keep
public API changes backward-compatible within a minor release when practical.
