/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_ACCESSIBILITY_BRIDGE_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_ACCESSIBILITY_BRIDGE_H_

#include <arkui/native_interface_accessibility.h>
#include <cstdint>
#include <string>
#include <map>
#include <utility>
#include <vector>
#include "flutter/fml/mapping.h"
#include "flutter/lib/ui/semantics/custom_accessibility_action.h"
#include "flutter/lib/ui/semantics/semantics_node.h"
#include "native_accessibility_channel.h"
#include "ohos_accessibility_features.h"

namespace flutter {

typedef flutter::SemanticsFlags FLAGS_;
typedef flutter::SemanticsAction ACTIONS_;

struct AbsoluteRect {
    float left;
    float top;
    float right;
    float bottom;
    static constexpr AbsoluteRect MakeEmpty() {
        return AbsoluteRect{0.0, 0.0, 0.0, 0.0};
    }
};
struct SemanticsNodeExtent : flutter::SemanticsNode {
    AbsoluteRect absoluteRect = AbsoluteRect::MakeEmpty();
    int32_t parentId = -1;
    bool hadPreviousConfig = false;
    int32_t previousNodeId = -1;
    int32_t previousFlags = 0;
    int32_t previousActions = 0;
    int32_t previousTextSelectionBase = -1;
    int32_t previousTextSelectionExtent = -1;
    double previousScrollPosition = std::nan("");
    double previousScrollExtentMax = std::nan("");
    double previousScrollExtentMin = std::nan("");
    std::string previousValue;
    std::string previousLabel;
    bool HasPrevAction(SemanticsAction action) const {
        return (previousActions & this->actions) != 0;
    }
    bool HasPrevFlag(SemanticsFlags flag) const {
        return (previousFlags & this->flags) != 0;
    }
};

/**
 * 桥接flutter无障碍功能和ohos无障碍系统服务
 */
class OhosAccessibilityBridge {
public:
    static OhosAccessibilityBridge* GetInstance();
    static void DestroyInstance();
    OhosAccessibilityBridge(const OhosAccessibilityBridge&) = delete;
    OhosAccessibilityBridge& operator=(const OhosAccessibilityBridge&) = delete;

    bool IS_FLUTTER_NAVIGATE = false;
    int64_t native_shell_holder_id_;
    ArkUI_AccessibilityProvider* provider_;

    void OnOhosAccessibilityStateChange(
        int64_t shellHolderId,
        bool ohosAccessibilityEnabled);

    void SetNativeShellHolderId(int64_t id);

    void updateSemantics(flutter::SemanticsNodeUpdates update,
                         flutter::CustomAccessibilityActionUpdates actions);

    void DispatchSemanticsAction(int32_t id,
                                 flutter::SemanticsAction action,
                                 fml::MallocMapping args);

    void Announce(std::unique_ptr<char[]>& message);

    SemanticsNodeExtent GetFlutterSemanticsNode(int32_t id);

    int32_t FindAccessibilityNodeInfosById(
        int64_t elementId,
        ArkUI_AccessibilitySearchMode mode,
        int32_t requestId,
        ArkUI_AccessibilityElementInfoList* elementList);
    int32_t FindAccessibilityNodeInfosByText(
        int64_t elementId,
        const char* text,
        int32_t requestId,
        ArkUI_AccessibilityElementInfoList* elementList);
    int32_t FindFocusedAccessibilityNode(
        int64_t elementId,
        ArkUI_AccessibilityFocusType focusType,
        int32_t requestId,
        ArkUI_AccessibilityElementInfo* elementinfo);
    int32_t FindNextFocusAccessibilityNode(
        int64_t elementId,
        ArkUI_AccessibilityFocusMoveDirection direction,
        int32_t requestId,
        ArkUI_AccessibilityElementInfo* elementList);
    int32_t ExecuteAccessibilityAction(
        int64_t elementId,
        ArkUI_Accessibility_ActionType action,
        ArkUI_AccessibilityActionArguments* actionArguments,
        int32_t requestId);
    int32_t ClearFocusedFocusAccessibilityNode();
    int32_t GetAccessibilityNodeCursorPosition(int64_t elementId,
                                               int32_t requestId,
                                               int32_t* index);

