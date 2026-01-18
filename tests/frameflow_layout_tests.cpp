#include <frameflow/layout.hpp>
#include <iostream>
#include <cassert>
#include <cmath>

using namespace frameflow;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "..." << std::endl; \
    test_##name(); \
    std::cout << "  ✓ " #name " passed" << std::endl; \
} while(0)

#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_NEAR(a, b, eps) assert(std::abs((a) - (b)) < (eps))
#define ASSERT_TRUE(x) assert(x)
#define ASSERT_FALSE(x) assert(!(x))

// Helper to print node info
void print_node(const System* sys, NodeId id, const char* name = "node") {
    const Node* n = get_node(sys, id);
    if (!n) {
        std::cout << name << ": INVALID" << std::endl;
        return;
    }
    std::cout << name << ": origin(" << n->bounds.origin.x << ", " << n->bounds.origin.y 
              << ") size(" << n->bounds.size.x << ", " << n->bounds.size.y << ")" << std::endl;
}

// ========== Basic Node Tests ==========

TEST(node_creation) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    
    ASSERT_TRUE(is_valid(&sys, root));
    ASSERT_FALSE(root.is_null());
    
    Node* node = get_node(&sys, root);
    ASSERT_TRUE(node != nullptr);
    ASSERT_EQ(node->type, NodeType::Generic);
    ASSERT_TRUE(node->parent.is_null());
}

TEST(node_hierarchy) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    NodeId child1 = add_generic(&sys, root);
    NodeId child2 = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    ASSERT_EQ(root_node->children.size(), 2);
    ASSERT_EQ(root_node->children[0], child1);
    ASSERT_EQ(root_node->children[1], child2);
    
    Node* child1_node = get_node(&sys, child1);
    ASSERT_EQ(child1_node->parent, root);
}

TEST(invalid_parent_returns_null) {
    System sys;
    NodeId fake = {999, 0};
    NodeId child = add_generic(&sys, fake);
    
    ASSERT_TRUE(child.is_null());
}

// ========== Deletion Tests ==========

TEST(delete_node_basic) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    NodeId child = add_generic(&sys, root);
    
    ASSERT_TRUE(delete_node(&sys, child));
    ASSERT_FALSE(is_valid(&sys, child));
    
    Node* root_node = get_node(&sys, root);
    ASSERT_EQ(root_node->children.size(), 0);
}

TEST(delete_node_with_children) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    NodeId child = add_generic(&sys, root);
    NodeId grandchild = add_generic(&sys, child);
    
    ASSERT_TRUE(delete_node(&sys, child));
    ASSERT_FALSE(is_valid(&sys, child));
    ASSERT_FALSE(is_valid(&sys, grandchild));
}

TEST(node_reuse_after_deletion) {
    System sys;
    NodeId first = add_generic(&sys, NullNode);
    uint32_t first_index = first.index;
    
    delete_node(&sys, first);
    
    NodeId second = add_generic(&sys, NullNode);
    ASSERT_EQ(second.index, first_index);  // Should reuse same slot
    ASSERT_TRUE(second.generation > first.generation);  // But different generation
    ASSERT_FALSE(is_valid(&sys, first));  // Old ID should be invalid
    ASSERT_TRUE(is_valid(&sys, second));
}

// ========== Reparenting Tests ==========

TEST(reparent_basic) {
    System sys;
    NodeId root1 = add_generic(&sys, NullNode);
    NodeId root2 = add_generic(&sys, NullNode);
    NodeId child = add_generic(&sys, root1);
    
    ASSERT_TRUE(reparent_node(&sys, child, root2));
    
    Node* r1 = get_node(&sys, root1);
    Node* r2 = get_node(&sys, root2);
    Node* c = get_node(&sys, child);
    
    ASSERT_EQ(r1->children.size(), 0);
    ASSERT_EQ(r2->children.size(), 1);
    ASSERT_EQ(c->parent, root2);
}

TEST(reparent_prevents_cycles) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    NodeId child = add_generic(&sys, root);
    NodeId grandchild = add_generic(&sys, child);
    
    // Try to make root a child of its grandchild (would create cycle)
    ASSERT_FALSE(reparent_node(&sys, root, grandchild));
}

