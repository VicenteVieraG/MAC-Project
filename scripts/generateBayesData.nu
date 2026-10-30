#!/usr/bin/env nu

# # Random Bayes dataset generator
#
# Generate one reproducible CSV dataset for testing the Bayes classifier.
#
# ## Usage
#
# Run these examples from the project root:
#
# ```nu
# nu scripts/generateBayesData.nu
# nu scripts/generateBayesData.nu 250 --seed 123
# nu scripts/generateBayesData.nu 0
# nu scripts/generateBayesData.nu --help
# ```
#
# ## Parameters
#
# | Parameter | Default | Description |
# | --- | --- | --- |
# | `records` | `100` | Nonnegative integer specifying the number of data rows. |
# | `--seed` | `42` | Integer from `1` through `2147483646`. |
#
# ## Output
#
# - Create `tests/data/bayes` relative to the script's project root if needed.
# - Write `dataset1.csv`, `dataset2.csv`, etc., using the first available suffix
#   starting at `1`. Gaps can be reused; existing files are not overwritten.
# - Include the header `Edad,Ingreso,Clase`. A record count of `0` writes only
#   the header.
#
# | Column | Generated values |
# | --- | --- |
# | `Edad` | Integer age from `18` through `80`, inclusive. |
# | `Ingreso` | Integer income from `0` through `80000`, inclusive. |
# | `Clase` | `Camina` if age is below `30` or income below `30000`; otherwise `Conduce`. |
#
# ## Reproducibility
#
# Identical record counts and seeds produce identical CSV contents, regardless
# of the generated filename. Generation uses a Park-Miller pseudorandom number
# generator and requires only Nushell. Invalid parameters fail before writing.
#
# These labels are synthetic test fixtures, not a real-world behavioral model.
def main [records: int = 100 --seed: int = 42] {
    if $records < 0 {
        error make {msg: "The record count must be zero or greater."}
    }
    if $seed < 1 or $seed > 2147483646 {
        error make {msg: "The seed must be between 1 and 2147483646."}
    }

    # Resolve from this script so invocation works from any working directory.
    let output_dir = ($env.FILE_PWD | path join ".." "tests" "data" "bayes" | path expand)
    mut number = 1
    mut output_file = ($output_dir | path join $"dataset($number).csv")
    while ($output_file | path exists) {
        $number = $number + 1
        $output_file = ($output_dir | path join $"dataset($number).csv")
    }

    let csv = if $records == 0 {
        "Edad,Ingreso,Clase\n"
    } else {
        # Park-Miller PRNG: explicit state keeps fixtures repeatable without dependencies.
        # Generate ages `18..80` and incomes `0..80000` (inclusive).
        0..<$records | generate { |_, state|
            let age_state = (($state * 48271) mod 2147483647)
            let income_state = (($age_state * 48271) mod 2147483647)
            let age = (18 + (($age_state - 1) mod 63))
            let income = (($income_state - 1) mod 80001)

            # Synthetic labels for fixtures, not a real-world behavioral model.
            let category = if $age < 30 or $income < 30000 { "Camina" } else { "Conduce" }
            {out: {Edad: $age, Ingreso: $income, Clase: $category}, next: $income_state}
        } $seed | to csv
    }

    mkdir $output_dir
    $csv | save $output_file
    print $"Generated ($records) records with seed ($seed) in ($output_file)"
}
