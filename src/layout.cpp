#include "frameflow/layout.hpp"

#include <iostream>

namespace frameflow {

static void layout_generic() { /* default layout */ }

static void layout_center(System& sys, const Node& node) {
    if (node.children.empty()) return;

    // Compute total bounding box of children based on minimum_size
    float2 max_child_size{0.f, 0.f};
    for (const auto child_id : node.children) {
        Node& child = get_node(sys, child_id);
        max_child_size.x = std::max(max_child_size.x, child.minimum_size.x);
        max_child_size.y = std::max(max_child_size.y, child.minimum_size.y);
    }

    // Compute origin to center each child
    for (const auto child_id : node.children) {
        Node& child = get_node(sys, child_id);
        float2 offset{
            (node.bounds.size.x - child.minimum_size.x) * 0.5f,
            (node.bounds.size.y - child.minimum_size.y) * 0.5f
        };
        child.bounds.origin = node.bounds.origin + offset;
        child.bounds.size = child.minimum_size;

        // Recursively layout child nodes
        compute_layout(sys, child_id);
    }
}

static void layout_box(System& sys, const Node& node, const BoxData& data) {
    if (node.children.empty()) return;

    float2 offset = node.bounds.origin;

    for (const auto child_id : node.children) {
        Node& child = get_node(sys, child_id);

        // Assign size from minimum_size
        child.bounds.size = child.minimum_size;

        // Position based on direction
        if (data.direction == Direction::Horizontal) {
            child.bounds.origin = offset;
            offset.x += child.bounds.size.x;
        } else { // Vertical
            child.bounds.origin = offset;
            offset.y += child.bounds.size.y;
        }

        // Recursively layout
        compute_layout(sys, child_id);
    }
}

static void layout_flow(System& sys, const Node& node, const FlowData& data) {
    if (node.children.empty()) return;

    float2 offset = node.bounds.origin;
    float line_height = 0.f;

    for (auto child_id : node.children) {
        Node& child = get_node(sys, child_id);
        child.bounds.size = child.minimum_size;

        if (data.direction == Direction::Horizontal) {
            // Wrap to next line if it exceeds parent width
            if (offset.x + child.bounds.size.x > node.bounds.origin.x + node.bounds.size.x) {
                offset.x = node.bounds.origin.x; // reset X
                offset.y += line_height;         // move Y down
                line_height = 0.f;               // reset line height
            }

            child.bounds.origin = offset;

            // Advance offset for next child
            offset.x += child.bounds.size.x;

            // Track the tallest child in this line for vertical offset
            line_height = std::max(line_height, child.bounds.size.y);

        } else { // Vertical flow (simple stack for now)
            child.bounds.origin = offset;
            offset.y += child.bounds.size.y;
        }

        // Recursively layout children of this child
        compute_layout(sys, child_id);
    }
}


NodeId add_generic(System& sys, NodeId parent) {
    // 1. Add component data
    // NA

    // 2. Add node
    const auto id = static_cast<NodeId>(sys.nodes.size());
    Node node;
    node.type = NodeType::Generic;
    node.bounds = {}; // default
    node.minimum_size = {};
    node.parent = parent;
    sys.nodes.push_back(node);

    // 3. Link to parent
    if (parent != NullNode) {
        sys.nodes[parent].children.push_back(id);
    }

    return id;
}

NodeId add_center(System& sys, NodeId parent) {
    // 1. Add component data

    // 2. Add node
    const auto id = static_cast<NodeId>(sys.nodes.size());
    Node node;
    node.type = NodeType::Center;
    node.bounds = {}; // default
    node.minimum_size = {};
    node.parent = parent;
    sys.nodes.push_back(node);

    // 3. Link to parent
    if (parent != NullNode) {
        sys.nodes[parent].children.push_back(id);
    }

    return id;
}

NodeId add_box(System& sys, NodeId parent, const BoxData& data) {
    // 1. Add component data
    const size_t comp_idx = sys.components.boxes.size();
    sys.components.boxes.push_back(data);

    // 2. Add node
    const auto id = static_cast<NodeId>(sys.nodes.size());
    Node node;
    node.type = NodeType::Box;
    node.bounds = {};
    node.minimum_size = {};
    node.parent = parent;
    node.component_index = comp_idx;
    sys.nodes.push_back(node);

    // 3. Link to parent
    if (parent != NullNode)
        sys.nodes[parent].children.push_back(id);

    return id;
}

NodeId add_flow(System& sys, const NodeId parent, const FlowData& data) {
    // 1. Add component data
    const size_t comp_idx = sys.components.flows.size();
    sys.components.flows.push_back(data);

    // 2. Add node
    const auto id = static_cast<NodeId>(sys.nodes.size());
    Node node;
    node.type = NodeType::Flow;
    node.bounds = {};
    node.minimum_size = {};
    node.parent = parent;
    node.component_index = comp_idx;
    sys.nodes.push_back(node);

    // 3. Link to parent
    if (parent != NullNode)
        sys.nodes[parent].children.push_back(id);

    return id;
}


void compute_layout(System& sys, NodeId node_id) {
    Node& node = sys.nodes[node_id];
    switch (node.type) {
        case NodeType::Generic: layout_generic(); break;
        case NodeType::Center:  
            layout_center(sys, node); break;
        case NodeType::Box:   
            layout_box(sys, node, sys.components.boxes[node.component_index]); break;
        case NodeType::Flow:   
            layout_flow(sys, node, sys.components.flows[node.component_index]); break;
        default: break;
    }

    for (auto child_id : node.children)
        compute_layout(sys, child_id);
}

Node& get_node(System& sys, NodeId id) {
    return sys.nodes[id];
}

} // namespace frameflow