TEST(reparent_to_self_fails) {
    System sys;
    NodeId node = add_generic(&sys, NullNode);
    
    ASSERT_FALSE(reparent_node(&sys, node, node));
}

// ========== Generic Layout Tests ==========

TEST(generic_respects_minimum_size) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    NodeId child = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 100}};
    
    Node* child_node = get_node(&sys, child);
    child_node->minimum_size = {50, 50};
    
    compute_layout(&sys, root);
    
    ASSERT_NEAR(child_node->bounds.size.x, 50, 0.01);
    ASSERT_NEAR(child_node->bounds.size.y, 50, 0.01);
}

TEST(generic_expand_fills_parent) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    NodeId child = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 100}};
    
    Node* child_node = get_node(&sys, child);
    child_node->expand = {1, 1};
    
    compute_layout(&sys, root);
    
    ASSERT_NEAR(child_node->bounds.size.x, 100, 0.01);
    ASSERT_NEAR(child_node->bounds.size.y, 100, 0.01);
}

// ========== Anchor Tests ==========

TEST(anchors_full_fill) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    NodeId child = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{10, 10}, {100, 100}};
    
    Node* child_node = get_node(&sys, child);
    child_node->anchors = {0, 0, 1, 1};  // Fill entire parent
    child_node->offsets = {0, 0, 0, 0};
    
    compute_layout(&sys, root);
    
    ASSERT_NEAR(child_node->bounds.origin.x, 10, 0.01);
    ASSERT_NEAR(child_node->bounds.origin.y, 10, 0.01);
    ASSERT_NEAR(child_node->bounds.size.x, 100, 0.01);
    ASSERT_NEAR(child_node->bounds.size.y, 100, 0.01);
}

TEST(anchors_centered_quarter) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    NodeId child = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 100}};
    
    Node* child_node = get_node(&sys, child);
    child_node->anchors = {0.25f, 0.25f, 0.75f, 0.75f};
    child_node->offsets = {0, 0, 0, 0};
    
    compute_layout(&sys, root);
    
    ASSERT_NEAR(child_node->bounds.origin.x, 25, 0.01);
    ASSERT_NEAR(child_node->bounds.origin.y, 25, 0.01);
    ASSERT_NEAR(child_node->bounds.size.x, 50, 0.01);
    ASSERT_NEAR(child_node->bounds.size.y, 50, 0.01);
}

TEST(anchors_with_offsets) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    NodeId child = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 100}};
    
    Node* child_node = get_node(&sys, child);
    child_node->anchors = {0, 0, 1, 1};
    child_node->offsets = {10, 10, 10, 10};  // 10px inset from all sides
    
    compute_layout(&sys, root);
    
    ASSERT_NEAR(child_node->bounds.origin.x, 10, 0.01);
    ASSERT_NEAR(child_node->bounds.origin.y, 10, 0.01);
    ASSERT_NEAR(child_node->bounds.size.x, 80, 0.01);  // 100 - 10 - 10
    ASSERT_NEAR(child_node->bounds.size.y, 80, 0.01);
}

// ========== Center Layout Tests ==========

TEST(center_centers_child) {
    System sys;
    NodeId root = add_center(&sys, NullNode);
    NodeId child = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 100}};
    
    Node* child_node = get_node(&sys, child);
    child_node->minimum_size = {20, 20};
    
    compute_layout(&sys, root);
    
    print_node(&sys, root, "root");
    print_node(&sys, child, "child");
    
    ASSERT_NEAR(child_node->bounds.origin.x, 40, 0.01);  // (100 - 20) / 2
    ASSERT_NEAR(child_node->bounds.origin.y, 40, 0.01);
    ASSERT_NEAR(child_node->bounds.size.x, 20, 0.01);
    ASSERT_NEAR(child_node->bounds.size.y, 20, 0.01);
}

// ========== Box Layout Tests ==========

