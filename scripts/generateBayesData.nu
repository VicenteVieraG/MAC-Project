#!/usr/bin/env nu

# Generate repeatable Bayes fixtures as tests/data/bayes/dataset1.csv, dataset2.csv, etc.
# Usage: nu scripts/generateBayesData.nu [records] --seed 42
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
        # Generate ages 18..80 and incomes 0..80000 (inclusive).
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
