# Bayes tests and benchmarks

The tests use the existing `tests` target and `BAYES_TEST_DATA_DIR` definition.
All Bayes cases live in `bayes-tests.cpp`, so no additional CMake declarations
are needed. `tests/main.cpp` retains the template's factorial example.

## Fixtures

The generated CSVs are intentionally ignored by Git. On a fresh checkout, run
these commands from the repository root, in order:

```sh
nu scripts/generateBayesData.nu 100 --seed 42
nu scripts/generateBayesData.nu 1000 --seed 42
nu scripts/generateBayesData.nu 100000 --seed 42
nu scripts/generateBayesData.nu 1000000 --seed 42
```

With an empty output directory this creates `dataset1.csv` through `dataset4.csv`
under `tests/data/bayes`. The script skips occupied names: if fixtures already
exist, inspect their counts instead of blindly rerunning these commands. Existing
files are preserved, and later generated filenames are not automatically used by
the tests.

Correctness tests require only `dataset1.csv` with 100 data rows. The prediction
benchmark requires `dataset2.csv` with 1,000 rows. Training benchmarks use all
four files with the counts above. Missing files and incorrect counts fail with
a diagnostic rather than silently passing on an empty model. The assertions do
not depend on the seed; the seed above is a reproducible fixture convention.

## Build and run

Initialize Catch2 if needed:

```sh
git submodule update --init --recursive tests/catch2
cmake -S . -B build-tests -DBUILD_TESTS=ON -DBUILD_TESTS_WITH_MAIN=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-tests --target tests -j 4
```

Run Bayes correctness tests directly, or all non-benchmark tests through CTest:

```sh
./build-tests/bin/tests '[bayes]~[.benchmark]'
ctest --test-dir build-tests --output-on-failure -E benchmark
```

Run benchmarks explicitly:

```sh
./build-tests/bin/tests '[.benchmark]'
```

The benchmark cases have hidden tags to exclude them from a plain Catch2 run.
Use the explicit CTest exclusion above because CMake's test discovery can register
hidden cases individually. Benchmark cases skip themselves in Debug builds,
where training prints the entire dataset and would distort the timings.

## Coverage and interpretation

- CSV loading, the 80/20 split, held-out row identity, and reset on retraining.
- Both categories at every combination of age/income boundary test values.
- Ties, unequal priors, smoothing of unseen bins, and single-class training.
- Adding a record updates training counts and preserves the held-out set.
- Each confusion-matrix outcome, reset on repeated evaluation, undefined metrics,
  and rejection of evaluation on an untrained model.

Prediction fixtures are constructed by hand to give independently known answers.
Random fixtures complement these cases; they do not guarantee boundary coverage
or establish a required accuracy percentage.

Prediction timing excludes training. Training timing includes file reading,
CSV parsing, resetting, and fitting. It reuses a model and runs after an initial
training pass, so it represents warm filesystem caches and reused allocation
capacity, not cold process startup. Timings are reported without pass/fail speed
thresholds.

Missing/malformed CSV error contracts and prediction on an untrained model remain
production-code decisions; these tests do not treat the current accidental
behavior as a desired contract.
