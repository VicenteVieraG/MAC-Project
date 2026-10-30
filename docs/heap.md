# Search frontier heap

`Heap` in `<heap.hpp>` is a manually implemented binary min-heap provided by
`search_core`. It stores a unique `std::size_t` node ID and a `double` priority
per entry. Use the accumulated cost `g(n)` for Dijkstra, or `g(n) + h(n)` for A*.
Lower priorities come first; ties use the smaller node ID for repeatable runs.

```cpp
#include <heap.hpp>

Heap frontier;
frontier.push(0, 0.0);
frontier.push(1, 8.0);
frontier.push(2, 3.0);
frontier.decreaseKey(1, 2.0); // A better route to an existing frontier node.

while(!frontier.empty()) {
    const auto next = frontier.pop(); // Nodes 0, 1, then 2.
    // Expand next.node using next.priority.
}
```

`pop` and `decreaseKey` take expected O(log n) time; `push` takes amortized
expected O(log n), including storage growth. A node-to-index hash table supports
expected O(1) `contains`; `top`, `size`, and `empty` take O(1). Storage is O(n).
`clear` empties the heap so it can be reused.

- `top()` returns a const reference to the minimum without removing it; do not
  retain the reference across mutations. `pop()` returns the removed entry by value.
- Empty `top`/`pop` and `decreaseKey` on a missing node throw `std::out_of_range`.
- Duplicate insertion, increasing a priority with `decreaseKey`, and NaN
  priorities throw `std::invalid_argument` without changing the heap.
- Equal-key updates are allowed. Finite negative costs and infinities are
  allowed; algorithms remain responsible for validating graph edge weights.
- Popped nodes are no longer members and may be inserted again. Use `contains`
  to choose between inserting a newly discovered node and decreasing a queued
  node's priority. Track visited/closed nodes separately in the search algorithm.

## GNU GCC attributes

The heap targets GCC with C++23. Its attributes express implementation contracts
and optimization hints; they do not change priority ordering or exception types.

| Attribute | Application |
| --- | --- |
| `[[nodiscard]]` with diagnostics | Observers, extracted entries, and the comparator; use `(void)frontier.pop()` for an intentional discard. |
| `[[gnu::pure]]` | `empty`, `size`, `contains`, and `precedes` only read memory and have no observable side effects. |
| `[[gnu::always_inline]]` | The two trivial, inline observers `empty` and `size`. |
| `[[gnu::access(read_only, ...)]]` | Comparator inputs and exception messages are read without modification. |
| `[[noreturn]] [[gnu::cold]] [[gnu::noinline]]` | Internal throwing helpers keep exception construction separate from normal operations. |
| `[[gnu::nonnull(1)]]` | Internal throwing helpers always receive a non-null message literal. |
| `[[unlikely]]` | Invalid-input and empty-access branches are expected to be uncommon. |

`contains` is also `noexcept`: its fixed integer hash/equality operations do not
throw and lookup does not allocate. Throwing operations retain their exception
contracts. In particular, `top` is not marked `pure`, because its empty-heap
exception is observable; memory-reading observers cannot use `gnu::const`.
No fast-math options are required or enabled: NaN rejection and infinity
ordering depend on normal floating-point semantics.

Attribute contracts follow the
[GCC function attribute documentation](https://gcc.gnu.org/onlinedocs/gcc-15.2.0/gcc/Common-Function-Attributes.html).

After configuring with `BUILD_TESTS=ON` and building the `tests` target, run:

```sh
./build-tests/bin/tests '[heap]'
```