TEST(box_horizontal_basic) {
    System sys;
    NodeId root = add_box(&sys, NullNode, {Direction::Horizontal, Align::Start});
    NodeId child1 = add_generic(&sys, root);
    NodeId child2 = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 50}};
    
    Node* c1 = get_node(&sys, child1);
    c1->minimum_size = {30, 50};
    
    Node* c2 = get_node(&sys, child2);
    c2->minimum_size = {40, 50};
    
    compute_layout(&sys, root);
    
    print_node(&sys, child1, "child1");
    print_node(&sys, child2, "child2");
    
    ASSERT_NEAR(c1->bounds.origin.x, 0, 0.01);
    ASSERT_NEAR(c2->bounds.origin.x, 30, 0.01);
    ASSERT_NEAR(c1->bounds.size.x, 30, 0.01);
    ASSERT_NEAR(c2->bounds.size.x, 40, 0.01);
}

TEST(box_horizontal_expand_stretch) {
    System sys;
    NodeId root = add_box(&sys, NullNode, {Direction::Horizontal, Align::Start});
    NodeId child1 = add_generic(&sys, root);
    NodeId child2 = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 50}};
    
    Node* c1 = get_node(&sys, child1);
    c1->minimum_size = {20, 50};
    c1->expand = {1, 0};
    c1->stretch = {1, 1};
    
    Node* c2 = get_node(&sys, child2);
    c2->minimum_size = {20, 50};
    c2->expand = {1, 0};
    c2->stretch = {2, 1};  // 2x stretch weight
    
    compute_layout(&sys, root);
    
    print_node(&sys, child1, "child1");
    print_node(&sys, child2, "child2");
    
    // Total leftover = 100 - 40 = 60
    // c1 gets 60 * (1/3) = 20, total = 40
    // c2 gets 60 * (2/3) = 40, total = 60
    ASSERT_NEAR(c1->bounds.size.x, 40, 0.01);
    ASSERT_NEAR(c2->bounds.size.x, 60, 0.01);
}

TEST(box_horizontal_align_center) {
    System sys;
    NodeId root = add_box(&sys, NullNode, {Direction::Horizontal, Align::Center});
    NodeId child = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 50}};
    
    Node* c = get_node(&sys, child);
    c->minimum_size = {40, 50};
    
    compute_layout(&sys, root);
    
    print_node(&sys, child, "child");
    
    // Leftover = 100 - 40 = 60, centered = 30
    ASSERT_NEAR(c->bounds.origin.x, 30, 0.01);
}

TEST(box_horizontal_align_end) {
    System sys;
    NodeId root = add_box(&sys, NullNode, {Direction::Horizontal, Align::End});
    NodeId child = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 50}};
    
    Node* c = get_node(&sys, child);
    c->minimum_size = {40, 50};
    
    compute_layout(&sys, root);
    
    print_node(&sys, child, "child");
    
    ASSERT_NEAR(c->bounds.origin.x, 60, 0.01);
}

TEST(box_horizontal_space_between) {
    System sys;
    NodeId root = add_box(&sys, NullNode, {Direction::Horizontal, Align::SpaceBetween});
    NodeId child1 = add_generic(&sys, root);
    NodeId child2 = add_generic(&sys, root);
    NodeId child3 = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 50}};
    
    Node* c1 = get_node(&sys, child1);
    c1->minimum_size = {20, 50};
    
    Node* c2 = get_node(&sys, child2);
    c2->minimum_size = {20, 50};
    
    Node* c3 = get_node(&sys, child3);
    c3->minimum_size = {20, 50};
    
    compute_layout(&sys, root);
    
    print_node(&sys, child1, "child1");
    print_node(&sys, child2, "child2");
    print_node(&sys, child3, "child3");
    
    // Total = 60, leftover = 40, spacing = 40/2 = 20
    ASSERT_NEAR(c1->bounds.origin.x, 0, 0.01);
    ASSERT_NEAR(c2->bounds.origin.x, 40, 0.01);
    ASSERT_NEAR(c3->bounds.origin.x, 80, 0.01);
}

