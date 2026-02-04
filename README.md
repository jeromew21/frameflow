# Frameflow

Frameflow is a small, deterministic, rectangular layout engine inspired by Godot’s UI system.
It is designed to be embedded inside game engines and real-time applications where you want explicit control over layout without inheriting a heavyweight widget framework.

Note: this is a mostly AI-generated project. I guided the API design, but most of the interal logic and this readme are AI generated.

## Goals

* **Minimal core**

  * Frameflow is a layout solver, not a UI toolkit.
  * Rendering, input, and widget behavior are intentionally out of scope.

* **Predictable behavior**

  * Layout is fully deterministic.
  * No hidden state, no implicit global configuration.

* **Engine-friendly**

  * Data-oriented structures.
  * No dynamic polymorphism.
  * No allocations during layout evaluation.

* **Composable**

  * Nodes form a tree.
  * Behavior is expressed through explicit node types and components.

## Core Concepts

### Nodes

Each element in the layout tree is a `Node`.

A node contains:

* Local bounds and minimum size
* Expansion and stretch weights
* Anchor and offset information
* Parent and child relationships
* A type tag and optional component data

Nodes are referenced using a generational handle:

```cpp
struct NodeId {
    uint32_t index;
    uint32_t generation;
};
```

This allows safe reuse of internal storage while preventing stale references.

### Node Types

Frameflow supports several built-in layout behaviors:

| Type    | Description                                |
| ---- | ---- |
| Generic | Passive container with no special behavior |
| Center  | Centers its single child                   |
| Box     | Linear layout (horizontal or vertical)     |
| Flow    | Flow layout with wrapping behavior         |
| Margin  | Adds padding around its child              |

Each specialized node stores its configuration in a component pool.

### System

A `System` owns all nodes and components:

```cpp
struct System {
    std::vector<Node> nodes;
    Components components;
    std::vector<NodeId> children;
    std::vector<uint32_t> free_list;
};
```

Multiple independent trees can coexist inside a single system.

## Basic Usage

### Creating Nodes

```cpp
frameflow::System sys;

NodeId root = add_generic(&sys, frameflow::NullNode);
NodeId box  = add_box(&sys, root, {Direction::Horizontal, Align::Start});
NodeId item = add_generic(&sys, box);
```

### Computing Layout

```cpp
compute_layout(&sys, root);
```

After computation, each node’s `bounds` field contains its resolved rectangle.

### Deleting Nodes

```cpp
delete_node(&sys, item);
```

Deletion is recursive and safe with respect to existing `NodeId` references.

### Reparenting

```cpp
reparent_node(&sys, item, new_parent);
```

## Philosophy

* **Bring your own abstraction**

  * Frameflow exposes a low-level API.
  * Wrapping it in your own widget or scene system is expected.

* **Explicit over implicit**

  * Every behavior is expressed through data.
  * There are no hidden layout rules.

* **Simple to integrate**

  * No external dependencies.

## Non-Goals

Frameflow does not provide:

* Rendering
* Input handling
* Widget logic
* Styling or theming
* Event systems

Those concerns belong to the host engine.

## License

MIT

## Status

Frameflow is intended for engine and tooling use where deterministic, lightweight UI layout is required.
The API is small and stable, and the internal design favors clarity over cleverness.
