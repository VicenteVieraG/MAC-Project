#!/usr/bin/env nu

# # Random search graph generator
#
# Generate one reproducible, weighted, directed graph in the `GraphReader` format.
#
# ## Usage
#
# Run these examples from the project root:
#
# ```nu
# nu scripts/generateSearchGraphs.nu
# nu scripts/generateSearchGraphs.nu 20 --seed 123 --destination ./graphs
# nu scripts/generateSearchGraphs.nu 20 --density 0.5 --allow-self-connections
# nu scripts/generateSearchGraphs.nu 20 --density 0 --no-islands=false
# nu scripts/generateSearchGraphs.nu --help
# ```
#
# ## Parameters
#
# | Parameter | Default | Description |
# | --- | --- | --- |
# | `nodes` | `10` | Positive integer specifying the number of nodes. |
# | `--seed` | `42` | Integer from `1` through `2147483646`. |
# | `--destination` | `data/searchGraphs` | Output folder, created if needed. |
# | `--density` | `0.3` | Finite probability of optional edges, from `0` through `1`. |
# | `--allow-self-connections` | `false` | Allow diagonal edges at the chosen density. |
# | `--no-islands` | `true` | Guarantee reachability between every pair of nodes. Disable with `--no-islands=false`. |
#
# ## Output
#
# - Write one file named `graph1.txt`, `graph2.txt`, etc.
# - Continue after the highest existing numeric suffix, counting both `.txt`
#   files and older extensionless graphs; do not overwrite files.
# - Resolve the default folder from the script's project root. Resolve a custom
#   relative destination from the current working directory.
# - Store a square adjacency matrix with rows such as `| INF, 12, INF |`.
# - Use integer weights from `1` through `100`. Use `INF` for absent edges,
#   including diagonal entries when self-connections are disabled.
# - Print a summary with the graph name, settings, and saved file path. Terminal
#   output uses color unless disabled by Nushell settings or `NO_COLOR`;
#   redirected output stays plain text.
#
# ## Density and reproducibility
#
# With `--no-islands=true`, graphs with more than one node include a directed
# cycle `0 -> 1 -> ... -> n-1 -> 0`. This guarantees that every node can reach
# every other node, even when density is `0`. A single-node graph needs no
# additional edge, and the guarantee never forces a self-connection.
#
# Density is the probability of each remaining eligible edge, not an exact
# edge count. `0` adds no optional edges; `1` creates every eligible edge.
# Self-connections are eligible only when enabled. Opposite directions are
# sampled independently outside the required cycle, so the matrix need not
# be symmetric. With `--no-islands=false`, all eligible edges are optional,
# density `0` creates no edges, and disconnected graphs are possible.
#
# Identical node counts, seeds, densities, self-connection settings, and
# no-islands settings produce identical matrices, regardless of the destination
# or generated filename.
# Generation uses a Park-Miller pseudorandom number generator and requires only
# Nushell. Invalid parameter values fail before creating the output folder.
def main [
    nodes: int = 10          # Number of nodes (must be positive).
    --seed: int = 42         # Repeatable seed, from 1 through 2147483646.
    --destination: path     # Output folder; defaults to the project's data/searchGraphs.
    --density: float = 0.3   # Optional edge probability from 0 (none) to 1 (all).
    --allow-self-connections # Allow diagonal edges at the chosen density (default: false).
    --no-islands = true      # Guarantee mutual reachability; disable with --no-islands=false.
] {
    if $nodes < 1 {
        error make {msg: "The node count must be greater than zero."}
    }
    if $seed < 1 or $seed > 2147483646 {
        error make {msg: "The seed must be between 1 and 2147483646."}
    }
    # Nonfinite values subtract to `NaN`, which is unequal to zero.
    if ($density - $density) != 0 or $density < 0 or $density > 1 {
        error make {msg: "The density must be a finite number between 0 and 1, inclusive."}
    }

    # Resolve the default from this script, regardless of the working directory.
    let output_dir = if $destination == null {
        $env.FILE_PWD | path join ".." "data" "searchGraphs" | path expand
    } else {
        $destination | path expand
    }
    mkdir $output_dir

    # Count both `.txt` and legacy extensionless graphs, including gaps in numbering.
    let last_number = (ls $output_dir
        | get name
        | path basename
        | parse --regex '^graph(?<number>[1-9][0-9]*)(?:\.txt)?$'
        | get number
        | into int
        | append 0
        | math max)
    let output_file = ($output_dir | path join $"graph($last_number + 1).txt")

    # Park-Miller PRNG, matching `generateBayesData.nu`: no external dependencies.
    # Separate draws choose edge presence and weight; filenames do not affect the seed.
    let matrix = (0..<($nodes * $nodes) | generate { |index, state|
        let edge_state = (($state * 48271) mod 2147483647)
        let weight_state = (($edge_state * 48271) mod 2147483647)
        let source = ($index // $nodes)
        let target = ($index mod $nodes)
        let is_self_connection = ($source == $target)
        # A directed cycle guarantees connectivity without changing the PRNG draws.
        let required_connection = ($no_islands and $nodes > 1
            and $target == (($source + 1) mod $nodes))
        # Normalize the PRNG to `[0, 1)`, so both density endpoints are exact.
        let edge_draw = (($edge_state - 1) / 2147483646)
        let weight = if $is_self_connection and not $allow_self_connections {
            "INF"
        } else if $required_connection or $edge_draw < $density {
            (1 + (($weight_state - 1) mod 100)) | into string
        } else {
            "INF"
        }
        {out: $weight, next: $weight_state}
    } $seed
        | chunks $nodes
        | each { |row| $"| ($row | str join ', ') |" }
        | str join "\n")

    # `save` refuses to overwrite an existing file.
    $"($matrix)\n" | save --raw $output_file

    # Keep redirected logs readable and respect the user's color preferences.
    let use_color = ((is-terminal --stdout)
        and ($env.NO_COLOR? | is-empty)
        and (($env.config.use_ansi_coloring | into string) != "false"))
    let success_style = if $use_color { ansi green_bold } else { "" }
    let label_style = if $use_color { ansi cyan } else { "" }
    let reset_style = if $use_color { ansi reset } else { "" }
    let density_percent = (($density * 100) | math round --precision 2)
    let density_label = if $no_islands { "optional edge probability" } else { "edge probability" }
    let self_connections = if $allow_self_connections { "Enabled" } else { "Disabled" }
    let no_islands_label = if $no_islands { "Guaranteed" } else { "Disabled" }

    print ""
    print $"($success_style)✓ Generated ($output_file | path basename)($reset_style)"
    print $"  ($label_style)Nodes:            ($reset_style)($nodes)"
    print $"  ($label_style)Seed:             ($reset_style)($seed)"
    print $"  ($label_style)Density:          ($reset_style)($density_percent)% ($density_label)"
    print $"  ($label_style)Self-connections: ($reset_style)($self_connections)"
    print $"  ($label_style)No islands:       ($reset_style)($no_islands_label)"
    print $"  ($label_style)Saved to:         ($reset_style)($output_file)"
    print ""
}
