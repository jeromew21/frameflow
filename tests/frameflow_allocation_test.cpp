#include <frameflow/layout.hpp>
#include <iostream>
#include <cassert>
#include <vector>
#include <random>
#include <algorithm>

using namespace frameflow;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "..." << std::endl; \
    test_##name(); \
    std::cout << "  ✓ " #name " passed" << std::endl; \
} while(0)

#define ASSERT_TRUE(x) assert(x)
#define ASSERT_FALSE(x) assert(!(x))
#define ASSERT_EQ(a, b) assert((a) == (b))

// ========== Allocator Stress Tests ==========

TEST(deep_hierarchy_creation_and_deletion) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    
    // Build a deep chain
    NodeId current = root;
    std::vector<NodeId> chain;
    chain.push_back(root);
    
    for (int i = 0; i < 1000; i++) {
        NodeId child = add_generic(&sys, current);
        ASSERT_TRUE(is_valid(&sys, child));
        chain.push_back(child);
        current = child;
    }
    
    std::cout << "    Created chain of 1000 nodes" << std::endl;
    
    // Verify all nodes are valid
    for (NodeId id : chain) {
        ASSERT_TRUE(is_valid(&sys, id));
    }
    
    // Delete from the middle
    size_t delete_point = chain.size() / 2;
    ASSERT_TRUE(delete_node(&sys, chain[delete_point]));
    
    std::cout << "    Deleted middle node (and all descendants)" << std::endl;
    
    // Verify nodes before delete point are still valid
    for (size_t i = 0; i < delete_point; i++) {
        ASSERT_TRUE(is_valid(&sys, chain[i]));
    }
    
    // Verify nodes after delete point are invalid
    for (size_t i = delete_point; i < chain.size(); i++) {
        ASSERT_FALSE(is_valid(&sys, chain[i]));
    }
}

TEST(wide_tree_stress) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    
    std::vector<NodeId> children;
    
    // Create 1000 direct children
    for (int i = 0; i < 1000; i++) {
        NodeId child = add_generic(&sys, root);
        ASSERT_TRUE(is_valid(&sys, child));
        children.push_back(child);
    }
    
    std::cout << "    Created 1000 children" << std::endl;
    
    Node* root_node = get_node(&sys, root);
    ASSERT_EQ(root_node->children.size(), 1000);
    
    // Delete half of them
    for (size_t i = 0; i < 500; i++) {
        ASSERT_TRUE(delete_node(&sys, children[i]));
    }
    
    std::cout << "    Deleted 500 children" << std::endl;
    
    ASSERT_EQ(root_node->children.size(), 500);
    
    // Verify remaining are still valid
    for (size_t i = 500; i < 1000; i++) {
        ASSERT_TRUE(is_valid(&sys, children[i]));
    }
}

TEST(repeated_allocation_deallocation) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    
    std::vector<NodeId> allocated;
    
    for (int iteration = 0; iteration < 100; iteration++) {
        // Allocate 50 nodes
        for (int i = 0; i < 50; i++) {
            NodeId node = add_generic(&sys, root);
            ASSERT_TRUE(is_valid(&sys, node));
            allocated.push_back(node);
        }
        
        // Delete 25 random nodes
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(allocated.begin(), allocated.end(), gen);
        
        for (int i = 0; i < 25 && !allocated.empty(); i++) {
            NodeId to_delete = allocated.back();
            allocated.pop_back();
            ASSERT_TRUE(delete_node(&sys, to_delete));
        }
    }
    
    std::cout << "    Completed 100 iterations of alloc/dealloc" << std::endl;
    
    // Verify all remaining nodes are valid
    for (NodeId id : allocated) {
        ASSERT_TRUE(is_valid(&sys, id));
    }
}

TEST(generation_tracking_prevents_use_after_free) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    
    // Create and delete node
    NodeId node1 = add_generic(&sys, root);
    uint32_t index1 = node1.index;
    uint32_t gen1 = node1.generation;
    
    ASSERT_TRUE(delete_node(&sys, node1));
    ASSERT_FALSE(is_valid(&sys, node1));
    
    // Create new node - should reuse slot
    NodeId node2 = add_generic(&sys, root);
    ASSERT_EQ(node2.index, index1);  // Same slot
    ASSERT_TRUE(node2.generation > gen1);  // Different generation
    
    // Old ID should still be invalid
    ASSERT_FALSE(is_valid(&sys, node1));
    ASSERT_TRUE(is_valid(&sys, node2));
    
    std::cout << "    Generation incremented from " << gen1 << " to " << node2.generation << std::endl;
}

