#pragma once
#include <frameflow/layout.hpp>
#include <iostream>
#include <string>
#include <iomanip>

namespace frameflow {

inline const char* node_type_name(NodeType type) {
    switch (type) {
        case NodeType::Generic: return "Generic";
        case NodeType::Center: return "Center";
        case NodeType::Box: return "Box";
        case NodeType::Flow: return "Flow";
        case NodeType::Margin: return "Margin";
        default: return "Unknown";
    }
}

inline const char* direction_name(Direction dir) {
    return dir == Direction::Horizontal ? "Horizontal" : "Vertical";
}

inline const char* align_name(Align align) {
    switch (align) {
        case Align::Start: return "Start";
        case Align::Center: return "Center";
        case Align::End: return "End";
        case Align::SpaceBetween: return "SpaceBetween";
        default: return "Unknown";
    }
}

inline void print_node_recursive(const System* sys, NodeId id, int indent = 0, const std::string& prefix = "") {
    if (!is_valid(sys, id)) {
        std::cout << std::string(indent, ' ') << prefix << "INVALID NODE" << std::endl;
        return;
    }

    const Node* node = get_node(sys, id);
    std::string indent_str(indent, ' ');

    // Print node header
    std::cout << indent_str << prefix << "[" << node_type_name(node->type) << "] "
              << "ID(" << id.index << ":" << id.generation << ")" << std::endl;

    // Print bounds
    std::cout << indent_str << "  Bounds: origin(" << std::fixed << std::setprecision(1)
              << node->bounds.origin.x << ", " << node->bounds.origin.y << ") "
              << "size(" << node->bounds.size.x << ", " << node->bounds.size.y << ")" << std::endl;

    // Print minimum size
    std::cout << indent_str << "  MinSize: (" << node->minimum_size.x << ", "
              << node->minimum_size.y << ")" << std::endl;

    // Print expand/stretch if non-default
    if (node->expand.x > 0 || node->expand.y > 0) {
        std::cout << indent_str << "  Expand: (" << node->expand.x << ", " << node->expand.y << ")";
        if (node->expand.x > 0 || node->expand.y > 0) {
            std::cout << " Stretch: (" << node->stretch.x << ", " << node->stretch.y << ")";
        }
        std::cout << std::endl;
    }

    // Print anchors if non-default
    if (node->anchors.left != 0 || node->anchors.top != 0 ||
        node->anchors.right != 0 || node->anchors.bottom != 0) {
        std::cout << indent_str << "  Anchors: L=" << node->anchors.left
                  << " T=" << node->anchors.top
                  << " R=" << node->anchors.right
                  << " B=" << node->anchors.bottom << std::endl;
    }

    // Print offsets if non-default
    if (node->offsets.left != 0 || node->offsets.top != 0 ||
        node->offsets.right != 0 || node->offsets.bottom != 0) {
        std::cout << indent_str << "  Offsets: L=" << node->offsets.left
                  << " T=" << node->offsets.top
                  << " R=" << node->offsets.right
                  << " B=" << node->offsets.bottom << std::endl;
    }

    // Print component-specific data
    switch (node->type) {
        case NodeType::Box: {
            const BoxData& box = sys->components.boxes[node->component_index];
            std::cout << indent_str << "  Box: " << direction_name(box.direction)
                      << ", " << align_name(box.align) << std::endl;
            break;
        }
        case NodeType::Flow: {
            const FlowData& flow = sys->components.flows[node->component_index];
            std::cout << indent_str << "  Flow: " << direction_name(flow.direction)
                      << ", " << align_name(flow.align) << std::endl;
            break;
        }
        case NodeType::Margin: {
            const MarginData& margin = sys->components.margins[node->component_index];
            std::cout << indent_str << "  Margin: L=" << margin.left
                      << " R=" << margin.right
                      << " T=" << margin.top
                      << " B=" << margin.bottom << std::endl;
            break;
        }
        default:
            break;
    }

    // Print children count
    if (!node->children.empty()) {
        std::cout << indent_str << "  Children: " << node->children.size() << std::endl;

        // Recursively print children
        for (size_t i = 0; i < node->children.size(); i++) {
            std::string child_prefix = "└─ Child " + std::to_string(i) + ": ";
            print_node_recursive(sys, node->children[i], indent + 4, child_prefix);
        }
    } else {
        std::cout << indent_str << "  Children: none" << std::endl;
    }
}

// Main pretty print function
inline void pretty_print(const System* sys, NodeId root_id) {
    std::cout << "\n========== Layout Tree ==========" << std::endl;
    print_node_recursive(sys, root_id);
    std::cout << "================================\n" << std::endl;
}

// Compact single-line print for quick debugging
inline void print_node_compact(const System* sys, NodeId id) {
    if (!is_valid(sys, id)) {
        std::cout << "INVALID" << std::endl;
        return;
    }

    const Node* node = get_node(sys, id);
    std::cout << node_type_name(node->type)
              << " ID(" << id.index << ":" << id.generation << ") "
              << "pos(" << node->bounds.origin.x << "," << node->bounds.origin.y << ") "
              << "size(" << node->bounds.size.x << "x" << node->bounds.size.y << ") "
              << "min(" << node->minimum_size.x << "x" << node->minimum_size.y << ")"
              << std::endl;
}

} // namespace frameflow
