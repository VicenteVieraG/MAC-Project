# Graph visualization

Open **Search → Graph** to follow Dijkstra's shortest-path search on a weighted,
directed graph. This tab uses no heuristic. The sidebar scrolls in short windows
and moves above the visualization in narrow windows.

## Load and configure

- Enter a file path and choose **Load graph**, use **Browse...**, or choose
  **Load example** to try the included demonstration.
- Files use the [adjacency matrix format](graph-format.md): each row is enclosed
  in `|`, weights are comma-separated, and uppercase `INF` means no connection.
  A zero weight is a real connection. Dijkstra requires nonnegative weights.
- The visualization accepts up to **512 nodes** and **8 MiB per file**. These
  limits apply to this tab, not the reader or search APIs. A failed load reports
  the error and preserves the current graph.
- Set **Start node** and **Goal node** using zero-based node IDs. They may be
  the same node. Endpoint changes reset the search; pause playback before
  changing them or loading another graph.

## Playback

- **Play / Pause** starts or freezes playback. **Step back / Step forward**
  pauses and moves by one explored node, restoring that step's frontier,
  visited nodes, tentative costs, and predecessors.
- Step 0 contains only the starting frontier. The goal is reached when it is
  removed from the frontier, not when it is first discovered.
- **Restart** returns to step 0 and pauses while preserving the graph, node
  positions, endpoints, and speed.
- **Speed** ranges from **1 to 1,000 steps per second**, initially 2. Actual
  playback speed depends on the frame rate.
- Completion highlights the shortest path and its total cost, or reports
  **No path** when the goal is unreachable. The explored-node count tracks
  playback. **Search computation** measures the complete Dijkstra run, including
  recording its steps, and excludes animation and rendering.

## Explore the graph

- Left-drag a node to move it. This changes only its position, not edge weights
  or search results, and remains available during playback. Hover over a node
  to inspect its state, cost, predecessor, and incoming/outgoing edge counts.
- Use the mouse wheel to zoom at the pointer and right or middle drag to pan.
  **Fit graph** centers the current layout.
- **Node layout** defaults to **Circular**. Choose **Random** for a scattered
  arrangement; **Randomize** generates another. Choose **Circular** to return
  to a circle, and use **Reset layout** to restore it after dragging nodes.
  Layout changes preserve playback progress, endpoints, and edge weights.
  **Zoom** ranges from **0.1× to 10×** relative to the fitted layout. **Node IDs**
  and **Edge weights** can be toggled in the sidebar.
- Arrowheads indicate direction; opposite connections use separate curves.
  Zero-valued diagonal entries are omitted from the drawing but retained in
  the matrix.
- The legend identifies start, goal, frontier, visited nodes, and the final
  path; a white ring identifies the current node.
- Dense graphs show at most **600 edges**, prioritizing the final path and
  connections of the hovered node. A notice identifies this reduced view.
  Weight labels are suppressed when the graph has more than 80 drawable edges.
- Switch between **Graph**, **Node costs**, and **Adjacency matrix** to inspect
  the same search. Only visited costs are final; undiscovered nodes have cost
  `INF`. The matrix always retains every entry and displays columns in pages
  of 16, with rows as sources and columns as destinations.
