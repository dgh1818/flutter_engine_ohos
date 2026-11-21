/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_inspector.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_bridge.h"
#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_tree.h"
#include "rapidjson/document.h"

namespace flutter {

int32_t RegisterInspectorTreeGetterWithApi(
    ArkUI_NodeHandle node,
    std::shared_ptr<InspectorSnapshot> snapshot,
    int32_t (*register_getter)(ArkUI_NodeHandle, void*, InspectorTreeGetterFn));
size_t InspectorRetainedContextCountForTesting();

namespace {

class MockInspectorTreeSource : public InspectorTreeSource {
 public:
  struct Entry {
    std::shared_ptr<SemanticsNodeExtend> node;
    std::vector<int32_t> children;
  };

  std::map<int32_t, Entry> nodes;
  int32_t root_id = -1;

  SemanticsNodeExtend* AddNode(int32_t id) {
    Entry entry;
    entry.node = std::make_shared<SemanticsNodeExtend>();
    entry.node->id = id;
    nodes[id] = entry;
    return nodes[id].node.get();
  }

  void SetChildren(int32_t id, std::vector<int32_t> children) {
    nodes[id].children = std::move(children);
  }

  int32_t RootId() const override { return root_id; }

  bool NodeAt(int32_t id, SemanticsNodeExtend** node) const override {
    const auto it = nodes.find(id);
    if (it == nodes.end()) {
      return false;
    }
    *node = it->second.node.get();
    return true;
  }