TEST(box_vertical_basic) {
    System sys;
    NodeId root = add_box(&sys, NullNode, {Direction::Vertical, Align::Start});
    NodeId child1 = add_generic(&sys, root);
    NodeId child2 = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {50, 100}};
    
    Node* c1 = get_node(&sys, child1);
    c1->minimum_size = {50, 30};
    
    Node* c2 = get_node(&sys, child2);
    c2->minimum_size = {50, 40};
    
    compute_layout(&sys, root);
    
    ASSERT_NEAR(c1->bounds.origin.y, 0, 0.01);
    ASSERT_NEAR(c2->bounds.origin.y, 30, 0.01);
}

// ========== Flow Layout Tests ==========

TEST(flow_horizontal_no_wrap) {
    System sys;
    NodeId root = add_flow(&sys, NullNode, {Direction::Horizontal, Align::Start});
    NodeId child1 = add_generic(&sys, root);
    NodeId child2 = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 100}};
    
    Node* c1 = get_node(&sys, child1);
    c1->minimum_size = {30, 20};
    
    Node* c2 = get_node(&sys, child2);
    c2->minimum_size = {30, 20};
    
    compute_layout(&sys, root);
    
    print_node(&sys, child1, "child1");
    print_node(&sys, child2, "child2");
    
    ASSERT_NEAR(c1->bounds.origin.x, 0, 0.01);
    ASSERT_NEAR(c1->bounds.origin.y, 0, 0.01);
    ASSERT_NEAR(c2->bounds.origin.x, 30, 0.01);
    ASSERT_NEAR(c2->bounds.origin.y, 0, 0.01);
}

TEST(flow_horizontal_with_wrap) {
    System sys;
    NodeId root = add_flow(&sys, NullNode, {Direction::Horizontal, Align::Start});
    NodeId child1 = add_generic(&sys, root);
    NodeId child2 = add_generic(&sys, root);
    NodeId child3 = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {70, 100}};  // Only fits 2 on first row
    
    Node* c1 = get_node(&sys, child1);
    c1->minimum_size = {30, 20};
    
    Node* c2 = get_node(&sys, child2);
    c2->minimum_size = {30, 20};
    
    Node* c3 = get_node(&sys, child3);
    c3->minimum_size = {30, 20};
    
    compute_layout(&sys, root);
    
    print_node(&sys, child1, "child1");
    print_node(&sys, child2, "child2");
    print_node(&sys, child3, "child3");
    
    // First row
    ASSERT_NEAR(c1->bounds.origin.x, 0, 0.01);
    ASSERT_NEAR(c1->bounds.origin.y, 0, 0.01);
    ASSERT_NEAR(c2->bounds.origin.x, 30, 0.01);
    ASSERT_NEAR(c2->bounds.origin.y, 0, 0.01);
    
    // Second row (wraps)
    ASSERT_NEAR(c3->bounds.origin.x, 0, 0.01);
    ASSERT_NEAR(c3->bounds.origin.y, 20, 0.01);
}

// ========== Margin Layout Tests ==========

TEST(margin_insets_children) {
    System sys;
    MarginData margin = {10, 10, 10, 10};
    NodeId root = add_margin(&sys, NullNode, margin);
    NodeId child = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 100}};
    
    Node* child_node = get_node(&sys, child);
    child_node->expand = {1, 1};
    
    compute_layout(&sys, root);
    
    print_node(&sys, child, "child");
    
    ASSERT_NEAR(child_node->bounds.origin.x, 10, 0.01);
    ASSERT_NEAR(child_node->bounds.origin.y, 10, 0.01);
    ASSERT_NEAR(child_node->bounds.size.x, 80, 0.01);  // 100 - 10 - 10
    ASSERT_NEAR(child_node->bounds.size.y, 80, 0.01);
}

TEST(margin_asymmetric) {
    System sys;
    MarginData margin = {5, 15, 10, 20};  // left, right, top, bottom
    NodeId root = add_margin(&sys, NullNode, margin);
    NodeId child = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 100}};
    
    Node* child_node = get_node(&sys, child);
    child_node->expand = {1, 1};
    
    compute_layout(&sys, root);
    
    ASSERT_NEAR(child_node->bounds.origin.x, 5, 0.01);
    ASSERT_NEAR(child_node->bounds.origin.y, 10, 0.01);
    ASSERT_NEAR(child_node->bounds.size.x, 80, 0.01);  // 100 - 5 - 15
    ASSERT_NEAR(child_node->bounds.size.y, 70, 0.01);  // 100 - 10 - 20
}

