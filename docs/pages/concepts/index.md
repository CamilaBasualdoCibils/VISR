---
title: Concepts
sidebar_position: 3
---

# Concepts

VISR uses a few distinct layers to keep application meaning, interface structure, and device work from collapsing into one component.

## Application

An application exposes state, actions, metadata, and optional templates through the Application contract. It does not own the environment’s window or visual surfaces.

## Document and runtime

VISR documents describe interface structure and styles. Runtime resolves those documents against application data and produces a live interface tree with layout and interaction behavior.

## Capabilities and implementations

Display views, tracked input, and presentation are separate device-facing responsibilities. OpenXR and the simulator provide implementations for development and runtime integration.

## Still taking shape

Discovery, capability negotiation, portability guarantees, and fallback policies need further design. The current source boundaries should not be read as a complete system specification.
