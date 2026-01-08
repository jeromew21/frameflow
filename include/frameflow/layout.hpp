#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace frameflow {

struct Point {
    float x = 0.f;
    float y = 0.f;
};

struct Size {
    float width = 0.f;
    float height = 0.f;
};

// Axis-aligned rectangle.
// Coordinate system is intentionally unspecified.
struct Rect {
    Point origin;
    Size size;
};

using NodeId = uint32_t; // index into arrays
constexpr NodeId NullNode = UINT32_MAX;

enum class NodeType : uint8_t {
    Generic,
    Center,
    Box,
    Flow,
    // Scroll?
    // 
};

enum class Direction
{
    Horizontal,
    Vertical,
};

struct CenterData {};

struct BoxData {};

struct FlowData {};

struct Components {
    std::vector<CenterData> centers;
    std::vector<BoxData> boxes;
    std::vector<FlowData> flows;
};

struct Node {
    Rect bounds;
    Rect minimum_size;
    NodeId parent = NullNode;
    std::vector<NodeId> children; // or a contiguous child arena for performance
    NodeType type;
    size_t component_index;
};

// A tree root, all ancestors of root are have relative positions to this System
// Analagous to CanvasLayer in Godot
struct System {
    std::vector<Node> nodes;
    Components components;
    Rect bounds;
    std::vector<NodeId> children;
};

NodeId add_center(System& sys,  NodeId parent, CenterData data);

NodeId add_generic(System& sys,  NodeId parent);

Node& get_node(System& sys, NodeId id);

void compute_layout(System& sys, NodeId node_id);

} // namespace frameflow