// ========== Complex Nested Tests ==========

TEST(nested_box_in_center) {
    System sys;
    NodeId center = add_center(&sys, NullNode);
    NodeId box = add_box(&sys, center, {Direction::Horizontal, Align::Start});
    NodeId child1 = add_generic(&sys, box);
    NodeId child2 = add_generic(&sys, box);
    
    Node* center_node = get_node(&sys, center);
    center_node->bounds = {{0, 0}, {200, 200}};
    
    Node* box_node = get_node(&sys, box);
    box_node->minimum_size = {100, 50};
    
    Node* c1 = get_node(&sys, child1);
    c1->minimum_size = {40, 50};
    
    Node* c2 = get_node(&sys, child2);
    c2->minimum_size = {60, 50};
    
    compute_layout(&sys, center);
    
    // Box should be centered
    ASSERT_NEAR(box_node->bounds.origin.x, 50, 0.01);  // (200-100)/2
    ASSERT_NEAR(box_node->bounds.origin.y, 75, 0.01);  // (200-50)/2
    
    // Children should be laid out horizontally within box
    ASSERT_NEAR(c1->bounds.origin.x, 50, 0.01);
    ASSERT_NEAR(c2->bounds.origin.x, 90, 0.01);
}

// ========== Edge Cases ==========

TEST(empty_children_doesnt_crash) {
    System sys;
    NodeId root = add_box(&sys, NullNode, {Direction::Horizontal, Align::Start});
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {100, 100}};
    
    compute_layout(&sys, root);  // Should not crash
}

TEST(zero_size_parent) {
    System sys;
    NodeId root = add_generic(&sys, NullNode);
    NodeId child = add_generic(&sys, root);
    
    Node* root_node = get_node(&sys, root);
    root_node->bounds = {{0, 0}, {0, 0}};
    
    Node* child_node = get_node(&sys, child);
    child_node->expand = {1, 1};
    
    compute_layout(&sys, root);
    
    // Should not crash, child should be 0 size
    ASSERT_NEAR(child_node->bounds.size.x, 0, 0.01);
    ASSERT_NEAR(child_node->bounds.size.y, 0, 0.01);
}

// ========== Main ==========

int main() {
    // Basic tests
    RUN_TEST(node_creation);
    RUN_TEST(node_hierarchy);
    RUN_TEST(invalid_parent_returns_null);
    
    // Deletion tests
    RUN_TEST(delete_node_basic);
    RUN_TEST(delete_node_with_children);
    RUN_TEST(node_reuse_after_deletion);
    
    // Reparenting tests
    RUN_TEST(reparent_basic);
    RUN_TEST(reparent_prevents_cycles);
    RUN_TEST(reparent_to_self_fails);
    
    // Generic layout
    RUN_TEST(generic_respects_minimum_size);
    RUN_TEST(generic_expand_fills_parent);
    
    // Anchors
    RUN_TEST(anchors_full_fill);
    RUN_TEST(anchors_centered_quarter);
    RUN_TEST(anchors_with_offsets);
    
    // Center layout
    RUN_TEST(center_centers_child);
    
    // Box layout
    RUN_TEST(box_horizontal_basic);
    RUN_TEST(box_horizontal_expand_stretch);
    RUN_TEST(box_horizontal_align_center);
    RUN_TEST(box_horizontal_align_end);
    RUN_TEST(box_horizontal_space_between);
    RUN_TEST(box_vertical_basic);
    
    // Flow layout
    RUN_TEST(flow_horizontal_no_wrap);
    RUN_TEST(flow_horizontal_with_wrap);
    
    // Margin layout
    RUN_TEST(margin_insets_children);
    RUN_TEST(margin_asymmetric);
    
    // Complex cases
    RUN_TEST(nested_box_in_center);
    
    // Edge cases
    RUN_TEST(empty_children_doesnt_crash);
    RUN_TEST(zero_size_parent);
    
    std::cout << "\n✓ All tests passed!" << std::endl;
    return 0;
}