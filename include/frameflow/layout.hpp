#pragma once

#include <cstdint>
#include <vector>

namespace frameflow {
    struct float2 {
        float x = 0.f;
        float y = 0.f;

        float2 operator+(const float2 &other) const { return {x + other.x, y + other.y}; }
        float2 operator-(const float2 &other) const { return {x - other.x, y - other.y}; }
        float2 operator*(const float scalar) const { return {x * scalar, y * scalar}; }

        float2 &operator+=(const float2 &other) {
            x += other.x;
            y += other.y;
            return *this;
        }

        float2 &operator-=(const float2 &other) {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        static float2 min(const float2 &a, const float2 &b) {
            return {std::min(a.x, b.x), std::min(a.y, b.y)};
        }

        static float2 max(const float2 &a, const float2 &b) {
            return {std::max(a.x, b.x), std::max(a.y, b.y)};
        }
    };

    // Axis-aligned rectangle.
    // Coordinate system is intentionally unspecified.
    struct Rect {
        float2 origin;
        float2 size;
    };

    // Generational index for safe node references
    struct NodeId {
        uint32_t index = UINT32_MAX;
        uint32_t generation = 0;

        bool operator==(const NodeId &other) const {
            return index == other.index && generation == other.generation;
        }

        bool operator!=(const NodeId &other) const {
            return !(*this == other);
        }

        [[nodiscard]] bool is_null() const {
            return index == UINT32_MAX;
        }
    };

    constexpr NodeId NullNode = {UINT32_MAX, 0};

    enum class NodeType : uint8_t {
        Generic,
        Center,
        Box,
        Flow,
        Margin
        // Scroll?
        //
    };

    enum class Direction {
        Horizontal,
        Vertical,
    };

    enum class Align {
        Start,
        Center,
        End,
        SpaceBetween
    };

    struct BoxData {
        Direction direction = Direction::Horizontal;
        Align align = Align::Start;
    };

    struct FlowData {
        Direction direction = Direction::Horizontal;
        Align align = Align::Start;
    };

    struct MarginData {
        float left = 0.f;
        float right = 0.f;
        float top = 0.f;
        float bottom = 0.f;
    };

    struct Components {
        std::vector<BoxData> boxes;
        std::vector<FlowData> flows;
        std::vector<MarginData> margins;

        std::vector<size_t> free_boxes;
        std::vector<size_t> free_flows;
        std::vector<size_t> free_margins;
    };;

    // Anchors normalized [0..1] relative to parent
    struct Anchors {
        float left = 0.f;
        float top = 0.f;
        float right = 0.f;
        float bottom = 0.f;
    };

    // Pixel offsets from anchors
    struct Offsets {
        float left = 0.f;
        float top = 0.f;
        float right = 0.f;
        float bottom = 0.f;
    };

    struct Node {
        Rect bounds;
        float2 minimum_size;

        // Godot-style sizing
        float2 expand = {0.f, 0.f};    // 1 = expand along axis
        float2 stretch = {1.f, 1.f};   // relative weighting when expanding

        // Anchors
        Anchors anchors;
        Offsets offsets;

        NodeId parent = NullNode;
        std::vector<NodeId> children;

        NodeType type;
        size_t component_index = 0;

        // Generation tracking
        uint32_t generation = 0;
        bool alive = true;
    };;

    // A tree root, all ancestors of root are have relative positions to this System
    // Analogous to CanvasLayer in Godot
    // This is designed to have multiple root nodes if you wish.
    // In your engine, you can abstract over this to have only one root.
    struct System {
        std::vector<Node> nodes;
        Components components;
        std::vector<NodeId> children;
        std::vector<uint32_t> free_list; // Indices available for reuse
    };

    NodeId add_center(System *sys, NodeId parent);

    NodeId add_generic(System *sys, NodeId parent);

    NodeId add_box(System *sys, NodeId parent, const BoxData &data);

    NodeId add_flow(System *sys, NodeId parent, const FlowData &data);

    NodeId add_margin(System *sys, NodeId parent, const MarginData &data);

    Node *get_node(System *sys, NodeId id);
    const Node *get_node(const System *sys, NodeId id);
    // add const version?

    // Check if a NodeId is valid
    bool is_valid(const System *sys, NodeId id);

    // Delete a node and all its descendants
    // Returns false if the node doesn't exist or is already deleted
    bool delete_node(System *sys, NodeId id);

    // Move a node to a new parent
    // Returns false if either node doesn't exist or if it would create a cycle
    bool reparent_node(System *sys, NodeId node_id, NodeId new_parent);

    void compute_layout(System *sys, NodeId node_id);
} // namespace frameflow
