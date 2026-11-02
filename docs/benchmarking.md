# Search benchmarking

Open **Search → Benchmarking** to compare **Dijkstra**, **Euclidean A*** and
**Manhattan A*** on the same weighted, directed graph. This tab has no algorithm
visualization or playback.

1. Enter a graph file path and press **Load graph**, or choose **Browse...** and
   select a file. Use the existing [adjacency matrix format](graph-format.md):
   pipe-enclosed rows, comma-separated weights, and uppercase `INF` for missing
   edges. Zero weights are valid; negative weights are rejected.
2. Select zero-based **Start node** and **Goal node** IDs. A newly loaded graph
   defaults to node `0` and node `N-1`.
3. Press **Start comparison**. The table shows execution time in **milliseconds**,
   explored nodes, path cost, and whether the destination is reachable for each
   search. Press again to replace the measurements with a fresh comparison.

Loading prepares the graph without running a search. Loading another graph or
changing an endpoint clears the results. Failed loads preserve the current graph
and its results and display an error. The comparison button is disabled until a
valid graph is loaded. This tab is independent of the Grid and Graph tabs.

## Timing

All three modes use the existing `Dijkstra` implementation. Each comparison runs
each mode once, calling **`run(false)`** to disable step recording. Timers use
`std::chrono::steady_clock` and `std::chrono::duration<double, std::milli>` around
only that call. File loading, graph copies, constructor validation, heuristic
preparation, copying completed results, and UI rendering are outside the timed
region. Search state initialization and final path reconstruction are part of
the algorithm call and are included.

Times are shown with six decimal places in milliseconds. Single-run measurements
can vary with system load and timer resolution, especially for small graphs;
they are not an average or a guarantee that a heuristic will be faster.

## Coordinates for distance heuristics

The file format has no node coordinates. For a reproducible comparison without
changing that format, node IDs are assigned artificial positions in row-major
order on a grid with `ceil(sqrt(N))` columns. These positions do not represent
physical locations and do not use the draggable layout in the Graph tab.

A shared coordinate scale is half the smallest ratio of a finite, nonself edge
weight to its unscaled Manhattan distance. This makes both distance heuristics
consistent with the directed edge weights, with room for rounding. The scale is
also bounded to keep coordinates and estimates finite. The original adjacency
matrix and its weights are never changed. The existing search constructor checks
heuristic consistency before timing starts.

A zero-cost edge between distinct nodes, or an underflowed scale, reduces all
estimates to zero. Graphs with no nonself edges also use zero estimates. In those
cases, the tab explains that Euclidean and Manhattan behave like Dijkstra.
For other graphs, these synthetic estimates may still be weak: the comparison
measures this documented layout, not a geometric map absent from the file.

## Verification

After building the `tests` target, run `build/bin/tests "[search][benchmark]"` (or
`build/bin/tests.exe "[search][benchmark]"` on Windows). Tests cover loading, invalidation,
empty step histories, fractional and zero weights, unreachable goals, identical
endpoints, and agreement with Dijkstra on generated weighted directed graphs.
