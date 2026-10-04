---
title: Internals
sidebar_position: 7
---

# Internals

The codebase is split into CMake components that roughly follow the system boundaries. A quick source-tree map:

| Area | Responsibility |
| --- | --- |
| `Application/` | Native application contract |
| `Language/` | VISR documents, nodes, styles, parsing |
| `Runtime/` | Live trees, layout, style, interaction, animation |
| `Render/` | Renderer, render graph, graphics abstractions and backends |
| `XR/` | Display, tracking, and OpenXR contracts/implementation |
| `Presentation/` | Privileged presentation controller and RPC endpoint |
| `Scripting/` | JavaScript hosting and application bindings |
| `Server/Runtime/` | Long-lived runtime composition root |
| `Tools/Simulator/` | Development display/tracking implementations |

The separation is useful for orientation, but the tree is evolving. See [Architecture](../architecture/index.md) for subsystem ownership and integration notes.
