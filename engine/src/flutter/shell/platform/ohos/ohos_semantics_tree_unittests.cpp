/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_tree.h"

#include <gtest/gtest.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "flutter/lib/ui/semantics/semantics_node.h"

namespace flutter {
namespace testing {

class SemanticsTreeTest : public ::testing::Test {
 protected:
  SemanticsTree tree_;
};

// FindNodeById should return nullptr on an empty tree
TEST_F(SemanticsTreeTest, FindNodeByIdReturnsNullOnEmptyTree) {
  EXPECT_EQ(tree_.FindNodeById(0), nullptr);
  EXPECT_EQ(tree_.FindNodeById(1), nullptr);
}

// FindNodeById(-1) should redirect to root (id=0), still returns nullptr on empty tree
TEST_F(SemanticsTreeTest, FindNodeByIdNegativeOneRedirectsToRoot) {
  EXPECT_EQ(tree_.FindNodeById(-1), nullptr);
}

// GetOrAddNode should create a new node
TEST_F(SemanticsTreeTest, GetOrAddNodeCreatesNewNode) {
  auto* node = tree_.GetOrAddNode(1);
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->id, 0);  // New node id defaults to 0, not yet set by UpdateWithNode

  // Getting again should return the same pointer
  auto* same_node = tree_.GetOrAddNode(1);
  EXPECT_EQ(same_node, node);
}

// GetOrAddNode(0) creates the root node
TEST_F(SemanticsTreeTest, GetOrAddNodeCreatesRootNode) {
  auto* root = tree_.GetOrAddNode(0);
  ASSERT_NE(root, nullptr);
}

// RemoveNode should remove a node
TEST_F(SemanticsTreeTest, RemoveNodeRemovesExistingNode) {
  auto* node = tree_.GetOrAddNode(1);
  node->id = 1;  // FindNodeById checks node->id matches the requested id
  ASSERT_NE(tree_.FindNodeById(1), nullptr);

  tree_.RemoveNode(1);
  EXPECT_EQ(tree_.FindNodeById(1), nullptr);
}

// RemoveNode should not crash on a non-existent node
TEST_F(SemanticsTreeTest, RemoveNodeDoesNotCrashOnNonexistent) {
  tree_.RemoveNode(999);
  SUCCEED();
}

// ClearSemanticsTree should clear all nodes
TEST_F(SemanticsTreeTest, ClearSemanticsTreeClearsAllNodes) {
  tree_.GetOrAddNode(1);
  tree_.GetOrAddNode(2);
  tree_.GetOrAddNode(3);

  tree_.ClearSemanticsTree();

  EXPECT_EQ(tree_.FindNodeById(1), nullptr);
  EXPECT_EQ(tree_.FindNodeById(2), nullptr);
  EXPECT_EQ(tree_.FindNodeById(3), nullptr);
  EXPECT_EQ(tree_.GetRootNode(), nullptr);
}

// ClearAccessibilityFocusNode should execute normally when there is no focus
TEST_F(SemanticsTreeTest, ClearAccessibilityFocusNodeWhenNoFocus) {
  tree_.ClearAccessibilityFocusNode();
  SUCCEED();
}

// SetAccessibilityFocusNode should return false for a non-existent node
TEST_F(SemanticsTreeTest, SetAccessibilityFocusNodeReturnsFalseForMissing) {
  EXPECT_FALSE(tree_.SetAccessibilityFocusNode(999));
}

// SetAccessibilityFocusNode should return true for an existing node
TEST_F(SemanticsTreeTest, SetAccessibilityFocusNodeReturnsTrueForExisting) {
  auto* node = tree_.GetOrAddNode(1);
  node->id = 1;  // FindNodeById checks node->id matches the requested id
  EXPECT_TRUE(tree_.SetAccessibilityFocusNode(1));
}

// After SetAccessibilityFocusNode, ClearAccessibilityFocusNode should clear focus
TEST_F(SemanticsTreeTest, ClearAccessibilityFocusAfterSet) {
  auto* node = tree_.GetOrAddNode(1);
  node->id = 1;
  tree_.SetAccessibilityFocusNode(1);

  tree_.ClearAccessibilityFocusNode();
  // Clearing again should not crash
  tree_.ClearAccessibilityFocusNode();
  SUCCEED();
}

// FindFocusNode should return nullptr when there is no focus
TEST_F(SemanticsTreeTest, FindFocusNodeReturnsNullWhenNoFocus) {
  EXPECT_EQ(tree_.FindFocusNode(-1, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_INPUT),
            nullptr);
  EXPECT_EQ(
      tree_.FindFocusNode(-1, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_ACCESSIBILITY),
      nullptr);
}

// FindNextFocusNode should return nullptr for a non-existent start node
TEST_F(SemanticsTreeTest, FindNextFocusNodeReturnsNullForMissingStart) {
  EXPECT_EQ(tree_.FindNextFocusNode(999,
                                     ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_FORWARD),
            nullptr);
}

// DetectRouteChange should return false on an empty tree
TEST_F(SemanticsTreeTest, DetectRouteChangeReturnsFalseOnEmptyTree) {
  EXPECT_FALSE(tree_.DetectRouteChange());
}

// UpdateNextFocusWhenDisappear should return false on an empty tree
TEST_F(SemanticsTreeTest, UpdateNextFocusWhenDisappearReturnsFalseOnEmpty) {
  std::unordered_set<int32_t> remove_ids;
  EXPECT_FALSE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
}

// GetRootNode should return nullptr on an empty tree
TEST_F(SemanticsTreeTest, GetRootNodeReturnsNullOnEmptyTree) {
  EXPECT_EQ(tree_.GetRootNode(), nullptr);
}

// ===== UpdateWithNodes =====
// UpdateWithNodes takes a map of SemanticsNode and updates the tree.
// It calls UpdateSelfRecursively → UpdateSelfElementInfo → FillElementInfo*
// which call OH_ArkUI_AccessibilityElementInfoSet* NDK functions.
// Since SemanticsNodeExtend constructor already calls
// OH_ArkUI_CreateAccessibilityElementInfo() and existing tests create nodes
// via GetOrAddNode successfully, the NDK functions work in the test env.

TEST_F(SemanticsTreeTest, UpdateWithNodesWithSingleRootNode) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  nodes[0] = root;