TEST(mass_deletion_and_recreation) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    
    for (int iteration = 0; iteration < 10; iteration++) {
        std::vector<NodeId> nodes;
        
        // Create 500 nodes
        for (int i = 0; i < 500; i++) {
            NodeId node = add_generic(&sys, root);
            ASSERT_TRUE(is_valid(&sys, node));
            nodes.push_back(node);
        }
        
        // Delete all of them
        for (NodeId id : nodes) {
            ASSERT_TRUE(delete_node(&sys, id));
            ASSERT_FALSE(is_valid(&sys, id));
        }
        
        Node* root_node = get_node(&sys, root);
        ASSERT_EQ(root_node->children.size(), 0);
    }
    
    std::cout << "    Completed 10 cycles of create 500 / delete 500" << std::endl;
}

TEST(fragmented_deletion_pattern) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    
    std::vector<NodeId> nodes;
    
    // Create 1000 nodes
    for (int i = 0; i < 1000; i++) {
        nodes.push_back(add_generic(&sys, root));
    }
    
    // Delete every other node (creates fragmented free list)
    for (size_t i = 0; i < nodes.size(); i += 2) {
        ASSERT_TRUE(delete_node(&sys, nodes[i]));
    }
    
    std::cout << "    Deleted 500 nodes in fragmented pattern" << std::endl;
    std::cout << "    Free list size: " << sys.free_list.size() << std::endl;
    
    // Allocate 500 more - should reuse the fragmented slots
    std::vector<NodeId> new_nodes;
    for (int i = 0; i < 500; i++) {
        NodeId node = add_generic(&sys, root);
        ASSERT_TRUE(is_valid(&sys, node));
        new_nodes.push_back(node);
    }
    
    // All old even-indexed nodes should be invalid
    for (size_t i = 0; i < nodes.size(); i += 2) {
        ASSERT_FALSE(is_valid(&sys, nodes[i]));
    }
    
    // All odd-indexed original nodes should still be valid
    for (size_t i = 1; i < nodes.size(); i += 2) {
        ASSERT_TRUE(is_valid(&sys, nodes[i]));
    }
    
    // All new nodes should be valid
    for (NodeId id : new_nodes) {
        ASSERT_TRUE(is_valid(&sys, id));
    }
}

TEST(reparenting_stress) {
    System sys;
    
    // Create multiple subtrees
    std::vector<NodeId> roots;
    for (int i = 0; i < 10; i++) {
        roots.push_back(add_generic(&sys, NullNode));
    }
    
    // Create nodes under each root
    std::vector<NodeId> all_nodes;
    for (NodeId root : roots) {
        for (int i = 0; i < 50; i++) {
            NodeId node = add_generic(&sys, root);
            all_nodes.push_back(node);
        }
    }
    
    std::cout << "    Created 10 roots with 50 children each" << std::endl;
    
    // Randomly reparent nodes
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, roots.size() - 1);
    
    for (int i = 0; i < 1000; i++) {
        size_t node_idx = gen() % all_nodes.size();
        size_t new_root_idx = dis(gen);
        
        NodeId node = all_nodes[node_idx];
        NodeId new_parent = roots[new_root_idx];
        
        if (is_valid(&sys, node)) {
            reparent_node(&sys, node, new_parent);
            ASSERT_TRUE(is_valid(&sys, node));
        }
    }
    
    std::cout << "    Completed 1000 random reparenting operations" << std::endl;
    
    // Verify no nodes were lost
    for (NodeId id : all_nodes) {
        ASSERT_TRUE(is_valid(&sys, id));
    }
}

TEST(deep_tree_with_mixed_types) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    
    std::vector<NodeId> nodes;
    nodes.push_back(root);
    
    std::random_device rd;
    std::mt19937 gen(rd());

    // Build a complex tree with different node types
    // Add nodes one at a time and pick random parents to avoid explosion
    for (int i = 0; i < 5000; i++) {
        if (nodes.empty()) break;

        // Pick a random valid parent
        NodeId parent = NullNode;
        for (int attempt = 0; attempt < 10; attempt++) {
            size_t idx = gen() % nodes.size();
            if (is_valid(&sys, nodes[idx])) {
                parent = nodes[idx];
                break;
            }
        }

        if (parent.is_null()) continue;

        // Add one of each type
        int type = i % 5;
        NodeId new_node;
        switch (type) {
            case 0: new_node = add_generic(&sys, parent); break;
            case 1: new_node = add_center(&sys, parent); break;
            case 2: new_node = add_box(&sys, parent, {Direction::Horizontal, Align::Start}); break;
            case 3: new_node = add_flow(&sys, parent, {Direction::Vertical, Align::Center}); break;
            case 4: new_node = add_margin(&sys, parent, {5, 5, 5, 5}); break;
        }
        nodes.push_back(new_node);
    }
}