    void Flutter_SendAccessibilityAsyncEvent(
        int64_t elementId,
        ArkUI_AccessibilityEventType eventType);
    void FlutterNodeToElementInfoById(
        ArkUI_AccessibilityElementInfo* elementInfoFromList,
        int64_t elementId);
    int32_t GetParentId(int64_t elementId);

    void ConvertChildRelativeRectToScreenRect(SemanticsNodeExtent node);
    std::pair<std::pair<float, float>, std::pair<float, float>>
    GetAbsoluteScreenRect(int32_t flutterNodeId);
    void SetAbsoluteScreenRect(int32_t flutterNodeId,
                               float left,
                               float top,
                               float right,
                               float bottom);

    SemanticsNodeExtent UpdatetSemanticsNodeExtent(flutter::SemanticsNode node);

    void FlutterScrollExecution(
        SemanticsNodeExtent node,
        ArkUI_AccessibilityElementInfo* elementInfoFromList);
    
    void ClearFlutterSemanticsCaches();

private:
    OhosAccessibilityBridge();
    static OhosAccessibilityBridge* bridgeInstance;
    std::shared_ptr<NativeAccessibilityChannel> nativeAccessibilityChannel_;
    std::shared_ptr<OhosAccessibilityFeatures> accessibilityFeatures_;

    std::vector<std::pair<int32_t, int32_t>> g_parentChildIdVec;
    std::map<int32_t, SemanticsNodeExtent> g_flutterSemanticsTree;
    std::unordered_map<int32_t,
                       std::pair<std::pair<float, float>, std::pair<float, float>>> g_screenRectMap;
    std::unordered_map<int32_t, flutter::CustomAccessibilityAction> g_actions_mp;
    std::vector<int32_t> g_flutterNavigationVec;

    SemanticsNodeExtent inputFocusedNode;
    SemanticsNodeExtent lastInputFocusedNode;
    SemanticsNodeExtent accessibilityFocusedNode;

    // arkui的root节点的父节点id
    static const int32_t ARKUI_ACCESSIBILITY_ROOT_PARENT_ID = -2100000;
    static const int32_t RET_ERROR_STATE_CODE = -999;
    static const int32_t ROOT_NODE_ID = 0;
    constexpr static const double SCROLL_EXTENT_FOR_INFINITY = 100000.0;
    constexpr static const double SCROLL_POSITION_CAP_FOR_INFINITY = 70000.0;
    
    const std::string OTHER_WIDGET_NAME = "View";
    const std::string TEXT_WIDGET_NAME = "Text";
    const std::string EDIT_TEXT_WIDGET_NAME = "TextInput";
    const std::string EDIT_MULTILINE_TEXT_WIDGET_NAME = "TextArea";
    const std::string IMAGE_WIDGET_NAME = "Image";
    const std::string SCROLL_WIDGET_NAME = "Scroll";
    const std::string BUTTON_WIDGET_NAME = "Button";
    const std::string LINK_WIDGET_NAME = "Link";
    const std::string SLIDER_WIDGET_NAME = "Slider";
    const std::string HEADER_WIDGET_NAME = "Header";
    const std::string RADIO_BUTTON_WIDGET_NAME = "Radio";
    const std::string CHECK_BOX_WIDGET_NAME = "Checkbox";
    const std::string SWITCH_WIDGET_NAME = "Toggle";
    const std::string SEEKBAR_WIDGET_NAME = "SeekBar";