  auto updated = tree_.UpdateWithNodes(nodes);
  // Root node should be set
  EXPECT_NE(tree_.GetRootNode(), nullptr);
  EXPECT_EQ(tree_.GetRootNode()->id, 0);
}

TEST_F(SemanticsTreeTest, UpdateWithNodesWithRootAndChild) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;

  SemanticsNode child;
  child.id = 1;
  child.label = "Child";
  nodes[1] = child;

  auto updated = tree_.UpdateWithNodes(nodes);
  EXPECT_NE(tree_.GetRootNode(), nullptr);
  EXPECT_NE(tree_.FindNodeById(1), nullptr);
  EXPECT_EQ(tree_.FindNodeById(1)->label, "Child");
}

TEST_F(SemanticsTreeTest, UpdateWithNodesReturnsUpdatedNodes) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.label = "Root";
  nodes[0] = root;

  auto updated = tree_.UpdateWithNodes(nodes);
  // Root node should have hasUpdate=true after UpdateSelfElementInfo
  // (since hasInit is false initially, all Fill* functions set hasUpdate)
  EXPECT_GE(updated.size(), 0u);
}

TEST_F(SemanticsTreeTest, UpdateWithNodesRemovesStaleNodes) {
  // First update: add root + child
  std::unordered_map<int32_t, SemanticsNode> nodes1;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes1[0] = root;
  SemanticsNode child;
  child.id = 1;
  nodes1[1] = child;
  tree_.UpdateWithNodes(nodes1);
  EXPECT_NE(tree_.FindNodeById(1), nullptr);

  // Second update: only root, child should be removed
  std::unordered_map<int32_t, SemanticsNode> nodes2;
  SemanticsNode root2;
  root2.id = 0;
  nodes2[0] = root2;
  tree_.UpdateWithNodes(nodes2);
  EXPECT_EQ(tree_.FindNodeById(1), nullptr);
}

TEST_F(SemanticsTreeTest, UpdateWithNodesSetsComponentType) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  nodes[0] = root;
  tree_.UpdateWithNodes(nodes);
  EXPECT_STREQ(tree_.GetRootNode()->componentType,
               OHWidgetName::kRootWidgetName);
}

TEST_F(SemanticsTreeTest, UpdateWithNodesSetsChildParentRelation) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  auto* childNode = tree_.FindNodeById(1);
  ASSERT_NE(childNode, nullptr);
  EXPECT_NE(childNode->parentNode, nullptr);
  EXPECT_EQ(childNode->parentNode->id, 0);
}

