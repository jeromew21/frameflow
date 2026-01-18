#include "frameflow/layout.hpp"

#include <iostream>
#include <algorithm>

namespace frameflow {
    static void resolve_anchors(Node &child, const Node &parent) {
        // Compute rectangle from anchors + offsets
        float parent_left = parent.bounds.origin.x;
        float parent_top = parent.bounds.origin.y;
        /*
        float parent_right = parent_left + parent.bounds.size.x;
        float parent_bottom = parent_top + parent.bounds.size.y;
        */

        float x0 = parent_left + child.anchors.left * parent.bounds.size.x + child.offsets.left;
        float y0 = parent_top + child.anchors.top * parent.bounds.size.y + child.offsets.top;
        float x1 = parent_left + child.anchors.right * parent.bounds.size.x - child.offsets.right;
        float y1 = parent_top + child.anchors.bottom * parent.bounds.size.y - child.offsets.bottom;

        // Only override bounds if anchors define a nonzero area
        if (x1 > x0) child.bounds.origin.x = x0, child.bounds.size.x = x1 - x0;
        if (y1 > y0) child.bounds.origin.y = y0, child.bounds.size.y = y1 - y0;
    }

    static void layout_generic(System *sys, const Node &node) {
        for (NodeId child_id: node.children) {
            Node &child = *get_node(sys, child_id);

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


    static void layout_center(System *sys, const Node &node) {
        if (node.children.empty()) return;

        for (NodeId child_id: node.children) {
            Node &child = *get_node(sys, child_id);

            resolve_anchors(child, node);

            // Start with minimum size
            float2 size = child.minimum_size;
            // how to get size to be ratio of parent?

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


    static void layout_box(System *sys, const Node &node, const BoxData &data) {
        if (node.children.empty()) return;

        // Precompute total fixed size & total stretch
        float total_main = 0.f;
        float total_stretch = 0.f;
        for (NodeId child_id: node.children) {
            Node &c = *get_node(sys, child_id);
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
            Node &c = *get_node(sys, child_id);

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

    static void layout_flow(System *sys, const Node &node, const FlowData &data) {
        if (node.children.empty()) return;

        float2 offset = node.bounds.origin;
        float cross_line = 0.f;

        for (NodeId child_id: node.children) {
            Node &child = *get_node(sys, child_id);

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

    static void layout_margin(System *sys, const Node &node, const MarginData &data) {
        if (node.children.empty()) return;

        // Compute inner rect
        float2 inner_origin{
            node.bounds.origin.x + data.left,
            node.bounds.origin.y + data.top
        };

        float2 inner_size{
            std::max(0.f, node.bounds.size.x - data.left - data.right),
            std::max(0.f, node.bounds.size.y - data.top - data.bottom)
        };

        for (NodeId child_id: node.children) {
            Node &child = *get_node(sys, child_id);

            // Override parent bounds temporarily
            Node fake_parent = node;
            fake_parent.bounds.origin = inner_origin;
            fake_parent.bounds.size = inner_size;

            resolve_anchors(child, fake_parent);

            // Apply minimum size
            child.bounds.size.x = std::max(child.bounds.size.x, child.minimum_size.x);
            child.bounds.size.y = std::max(child.bounds.size.y, child.minimum_size.y);

            // Expand behavior
            if (child.expand.x > 0.f)
                child.bounds.size.x = std::max(child.bounds.size.x, inner_size.x);
            if (child.expand.y > 0.f)
                child.bounds.size.y = std::max(child.bounds.size.y, inner_size.y);
        }
    }


    static NodeId allocate_node(System *sys) {
        uint32_t index;
        uint32_t generation;

        if (!sys->free_list.empty()) {
            // Reuse a freed slot
            index = sys->free_list.back();
            sys->free_list.pop_back();
            generation = sys->nodes[index].generation;

            Node &node = sys->nodes[index];
            node.bounds = {};
            node.minimum_size = {};
            node.expand = {0.f, 0.f};
            node.stretch = {1.f, 1.f};
            node.anchors = {0.f, 0.f, 0.f, 0.f};
            node.offsets = {0.f, 0.f, 0.f, 0.f};
            node.children.clear();
        } else {
            // Allocate new slot
            index = static_cast<uint32_t>(sys->nodes.size());
            generation = 0;
            sys->nodes.emplace_back();
        }

        return {index, generation};
    }

    NodeId add_generic(System *sys, const NodeId parent) {
        // Validate parent
        if (!parent.is_null() && !is_valid(sys, parent)) {
            return NullNode;
        }

        NodeId id = allocate_node(sys);
        Node &node = sys->nodes[id.index];

        node.type = NodeType::Generic;
        node.bounds = {};
        node.minimum_size = {};
        node.parent = parent;
        node.generation = id.generation;
        node.alive = true;
        node.children.clear();

        // Link to parent
        if (!parent.is_null()) {
            sys->nodes[parent.index].children.push_back(id);
        }

        return id;
    }

    NodeId add_center(System *sys, const NodeId parent) {
        if (!parent.is_null() && !is_valid(sys, parent)) {
            return NullNode;
        }

        NodeId id = allocate_node(sys);
        Node &node = sys->nodes[id.index];

        node.type = NodeType::Center;
        node.bounds = {};
        node.minimum_size = {};
        node.parent = parent;
        node.generation = id.generation;
        node.alive = true;
        node.children.clear();

        if (!parent.is_null()) {
            sys->nodes[parent.index].children.push_back(id);
        }

        return id;
    }

    NodeId add_box(System *sys, const NodeId parent, const BoxData &data) {
        if (!parent.is_null() && !is_valid(sys, parent)) {
            return NullNode;
        }

        // Reuse or allocate component slot
        size_t comp_idx;
        if (!sys->components.free_boxes.empty()) {
            comp_idx = sys->components.free_boxes.back();
            sys->components.free_boxes.pop_back();
            sys->components.boxes[comp_idx] = data; // Overwrite old data
        } else {
            comp_idx = sys->components.boxes.size();
            sys->components.boxes.push_back(data);
        }

        NodeId id = allocate_node(sys);
        Node &node = sys->nodes[id.index];

        node.type = NodeType::Box;
        node.bounds = {};
        node.minimum_size = {};
        node.parent = parent;
        node.component_index = comp_idx;
        node.generation = id.generation;
        node.alive = true;
        node.children.clear();

        if (!parent.is_null()) {
            sys->nodes[parent.index].children.push_back(id);
        }

        return id;
    }

    NodeId add_flow(System *sys, const NodeId parent, const FlowData &data) {
        if (!parent.is_null() && !is_valid(sys, parent)) {
            return NullNode;
        }

        size_t comp_idx;
        if (!sys->components.free_flows.empty()) {
            comp_idx = sys->components.free_flows.back();
            sys->components.free_flows.pop_back();
            sys->components.flows[comp_idx] = data;
        } else {
            comp_idx = sys->components.flows.size();
            sys->components.flows.push_back(data);
        }

        NodeId id = allocate_node(sys);
        Node &node = sys->nodes[id.index];

        node.type = NodeType::Flow;
        node.bounds = {};
        node.minimum_size = {};
        node.parent = parent;
        node.component_index = comp_idx;
        node.generation = id.generation;
        node.alive = true;
        node.children.clear();

        if (!parent.is_null()) {
            sys->nodes[parent.index].children.push_back(id);
        }

        return id;
    }

    NodeId add_margin(System *sys, const NodeId parent, const MarginData &data) {
        if (!parent.is_null() && !is_valid(sys, parent)) {
            return NullNode;
        }

        size_t comp_idx;
        if (!sys->components.free_margins.empty()) {
            comp_idx = sys->components.free_margins.back();
            sys->components.free_margins.pop_back();
            sys->components.margins[comp_idx] = data;
        } else {
            comp_idx = sys->components.margins.size();
            sys->components.margins.push_back(data);
        }

        NodeId id = allocate_node(sys);
        Node &node = sys->nodes[id.index];

        node.type = NodeType::Margin;
        node.bounds = {};
        node.minimum_size = {};
        node.parent = parent;
        node.component_index = comp_idx;
        node.generation = id.generation;
        node.alive = true;
        node.children.clear();

        if (!parent.is_null()) {
            sys->nodes[parent.index].children.push_back(id);
        }

        return id;
    }

    bool is_valid(const System *sys, NodeId id) {
        if (id.is_null()) return false;
        if (id.index >= sys->nodes.size()) return false;

        const Node &node = sys->nodes[id.index];
        return node.alive && node.generation == id.generation;
    }

    // Helper to check if new_parent is a descendant of node_id (would create cycle)
    static bool is_descendant(System *sys, NodeId ancestor, NodeId potential_descendant) {
        if (ancestor == potential_descendant) return true;

        Node *node = get_node(sys, ancestor);
        if (!node) return false;

        for (NodeId child_id: node->children) {
            if (is_descendant(sys, child_id, potential_descendant)) {
                return true;
            }
        }
        return false;
    }

    bool delete_node(System *sys, NodeId id) {
        if (!is_valid(sys, id)) return false;

        Node &node = sys->nodes[id.index];

        // 1. Recursively delete all children first
        std::vector<NodeId> children_copy = node.children;
        for (NodeId child_id : children_copy) {
            delete_node(sys, child_id);
        }

        // 2. Free component data if this node has any
        switch (node.type) {
            case NodeType::Box:
                sys->components.free_boxes.push_back(node.component_index);
                break;
            case NodeType::Flow:
                sys->components.free_flows.push_back(node.component_index);
                break;
            case NodeType::Margin:
                sys->components.free_margins.push_back(node.component_index);
                break;
            default:
                break;
        }

        // 3. Remove from parent's children list
        if (!node.parent.is_null()) {
            Node *parent = get_node(sys, node.parent);
            if (parent) {
                auto it = std::find(parent->children.begin(), parent->children.end(), id);
                if (it != parent->children.end()) {
                    parent->children.erase(it);
                }
            }
        }

        // 4. Mark as dead and increment generation
        node.alive = false;
        node.generation++;
        node.children.clear();
        node.parent = NullNode;

        // 5. Add to free list for reuse
        sys->free_list.push_back(id.index);

        return true;
    }

    bool reparent_node(System *sys, NodeId node_id, NodeId new_parent) {
        // Validate both nodes exist
        if (!is_valid(sys, node_id)) return false;
        if (!new_parent.is_null() && !is_valid(sys, new_parent)) return false;

        Node &node = sys->nodes[node_id.index];

        // Can't reparent to self
        if (node_id == new_parent) return false;

        // Check for cycles: new_parent can't be a descendant of node_id
        if (!new_parent.is_null() && is_descendant(sys, node_id, new_parent)) {
            return false;
        }

        // 1. Remove from old parent's children list
        if (!node.parent.is_null()) {
            Node *old_parent = get_node(sys, node.parent);
            if (old_parent) {
                auto it = std::find(old_parent->children.begin(), old_parent->children.end(), node_id);
                if (it != old_parent->children.end()) {
                    old_parent->children.erase(it);
                }
            }
        }

        // 2. Add to new parent's children list
        if (!new_parent.is_null()) {
            sys->nodes[new_parent.index].children.push_back(node_id);
        }

        // 3. Update parent reference
        node.parent = new_parent;

        return true;
    }

    // This could be a bad reference after the end of the frame.
    // Make sure you're storing handles, and not Node references.
    // Might be more aptly named "GetTemporaryNode"
    Node *get_node(System *sys, NodeId id) {
        if (!is_valid(sys, id)) return nullptr;
        return &sys->nodes[id.index];
    }

    const Node *get_node(const System *sys, NodeId id) {
        if (!is_valid(sys, id)) return nullptr;
        return &sys->nodes[id.index];
    }

    void compute_layout(System *sys, const NodeId node_id) {
        Node *node = get_node(sys, node_id);
        if (!node) return;

        switch (node->type) {
            case NodeType::Generic: layout_generic(sys, *node);
                break;
            case NodeType::Center:
                layout_center(sys, *node);
                break;
            case NodeType::Box:
                layout_box(sys, *node, sys->components.boxes[node->component_index]);
                break;
            case NodeType::Flow:
                layout_flow(sys, *node, sys->components.flows[node->component_index]);
                break;
            case NodeType::Margin:
                layout_margin(sys, *node, sys->components.margins[node->component_index]);
                break;
            default: break;
        }

        for (const auto child_id: node->children)
            compute_layout(sys, child_id);
    }
} // namespace frameflow