    const std::map<std::string, ArkUI_Accessibility_ActionType>
        ArkUI_ACTION_TYPE_MAP_ = {
            {"invalid", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_INVALID},
            {"click", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK},
            {"long press", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_LONG_CLICK},
            {"focus acquisition", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_GAIN_ACCESSIBILITY_FOCUS},
            {"focus clearance", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLEAR_ACCESSIBILITY_FOCUS},
            {"forward scroll", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD},
            {"backward scroll", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD},
            {"copy text", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_COPY},
            {"paste text", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_PASTE},
            {"cut text", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CUT},
            {"text selection", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT},
            {"set text", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_TEXT},
            {"text cursor position setting", ArkUI_Accessibility_ActionType::ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION},
    };

    static const int32_t FOCUSABLE_FLAGS =
        static_cast<int32_t>(FLAGS_::kHasCheckedState) |
        static_cast<int32_t>(FLAGS_::kIsChecked) |
        static_cast<int32_t>(FLAGS_::kIsSelected) |
        static_cast<int32_t>(FLAGS_::kIsTextField) |
        static_cast<int32_t>(FLAGS_::kIsFocused) |
        static_cast<int32_t>(FLAGS_::kHasEnabledState) |
        static_cast<int32_t>(FLAGS_::kIsEnabled) |
        static_cast<int32_t>(FLAGS_::kIsInMutuallyExclusiveGroup) |
        static_cast<int32_t>(FLAGS_::kHasToggledState) |
        static_cast<int32_t>(FLAGS_::kIsToggled) |
        static_cast<int32_t>(FLAGS_::kHasToggledState) |
        static_cast<int32_t>(FLAGS_::kIsFocusable) |
        static_cast<int32_t>(FLAGS_::kIsSlider);

    static const int32_t SCROLLABLE_ACTIONS =
        static_cast<int32_t>(ACTIONS_::kScrollUp) |
        static_cast<int32_t>(ACTIONS_::kScrollDown) |
        static_cast<int32_t>(ACTIONS_::kScrollLeft) |
        static_cast<int32_t>(ACTIONS_::kScrollRight);

    void FlutterSetElementInfoProperties(
        ArkUI_AccessibilityElementInfo* elementInfoFromList,
        int64_t elementId);
    void FlutterSetElementInfoOperationActions(
        ArkUI_AccessibilityElementInfo* elementInfoFromList,
        std::string widget_type);
    void FlutterTreeToArkuiTree(
        ArkUI_AccessibilityElementInfoList* elementInfoList);
    void BuildArkUISemanticsTree(
        int64_t elementId,
        ArkUI_AccessibilityElementInfo* elementInfoFromList,
        ArkUI_AccessibilityElementInfoList* elementList);

    std::vector<int64_t> GetLevelOrderTraversalTree(int32_t rootId);
    SemanticsNodeExtent GetFlutterRootSemanticsNode();
    std::string GetNodeComponentType(const SemanticsNodeExtent& node);
    flutter::SemanticsAction ArkuiActionsToFlutterActions(
        ArkUI_Accessibility_ActionType arkui_action);

    bool HasScrolled(const SemanticsNodeExtent& flutterNode);
    bool HasChangedLabel(const SemanticsNodeExtent& flutterNode);

    bool IsNodeFocusable(const SemanticsNodeExtent& flutterNode);
    bool IsNodeFocused(const SemanticsNodeExtent& flutterNode);
    bool IsNodeCheckable(SemanticsNodeExtent flutterNode);
    bool IsNodeChecked(SemanticsNodeExtent flutterNode);
    bool IsNodeSelected(SemanticsNodeExtent flutterNode);
    bool IsNodeClickable(SemanticsNodeExtent flutterNode);
    bool IsNodeScrollable(SemanticsNodeExtent flutterNode);
    bool IsNodePassword(SemanticsNodeExtent flutterNode);
    bool IsNodeVisible(SemanticsNodeExtent flutterNode);
    bool IsNodeEnabled(SemanticsNodeExtent flutterNode);
    bool IsNodeHasLongPress(SemanticsNodeExtent flutterNode);
    bool IsNodeShowOnScreen(SemanticsNodeExtent flutterNode);