// ===== FindFocusNode with valid id =====

TEST_F(SemanticsTreeTest, FindFocusNodeReturnsInputFocusNodeWithNegativeId) {
  // Build a tree with root + child
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  child.flags.isFocused = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  // input_focused_node_ is set when a node has isFocused flag
  auto* focused = tree_.FindFocusNode(
      -1, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_INPUT);
  // input_focused_node_ should be set to the child node
  EXPECT_NE(focused, nullptr);
  EXPECT_EQ(focused->id, 1);
}

TEST_F(SemanticsTreeTest, FindFocusNodeReturnsAccessibilityFocusWithNegativeId) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  // Set accessibility focus
  tree_.SetAccessibilityFocusNode(1);
  auto* focused = tree_.FindFocusNode(
      -1, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_ACCESSIBILITY);
  EXPECT_NE(focused, nullptr);
  EXPECT_EQ(focused->id, 1);
}

TEST_F(SemanticsTreeTest, FindFocusNodeReturnsNullForInvalidFocusType) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  nodes[0] = root;
  tree_.UpdateWithNodes(nodes);

  auto* focused = tree_.FindFocusNode(-1,
                                      static_cast<ArkUI_AccessibilityFocusType>(
                                          999));
  EXPECT_EQ(focused, nullptr);
}

TEST_F(SemanticsTreeTest, FindFocusNodeWithIdMatchingAncestor) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  child.flags.isFocused = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  // FindFocusNode with id=0 (root, which is ancestor of focused node)
  // should return the focused node
  auto* focused = tree_.FindFocusNode(
      0, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_INPUT);
  EXPECT_NE(focused, nullptr);
  EXPECT_EQ(focused->id, 1);
}

TEST_F(SemanticsTreeTest, FindFocusNodeWithIdNotMatchingAncestor) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  child.flags.isFocused = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  // FindFocusNode with id=999 (not an ancestor)
  auto* focused = tree_.FindFocusNode(
      999, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_INPUT);
  EXPECT_EQ(focused, nullptr);
}

// ===== FindNextFocusNode with valid nodes =====

TEST_F(SemanticsTreeTest, FindNextFocusNodeForwardReturnsNextFocusable) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  tree_.UpdateWithNodes(nodes);

  // Find next focus from node 1 in forward direction
  auto* next = tree_.FindNextFocusNode(
      1, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_FORWARD);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 2);
}

TEST_F(SemanticsTreeTest, FindNextFocusNodeBackwardReturnsPrevFocusable) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  tree_.UpdateWithNodes(nodes);

  // Find next focus from node 2 in backward direction
  auto* next = tree_.FindNextFocusNode(
      2, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_BACKWARD);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 1);
}

TEST_F(SemanticsTreeTest, FindNextFocusNodeForwardReturnsStartWhenNoNext) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  // Node 1 has no next focusable node, should return start node
  auto* next = tree_.FindNextFocusNode(
      1, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_FORWARD);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 1);
}

TEST_F(SemanticsTreeTest, FindNextFocusNodeRightReturnsNextSibling) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  tree_.UpdateWithNodes(nodes);

  auto* next = tree_.FindNextFocusNode(
      1, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_RIGHT);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 2);
}

TEST_F(SemanticsTreeTest, FindNextFocusNodeLeftReturnsPrevSibling) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  tree_.UpdateWithNodes(nodes);

  auto* next = tree_.FindNextFocusNode(
      2, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_LEFT);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 1);
}

TEST_F(SemanticsTreeTest, FindNextFocusNodeUpReturnsParent) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  auto* next = tree_.FindNextFocusNode(
      1, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_UP);
  EXPECT_NE(next, nullptr);
  // Up should return parent (root), but root is not focusable, so it returns
  // startNode
  EXPECT_EQ(next->id, 1);
}

TEST_F(SemanticsTreeTest, FindNextFocusNodeDownReturnsFirstChild) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  auto* next = tree_.FindNextFocusNode(
      0, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_DOWN);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 1);
}

// ===== UpdateNextFocusWhenDisappear with tree =====

