# ComboGraph Plugin

**Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.**

A data-driven combo system plugin for Unreal Engine 5, built around a node-graph architecture with tag-based input routing, montage playback, and decay windows.

---

## Features

- **Data-driven combos** — Define entire combo chains in `UComboGraphDataAsset` without writing code
- **Tag-based input routing** — Uses Gameplay Tags to drive transitions between combo nodes
- **Montage integration** — Each node plays an `UAnimMontage` with optional section targeting
- **Decay window** — Configurable post-montage input window before the combo resets
- **Input buffering** — Inputs received during a montage are buffered and honored on completion
- **Blueprint-exposed delegates** — `OnComboBegin`, `OnComboOnGoing`, `OnComboEnd` for game logic
- **Query library** — Blueprint-callable utilities for combo depth, history, and active state
- **DataTable support** — Bulk-load multiple combo chains via `FComboGraphTableRow`

---

## Plugin Structure

```
Plugins/ComboGraph/
├── ComboGraph.uplugin
└── Source/ComboGraph/
    ├── Public/
    │   ├── ComboGraph/          # UComboGraph — runtime graph object
    │   ├── ComboNode/           # UComboNode — individual combo step
    │   ├── ComboPath/           # UComboPath — transition link between nodes
    │   ├── ComboGraphBuilder/   # FComboGraphBuilder — constructs runtime graph from data assets
    │   ├── ComboGraphDataAsset/ # UComboGraphDataAsset + FComboNodeData + FComboGraphTableRow
    │   ├── ComboGraphQueryLibrary/ # Blueprint query functions
    │   ├── Interfaces/          # IComboGraphContract — owner interface
    │   └── ComboGraphModule.h
    └── Private/                 # Implementations
```

---

## Getting Started

### 1. Enable the Plugin
Add `ComboGraph` to your project's plugin list or drop the plugin folder into your project's `Plugins/` directory.

### 2. Implement the Contract
Your character or component must implement `IComboGraphContract`:
- `GetRootNode()` — return the root `UComboNode` of the active graph
- `NotifyTransition()`, `NotifyComboEnd()`, `NotifyComboInterruption()`, `NotifyNodeActivated()` — handle graph events

### 3. Build a Combo Graph
Use `FComboGraphBuilder::BuildFromDataAsset()` to construct a runtime `UComboGraph` from a `UComboGraphDataAsset`, or `BuildFromDataTable()` to load multiple chains at once.

### 4. Route Input
Call `UComboGraph::RouteInput(FGameplayTag)` from your input handler to drive transitions.

---

## Requirements

- Unreal Engine 5.5 or later
- Plugins: `GameplayTags`, `StructUtils`

---

## License

Copyright 2026 Prasanna Keerthivasan. All Rights Reserved.

Unauthorized copying, distribution, or modification of this plugin or its source code is strictly prohibited.