    bool IsTextField(SemanticsNodeExtent flutterNode);
    bool IsSlider(SemanticsNodeExtent flutterNode);
    bool IsScrollableWidget(SemanticsNodeExtent flutterNode);
    void PerformSetText(SemanticsNodeExtent flutterNode,
                        ArkUI_Accessibility_ActionType action,
                        ArkUI_AccessibilityActionArguments* actionArguments);
    void PerformSelectText(SemanticsNodeExtent flutterNode,
                            ArkUI_Accessibility_ActionType action,
                            ArkUI_AccessibilityActionArguments* actionArguments);

    void AddRouteNodes(std::vector<SemanticsNodeExtent> edges,
                        SemanticsNodeExtent node);
    std::string GetRouteName(SemanticsNodeExtent node);
    void onWindowNameChange(SemanticsNodeExtent route);
    void removeSemanticsNode(SemanticsNodeExtent nodeToBeRemoved);

    void GetSemanticsNodeDebugInfo(SemanticsNodeExtent node);
    void GetSemanticsFlagsDebugInfo(SemanticsNodeExtent node);
    void GetCustomActionDebugInfo(
        flutter::CustomAccessibilityAction customAccessibilityAction);

    void RequestFocusWhenPageUpdate(int32_t requestFocusId);
    bool Contains(const std::string source, const std::string target);
    std::pair<float, float> GetRealScaleFactor();
    void FlutterSemanticsTreeUpdateCallOnce();
};

enum class AccessibilityAction : int32_t {
    kTap = 1 << 0,
    kLongPress = 1 << 1,
    kScrollLeft = 1 << 2,
    kScrollRight = 1 << 3,
    kScrollUp = 1 << 4,
    kScrollDown = 1 << 5,
    kIncrease = 1 << 6,
    kDecrease = 1 << 7,
    kShowOnScreen = 1 << 8,
    kMoveCursorForwardByCharacter = 1 << 9,
    kMoveCursorBackwardByCharacter = 1 << 10,
    kSetSelection = 1 << 11,
    kCopy = 1 << 12,
    kCut = 1 << 13,
    kPaste = 1 << 14,
    kDidGainAccessibilityFocus = 1 << 15,
    kDidLoseAccessibilityFocus = 1 << 16,
    kCustomAction = 1 << 17,
    kDismiss = 1 << 18,
    kMoveCursorForwardByWord = 1 << 19,
    kMoveCursorBackwardByWord = 1 << 20,
    kSetText = 1 << 21,
};

enum class AccessibilityFlags : int32_t {
    kHasCheckedState = 1 << 0,
    kIsChecked = 1 << 1,
    kIsSelected = 1 << 2,
    kIsButton = 1 << 3,
    kIsTextField = 1 << 4,
    kIsFocused = 1 << 5,
    kHasEnabledState = 1 << 6,
    kIsEnabled = 1 << 7,
    kIsInMutuallyExclusiveGroup = 1 << 8,
    kIsHeader = 1 << 9,
    kIsObscured = 1 << 10,
    kScopesRoute = 1 << 11,
    kNamesRoute = 1 << 12,
    kIsHidden = 1 << 13,
    kIsImage = 1 << 14,
    kIsLiveRegion = 1 << 15,
    kHasToggledState = 1 << 16,
    kIsToggled = 1 << 17,
    kHasImplicitScrolling = 1 << 18,
    kIsMultiline = 1 << 19,
    kIsReadOnly = 1 << 20,
    kIsFocusable = 1 << 21,
    kIsLink = 1 << 22,
    kIsSlider = 1 << 23,
    kIsKeyboardKey = 1 << 24,
    kIsCheckStateMixed = 1 << 25,
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_ACCESSIBILITY_BRIDGE_H_