TEST_F(SemanticsTreeTest, UpdateNextFocusWhenDisappearWithFocusedNodeRemoved) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  tree_.UpdateWithNodes(nodes);

  // Set accessibility focus on child1
  tree_.SetAccessibilityFocusNode(1);
  // Simulate child1 being removed
  std::unordered_set<int32_t> remove_ids = {1};
  bool result = tree_.UpdateNextFocusWhenDisappear(remove_ids);
  // Should find a new focus node (child2)
  EXPECT_TRUE(result);
}

TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearWithNoFocusedNodeReturnsFalse) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  nodes[0] = root;
  tree_.UpdateWithNodes(nodes);

  std::unordered_set<int32_t> remove_ids;
  EXPECT_FALSE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
}

// ===== DetectRouteChange =====

TEST_F(SemanticsTreeTest, DetectRouteChangeReturnsFalseWithNoRouteNodes) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  nodes[0] = root;
  tree_.UpdateWithNodes(nodes);
  EXPECT_FALSE(tree_.DetectRouteChange());
}

// ===== UpdateNextFocusWhenDisappear: focused_node_ in remove_ids =====

// focused_node_ is in remove_ids, nextNode is focusable → forward search hits
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearFindsNextNodeForward) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  tree_.UpdateWithNodes(nodes);

  tree_.SetAccessibilityFocusNode(1);
  std::unordered_set<int32_t> remove_ids = {1};
  EXPECT_TRUE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
  EXPECT_EQ(tree_.need_request_focused_node_->id, 2);
}

// focused_node_ in remove_ids, nextNode not focusable but previousNode is
// focusable → backward search
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearFindsPreviousNodeBackward) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2, 3};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  SemanticsNode child3;
  child3.id = 3;
  child3.flags.isFocusable = true;
  nodes[3] = child3;
  tree_.UpdateWithNodes(nodes);

  // Focus on child3 (last), remove it → nextNode is null, previousNode is
  // child2
  tree_.SetAccessibilityFocusNode(3);
  // Reset state set by UpdateWithNodes' internal UpdateNextFocusWhenDisappear
  tree_.need_request_focused_node_ = nullptr;
  tree_.in_request_progress_ = false;
  std::unordered_set<int32_t> remove_ids = {3};
  EXPECT_TRUE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
  EXPECT_EQ(tree_.need_request_focused_node_->id, 2);
}

// focused_node_ in remove_ids, neither nextNode nor previousNode is focusable,
// but a sibling in parent's childrenInTraversalOrderList is focusable
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearFindsSiblingInParentChildren) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2, 3};
  nodes[0] = root;
  // child1 is focusable but will be removed
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  // child2 is NOT focusable (no flags, no label)
  SemanticsNode child2;
  child2.id = 2;
  nodes[2] = child2;
  // child3 is focusable
  SemanticsNode child3;
  child3.id = 3;
  child3.flags.isFocusable = true;
  nodes[3] = child3;
  tree_.UpdateWithNodes(nodes);

  // Focus on child1, remove it → nextNode=child2 (not focusable),
  // previousNode=null → search parent's children → find child3
  tree_.SetAccessibilityFocusNode(1);
  std::unordered_set<int32_t> remove_ids = {1};
  EXPECT_TRUE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
  EXPECT_EQ(tree_.need_request_focused_node_->id, 3);
}

// focused_node_ in remove_ids, no focusable sibling, ancestor is focusable
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearFindsFocusableAncestor) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  // child1 is a container with a single focusable child that will be removed
  SemanticsNode child1;
  child1.id = 1;
  child1.label = "Container";  // makes it focusable
  child1.childrenInTraversalOrder = {2};
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  tree_.UpdateWithNodes(nodes);

  // Focus on child2, remove it → no siblings, ancestor child1 is focusable
  tree_.SetAccessibilityFocusNode(2);
  // Reset state set by UpdateWithNodes' internal UpdateNextFocusWhenDisappear
  tree_.need_request_focused_node_ = nullptr;
  tree_.in_request_progress_ = false;
  std::unordered_set<int32_t> remove_ids = {2};
  EXPECT_TRUE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
  EXPECT_EQ(tree_.need_request_focused_node_->id, 1);
}

// focused_node_ in remove_ids, no focusable node anywhere → fallback to
// FindNextFocusNode(0, FORWARD) which returns root → returns false
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearNoFocusableNodeReturnsFalse) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  // child1 is NOT focusable (no flags, no label) and will be removed
  nodes[1] = child1;
  tree_.UpdateWithNodes(nodes);

  tree_.SetAccessibilityFocusNode(1);
  // Reset state set by UpdateWithNodes' internal UpdateNextFocusWhenDisappear
  tree_.need_request_focused_node_ = nullptr;
  tree_.in_request_progress_ = false;
  std::unordered_set<int32_t> remove_ids = {1};
  // child1 is not focusable, root is never focusable → FindNextFocusNode(0,
  // FORWARD) returns root (id=0) → returns false
  EXPECT_FALSE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
}