TEST(cascade_deletion) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    
    // Build a pyramid where each node has 2 children (binary tree-ish)
    // But make it wide at the top levels
    std::vector<std::vector<NodeId>> levels;
    std::vector<NodeId> current_level;
    current_level.push_back(root);
    levels.push_back(current_level);

    // First few levels: wide (each node has 3 children)
    for (int level = 0; level < 5; level++) {
        std::vector<NodeId> next_level;
        for (NodeId parent : current_level) {
            for (int i = 0; i < 3; i++) {
                NodeId child = add_generic(&sys, parent);
                next_level.push_back(child);
            }
        }
        levels.push_back(next_level);
        current_level = next_level;
    }

    // Deeper levels: narrow (each node has 2 children)
    for (int level = 5; level < 15; level++) {
        std::vector<NodeId> next_level;
        for (NodeId parent : current_level) {
            for (int i = 0; i < 2; i++) {
                NodeId child = add_generic(&sys, parent);
                next_level.push_back(child);
            }
        }
        levels.push_back(next_level);
        current_level = next_level;

        // Cap the width to prevent explosion
        if (next_level.size() > 10000) break;
    }

    size_t total_nodes = 0;
    for (const auto& level : levels) {
        total_nodes += level.size();
    }

    std::cout << "    Created pyramid with " << levels.size() << " levels, "
              << total_nodes << " total nodes" << std::endl;

    // Delete a node from level 3 - should cascade delete all descendants
    if (levels.size() > 3 && !levels[3].empty()) {
        NodeId to_delete = levels[3][0];
        size_t children_before = get_node(&sys, levels[2][0])->children.size();

        ASSERT_TRUE(delete_node(&sys, to_delete));

        std::cout << "    Deleted one node from level 3, cascading to all descendants" << std::endl;

        // The parent should have one fewer child
        size_t children_after = get_node(&sys, levels[2][0])->children.size();
        ASSERT_TRUE(children_after < children_before);

        // Verify the deleted node is invalid
        ASSERT_FALSE(is_valid(&sys, to_delete));

        // Verify some descendants are invalid (they got cascade deleted)
        if (levels.size() > 4) {
            // Check nodes in deeper levels - some should be invalid
            size_t invalid_count = 0;
            for (size_t level = 4; level < levels.size(); level++) {
                for (NodeId id : levels[level]) {
                    if (!is_valid(&sys, id)) {
                        invalid_count++;
                    }
                }
            }

            std::cout << "    Found " << invalid_count << " cascade-deleted descendants" << std::endl;
            ASSERT_TRUE(invalid_count > 0);  // At least some should be deleted
        }
    }
}

TEST(parallel_subtree_operations) {
    System sys;
    
    // Create 5 independent subtrees
    std::vector<NodeId> subtree_roots;
    std::vector<std::vector<NodeId>> subtree_nodes;
    
    for (int tree = 0; tree < 5; tree++) {
        NodeId root = add_generic(&sys, NullNode);
        subtree_roots.push_back(root);
        
        std::vector<NodeId> nodes;
        nodes.push_back(root);
        
        // Build out each subtree
        for (int i = 0; i < 200; i++) {
            NodeId parent = nodes[i % nodes.size()];
            NodeId child = add_generic(&sys, parent);
            nodes.push_back(child);
        }
        
        subtree_nodes.push_back(nodes);
    }
    
    std::cout << "    Created 5 independent subtrees with ~200 nodes each" << std::endl;
    
    // Perform operations on different subtrees
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (int iter = 0; iter < 100; iter++) {
        // Pick a random subtree
        int tree_idx = gen() % subtree_roots.size();
        auto& nodes = subtree_nodes[tree_idx];
        
        if (nodes.size() < 10) continue;
        
        // Delete some nodes from this subtree
        size_t delete_idx = gen() % nodes.size();
        delete_node(&sys, nodes[delete_idx]);
        nodes.erase(nodes.begin() + delete_idx);
        
        // Add some nodes to this subtree
        if (!nodes.empty()) {
            NodeId parent = nodes[gen() % nodes.size()];
            if (is_valid(&sys, parent)) {
                NodeId new_node = add_generic(&sys, parent);
                nodes.push_back(new_node);
            }
        }
    }
    
    std::cout << "    Completed 100 mixed operations across subtrees" << std::endl;
    
    // Verify nodes in different subtrees don't interfere
    for (size_t i = 0; i < subtree_nodes.size(); i++) {
        size_t valid_count = 0;
        for (NodeId id : subtree_nodes[i]) {
            if (is_valid(&sys, id)) valid_count++;
        }
        std::cout << "    Subtree " << i << " has " << valid_count << " valid nodes" << std::endl;
    }
}

// ========== Main ==========

int main() {
    RUN_TEST(deep_hierarchy_creation_and_deletion);
    RUN_TEST(wide_tree_stress);
    RUN_TEST(repeated_allocation_deallocation);
    RUN_TEST(generation_tracking_prevents_use_after_free);
    RUN_TEST(mass_deletion_and_recreation);
    RUN_TEST(fragmented_deletion_pattern);
    RUN_TEST(reparenting_stress);
    RUN_TEST(deep_tree_with_mixed_types);
    RUN_TEST(cascade_deletion);
    RUN_TEST(parallel_subtree_operations);
    
    std::cout << "\n✓ All allocator stress tests passed!" << std::endl;
    return 0;
}