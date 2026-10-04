---
title: Design
sidebar_position: 2
---

# Design

VISR explores a simple idea: an interface should be able to accompany a person across devices and contexts instead of being bound to a single screen or operating system.

The project separates application intent from presentation and device implementation. Applications describe state and actions; the runtime interprets interface documents; device-specific systems provide capabilities such as views, tracking, and presentation. That separation is visible in the current Application, Runtime, XR, and Presentation boundaries, while the broader experience remains a work in progress.

# Experience
Visr aims to adapt to the capabilities of the device and the context in which it is used, providing a seamless and consistent experience across different environments and hardware configurations.
**Legend:** ✅ Available · ❌ Unavailable · ◐ Partial / implementation-dependent

| Capability | Meta Ray-Ban Display | RayNeo iO | XREAL Air 2 Ultra | Meta Quest 3 | Apple Vision Pro |
|---|:---:|:---:|:---:|:---:|:---:|
| | <img src="/img/devices/meta-rayban-display.png" height="90"/> | <img src="/img/devices/rayneo-io.png" height="90"/> | <img src="/img/devices/xreal-air-2-ultra.png" height="90"/> | <img src="/img/devices/quest-3.png" height="90"/> | <img src="/img/devices/vision-pro.png" height="90"/> |
| **Display** | ✅ | ✅ | ✅ | ✅ | ✅ |
| ↳ Stereo | ❌ | ✅ | ✅ | ✅ | ✅ |
| ↳ Color | ✅ | ❌ | ✅ | ✅ | ✅ |
| **Environment View** | | | | | |
| ↳ Optical See-Through | ✅ | ✅ | ✅ | ❌ | ❌ |
| ↳ Video Passthrough | ❌ | ❌ | ❌ | ✅ | ✅ |
| **Tracking** | | | | | |
| ↳ Head Orientation | ◐ | ✅ | ✅ | ✅ | ✅ |
| ↳ Head Position | ❌ | ❌ | ✅ | ✅ | ✅ |
| ↳ Hands | ❌ | ❌ | ◐ | ✅ | ✅ |
| ↳ Eyes | ❌ | ❌ | ❌ | ❌ | ✅ |
| ↳ Body | ❌ | ❌ | ❌ | ◐ | ◐ |
| **Environment** | | | | | |
| ↳ RGB Camera | ✅ | ❌ | ◐ | ✅ | ✅ |
| ↳ Depth | ❌ | ❌ | ◐ | ✅ | ✅ |
| ↳ Spatial Mapping | ❌ | ❌ | ◐ | ✅ | ✅ |
| ↳ Plane Detection | ❌ | ❌ | ◐ | ✅ | ✅ |
| **Audio** | | | | | |
| ↳ Microphone | ✅ | ✅ | ◐ | ✅ | ✅ |
| ↳ Output | ✅ | ❌ | ◐ | ✅ | ✅ |
| **Input Devices** | Neural Band, Touch, Voice | Smart Crown, Voice, Head Gestures | Hand Tracking, External Controller | Touch Plus Controllers, Hand Tracking, Voice | Hand Tracking, Eye Gaze, Voice |
| **Haptics** | ✅ Neural Band | ❌ | ◐ | ✅ Controllers | ❌ |
## In this section

- [Vision](./vision.md)
- [Goals](./goals.md)
- [Inspirations](./inspirations.md)
