# Grid visualization

Open **Search → Grid**. The sidebar scrolls when the window is short and moves above the canvas in narrow windows.

- **Play / Pause** runs or freezes the search. **Previous step / Next step** pauses and moves by one explored node. Step 0 contains only the starting frontier.
- **Restart** returns to step 0 and pauses, preserving obstacles, endpoints, heuristic, and speed.
- Choose **None (Dijkstra)**, **Manhattan (A\*)**, or **Euclidean (A\*)**. Each move costs 1 and moves up, down, left, or right.
- **Speed** ranges from 1 to 100,000 steps per second. Work is capped at 4,096 steps per frame to keep controls responsive; actual speed depends on frame rate.
- **Grid size** is the total number of cells, from **30 to 10,000,000**. Ctrl+click the slider to type an exact value. The cells form near-square rows, with a partial last row when needed. Resizing applies on release and clears the map. Practical sizes are constrained by available memory and the GPU's maximum texture dimensions; the configured upper limit is not a guarantee that a 10-million-cell map can be loaded.
- Select **Obstacle**, **Erase**, **Start**, or **Goal** in the paint tool. Left drag paints or erases; click places an endpoint. Endpoints cannot overlap or become obstacles. Moving an endpoint onto an obstacle clears that cell.
- **Zoom** ranges from **0.25× to 64×** relative to fitting the grid in the canvas. Use the sidebar slider or mouse wheel; wheel zoom stays anchored to the pointer. Right or middle drag pans. **Fit grid** restores 1× and centers the map.

While playing, painting, endpoint changes, resizing, clearing obstacles, and heuristic selection are disabled. Pause to edit; an edit discards the old search history. Speed, zoom, panning, and playback controls remain available. A completed search shows its shortest path, or **No path** if the goal is unreachable.

The visualization uses implicit neighbors and a one-pixel-per-cell texture. Backward playback stores only changed search values, so million-cell maps do not require dense adjacency matrices or complete state copies for every step. The existing `Grid::matrix()` API remains available, materialized on demand for small graph searches.
