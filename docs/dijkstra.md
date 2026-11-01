# Dijkstra and heuristic search

`Dijkstra` in `<dijkstra.hpp>` runs a shortest-path search using the manual
`Heap`. Each instance owns a copy of its adjacency matrix, start/goal IDs, and
latest completed result. It has no GUI dependencies or playback controls.
The original constructor selects Dijkstra; the coordinate-aware overload selects
Dijkstra (`None`) or A* (`Manhattan` / `Euclidean`) using the same search loop.

```cpp
#include <dijkstra.hpp>
#include <graphReader.hpp>

Dijkstra search(GraphReader::read("graph.txt"), 0, 4);
const auto& result = search.run();
if(result.found) {
    // result.path includes both endpoints; result.totalCost is the path cost.
}
// result.steps[index] provides a saved state for later playback.
```

The matrix must be nonempty, square, and contain only nonnegative finite
weights or positive infinity (`INF`, meaning no edge). Zero-cost edges and
directed graphs are supported. The constructor rejects invalid weights and
shapes with `std::invalid_argument`, and invalid node IDs with
`std::out_of_range`. Any considered finite path sum that overflows throws
`std::overflow_error` rather than silently treating it as unreachable.

Search stops when the goal is popped from the frontier, before expanding its
outgoing edges. `exploredNodes` counts popped nodes, including the goal. An
unreachable goal produces `found == false`, infinite cost, and an empty path.
When start equals goal, the path contains just that node and costs zero.
Ties use the smaller node ID; equal-cost routes retain their first predecessor.

## Four-direction unit-cost grids

`Grid(sideLength)` in `<grid.hpp>` creates a square map's adjacency matrix and
matching positions. The argument is the number of cells along each side:
`Grid(3)` creates 3 x 3 cells (9 nodes), and `Grid(5)` creates 5 x 5 cells (25 nodes).
The constructor calculates the total node count as `sideLength * sideLength`.
A zero side length throws `std::invalid_argument`; a node count that would overflow
`std::size_t` throws `std::overflow_error` before allocation. All cells start open.
Each cell connects only to its unblocked neighbors above, below, left, and right.
Every movement costs exactly `1`. Diagonal movement and custom movement costs
are not grid options. Obstacles have no connections to other cells.

```cpp
#include <dijkstra.hpp>
#include <grid.hpp>

// A 2 x 2 grid: 0--1, 2--3, with vertical edges 0--2 and 1--3.
Grid grid(2);
const auto start = grid.nodeId(0, 0);
const auto goal = grid.nodeId(1, 1);

Dijkstra baseline(grid.matrix(), start, goal);
Dijkstra manhattan(grid.matrix(), start, goal, grid.positions(), Heuristics::Type::Manhattan);
Dijkstra euclidean(grid.matrix(), start, goal, grid.positions(), Heuristics::Type::Euclidean);
const auto& result = manhattan.run(); // Optimal cost 2, including both endpoints.
```

Node IDs use row-major order: `row * sideLength + column`. Positions contain the
corresponding integer row and column stored as doubles. The matrix uses positive
infinity for absent edges and zero on its diagonal; the latter represents staying
at the same node, not a zero-cost grid movement. The getters return const references.

Place or remove obstacles after construction using `setBlocked(nodeId, blocked)`.
The second argument defaults to `true`; pass `false` to reopen a cell:

```cpp
Grid grid(3);
grid.setBlocked(grid.nodeId(1, 2));        // Block row 1, column 2.
grid.setBlocked(grid.nodeId(1, 2), false); // Reopen it.
```

Obstacle edits update the cell's movement edges in both directions. Reopening a
cell restores only edges to open orthogonal neighbors; adjacent obstacles remain
blocked. `setBlocked()` rejects out-of-bounds IDs, and `nodeId()` rejects
out-of-bounds coordinates. Construct a new search after editing obstacles, since
each search owns a copy of the matrix. Keep start and goal on open
cells; blocked cells remain isolated nodes so IDs stay stable. A successful path's
cost equals its number of moves (`path.size() - 1`). The grid model has no GUI
dependencies; the interactive grid editor is still a placeholder.

## Manhattan and Euclidean A*