  std::vector<int32_t> ChildIdsInTraversalOrder(
      SemanticsNodeExtend& node) const override {
    const auto it = nodes.find(node.id);
    if (it == nodes.end()) {
      return {};
    }
    return it->second.children;
  }
};

TEST(OhosSemanticsInspectorTest, SingleNodeAllFields) {
  MockInspectorTreeSource source;
  SemanticsNodeExtend* node = source.AddNode(15);
  node->componentType = OHWidgetName::kTextWidgetName;
  node->setAbsoluteRect(0.0f, 72.0f, 720.0f, 1208.0f);
  node->fontSize = 24.0;
  node->backgroundColor = 0xFF112233u;
  node->label = "Hello";
  node->actions = static_cast<int32_t>(SemanticsAction::kTap);
  source.root_id = 15;

  EXPECT_EQ(
      BuildInspectorTreeJson(source),
      "[{\"$type\": \"Text\",\"$ID\": 15,\"type\": \"build-in\","
      "\"$rect\": \"[0.00, 72.00],[720.00,1208.00]\",\"$debugLine\": \"\","
      "\"$attrs\": {\"text\": \"Hello\",\"clickable\": true,"
      "\"scrollable\": false,\"fontSize\": 24.0,"
      "\"backgroundColor\": \"#FF112233\"},\"$children\": []}]");
}

TEST(OhosSemanticsInspectorTest, LargeFiniteRectRemainsValidJson) {
  MockInspectorTreeSource source;
  SemanticsNodeExtend* node = source.AddNode(1);
  const float max = std::numeric_limits<float>::max();
  node->setAbsoluteRect(-max, -max, max, max);
  source.root_id = 1;

  const std::string json = BuildInspectorTreeJson(source);
  rapidjson::Document document;
  document.Parse(json.c_str());
  ASSERT_FALSE(document.HasParseError());
  ASSERT_TRUE(document.IsArray());
  ASSERT_EQ(document.Size(), 1u);
  ASSERT_TRUE(document[0].IsObject());
  ASSERT_TRUE(document[0].HasMember("$rect"));
  ASSERT_TRUE(document[0]["$rect"].IsString());
  const std::string rect = document[0]["$rect"].GetString();
  EXPECT_GT(rect.size(), 96u);
  EXPECT_EQ(rect.front(), '[');
  EXPECT_EQ(rect.back(), ']');
}

TEST(OhosSemanticsInspectorTest, LargeFiniteFontSizeIsNotTruncated) {
  MockInspectorTreeSource source;
  SemanticsNodeExtend* node = source.AddNode(1);
  node->fontSize = 1e80;
  source.root_id = 1;

  const std::string json = BuildInspectorTreeJson(source);
  rapidjson::Document document;
  document.Parse(json.c_str());
  ASSERT_FALSE(document.HasParseError());
  ASSERT_TRUE(document.IsArray());
  ASSERT_EQ(document.Size(), 1u);
  ASSERT_TRUE(document[0].IsObject());
  ASSERT_TRUE(document[0].HasMember("$attrs"));
  ASSERT_TRUE(document[0]["$attrs"].IsObject());
  ASSERT_TRUE(document[0]["$attrs"].HasMember("fontSize"));
  ASSERT_TRUE(document[0]["$attrs"]["fontSize"].IsNumber());
  EXPECT_NEAR(document[0]["$attrs"]["fontSize"].GetDouble(), 1e80, 1e65);
}

TEST(OhosSemanticsInspectorTest, FormatsSignedNumbersAndColor) {
  MockInspectorTreeSource source;
  SemanticsNodeExtend* node = source.AddNode(1);
  node->setAbsoluteRect(-0.0f, -1.5f, 2.25f, 4.0f);
  node->fontSize = 12.75;
  node->backgroundColor = 1;
  source.root_id = 1;

  const std::string json = BuildInspectorTreeJson(source);
  EXPECT_NE(json.find("\"$rect\": \"[-0.00, -1.50],[2.25,4.00]\""),
            std::string::npos);
  EXPECT_NE(json.find("\"fontSize\": 12.8"), std::string::npos);
  EXPECT_NE(json.find("\"backgroundColor\": \"#00000001\""), std::string::npos);
}

TEST(OhosSemanticsInspectorTest, EscapesControlCharacters) {
  MockInspectorTreeSource source;
  SemanticsNodeExtend* node = source.AddNode(1);
  node->label = std::string("\0\1\x1f", 3);
  source.root_id = 1;

  const std::string json = BuildInspectorTreeJson(source);
  EXPECT_NE(json.find("\\u0000\\u0001\\u001f"), std::string::npos);
  rapidjson::Document document;
  document.Parse(json.c_str());
  ASSERT_FALSE(document.HasParseError());
  ASSERT_TRUE(document.IsArray());
  ASSERT_EQ(document.Size(), 1u);
  ASSERT_TRUE(document[0].IsObject());
  ASSERT_TRUE(document[0].HasMember("$attrs"));
  ASSERT_TRUE(document[0]["$attrs"].IsObject());
  ASSERT_TRUE(document[0]["$attrs"].HasMember("text"));
  const auto& text = document[0]["$attrs"]["text"];
  ASSERT_TRUE(text.IsString());
  EXPECT_EQ(std::string(text.GetString(), text.GetStringLength()), node->label);
}

TEST(OhosSemanticsInspectorTest, PayloadIsJsonArrayOfNodes) {
  // ArkUI contract: single-object payloads are rejected and skipped whole;
  // an empty tree must serialize as an empty array.
  MockInspectorTreeSource empty;
  EXPECT_EQ(BuildInspectorTreeJson(empty), "[]");

  MockInspectorTreeSource source;
  source.AddNode(3);
  source.root_id = 3;
  const std::string json = BuildInspectorTreeJson(source);
  EXPECT_EQ(json.front(), '[');
  EXPECT_EQ(json.back(), ']');
}

TEST(OhosSemanticsInspectorTest, RecursiveTreeInTraversalOrder) {
  MockInspectorTreeSource source;
  source.AddNode(1)->componentType = OHWidgetName::kOtherWidgetName;
  source.AddNode(2)->componentType = OHWidgetName::kTextWidgetName;
  source.AddNode(3)->componentType = OHWidgetName::kButtonWidgetName;
  source.SetChildren(1, {2, 3});
  source.root_id = 1;

  const std::string json = BuildInspectorTreeJson(source);
  EXPECT_NE(json.find("\"$children\": [{\"$type\": \"Text\""),
            std::string::npos);
  EXPECT_NE(json.find("{\"$type\": \"Button\""), std::string::npos);
  // Traversal order: Text ($ID 2) must come before Button ($ID 3).
  EXPECT_LT(json.find("\"$ID\": 2"), json.find("\"$ID\": 3"));
}

TEST(OhosSemanticsInspectorTest, EscapesUserControlledStrings) {
  MockInspectorTreeSource source;
  SemanticsNodeExtend* node = source.AddNode(7);
  node->label = "a\"b\\c\nd";
  source.root_id = 7;

  const std::string json = BuildInspectorTreeJson(source);
  EXPECT_NE(json.find("a\\\"b\\\\c\\nd"), std::string::npos);
}

TEST(OhosSemanticsInspectorTest, OmitsSentinelAttributes) {
  MockInspectorTreeSource source;
  source.AddNode(9);
  source.root_id = 9;

  const std::string json = BuildInspectorTreeJson(source);
  // fontSize / backgroundColor stay unreported when sentinels are set;
  // scrollable / clickable are always present.
  EXPECT_EQ(json.find("fontSize"), std::string::npos);
  EXPECT_EQ(json.find("backgroundColor"), std::string::npos);
  EXPECT_NE(json.find("\"scrollable\": false"), std::string::npos);
  EXPECT_NE(json.find("\"clickable\": false"), std::string::npos);
}

TEST(OhosSemanticsInspectorTest, ReportsWhiteBackground) {
  MockInspectorTreeSource source;
  source.AddNode(9)->backgroundColor = 0xFFFFFFFFu;
  source.root_id = 9;

  EXPECT_NE(
      BuildInspectorTreeJson(source).find("\"backgroundColor\": \"#FFFFFFFF\""),
      std::string::npos);
}

TEST(OhosSemanticsInspectorTest, DropsOversizedRoot) {
  MockInspectorTreeSource source;
  source.AddNode(1)->label = std::string(4194304, 'x');
  source.root_id = 1;

  EXPECT_EQ(BuildInspectorTreeJson(source), "[]");
}

TEST(OhosSemanticsInspectorTest, DropsOversizedChildWithoutTrailingComma) {
  MockInspectorTreeSource source;
  source.AddNode(1);
  source.AddNode(2);
  source.AddNode(3)->label = std::string(4194304, '\n');
  source.SetChildren(1, {2, 3});
  source.root_id = 1;

  const std::string json = BuildInspectorTreeJson(source);
  EXPECT_LT(json.size(), 4194304u);
  EXPECT_EQ(json.find(",]"), std::string::npos);
  EXPECT_NE(json.find("\"truncated\":true"), std::string::npos);
  rapidjson::Document document;
  document.Parse(json.c_str());
  EXPECT_FALSE(document.HasParseError());
  EXPECT_TRUE(document.IsArray());
}

TEST(OhosSemanticsInspectorTest, DerivesScrollableFromScrollActions) {
  MockInspectorTreeSource source;
  SemanticsNodeExtend* node = source.AddNode(11);
  node->actions = static_cast<int32_t>(SemanticsAction::kScrollUp);
  source.root_id = 11;

  EXPECT_NE(BuildInspectorTreeJson(source).find("\"scrollable\": true"),
            std::string::npos);
}

TEST(OhosSemanticsInspectorTest, TruncatesCyclicTree) {
  MockInspectorTreeSource source;
  source.AddNode(1);
  source.AddNode(2);
  source.SetChildren(1, {2});
  source.SetChildren(2, {1});  // Cycle: 1 -> 2 -> 1 -> ...
  source.root_id = 1;

  const std::string json = BuildInspectorTreeJson(source);
  EXPECT_NE(json.find("\"truncated\":true"), std::string::npos);
}

TEST(OhosSemanticsInspectorTest, SemanticsTreeSourceAdaptsTree) {
  SemanticsTree tree;
  // Build the tree through the production path (UpdateWithNodes): only nodes
  // delivered by a semantics update carry their id; GetOrAddNode placeholders
  // keep id 0 by design (see SemanticsTreeTest.GetOrAddNodeCreatesNewNode).
  std::unordered_map<int32_t, SemanticsNode> nodes;
  SemanticsNode root;
  root.id = 0;
  root.childrenInTraversalOrder = {5};
  nodes[0] = root;
  SemanticsNode child;
  child.id = 5;
  nodes[5] = child;
  tree.UpdateWithNodes(nodes);

  SemanticsTreeInspectorSource source(&tree);
  EXPECT_EQ(source.RootId(), 0);
  SemanticsNodeExtend* found = nullptr;
  EXPECT_TRUE(source.NodeAt(5, &found));
  EXPECT_EQ(found, tree.GetOrAddNode(5));
  EXPECT_FALSE(source.NodeAt(42, &found));
  SemanticsNodeExtend* root_ext = tree.GetOrAddNode(0);
  EXPECT_EQ(source.ChildIdsInTraversalOrder(*root_ext),
            (std::vector<int32_t>{5}));
}

TEST(OhosSemanticsInspectorTest, SemanticsUpdatePreservesSidecarAttributes) {
  SemanticsTree tree;
  SemanticsNode root;
  root.id = 0;
  root.label = "Inspector attribute test";
  root.actions = static_cast<int32_t>(SemanticsAction::kScrollUp);
  std::unordered_map<int32_t, SemanticsNode> updates;
  updates[0] = root;
  tree.UpdateWithNodes(updates);

  SemanticsNodeExtend* root_ext = tree.FindNodeById(0);
  ASSERT_NE(root_ext, nullptr);
  root_ext->fontSize = 24.0;
  root_ext->backgroundColor = 0xFF28A87A;

  root.actions = 0;
  updates[0] = root;
  tree.UpdateWithNodes(updates);
  SemanticsTreeInspectorSource source(&tree);
  const std::string json = BuildInspectorTreeJson(source);
  EXPECT_NE(json.find("\"scrollable\": false"), std::string::npos);
  EXPECT_NE(json.find("\"fontSize\": 24.0"), std::string::npos);
  EXPECT_NE(json.find("\"backgroundColor\": \"#FF28A87A\""), std::string::npos);
}

TEST(OhosSemanticsInspectorTest, TrampolineProducesMallocatedJson) {
  char* out = nullptr;
  InspectorTreeGetterTrampoline(nullptr, &out);
  ASSERT_NE(out, nullptr);
  EXPECT_EQ(std::string(out), "[]");
  free(out);
}

TEST(OhosSemanticsInspectorTest, SnapshotPublishesImmutableValues) {
  InspectorSnapshot snapshot;
  auto first = snapshot.Read();
  EXPECT_EQ(*first, "[]");
  snapshot.Publish("[{\"fontSize\":24.0}]");
  EXPECT_EQ(*first, "[]");
  EXPECT_EQ(*snapshot.Read(), "[{\"fontSize\":24.0}]");
}

TEST(OhosSemanticsInspectorTest, GetterReadsAndDeactivatesSnapshot) {
  auto snapshot = std::make_shared<InspectorSnapshot>();
  snapshot->Publish("[{\"text\":\"hello\"}]");
  InspectorCallbackContext context(snapshot);
  char* out = nullptr;
  InspectorTreeGetterTrampoline(&context, &out);
  ASSERT_NE(out, nullptr);
  EXPECT_EQ(std::string(out), "[{\"text\":\"hello\"}]");
  free(out);
  context.Deactivate();
  InspectorTreeGetterTrampoline(&context, &out);
  ASSERT_NE(out, nullptr);
  EXPECT_EQ(std::string(out), "[]");
  free(out);
}

TEST(OhosSemanticsInspectorTest, GetterCanReadWhileSnapshotIsPublished) {
  auto snapshot = std::make_shared<InspectorSnapshot>();
  InspectorCallbackContext context(snapshot);
  std::atomic<bool> invalid{false};
  std::thread reader([&context, &invalid] {
    for (int i = 0; i < 2000; ++i) {
      char* out = nullptr;
      InspectorTreeGetterTrampoline(&context, &out);
      if (!out || (std::string(out) != "[]" && std::string(out) != "[1]" &&
                   std::string(out) != "[2]")) {
        invalid = true;
      }
      free(out);
    }
  });
  for (int i = 0; i < 2000; ++i) {
    snapshot->Publish(i % 2 ? "[1]" : "[2]");
  }
  context.Deactivate();
  reader.join();
  EXPECT_FALSE(invalid.load());
}

TEST(OhosSemanticsInspectorTest, IndependentContextsDoNotShareSnapshots) {
  auto first = std::make_shared<InspectorSnapshot>();
  auto second = std::make_shared<InspectorSnapshot>();
  first->Publish("[1]");
  second->Publish("[2]");
  InspectorCallbackContext first_context(first);
  InspectorCallbackContext second_context(second);

  first_context.Deactivate();
  first.reset();
  second->Publish("[3]");

  char* output = nullptr;
  InspectorTreeGetterTrampoline(&first_context, &output);
  ASSERT_NE(output, nullptr);
  EXPECT_EQ(std::string(output), "[]");
  free(output);
  InspectorTreeGetterTrampoline(&second_context, &output);
  ASSERT_NE(output, nullptr);
  EXPECT_EQ(std::string(output), "[3]");
  free(output);
}

TEST(OhosSemanticsInspectorTest, InFlightSnapshotSurvivesDeactivation) {
  auto snapshot = std::make_shared<InspectorSnapshot>();
  snapshot->Publish("[42]");
  std::weak_ptr<InspectorSnapshot> weak_snapshot = snapshot;
  InspectorCallbackContext context(snapshot);
  auto in_flight = context.Read();

  snapshot.reset();
  context.Deactivate();
  EXPECT_EQ(context.Read(), nullptr);
  EXPECT_FALSE(weak_snapshot.expired());
  ASSERT_NE(in_flight, nullptr);
  EXPECT_EQ(*in_flight->Read(), "[42]");

  in_flight.reset();
  EXPECT_TRUE(weak_snapshot.expired());
}

TEST(OhosSemanticsInspectorTest, SharedSnapshotOutlivesOneContext) {
  auto snapshot = std::make_shared<InspectorSnapshot>();
  snapshot->Publish("[1]");
  InspectorCallbackContext first(snapshot);
  InspectorCallbackContext second(snapshot);
  first.Deactivate();
  snapshot->Publish("[2]");
  snapshot.reset();

  char* output = nullptr;
  InspectorTreeGetterTrampoline(&second, &output);
  ASSERT_NE(output, nullptr);
  EXPECT_EQ(std::string(output), "[2]");
  free(output);
}

constexpr int32_t kInspectorRegistrationFailure = -42;

int32_t FailInspectorRegistration(ArkUI_NodeHandle,
                                  void*,
                                  InspectorTreeGetterFn) {
  return kInspectorRegistrationFailure;
}

TEST(OhosSemanticsInspectorTest, FailedRegistrationDoesNotRetainContext) {
  const size_t initial_count = InspectorRetainedContextCountForTesting();
  auto snapshot = std::make_shared<InspectorSnapshot>();
  // An opaque identity token; the registration stub never dereferences it.
  int node_token = 0;
  auto node = reinterpret_cast<ArkUI_NodeHandle>(&node_token);
  for (int i = 0; i < 1025; ++i) {
    EXPECT_EQ(RegisterInspectorTreeGetterWithApi(node, snapshot,
                                                 FailInspectorRegistration),
              kInspectorRegistrationFailure);
  }
  EXPECT_EQ(InspectorRetainedContextCountForTesting(), initial_count);
}

TEST(OhosSemanticsInspectorTest, RegisterRejectsNullBridge) {
  EXPECT_EQ(RegisterInspectorTreeGetter(nullptr, nullptr), -1);
}

}  // namespace
}  // namespace flutter
