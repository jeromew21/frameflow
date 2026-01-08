#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace frameflow {

struct float2 {
    float x = 0.f;
    float y = 0.f;

    float2 operator+(const float2& other) const { return {x + other.x, y + other.y}; }
    float2 operator-(const float2& other) const { return {x - other.x, y - other.y}; }
    float2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    float2& operator+=(const float2& other) { x += other.x; y += other.y; return *this; }
    float2& operator-=(const float2& other) { x -= other.x; y -= other.y; return *this; }

    static float2 min(const float2& a, const float2& b) {
        return { std::min(a.x, b.x), std::min(a.y, b.y) };
    }
    static float2 max(const float2& a, const float2& b) {
        return { std::max(a.x, b.x), std::max(a.y, b.y) };
    }
};

// Axis-aligned rectangle.
// Coordinate system is intentionally unspecified.
struct Rect {
    float2 origin;
    float2 size;
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

struct BoxData {
    Direction direction;
};

struct FlowData {
    Direction direction;
};

struct Components {
    std::vector<BoxData> boxes;
    std::vector<FlowData> flows;
};

struct Node {
    Rect bounds;
    float2 minimum_size;
    NodeId parent = NullNode;
    std::vector<NodeId> children; // or a contiguous child arena for performance
    NodeType type;
    // anchors, stretch
    size_t component_index = SIZE_T_MAX;
};

// A tree root, all ancestors of root are have relative positions to this System
// Analagous to CanvasLayer in Godot
struct System {
    std::vector<Node> nodes;
    Components components;
    Rect bounds;
    std::vector<NodeId> children;
};

NodeId add_center(System& sys, NodeId parent);

NodeId add_generic(System& sys, NodeId parent);

NodeId add_box(System& sys, NodeId parent, const BoxData &data);

NodeId add_flow(System& sys, NodeId parent, const FlowData &data);

Node& get_node(System& sys, NodeId id);

void compute_layout(System& sys, NodeId node_id);

} // namespace frameflow