The search overload takes one map position per node, in matrix order. These
coordinates describe the problem, not screen pixels or an automatically arranged
graph layout. `Grid::positions()` supplies the appropriate coordinates for a grid.

`<heuristics.hpp>` also provides independent `manhattan(node, goal)`,
`euclidean(node, goal)`, and `estimate(type, node, goal)` functions.
Manhattan is `abs(row - goal.row) + abs(column - goal.column)`; Euclidean is
`hypot(row - goal.row, column - goal.column)`. Positions use doubles, allowing
negative and fractional coordinates without unsigned subtraction. Nonfinite
coordinates are rejected; unrepresentable distances throw `std::overflow_error`.

Both heuristics are consistent on the four-direction grid. Since every move costs
`1`, distance directly estimates remaining cost; there is no scaling parameter.
Obstacles remove edges; they do not change the estimates. Select `None` to disable
the heuristic. Unknown heuristic types and mismatched position counts are rejected.

Before searching, the constructor checks every finite directed edge for
`h(u) <= weight(u,v) + h(v)`, using double arithmetic. This consistency condition,
together with `h(goal) == 0`, justifies the algorithm's permanent visited set
and stopping when the goal is popped. Incompatible estimates throw
`std::invalid_argument`; they are not silently used to produce a potentially
suboptimal path. The check is conservative: floating-point rounding can reject
a borderline configuration on a generic graph. Grid construction enforces the
four-direction unit-cost rules, while the underlying search still accepts generic
weighted adjacency matrices. Arbitrary imported graph weights need not match
their geometric layout. Use the original constructor or `None`
when no geometric cost bound is known. A `None` run with positions still validates
the provided geometry.

Relaxation compares actual accumulated costs `g`; only heap ordering uses
`f = g + h`. The start is queued with `h(start)`. A nonfinite priority throws
`std::overflow_error`, even if the corresponding actual path cost is finite.
The reported `totalCost` is always the actual path cost, never a heuristic estimate.

## Recorded states

Recording is on by default. The first snapshot sets `current` to the start node,
with only the start in the frontier and no visited nodes. Every popped node
produces one further snapshot, after relaxing its outgoing edges (or immediately
after popping the goal).
Thus `steps.size() == exploredNodes + 1` for a recorded run.

Every snapshot owns copies of distances, predecessors, visited flags,
frontier membership, and priorities. Node IDs index these arrays:

- `step.distances[node]` is the accumulated cost `g`.
- `result.heuristics[node]` is the fixed remaining-cost estimate `h`, stored once per result.
- `step.priorities[node]` is `g + h`, or infinity for an undiscovered node.

For ordinary Dijkstra, all estimates are zero and priorities equal distances.
Only visited nodes have finalized distances. At early goal
termination, other nodes can still be in the frontier. There is no extra
snapshot for an empty frontier; the result's `found` flag reports the outcome.

Node IDs use ordinary value initialization, not optional values. Predecessors
start at zero and are meaningful only for discovered nodes other than the start;
an infinite distance identifies an undiscovered node. Path reconstruction stops
explicitly at the start node, so zero remains a valid node ID along a path.

`run(false)` disables recording for timing comparisons and returns an empty
`steps` vector. Each run starts fresh and replaces the stored result only upon
completion. `result()` returns that result, or throws `std::logic_error` if no
run has completed. Returned references should not be retained across reruns;
copy a result if it must survive another run. Create another instance to search
a different graph or different endpoints.

Matrix scanning takes O(V²) time, plus up to O(E log V) heap work. Search state
uses O(V) space beyond the owned O(V²) input matrix. Recording adds O(V²) time
and space in the worst case. Playback timing and rendering belong to the UI.
Heuristic preprocessing adds O(V) evaluations and O(V²) consistency validation
at construction. For comparisons, construct all searches first and time
`run(false)` on the same graph and endpoints; report preprocessing separately
if measuring end-to-end cost. Compare actual path cost and `exploredNodes` too.
A heuristic need not reduce explored nodes on every map, and equal-cost paths
may differ between modes.

After building the `tests` target, run:

```sh
./build-tests/bin/tests '[dijkstra]'
./build-tests/bin/tests '[heuristics],[astar]'
./build-tests/bin/tests '[grid]'
```
