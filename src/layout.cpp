#include "frameflow/layout.hpp"

namespace frameflow {

static void layout_generic(Node& node) { /* default layout */ }

static void layout_center(System& sys, Node node, CenterData data) {
    for (auto node_id : node.children) {
        auto &child_node = get_node(sys, node_id);
        child_node.bounds.origin = Point{node.bounds.origin.x + 100, node.bounds.origin.y+100};
    }
}

static void layout_box(System& sys, Node node, BoxData data) {

}

static void layout_flow(System& sys, Node node, FlowData data) {

}

NodeId add_generic(System& sys, NodeId parent) {
    // 1. Add component data
    // NA

    // 2. Add node
    NodeId id = static_cast<NodeId>(sys.nodes.size());
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

NodeId add_center(System& sys, NodeId parent, CenterData data) {
    // 1. Add component data
    size_t comp_idx = sys.components.centers.size();
    sys.components.centers.push_back(data);

    // 2. Add node
    NodeId id = static_cast<NodeId>(sys.nodes.size());
    Node node;
    node.type = NodeType::Center;
    node.bounds = {}; // default
    node.minimum_size = {};
    node.parent = parent;
    node.component_index = comp_idx;
    sys.nodes.push_back(node);

    // 3. Link to parent
    if (parent != NullNode) {
        sys.nodes[parent].children.push_back(id);
    }

    return id;
}

void compute_layout(System& sys, NodeId node_id) {
    Node& node = sys.nodes[node_id];
    switch (node.type) {
        case NodeType::Generic: layout_generic(node); break;
        case NodeType::Center:  
            layout_center(sys, node, sys.components.centers[node.component_index]); break;
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
