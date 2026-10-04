---
title: Architecture
sidebar_position: 4
---

# Architecture

VISR is organized around a set of portable contracts and the runtime services that compose them. The graph below is a project map, not a definitive specification: dashed links indicate intended or not-yet-wired integration, while solid links indicate the relationships described by the current architecture notes.

import Graphviz from '@site/src/components/Graphviz';

<Graphviz label="VISR subsystem responsibility graph">{`digraph VISR {
  graph [rankdir=LR, bgcolor="transparent", nodesep=0.35, ranksep=0.6];
  node [shape=box, style="rounded,filled", fontname="sans-serif", color="#718096", fillcolor="#eef0ff", fontcolor="#1f2937"];
  edge [fontname="sans-serif", color="#64748b", fontcolor="#475569"];
  app [label="Application\\nstate · actions · metadata"];
  language [label="Language\\ndocuments · nodes · styles"];
  runtime [label="Runtime\\ntree · layout · interaction"];
  render [label="Render\\nrenderer · render graph · backends"];
  xr [label="XR contracts\\nviews · tracking · presentation"];
  server [label="Server / Runtime\\ncomposition root"];
  app -> runtime [label="application contract"];
  language -> runtime [label="documents + styles"];
  runtime -> render [style=dashed, label="paint output"];
  xr -> render [style=dashed, label="per-view rendering"];
  server -> runtime [label="owns / drives"];
  server -> render [label="owns / drives"];
  server -> xr [label="integrates"];
}`}</Graphviz>

## Responsibility boundaries

- **Application** defines the native contract between an app and its environment.
- **Runtime** resolves documents and application data into a live interface.
- **Render** translates typed render objects into graphics work.
- **XR** separates view metrics, tracking data, and frame presentation.
- **Server/Runtime** is the long-lived composition root. Desktop and boot policy belong to VISR OS, outside this repository.

For implementation details and caveats, see the repository’s [architecture notes](https://github.com/CamilaBasualdoCibils/visr/blob/main/ARCHITECTURE.md).
