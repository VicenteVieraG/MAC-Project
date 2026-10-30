# Graph input format

`GraphReader` reads a square adjacency matrix from a file or an input stream:

```text
| 0, 1, INF |
| 0, 3, INF |
| 2, INF, 5 |
```

Each nonblank line contains one row, enclosed in `|`, with comma-separated
weights. Spaces, tabs, blank lines, and Windows line endings are accepted.
The final newline is optional. The matrix must be nonempty and square.

`INF` (uppercase) represents no connection and becomes positive infinity.
Finite decimal weights, including signed values and scientific notation, are
accepted. Zero is a valid weight. Entries are preserved in row/column order;
the reader does not impose symmetry or replace diagonal values. Algorithms
that require nonnegative weights, such as Dijkstra, must check that condition.

```cpp
#include <graphReader.hpp>

const auto matrix = GraphReader::read("graph.txt");
const double weight = matrix[0][1]; // Cost from vertex 0 to vertex 1: 1.0.
```

The return type is `GraphReader::Matrix` (`std::vector<std::vector<double>>`).
The `read(std::istream&)` overload consumes an existing stream through EOF.
Malformed input throws `std::invalid_argument`, with a line and matrix column
when applicable. Unreadable files or failed streams throw `std::runtime_error`.

Run the reader tests after building the `tests` target:

```sh
./build/bin/tests '[graph-reader]'
```
