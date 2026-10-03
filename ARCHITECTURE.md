# ARUI architecture

```text
Core
Protocol
Language
Application
Runtime
├── Document
├── Layout
├── Style
├── Interaction
└── Animation
Scripting
Render
XR
├── Display
├── Tracking
└── OpenXR
Tools
└── Simulator
Server
├── Manager
└── Desktop
```

## Ownership

- **Application** defines the fundamental native contract between an app and
  the ARUI environment. Apps register state, actions, metadata, and optional
  templates. Applications do not own windows or visual surfaces.
- **Scripting** owns JavaScript hosting, bindings, and programmable panels. It
  adapts scripts to the Application contract.
- **Runtime** turns registered capabilities and language documents into a live
  interface through its Document, Layout, Style, Interaction, and Animation
  subsystems.
- **XR/Display** and **XR/Tracking** define portable device contracts, while
  **XR/OpenXR** implements those contracts for an OpenXR runtime, including
  headless EGL presentation, stereo views, tracking, and environment sensing.
- **Tools/Simulator** provides a development-only display and tracking backend.
- **Server/Manager** coordinates runtime, rendering, and XR services.
  **Server/Desktop** is the desktop executable and owns its platform window.

The dependency direction is from integration layers toward contracts:

## Responsibility graph

The detailed Graphviz source is [`docs/architecture.dot`](docs/architecture.dot).
Render it with:

```sh
dot -Tsvg docs/architecture.dot -o docs/architecture.svg
```

The XR boundary deliberately separates three responsibilities:

- `IViewProvider` supplies per-view metrics: view/projection matrices and viewport.
- `IXRTracker` supplies tracked poses, joints, and events.
- `IPresenter` owns the frame/presentation lifecycle and displays a render target.

There is no standalone `RenderList` class today. The graph labels the typed
per-frame vectors owned by `Renderer` as the effective render list, without
implying an interface that does not exist. Solid edges describe current code;
dashed edges describe intended integration or data flow that is not wired yet.