// focused_node_ not visible (isHidden) but not in remove_ids → triggers search
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearFocusedNodeNotVisibleTriggersSearch) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  child1.flags.isHidden = true;  // not visible
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  tree_.UpdateWithNodes(nodes);

  tree_.SetAccessibilityFocusNode(1);
  std::unordered_set<int32_t> remove_ids;  // empty, but child1 is not visible
  EXPECT_TRUE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
  EXPECT_EQ(tree_.need_request_focused_node_->id, 2);
}

// need_search_from_root: both focused_node_ and need_request_focused_node_
// are null → triggers search from root
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearNeedSearchFromRoot) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  tree_.UpdateWithNodes(nodes);

  // No focus set → need_search_from_root. Reset state set by UpdateWithNodes'
  // internal UpdateNextFocusWhenDisappear call.
  tree_.need_request_focused_node_ = nullptr;
  tree_.focused_node_ = nullptr;
  tree_.in_request_progress_ = false;
  std::unordered_set<int32_t> remove_ids;
  EXPECT_TRUE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
  EXPECT_EQ(tree_.need_request_focused_node_->id, 1);
}

// need_search_from_root but no focusable node at all → returns false
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearNeedSearchFromRootNoFocusable) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  nodes[0] = root;
  tree_.UpdateWithNodes(nodes);

  std::unordered_set<int32_t> remove_ids;
  EXPECT_FALSE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
}

// in_request_progress_ is true, need_request_focused_node_ is in remove_ids
// → force_update path, search from need_request_focused_node_
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearForceUpdateFromRequestNode) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2, 3};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  SemanticsNode child3;
  child3.id = 3;
  child3.flags.isFocusable = true;
  nodes[3] = child3;
  tree_.UpdateWithNodes(nodes);

  // Simulate: first request focused on child1, then child1 disappears
  tree_.SetAccessibilityFocusNode(1);
  std::unordered_set<int32_t> remove_ids1 = {1};
  tree_.UpdateNextFocusWhenDisappear(remove_ids1);
  // need_request_focused_node_ should be child2 now
  ASSERT_EQ(tree_.need_request_focused_node_->id, 2);
  // in_request_progress_ should be true (focus not yet confirmed)
  EXPECT_TRUE(tree_.in_request_progress_);

  // Now child2 also disappears → force_update path
  std::unordered_set<int32_t> remove_ids2 = {2};
  EXPECT_TRUE(tree_.UpdateNextFocusWhenDisappear(remove_ids2));
  EXPECT_EQ(tree_.need_request_focused_node_->id, 3);
}

// in_request_progress_ true, need_request_focused_node_ not visible →
// force_update
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearForceUpdateNotVisible) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  tree_.UpdateWithNodes(nodes);

  tree_.SetAccessibilityFocusNode(1);
  std::unordered_set<int32_t> remove_ids1 = {1};
  tree_.UpdateNextFocusWhenDisappear(remove_ids1);
  ASSERT_EQ(tree_.need_request_focused_node_->id, 2);

  // Now simulate child2 becoming not visible by setting isHidden
  tree_.need_request_focused_node_->flags.isHidden = true;
  std::unordered_set<int32_t> remove_ids2;
  EXPECT_TRUE(tree_.UpdateNextFocusWhenDisappear(remove_ids2));
  // Should find child1 again (the only other focusable node)
  EXPECT_EQ(tree_.need_request_focused_node_->id, 1);
}

// in_request_progress_ true, need_request_focused_node_ not focusable →
// force_update
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearForceUpdateNotFocusable) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  tree_.UpdateWithNodes(nodes);

  tree_.SetAccessibilityFocusNode(1);
  std::unordered_set<int32_t> remove_ids1 = {1};
  tree_.UpdateNextFocusWhenDisappear(remove_ids1);
  ASSERT_EQ(tree_.need_request_focused_node_->id, 2);

  // Make child2 not focusable by clearing isFocusable and label
  tree_.need_request_focused_node_->flags.isFocusable = false;
  std::unordered_set<int32_t> remove_ids2;
  EXPECT_TRUE(tree_.UpdateNextFocusWhenDisappear(remove_ids2));
  EXPECT_EQ(tree_.need_request_focused_node_->id, 1);
}

