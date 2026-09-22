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
  **XR/OpenXR** implements those contracts for an OpenXR runtime.
- **Tools/Simulator** provides a development-only display and tracking backend.
- **Server/Manager** coordinates runtime, rendering, and XR services.
  **Server/Desktop** is the desktop executable and owns its platform window.

The dependency direction is from integration layers toward contracts:


