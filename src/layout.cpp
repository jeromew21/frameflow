#include "frameflow/layout.hpp"

#include <iostream>

namespace frameflow {
    static void resolve_anchors(Node &child, const Node &parent) {
        // Compute rectangle from anchors + offsets
        float parent_left = parent.bounds.origin.x;
        float parent_top = parent.bounds.origin.y;
        float parent_right = parent_left + parent.bounds.size.x;
        float parent_bottom = parent_top + parent.bounds.size.y;

        float x0 = parent_left + child.anchors.left * parent.bounds.size.x + child.offsets.left;
        float y0 = parent_top + child.anchors.top * parent.bounds.size.y + child.offsets.top;
        float x1 = parent_left + child.anchors.right * parent.bounds.size.x - child.offsets.right;
        float y1 = parent_top + child.anchors.bottom * parent.bounds.size.y - child.offsets.bottom;

        // Only override bounds if anchors define a nonzero area
        if (x1 > x0) child.bounds.origin.x = x0, child.bounds.size.x = x1 - x0;
        if (y1 > y0) child.bounds.origin.y = y0, child.bounds.size.y = y1 - y0;
    }

    static void layout_generic(System &sys, const Node &node) {
        for (NodeId child_id: node.children) {
            Node &child = get_node(sys, child_id);

            // Compute anchors
            resolve_anchors(child, node);

            // Apply minimum size
            child.bounds.size.x = std::max(child.bounds.size.x, child.minimum_size.x);
            child.bounds.size.y = std::max(child.bounds.size.y, child.minimum_size.y);

            // Apply expand (fill parent along axis)
            if (child.expand.x > 0.f)
                child.bounds.size.x = std::max(child.bounds.size.x, node.bounds.size.x);
            if (child.expand.y > 0.f)
                child.bounds.size.y = std::max(child.bounds.size.y, node.bounds.size.y);
        }
    }


    static void layout_center(System &sys, const Node &node) {
        if (node.children.empty()) return;

        for (NodeId child_id: node.children) {
            Node &child = get_node(sys, child_id);

            resolve_anchors(child, node);

            // Start with minimum size
            float2 size = child.minimum_size;

            // Apply expand
            if (child.expand.x > 0.f) size.x = node.bounds.size.x;
            if (child.expand.y > 0.f) size.y = node.bounds.size.y;

            // Center inside parent
            float2 offset{
                (node.bounds.size.x - size.x) * 0.5f,
                (node.bounds.size.y - size.y) * 0.5f
            };
            child.bounds.origin = node.bounds.origin + offset;
            child.bounds.size = size;
        }
    }


    static void layout_box(System &sys, const Node &node, const BoxData &data) {
        if (node.children.empty()) return;

        // Precompute total fixed size & total stretch
        float total_main = 0.f;
        float total_stretch = 0.f;
        for (NodeId child_id: node.children) {
            Node &c = get_node(sys, child_id);
            total_main += (data.direction == Direction::Horizontal ? c.minimum_size.x : c.minimum_size.y);
            if ((data.direction == Direction::Horizontal ? c.expand.x : c.expand.y) > 0.f)
                total_stretch += (data.direction == Direction::Horizontal ? c.stretch.x : c.stretch.y);
        }

        float parent_main_size = (data.direction == Direction::Horizontal ? node.bounds.size.x : node.bounds.size.y);
        float leftover = std::max(0.f, parent_main_size - total_main);

        // Determine starting cursor based on alignment
        float cursor = (data.direction == Direction::Horizontal ? node.bounds.origin.x : node.bounds.origin.y);
        float spacing = 0.f;

        float content_size = total_main + leftover; // for SpaceBetween, spacing will overwrite
        size_t child_count = node.children.size();

        switch (data.align) {
            case Align::Start: break;
            case Align::Center: cursor += leftover * 0.5f;
                break;
            case Align::End: cursor += leftover;
                break;
            case Align::SpaceBetween:
                if (child_count > 1) spacing = leftover / (child_count - 1);
                break;
        }

        // Layout children
        for (NodeId child_id: node.children) {
            Node &c = get_node(sys, child_id);

            resolve_anchors(c, node);

            float2 size = c.minimum_size;
            float expand_axis = (data.direction == Direction::Horizontal ? c.expand.x : c.expand.y);
            float stretch_axis = (data.direction == Direction::Horizontal ? c.stretch.x : c.stretch.y);

            if (expand_axis > 0.f && total_stretch > 0.f)
                size = (data.direction == Direction::Horizontal
                            ? float2{size.x + leftover * (stretch_axis / total_stretch), size.y}
                            : float2{size.x, size.y + leftover * (stretch_axis / total_stretch)});

            // Assign position and size
            if (data.direction == Direction::Horizontal) {
                c.bounds.origin = {cursor, node.bounds.origin.y};
                c.bounds.size.x = size.x;
                c.bounds.size.y = std::max(c.bounds.size.y, size.y);
                cursor += size.x + spacing;
            } else {
                c.bounds.origin = {node.bounds.origin.x, cursor};
                c.bounds.size.y = size.y;
                c.bounds.size.x = std::max(c.bounds.size.x, size.x);
                cursor += size.y + spacing;
            }
        }
    }

    static void layout_flow(System &sys, const Node &node, const FlowData &data) {
        if (node.children.empty()) return;

        float2 offset = node.bounds.origin;
        float cross_line = 0.f;

        for (NodeId child_id: node.children) {
            Node &child = get_node(sys, child_id);

            resolve_anchors(child, node);

            // Start with minimum size
            float2 size = child.minimum_size;

            // Expand on cross axis only
            if (data.direction == Direction::Horizontal && child.expand.y > 0.f) size.y = node.bounds.size.y;
            if (data.direction == Direction::Vertical && child.expand.x > 0.f) size.x = node.bounds.size.x;

            // Wrap if necessary
            if (data.direction == Direction::Horizontal) {
                float parent_right = node.bounds.origin.x + node.bounds.size.x;
                if (offset.x + size.x > parent_right) {
                    offset.x = node.bounds.origin.x;
                    offset.y += cross_line;
                    cross_line = 0.f;
                }
                child.bounds.origin = offset;
                child.bounds.size = size;
                offset.x += size.x;
                cross_line = std::max(cross_line, size.y);
            } else {
                float parent_bottom = node.bounds.origin.y + node.bounds.size.y;
                if (offset.y + size.y > parent_bottom) {
                    offset.y = node.bounds.origin.y;
                    offset.x += cross_line;
                    cross_line = 0.f;
                }
                child.bounds.origin = offset;
                child.bounds.size = size;
                offset.y += size.y;
                cross_line = std::max(cross_line, size.x);
            }
        }
    }

    NodeId add_generic(System &sys, const NodeId parent) {
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

    NodeId add_center(System &sys, const NodeId parent) {
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

    NodeId add_box(System &sys, const NodeId parent, const BoxData &data) {
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

    NodeId add_flow(System &sys, const NodeId parent, const FlowData &data) {
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

    void compute_layout(System &sys, const NodeId node_id) {
        const Node &node = sys.nodes[node_id];
        switch (node.type) {
            case NodeType::Generic: layout_generic(sys, node);
                break;
            case NodeType::Center:
                layout_center(sys, node);
                break;
            case NodeType::Box:
                layout_box(sys, node, sys.components.boxes[node.component_index]);
                break;
            case NodeType::Flow:
                layout_flow(sys, node, sys.components.flows[node.component_index]);
                break;
            default: break;
        }

        for (const auto child_id: node.children)
            compute_layout(sys, child_id);
    }

    Node &get_node(System &sys, const NodeId id) {
        return sys.nodes[id];
    }
} // namespace frameflow