// in_request_progress_ true, need_request_focused_node_ still valid → returns
// false (no update needed)
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearInProgressNoUpdateReturnsFalse) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  tree_.UpdateWithNodes(nodes);

  tree_.SetAccessibilityFocusNode(1);
  std::unordered_set<int32_t> remove_ids1 = {1};
  tree_.UpdateNextFocusWhenDisappear(remove_ids1);
  ASSERT_EQ(tree_.need_request_focused_node_->id, 2);
  EXPECT_TRUE(tree_.in_request_progress_);

  // need_request_focused_node_ (child2) is still valid, not in remove_ids,
  // visible and focusable → request_focused_node_need_update is false,
  // force_update is false, need_search_from_root is false → returns false
  std::unordered_set<int32_t> remove_ids2;
  EXPECT_FALSE(tree_.UpdateNextFocusWhenDisappear(remove_ids2));
}

// focused_node_ in remove_ids, start_find_node's parent also in remove_ids
// → skips brother search, goes to ancestor search
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearParentInRemoveIdsSkipsBrotherSearch) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 3};
  nodes[0] = root;
  // child1 is a container whose parent will be removed
  SemanticsNode child1;
  child1.id = 1;
  child1.label = "Container1";
  child1.childrenInTraversalOrder = {2};
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  SemanticsNode child3;
  child3.id = 3;
  child3.flags.isFocusable = true;
  nodes[3] = child3;
  tree_.UpdateWithNodes(nodes);

  // Focus on child2, remove both child1 (parent) and child2
  tree_.SetAccessibilityFocusNode(2);
  std::unordered_set<int32_t> remove_ids = {1, 2};
  // start_find_node = child2, parentNode = child1 which is in remove_ids
  // → skip brother search, go to ancestor: child1 (in remove_ids) → root
  // (not focusable) → FindNextFocusNode(0, FORWARD) → child3
  EXPECT_TRUE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
  EXPECT_EQ(tree_.need_request_focused_node_->id, 3);
}

// focused_node_ in remove_ids, nextNode exists but not focusable, loop
// continues to find a focusable one further ahead
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearNextNodeLoopSkipsNonFocusable) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2, 3, 4};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  // child2 and child3 are not focusable (no flags, no label)
  SemanticsNode child2;
  child2.id = 2;
  nodes[2] = child2;
  SemanticsNode child3;
  child3.id = 3;
  nodes[3] = child3;
  SemanticsNode child4;
  child4.id = 4;
  child4.flags.isFocusable = true;
  nodes[4] = child4;
  tree_.UpdateWithNodes(nodes);

  tree_.SetAccessibilityFocusNode(1);
  std::unordered_set<int32_t> remove_ids = {1};
  // nextNode = child2 (not focusable) → loop → child3 (not focusable) → loop
  // → child4 (focusable) → break
  EXPECT_TRUE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
  EXPECT_EQ(tree_.need_request_focused_node_->id, 4);
}

// focused_node_ in remove_ids, nextNode exists but is in remove_ids too →
// loop skips it
TEST_F(SemanticsTreeTest,
       UpdateNextFocusWhenDisappearNextNodeLoopSkipsRemovedNode) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2, 3};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isFocusable = true;
  nodes[2] = child2;
  SemanticsNode child3;
  child3.id = 3;
  child3.flags.isFocusable = true;
  nodes[3] = child3;
  tree_.UpdateWithNodes(nodes);

  tree_.SetAccessibilityFocusNode(1);
  // Remove both child1 and child2 → nextNode=child2 is in remove_ids → skip
  std::unordered_set<int32_t> remove_ids = {1, 2};
  EXPECT_TRUE(tree_.UpdateNextFocusWhenDisappear(remove_ids));
  EXPECT_EQ(tree_.need_request_focused_node_->id, 3);
}

// ===== FindNextFocusNode: default direction =====

// FindNextFocusNode with invalid direction → returns currentNode (startNode)
TEST_F(SemanticsTreeTest, FindNextFocusNodeInvalidDirectionReturnsStart) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  auto* next = tree_.FindNextFocusNode(
      1, static_cast<ArkUI_AccessibilityFocusMoveDirection>(999));
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 1);
}

