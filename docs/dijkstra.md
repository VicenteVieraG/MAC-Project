# Dijkstra search

`Dijkstra` in `<dijkstra.hpp>` runs a shortest-path search using the manual
`Heap`. Each instance owns a copy of its adjacency matrix, start/goal IDs, and
latest completed result. It has no GUI dependencies or playback controls.

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

## Recorded states

Recording is on by default. The first snapshot sets `current` to the start node,
with only the start in the frontier and no visited nodes. Every popped node
produces one further snapshot, after relaxing its outgoing edges (or immediately
after popping the goal).
Thus `steps.size() == exploredNodes + 1` for a recorded run.

Every snapshot owns copies of distances, predecessors, visited flags, and
frontier membership. Node IDs index these arrays; frontier priorities equal
their distances. Only visited nodes have finalized distances. At early goal
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

After building the `tests` target, run:

```sh
./build-tests/bin/tests '[dijkstra]'
```
