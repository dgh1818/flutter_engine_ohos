/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_bridge.h"
#include <gtest/gtest.h>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "flutter/lib/ui/semantics/semantics_node.h"

namespace flutter {
namespace testing {

class SemanticsBridgeTest : public ::testing::Test {
 protected:
  SemanticsBridge bridge_;

  void SetUp() override {
    bridge_.is_accessibility_enabled_ = true;
#if defined(OHOS_X64_UNITTEST)
    bridge_.provider_ohos_ = MakeProvider();
#endif
  }

  ArkUI_AccessibilityElementInfo* MakeInfo() {
    return OH_ArkUI_CreateAccessibilityElementInfo();
  }

  ArkUI_AccessibilityElementInfoList* MakeList() {
    static char storage;
    return reinterpret_cast<ArkUI_AccessibilityElementInfoList*>(&storage);
  }

  ArkUI_AccessibilityProvider* MakeProvider() {
    static char storage;
    return reinterpret_cast<ArkUI_AccessibilityProvider*>(&storage);
  }

  void BuildRootWithChildren(SemanticsNodeUpdates& nodes,
                             std::vector<int> child_ids) {
    SemanticsNode root;
    root.id = 0;
    root.childrenInTraversalOrder = std::move(child_ids);
    nodes[0] = root;
  }
};

TEST_F(SemanticsBridgeTest, SendSemanticsEventRespectsEnabledState) {
  SemanticsNodeUpdates nodes;
  SemanticsNode child;
  child.id = 1;
  child.label = "node1";
  nodes[1] = child;
  BuildRootWithChildren(nodes, {1});
  bridge_.tree_.UpdateWithNodes(nodes);
  auto* node1 = bridge_.tree_.FindNodeById(1);
  ASSERT_NE(node1, nullptr);
  EXPECT_TRUE(node1->hasUpdate);

  bridge_.is_accessibility_enabled_ = false;
  bridge_.SendSemanticsEvent(
      node1, ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_CLICKED, nullptr);
  EXPECT_TRUE(node1->hasUpdate);

  bridge_.is_accessibility_enabled_ = true;
  bridge_.provider_ohos_ = nullptr;
  bridge_.SendSemanticsEvent(
      node1, ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_CLICKED, nullptr);
  EXPECT_FALSE(node1->hasUpdate);

#if defined(OHOS_X64_UNITTEST)
  bridge_.provider_ohos_ = MakeProvider();
#endif
  bridge_.SendSemanticsEvent(
      node1, ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_FOCUS_NODE_UPDATE, nullptr);
  EXPECT_FALSE(node1->hasUpdate);

  auto msg = std::make_unique<char[]>(8);
  std::snprintf(msg.get(), 8, "hello");
  auto empty_msg = std::make_unique<char[]>(1);
  empty_msg[0] = '\0';
  bridge_.Announce(msg);
  bridge_.Announce(empty_msg);

  node1->hasUpdate = true;
  bridge_.OnTap(999);
  EXPECT_TRUE(node1->hasUpdate);
}

TEST_F(SemanticsBridgeTest, UpdateNodeTreeSendsScrolledEventAndClearsFlag) {
  SemanticsNodeUpdates nodes;
  SemanticsNode scroller;
  scroller.id = 1;
  scroller.scrollChildren = 4;
  scroller.scrollIndex = 0;
  scroller.childrenInTraversalOrder = {2, 3};
  nodes[1] = scroller;
  SemanticsNode visible_child;
  visible_child.id = 2;
  nodes[2] = visible_child;
  SemanticsNode hidden_child;
  hidden_child.id = 3;
  hidden_child.flags.isHidden = true;
  nodes[3] = hidden_child;
  BuildRootWithChildren(nodes, {1});
  bridge_.UpdateNodeTree(nodes);

  auto* scroller_node = bridge_.tree_.FindNodeById(1);
  ASSERT_NE(scroller_node, nullptr);
  EXPECT_TRUE(scroller_node->scrollChanged);
  EXPECT_EQ(scroller_node->scrollEndIndex, 0);

  ASSERT_TRUE(bridge_.tree_.SetAccessibilityFocusNode(2));
  bridge_.UpdateNodeTree(nodes);
  EXPECT_EQ(scroller_node->scrollCurrentIndex, 0);
  EXPECT_FALSE(scroller_node->scrollChanged);
  EXPECT_FALSE(scroller_node->hasUpdate);
}

TEST_F(SemanticsBridgeTest, UpdateNodeTreeKeepsScrollChangedWhenEndIndexInvalid) {
  SemanticsNodeUpdates nodes;
  SemanticsNode scroller;
  scroller.id = 1;
  scroller.scrollChildren = 4;
  scroller.scrollIndex = 0;
  nodes[1] = scroller;
  BuildRootWithChildren(nodes, {1});
  bridge_.UpdateNodeTree(nodes);

  auto* scroller_node = bridge_.tree_.FindNodeById(1);
  ASSERT_NE(scroller_node, nullptr);
  EXPECT_EQ(scroller_node->scrollEndIndex, -1);
  EXPECT_TRUE(scroller_node->scrollChanged);
}

TEST_F(SemanticsBridgeTest, UpdateNodeTreeSelectEventClearsFlags) {
  SemanticsNodeUpdates nodes;
  SemanticsNode child;
  child.id = 1;
  nodes[1] = child;
  BuildRootWithChildren(nodes, {1});
  bridge_.UpdateNodeTree(nodes);

  auto* node1 = bridge_.tree_.FindNodeById(1);
  ASSERT_NE(node1, nullptr);
  EXPECT_FALSE(node1->selectChanged);
  node1->performSelectAction = true;

  SemanticsNodeUpdates updated;
  SemanticsNode changed;
  changed.id = 1;
  changed.textSelectionBase = 5;
  changed.textSelectionExtent = 7;
  updated[1] = changed;
  BuildRootWithChildren(updated, {1});
  bridge_.UpdateNodeTree(updated);

  EXPECT_FALSE(node1->selectChanged);
  EXPECT_FALSE(node1->performSelectAction);
  EXPECT_FALSE(node1->hasUpdate);
}

TEST_F(SemanticsBridgeTest, UpdateNodeTreeEmptyMapIsNoop) {
  SemanticsNodeUpdates nodes;
  EXPECT_NO_FATAL_FAILURE(bridge_.UpdateNodeTree(nodes));
  EXPECT_EQ(bridge_.tree_.GetRootNode(), nullptr);
}

TEST_F(SemanticsBridgeTest, UpdateFocusedNodeSendsPageUpdateOnRouteChange) {
  SemanticsNodeUpdates nodes;
  SemanticsNode route;
  route.id = 1;
  route.flags.scopesRoute = true;
  nodes[1] = route;
  BuildRootWithChildren(nodes, {1});
  bridge_.tree_.UpdateWithNodes(nodes);

  auto* root = bridge_.tree_.GetRootNode();
  ASSERT_NE(root, nullptr);
  EXPECT_TRUE(root->hasUpdate);

  bridge_.UpdateFocusedNode();
  EXPECT_FALSE(root->hasUpdate);
}

TEST_F(SemanticsBridgeTest, UpdateFocusedNodeSkipsPageUpdateWithoutRoute) {
  SemanticsNodeUpdates nodes;
  SemanticsNode child;
  child.id = 1;
  child.label = "node1";
  nodes[1] = child;
  BuildRootWithChildren(nodes, {1});
  bridge_.UpdateNodeTree(nodes);

  auto* root = bridge_.tree_.GetRootNode();
  ASSERT_NE(root, nullptr);
  bridge_.UpdateFocusedNode();
  EXPECT_TRUE(root->hasUpdate);
}

TEST_F(SemanticsBridgeTest, UpdateFocusedNodeClearsFocusedNodeUpdate) {
  SemanticsNodeUpdates nodes;
  SemanticsNode child;
  child.id = 1;
  nodes[1] = child;
  BuildRootWithChildren(nodes, {1});
  bridge_.UpdateNodeTree(nodes);

  auto* node1 = bridge_.tree_.FindNodeById(1);
  ASSERT_NE(node1, nullptr);
  ASSERT_TRUE(bridge_.tree_.SetAccessibilityFocusNode(1));
  bridge_.tree_.need_request_focused_node_ = nullptr;
  bridge_.tree_.in_request_progress_ = false;
  EXPECT_TRUE(node1->hasUpdate);

  bridge_.UpdateFocusedNode();
  EXPECT_FALSE(node1->hasUpdate);
}

TEST_F(SemanticsBridgeTest, UpdateFocusedNodeRequestsNextFocusOnce) {
  SemanticsNodeUpdates nodes;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isTextField = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isTextField = true;
  nodes[2] = child2;
  BuildRootWithChildren(nodes, {1, 2});
  bridge_.UpdateNodeTree(nodes);

  ASSERT_TRUE(bridge_.tree_.SetAccessibilityFocusNode(1));
  bridge_.tree_.focus_request_has_send_ = false;
  std::unordered_set<int32_t> remove_ids = {1};
  ASSERT_TRUE(bridge_.tree_.UpdateNextFocusWhenDisappear(remove_ids));
  ASSERT_EQ(bridge_.tree_.need_request_focused_node_->id, 2);
  EXPECT_FALSE(bridge_.tree_.focus_request_has_send_);

  auto* node2 = bridge_.tree_.FindNodeById(2);
  ASSERT_NE(node2, nullptr);
  node2->hasUpdate = true;
  bridge_.UpdateFocusedNode();
  EXPECT_TRUE(bridge_.tree_.focus_request_has_send_);
  EXPECT_FALSE(node2->hasUpdate);

  node2->hasUpdate = true;
  bridge_.UpdateFocusedNode();
  EXPECT_TRUE(node2->hasUpdate);
}

TEST_F(SemanticsBridgeTest, FindFocusNodeSuccessAndFailure) {
  SemanticsNodeUpdates nodes;
  SemanticsNode child;
  child.id = 1;
  child.flags.isTextField = true;
  child.flags.isFocused = SemanticsTristate::kTrue;
  nodes[1] = child;
  BuildRootWithChildren(nodes, {1});
  bridge_.UpdateNodeTree(nodes);

  auto* info = MakeInfo();
  EXPECT_EQ(bridge_.FindFocusNode(0, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_INPUT,
                                  info),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  ASSERT_TRUE(bridge_.tree_.SetAccessibilityFocusNode(1));
  EXPECT_EQ(bridge_.FindFocusNode(
                -1, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_ACCESSIBILITY, info),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_EQ(bridge_.FindFocusNode(
                999, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_ACCESSIBILITY, info),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

TEST_F(SemanticsBridgeTest, FindNextFocusNodeSuccessAndFailure) {
  SemanticsNodeUpdates nodes;
  SemanticsNode child1;
  child1.id = 1;
  child1.flags.isTextField = true;
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.flags.isTextField = true;
  nodes[2] = child2;
  BuildRootWithChildren(nodes, {1, 2});
  bridge_.UpdateNodeTree(nodes);

  auto* info = MakeInfo();
  EXPECT_EQ(bridge_.FindNextFocusNode(
                1, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_FORWARD, info),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_EQ(bridge_.FindNextFocusNode(
                999, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_FORWARD, info),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

TEST_F(SemanticsBridgeTest, FillNodesWithSearchTextResults) {
  SemanticsNodeUpdates nodes;
  SemanticsNode child;
  child.id = 1;
  child.label = "match";
  nodes[1] = child;
  BuildRootWithChildren(nodes, {1});
  bridge_.UpdateNodeTree(nodes);
  auto* list = MakeList();

  EXPECT_EQ(bridge_.FillNodesWithSearchText(0, "match", list),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_EQ(bridge_.FillNodesWithSearchText(0, "nope", list),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_EQ(bridge_.FillNodesWithSearchText(999, "match", list),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

TEST_F(SemanticsBridgeTest, FillNodesWithSearchResults) {
  SemanticsNodeUpdates nodes;
  SemanticsNode child1;
  child1.id = 1;
  child1.label = "one";
  nodes[1] = child1;
  SemanticsNode child2;
  child2.id = 2;
  child2.label = "two";
  nodes[2] = child2;
  BuildRootWithChildren(nodes, {1, 2});
  bridge_.UpdateNodeTree(nodes);
  auto* list = MakeList();

  EXPECT_EQ(bridge_.FillNodesWithSearch(
                0, ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_CURRENT,
                list),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_EQ(bridge_.FillNodesWithSearch(
                0, ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_CHILDREN,
                list),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_EQ(bridge_.FillNodesWithSearch(
                1, ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_SIBLINGS,
                list),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_EQ(bridge_.FillNodesWithSearch(
                1,
                ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_PREDECESSORS,
                list),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_EQ(bridge_.FillNodesWithSearch(
                0, static_cast<ArkUI_AccessibilitySearchMode>(999), list),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  EXPECT_EQ(bridge_.FillNodesWithSearch(
                999, ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_CURRENT,
                list),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

TEST_F(SemanticsBridgeTest, ClearAccessibilityFocusBranches) {
  EXPECT_EQ(bridge_.ClearAccessibilityFocus(1),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);

  SemanticsNodeUpdates nodes;
  SemanticsNode child;
  child.id = 1;
  child.label = "node1";
  nodes[1] = child;
  BuildRootWithChildren(nodes, {1});
  bridge_.UpdateNodeTree(nodes);
  auto* node1 = bridge_.tree_.FindNodeById(1);
  ASSERT_NE(node1, nullptr);

  ASSERT_TRUE(bridge_.tree_.SetAccessibilityFocusNode(1));
  EXPECT_EQ(bridge_.ClearAccessibilityFocus(42),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_EQ(bridge_.tree_.focused_node_, node1);
  EXPECT_TRUE(node1->isAccessibilityFocued);

  EXPECT_EQ(bridge_.ClearAccessibilityFocus(1),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_EQ(bridge_.tree_.focused_node_, nullptr);
  EXPECT_FALSE(node1->isAccessibilityFocued);

  ASSERT_TRUE(bridge_.tree_.SetAccessibilityFocusNode(1));
  EXPECT_EQ(bridge_.ClearAccessibilityFocus(0),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_EQ(bridge_.tree_.focused_node_, nullptr);
}

TEST_F(SemanticsBridgeTest, GainAccessibilityFocusBranches) {
  EXPECT_EQ(bridge_.GainAccessibilityFocus(999, nullptr),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);

  SemanticsNodeUpdates nodes;
  SemanticsNode child;
  child.id = 1;
  child.label = "node1";
  nodes[1] = child;
  BuildRootWithChildren(nodes, {1});
  bridge_.UpdateNodeTree(nodes);
  auto* node1 = bridge_.tree_.FindNodeById(1);
  ASSERT_NE(node1, nullptr);

  node1->hasUpdate = true;
  bridge_.tree_.need_request_focused_node_ = nullptr;
  bridge_.is_accessibility_enabled_ = false;
  bool need_show = false;
  EXPECT_EQ(bridge_.GainAccessibilityFocus(1, &need_show),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_TRUE(need_show);
  EXPECT_TRUE(node1->hasUpdate);

  bridge_.is_accessibility_enabled_ = true;
  need_show = false;
  EXPECT_EQ(bridge_.GainAccessibilityFocus(1, &need_show),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_TRUE(need_show);
  EXPECT_FALSE(node1->hasUpdate);

  bridge_.tree_.need_request_focused_node_ = node1;
  bridge_.tree_.focus_request_has_send_ = true;
  need_show = true;
  EXPECT_EQ(bridge_.GainAccessibilityFocus(1, &need_show),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_FALSE(need_show);
  EXPECT_EQ(bridge_.tree_.need_request_focused_node_, nullptr);
  EXPECT_FALSE(bridge_.tree_.focus_request_has_send_);
}

TEST_F(SemanticsBridgeTest, GetAccessibilityNodeCursorPositionResults) {
  SemanticsNodeUpdates nodes;
  SemanticsNode child;
  child.id = 1;
  child.textSelectionBase = 7;
  nodes[1] = child;
  BuildRootWithChildren(nodes, {1});
  bridge_.UpdateNodeTree(nodes);

  int32_t index = -1;
  EXPECT_EQ(bridge_.GetAccessibilityNodeCursorPosition(1, &index),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  EXPECT_EQ(index, 7);

  index = 12345;
  EXPECT_EQ(bridge_.GetAccessibilityNodeCursorPosition(999, &index),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
  EXPECT_EQ(index, 12345);
}

TEST_F(SemanticsBridgeTest, OnTapAndOnLongPressClearHasUpdate) {
  SemanticsNodeUpdates nodes;
  SemanticsNode child;
  child.id = 1;
  nodes[1] = child;
  BuildRootWithChildren(nodes, {1});
  bridge_.UpdateNodeTree(nodes);
  auto* node1 = bridge_.tree_.FindNodeById(1);
  ASSERT_NE(node1, nullptr);
  ASSERT_TRUE(node1->hasUpdate);

  bridge_.OnTap(1);
  EXPECT_FALSE(node1->hasUpdate);

  node1->hasUpdate = true;
  bridge_.OnLongPress(1);
  EXPECT_FALSE(node1->hasUpdate);

  EXPECT_NO_FATAL_FAILURE(bridge_.OnTap(999));
}

TEST_F(SemanticsBridgeTest, OnTooltipAndStateChange) {
  SemanticsNodeUpdates nodes;
  SemanticsNode child;
  child.id = 1;
  child.label = "node1";
  nodes[1] = child;
  BuildRootWithChildren(nodes, {1});
  bridge_.UpdateNodeTree(nodes);
  auto* root = bridge_.tree_.GetRootNode();
  ASSERT_NE(root, nullptr);
  ASSERT_TRUE(root->hasUpdate);

  auto msg = std::make_unique<char[]>(8);
  std::snprintf(msg.get(), 8, "tip");
  bridge_.OnTooltip(msg);
  EXPECT_FALSE(root->hasUpdate);

  bridge_.OnAccessibilityStateChange(true);
  EXPECT_TRUE(bridge_.is_accessibility_enabled_);
  bridge_.OnAccessibilityNavigation(true);
  EXPECT_TRUE(bridge_.has_navigationed_);

  EXPECT_EQ(bridge_.GetNodeById(1), bridge_.tree_.FindNodeById(1));
  EXPECT_EQ(bridge_.GetNodeById(999), nullptr);
}

}
}