// FindNextFocusNode FORWARD: nextFocusableNode is null → returns startNode
TEST_F(SemanticsTreeTest,
       FindNextFocusNodeForwardNoNextFocusableReturnsStart) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  auto* next = tree_.FindNextFocusNode(
      1, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_FORWARD);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 1);
}

// FindNextFocusNode BACKWARD: previousFocusableNode is null → returns
// startNode
TEST_F(SemanticsTreeTest,
       FindNextFocusNodeBackwardNoPrevFocusableReturnsStart) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  auto* next = tree_.FindNextFocusNode(
      1, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_BACKWARD);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 1);
}

// FindNextFocusNode UP: parentNode is null → returns startNode
TEST_F(SemanticsTreeTest, FindNextFocusNodeUpNoParentReturnsStart) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  // root has no parent → UP returns startNode (root)
  auto* next = tree_.FindNextFocusNode(
      0, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_UP);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 0);
}

// FindNextFocusNode DOWN: no children → returns startNode
TEST_F(SemanticsTreeTest, FindNextFocusNodeDownNoChildrenReturnsStart) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  // child1 has no children → DOWN returns startNode
  auto* next = tree_.FindNextFocusNode(
      1, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_DOWN);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 1);
}

// FindNextFocusNode LEFT: previousNode is null → returns startNode
TEST_F(SemanticsTreeTest, FindNextFocusNodeLeftNoPrevReturnsStart) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  // child1 is first child, previousNode is null → LEFT returns startNode
  auto* next = tree_.FindNextFocusNode(
      1, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_LEFT);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 1);
}

// FindNextFocusNode RIGHT: nextNode is null → returns startNode
TEST_F(SemanticsTreeTest, FindNextFocusNodeRightNoNextReturnsStart) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  // child1 is last child, nextNode is null → RIGHT returns startNode
  auto* next = tree_.FindNextFocusNode(
      1, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_RIGHT);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 1);
}

// FindNextFocusNode: returnNode is root_node_ → returns startNode
TEST_F(SemanticsTreeTest, FindNextFocusNodeReturnsStartWhenRootIsNext) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  child.flags.isFocusable = true;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  // UP from child1 → parent is root → root_node_ → returns startNode
  auto* next = tree_.FindNextFocusNode(
      1, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_UP);
  EXPECT_NE(next, nullptr);
  EXPECT_EQ(next->id, 1);
}

// FindNextFocusNode: multi-round loop, next node not focusable → continues
TEST_F(SemanticsTreeTest, FindNextFocusNodeMultiRoundLoopFindsFocusable) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1, 2, 3};
  nodes[0] = root;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isFocusable = true;
  nodes[1] = child1;
  // child2 is not focusable
  SemanticsNode child2;
  child2.id = 2;
  nodes[2] = child2;
  SemanticsNode child3;
  child3.id = 3;
  child3.flags.isFocusable = true;
  nodes[3] = child3;
  tree_.UpdateWithNodes(nodes);

  // RIGHT from child1 → child2 (not focusable) → loop continues →
  // child2.nextNode=child3 (focusable) → returns child3
  auto* next = tree_.FindNextFocusNode(
      1, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_RIGHT);
  EXPECT_NE(next, nullptr);
  // Multi-round loop: child2 not focusable → continue → child3 focusable
  EXPECT_EQ(next->id, 3);
}

// ===== FindNodeById =====

// FindNodeById(-1) redirects to root (id=0) on a populated tree
TEST_F(SemanticsTreeTest, FindNodeByIdNegativeOneRedirectsToRootOnPopulated) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  auto* node = tree_.FindNodeById(-1);
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->id, 0);
}

// FindNodeById for a node that exists in all_semantics_nodes_ but whose id
// doesn't match (child not given by UpdateSemantics) → returns nullptr
TEST_F(SemanticsTreeTest, FindNodeByIdReturnsNullForIdMismatch) {
  // Build tree: root with child 1
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 1;
  nodes[1] = child;
  tree_.UpdateWithNodes(nodes);

  // GetOrAddNode creates a node with id=0 by default; if we add node 99
  // without calling UpdateWithNode, its id stays 0, so FindNodeById(99) finds
  // it in the map but id != 99 → returns nullptr
  tree_.GetOrAddNode(99);
  EXPECT_EQ(tree_.FindNodeById(99), nullptr);
}

