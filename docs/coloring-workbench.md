# Phase 3 graph coloring workbench

Build from the repository root with the project's C++26 compiler, CMake and initialized submodules.

```sh
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure -R "Phase 3|Deletion and contraction|Four experiments|Greedy order"
cd build/bin
./MAC_Project
```

Open **Graph Coloring**. Activity 17 is loaded on first use. Choose **Run coloring** to see an assignment, or **Calculate polynomial and chi** for the exact reference. The graph includes isolated vertices. Hovering a vertex shows its assigned color by name. A failed run identifies the first blocked vertex and its assigned neighbors.

## Repeating the four experiments

1. In **Graph input**, choose Activity 17, the odd cycle, the complete bipartite graph, or the irregular tree. You can also import a text file or edit the vertex and edge lists. **Validate and load** applies the edit. A rejected edit leaves the loaded graph available. The four input files are in [data/coloring](../data/coloring).
2. In **Coloring**, calculate the exact reference. Record the polynomial, chromatic number, recursive call count and time. For Activity 17, expand the deletion–contraction trace. Its vertex indices initially follow the input list.
3. Test each positive value among χ − 1, χ and χ + 1. Set **Available colors (k)** and enter exactly that many distinct, space-separated names in priority order. Use the same palette for all strategies at a given k.
4. In **Comparison**, select **Compare all 12**. The exact reference is calculated automatically for supported graphs. Compare the three Basic rows for experiment 2, their MRV rows for experiment 3, and the LCV and combined rows for experiment 4. Selecting a row displays its assignment order and makes its graph visible in **Coloring**.
5. Save a CSV for each graph and k before editing the graph or palette. It includes the inputs, reference, assignments, conflict details and counters. Supply a new filename for each export. **Clear runs** restores the uncolored graph with the current palette and exact reference.

## Interpreting the measurements

The parser indexes vertices in input order and stores both a neighbor list per vertex and the original edge list. Each constructive run starts with fresh assignments and a legality table indexed by vertex and color. Assigning a color marks it unavailable to every neighbor. The process stops at the first empty domain.

- **Greedy** orders by descending degree. Its provisional “grado de error” tie-break favors the largest sum of neighboring degrees, then the smallest vertex ID. Replace the isolated `errorDegree` function when the original activity's definition is available.
- **Welsh–Powell** uses descending degree, then vertex ID, with first-fit coloring. **Matula–Marble–Isaacson (MMI)** removes the vertex with the lowest remaining degree, breaking removal ties by smallest ID, and colors in reverse removal order. IDs are compared lexicographically. [Sequential coloring orderings](https://s2.smu.edu/~matula/Tech-Report82.pdf)
- **MRV** chooses the uncolored vertex with the fewest legal colors. Ties follow the base ordering. **LCV** scores each legal color by the number of uncolored neighbors that would lose that color. Ties follow palette order. Combined mode uses both decisions.
- **Checks** counts each lookup asking whether a colored neighbor has blocked a particular color. This includes lookups for first-fit, MRV domain counts and LCV scores. Cache writes are unconditional assignments. MRV selections count decisions, including the failed decision. An MRV or LCV tie counts once when several candidates share the minimum. **LCV changes** counts choices that differ from the first legal color on that same step.
- **Time** is the median of five fresh runs, including vertex ordering and domain initialization. File I/O, rendering and exact analysis are outside the coloring timer. The exact call count includes the root, leaves and cache hits.

## Exact reference and MVP limits

Deletion–contraction subtracts the contracted polynomial from the deleted polynomial. The solver stores a subproblem's vertex count and set of undirected edges, removes the chosen edge before merging its endpoints, and coalesces parallel edges. This preserves isolates and prevents the contracted edge from becoming a loop. Edgeless graphs return the monomial qⁿ. The first positive integer q for which the polynomial is positive gives χ.

The workbench accepts 1–64 vertices and 1–64 colors. Exact analysis supports up to 10 vertices and stops after 100,000 calls to bound interactive work. These bounds keep coefficients and evaluations within signed 64-bit integers. A full call trace is recorded through four vertices. The coloring strategies also work on graphs above the exact limit.

Activity 17 yields **q⁴ − 4q³ + 5q² − 2q**, with 0, 12 and 72 labeled colorings at k = 2, 3 and 4. The irregular tree illustrates why a failed run needs an exact reference. It has two labeled 2-colorings. Basic Welsh–Powell assigns the same color to A and B, then gets stuck at D. With MRV it follows constrained neighbors and colors the whole tree.