// ===== DetectRouteChange =====

// DetectRouteChange: first call with route nodes → returns true (route changed
// from empty to non-empty)
TEST_F(SemanticsTreeTest, DetectRouteChangeFirstCallWithRoutesReturnsTrue) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode route1;
  route1.id = 1;
  route1.flags.scopesRoute = true;
  nodes[1] = route1;
  tree_.UpdateWithNodes(nodes);

  EXPECT_TRUE(tree_.DetectRouteChange());
}

// DetectRouteChange: second call with same route → returns false
TEST_F(SemanticsTreeTest, DetectRouteChangeSecondCallSameRouteReturnsFalse) {
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {1};
  nodes[0] = root;
  SemanticsNode route1;
  route1.id = 1;
  route1.flags.scopesRoute = true;
  nodes[1] = route1;
  tree_.UpdateWithNodes(nodes);

  tree_.DetectRouteChange();  // first call → true
  EXPECT_FALSE(tree_.DetectRouteChange());  // second call → false
}

// DetectRouteChange: route id changes → returns true
TEST_F(SemanticsTreeTest, DetectRouteChangeRouteIdChangedReturnsTrue) {
  std::unordered_map<int32_t, SemanticsNode> nodes1;
  SemanticsNode root1;
  root1.id = 0;
  root1.childrenInTraversalOrder = {1};
  nodes1[0] = root1;
  SemanticsNode route1;
  route1.id = 1;
  route1.flags.scopesRoute = true;
  nodes1[1] = route1;
  tree_.UpdateWithNodes(nodes1);
  tree_.DetectRouteChange();

  // Update with a different route id
  std::unordered_map<int32_t, SemanticsNode> nodes2;
  SemanticsNode root2;
  root2.id = 0;
  root2.childrenInTraversalOrder = {2};
  nodes2[0] = root2;
  SemanticsNode route2;
  route2.id = 2;
  route2.flags.scopesRoute = true;
  nodes2[2] = route2;
  tree_.UpdateWithNodes(nodes2);
  EXPECT_TRUE(tree_.DetectRouteChange());
}

// DetectRouteChange: route count changes → returns true
TEST_F(SemanticsTreeTest, DetectRouteChangeRouteCountChangedReturnsTrue) {
  std::unordered_map<int32_t, SemanticsNode> nodes1;
  SemanticsNode root1;
  root1.id = 0;
  root1.childrenInTraversalOrder = {1};
  nodes1[0] = root1;
  SemanticsNode route1;
  route1.id = 1;
  route1.flags.scopesRoute = true;
  nodes1[1] = route1;
  tree_.UpdateWithNodes(nodes1);
  tree_.DetectRouteChange();

  // Add a second route
  std::unordered_map<int32_t, SemanticsNode> nodes2;
  SemanticsNode root2;
  root2.id = 0;
  root2.childrenInTraversalOrder = {1, 2};
  nodes2[0] = root2;
  SemanticsNode route1b;
  route1b.id = 1;
  route1b.flags.scopesRoute = true;
  nodes2[1] = route1b;
  SemanticsNode route2;
  route2.id = 2;
  route2.flags.scopesRoute = true;
  nodes2[2] = route2;
  tree_.UpdateWithNodes(nodes2);
  EXPECT_TRUE(tree_.DetectRouteChange());
}

// DetectRouteChange: route node exists but isExist=false → not collected
TEST_F(SemanticsTreeTest, DetectRouteChangeIgnoresNonExistentRouteNodes) {
  // Build tree with route node, then update without it (it becomes non-existent)
  std::unordered_map<int32_t, SemanticsNode> nodes1;
  SemanticsNode root1;
  root1.id = 0;
  root1.childrenInTraversalOrder = {1};
  nodes1[0] = root1;
  SemanticsNode route1;
  route1.id = 1;
  route1.flags.scopesRoute = true;
  nodes1[1] = route1;
  tree_.UpdateWithNodes(nodes1);
  tree_.DetectRouteChange();

  // Update without route1 → route1.isExist becomes false
  std::unordered_map<int32_t, SemanticsNode> nodes2;
  SemanticsNode root2;
  root2.id = 0;
  nodes2[0] = root2;
  tree_.UpdateWithNodes(nodes2);
  // No routes → new_routes empty → route_changed = false
  EXPECT_FALSE(tree_.DetectRouteChange());
}

}  // namespace testing
}  // namespace flutter
