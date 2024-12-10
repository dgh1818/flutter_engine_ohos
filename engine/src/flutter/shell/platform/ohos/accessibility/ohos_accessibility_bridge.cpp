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
#include "ohos_accessibility_bridge.h"
#include <limits>
#include <string>
#include <memory>
#include <mutex>
#include "flutter/fml/logging.h"
#include "flutter/shell/platform/ohos/ohos_logging.h"
#include "flutter/shell/common/platform_view.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkScalar.h"

#define ARKUI_SUCCEED_CODE 0
#define ARKUI_FAILED_CODE -1
#define ARKUI_BAD_PARAM_CODE -2
#define ARKUI_OOM_CODE  -3
#define FUNCTION_NAME_STR(n) #n
#define ARKUI_ACCESSIBILITY_CALL_CHECK(X)                                \
    do {                                                                 \
        int32_t RET = X;                                                 \
        if (RET != ARKUI_SUCCEED_CODE) {                                 \
          LOGE("Failed function %{public}s call, error code:%{public}d", \
              FUNCTION_NAME_STR(X), RET);                                \
        }                                                                \
    }  while (false)                                                     \

namespace flutter {
OhosAccessibilityBridge* OhosAccessibilityBridge::bridgeInstance = nullptr;

OhosAccessibilityBridge* OhosAccessibilityBridge::GetInstance() 
{
  if(!bridgeInstance) {
    bridgeInstance = new OhosAccessibilityBridge();
  }
  return bridgeInstance;
}

void OhosAccessibilityBridge::DestroyInstance() {
  delete bridgeInstance;
  bridgeInstance = nullptr;
}

OhosAccessibilityBridge::OhosAccessibilityBridge() {}
/**
 * 监听当前ohos平台是否开启无障碍屏幕朗读服务
 */
void OhosAccessibilityBridge::OnOhosAccessibilityStateChange(
    int64_t shellHolderId,
    bool ohosAccessibilityEnabled)
{
  native_shell_holder_id_ = shellHolderId;
  nativeAccessibilityChannel_ = std::make_shared<NativeAccessibilityChannel>();
  accessibilityFeatures_ = std::make_shared<OhosAccessibilityFeatures>();

  if (ohosAccessibilityEnabled) {
    nativeAccessibilityChannel_->OnOhosAccessibilityEnabled(native_shell_holder_id_);
  } else {
    accessibilityFeatures_->SetAccessibleNavigation(false, native_shell_holder_id_);
    nativeAccessibilityChannel_->OnOhosAccessibilityDisabled(native_shell_holder_id_);
  }
}

void OhosAccessibilityBridge::SetNativeShellHolderId(int64_t id)
{
  this->native_shell_holder_id_ = id;
}

void OhosAccessibilityBridge::FlutterSemanticsTreeUpdateCallOnce() {
    static std::once_flag flag; 
    std::call_once(flag, [this]() {
        this->RequestFocusWhenPageUpdate(0);
        LOGD("call once -> RequestFocusWhenPageUpdate(0)");
    }); 
}
/**
 * 从dart侧传递到c++侧的flutter无障碍语义树节点更新过程，
 * 路由新页面、滑动页面等操作会自动触发该语义树的更新
 */
void OhosAccessibilityBridge::updateSemantics(
    flutter::SemanticsNodeUpdates update,
    flutter::CustomAccessibilityActionUpdates actions)
{
  FML_DLOG(INFO) << ("OhosAccessibilityBridge::UpdateSemantics()");

  std::vector<SemanticsNodeExtent> updatedFlutterNodes;

  // 当flutter页面路由更新时，自动请求id=0节点组件获焦（规避滑动组件更新干扰）
  if (IS_FLUTTER_NAVIGATE) {
    RequestFocusWhenPageUpdate(0);
    IS_FLUTTER_NAVIGATE = false;
  }

  // 页面状态更新事件
  Flutter_SendAccessibilityAsyncEvent(0, ArkUI_AccessibilityEventType::ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_PAGE_CONTENT_UPDATE);
  LOGE("Flutter_SendAccessibilityAsyncEvent -> PAGE_CONTENT_UPDATE");

  /** 获取并分析每个语义节点的更新属性 */
  for (auto& item : update) {
    // 获取当前更新的节点node
    const auto& node = item.second;

    // 更新扩展的SemanticsNode信息
    auto nodeEx = UpdatetSemanticsNodeExtent(node);

    //print semantics node and flags info for debugging
    GetSemanticsNodeDebugInfo(nodeEx);
    GetSemanticsFlagsDebugInfo(nodeEx);

    /**
     * 构建flutter无障碍语义节点树
     * NOTE: 若使用g_flutterSemanticsTree.insert({node.id, node})方式
     * 来添加新增的语义节点会导致已有key值自动忽略，不会更新原有key对应的value
     */
    g_flutterSemanticsTree[nodeEx.id] = nodeEx;

    // 若当前节点为获焦
    if (IsNodeFocused(nodeEx)) {
        inputFocusedNode = nodeEx;
    }
    // 若当前节点和更新前节点信息不同，则加入更新节点数组
    if (nodeEx.hadPreviousConfig) {
        updatedFlutterNodes.emplace_back(nodeEx);
    }

    // 获取当前flutter节点的全部子节点数量，并构建父子节点id映射关系
    int32_t childNodeCount = nodeEx.childrenInTraversalOrder.size();
    for (int32_t i = 0; i < childNodeCount; i++) {
      g_parentChildIdVec.emplace_back(std::make_pair(nodeEx.id, nodeEx.childrenInTraversalOrder[i]));
    }
  }

  for (const auto& nodeEx: updatedFlutterNodes) {
    // 当滑动节点产生滑动，并执行滑动处理
    if (HasScrolled(nodeEx)) {
      ArkUI_AccessibilityElementInfo* _elementInfo =
          OH_ArkUI_CreateAccessibilityElementInfo();

      FlutterNodeToElementInfoById(_elementInfo, static_cast<int64_t>(nodeEx.id));
      FlutterScrollExecution(nodeEx, _elementInfo);

      OH_ArkUI_DestoryAccessibilityElementInfo(_elementInfo);
      _elementInfo = nullptr;
    }

    // 判断是否触发liveRegion活动区，当前节点是否活跃
    if (nodeEx.HasFlag(FLAGS_::kIsLiveRegion) && HasChangedLabel(nodeEx)) {
      FML_DLOG(INFO) << "UpdateSemantics -> page content update, nodeEx.id=" << nodeEx.id;
      Flutter_SendAccessibilityAsyncEvent(static_cast<int64_t>(nodeEx.id),
                                          ArkUI_AccessibilityEventType::ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_PAGE_CONTENT_UPDATE);
      LOGD("Flutter_SendAccessibilityAsyncEvent -> PAGE_CONTENT_UPDATE");
    }
  }

  // 遍历更新的actions，并将所有的actions的id添加进actionMap
  for (const auto& item : actions) {
    const flutter::CustomAccessibilityAction action = item.second;
    GetCustomActionDebugInfo(action);
    g_actions_mp[action.id] = action;
  }

  // 打印flutter语义树的不同节点的属性信息
  for (const auto& item : g_flutterSemanticsTree) {
    FML_DLOG(INFO) << "g_flutterSemanticsTree -> {" << item.first << ", "
                   << item.second.id << "}";
  }
  for (const auto& item : g_parentChildIdVec) {
    FML_DLOG(INFO) << "g_parentChildIdVec -> (" << item.first << ", "
                   << item.second << ")";
  }

  //打印按层次遍历排序的flutter语义树节点id数组
  std::vector<int64_t> levelOrderTraversalTree = GetLevelOrderTraversalTree(0);
  for (const auto& item: levelOrderTraversalTree) {
    FML_DLOG(INFO) << "LevelOrderTraversalTree: { " << item << " }";
  }
  FML_DLOG(INFO) << "=== UpdateSemantics is end ===";
}

/**
 * flutter可滑动组件的滑动逻辑处理实现
 */
void OhosAccessibilityBridge::FlutterScrollExecution(
    SemanticsNodeExtent node,
    ArkUI_AccessibilityElementInfo* elementInfoFromList) {
  double nodePosition = node.scrollPosition;
  double nodeScrollExtentMax = node.scrollExtentMax;
  double nodeScrollExtentMin = node.scrollExtentMin;
  double infinity = std::numeric_limits<double>::infinity();

  // 设置flutter可滑动的最大范围值
  if (nodeScrollExtentMax == infinity) {
    nodeScrollExtentMax = SCROLL_EXTENT_FOR_INFINITY;
    if (nodePosition > SCROLL_POSITION_CAP_FOR_INFINITY) {
      nodePosition = SCROLL_POSITION_CAP_FOR_INFINITY;
    }
  }
  if (nodeScrollExtentMin == infinity) {
    nodeScrollExtentMax += SCROLL_EXTENT_FOR_INFINITY;
    if (nodePosition < -SCROLL_POSITION_CAP_FOR_INFINITY) {
      nodePosition = -SCROLL_POSITION_CAP_FOR_INFINITY;
    }
    nodePosition += SCROLL_EXTENT_FOR_INFINITY;
  } else {
    nodeScrollExtentMax -= node.scrollExtentMin;
    nodePosition -= node.scrollExtentMin;
  }

  // 当可滑动组件存在滑动子节点
  if (node.scrollChildren > 0) {
    // 配置当前滑动组件的子节点总数
    int32_t itemCount = node.scrollChildren;
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetItemCount(
            elementInfoFromList, itemCount)
    );
    // 设置当前页面可见的起始滑动index
    int32_t startItemIndex = node.scrollIndex;
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetStartItemIndex(elementInfoFromList,
                                                           startItemIndex)
    );

    // 计算当前滑动位置页面的可见子滑动节点数量
    int visibleChildren = 0;
    // handle hidden children at the beginning and end of the list.
    for (const auto& childId : node.childrenInHitTestOrder) {
      auto childNode = GetFlutterSemanticsNode(childId);
      if (!childNode.HasFlag(FLAGS_::kIsHidden)) {
        visibleChildren += 1;
      }
    }
    // 当可见滑动子节点数量超过滑动组件总子节点数量
    if (node.scrollIndex + visibleChildren > node.scrollChildren) {
      FML_DLOG(WARNING)
          << "FlutterScrollExecution -> Scroll index is out of bounds";
    }
    // 当滑动击中子节点数量为0
    if (!node.childrenInHitTestOrder.size()) {
      FML_DLOG(WARNING) << "FlutterScrollExecution -> Had scrollChildren but no "
                           "childrenInHitTestOrder";
    }

    // 设置当前页面可见的末尾滑动index
    int32_t endItemIndex = node.scrollIndex + visibleChildren - 1;
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetEndItemIndex(elementInfoFromList,
                                                         endItemIndex)
    );

  }
}

/**
 * 特定节点的焦点请求 (当页面更新时自动请求id=0节点获焦)
 */
void OhosAccessibilityBridge::RequestFocusWhenPageUpdate(int32_t requestFocusId)
{
  if (provider_ == nullptr) {
    FML_DLOG(ERROR) << "RequestFocusWhenPageUpdate ->"
                       "AccessibilityProvider = nullptr";
    return;
  }
  ArkUI_AccessibilityEventInfo* reqFocusEventInfo =
      OH_ArkUI_CreateAccessibilityEventInfo();
  ArkUI_AccessibilityElementInfo* elementInfo =
      OH_ArkUI_CreateAccessibilityElementInfo();

  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityEventSetEventType(
          reqFocusEventInfo,
          ArkUI_AccessibilityEventType::
              ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_REQUEST_ACCESSIBILITY_FOCUS)
  );

  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityEventSetRequestFocusId(reqFocusEventInfo,
                                                   requestFocusId)
  );
  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityEventSetElementInfo(reqFocusEventInfo, elementInfo)
  );

  auto callback = [](int32_t errorCode) {
    FML_DLOG(WARNING) << "PageStateUpdate callback-> errorCode =" << errorCode;
  };
  OH_ArkUI_SendAccessibilityAsyncEvent(provider_, reqFocusEventInfo, callback);

  OH_ArkUI_DestoryAccessibilityEventInfo(reqFocusEventInfo);
  reqFocusEventInfo = nullptr;
  OH_ArkUI_DestoryAccessibilityElementInfo(elementInfo);
  elementInfo = nullptr;
}

/**
 * 主动播报特定文本
 */
void OhosAccessibilityBridge::Announce(std::unique_ptr<char[]>& message)
{
  if (provider_ == nullptr) {
    FML_DLOG(ERROR) << "announce ->"
                       "AccessibilityProvider = nullptr";
    return;
  }
  // 创建并设置屏幕朗读事件
  ArkUI_AccessibilityEventInfo* announceEventInfo =
      OH_ArkUI_CreateAccessibilityEventInfo();

  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityEventSetEventType(
          announceEventInfo,
          ArkUI_AccessibilityEventType::
              ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_ANNOUNCE_FOR_ACCESSIBILITY)
  );
  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityEventSetTextAnnouncedForAccessibility(
          announceEventInfo, message.get())
  );
  FML_DLOG(INFO) << ("announce -> message: ")
                 << (message.get());

  auto callback = [](int32_t errorCode) {
    FML_DLOG(WARNING) << "announce callback-> errorCode =" << errorCode;
  };
  OH_ArkUI_SendAccessibilityAsyncEvent(provider_, announceEventInfo, callback);

  OH_ArkUI_DestoryAccessibilityEventInfo(announceEventInfo);
  announceEventInfo = nullptr;

}

//获取根节点
SemanticsNodeExtent OhosAccessibilityBridge::GetFlutterRootSemanticsNode()
{
  if (!g_flutterSemanticsTree.size()) {
    LOGE("GetFlutterRootSemanticsNode: g_flutterSemanticsTree.size()=0");
    return {};
  }
  if (g_flutterSemanticsTree.find(0) == g_flutterSemanticsTree.end()) {
    LOGE("GetFlutterRootSemanticsNod: g_flutterSemanticsTree has no root id");
    return {};
  }
  return g_flutterSemanticsTree.at(0);
}

/**
 * 根据nodeid获取或创建flutter语义节点
 */
SemanticsNodeExtent OhosAccessibilityBridge::GetFlutterSemanticsNode(
    int32_t id)
{
  if (g_flutterSemanticsTree.count(id) > 0) {
    return g_flutterSemanticsTree.at(id);
  } else {
    LOGE("GetFlutterSemanticsNode g_flutterSemanticsTree = null");
    return {};
  }
}

/**
 * flutter的语义节点初始化配置给arkui创建的elementInfos
 */
void OhosAccessibilityBridge::FlutterTreeToArkuiTree(
    ArkUI_AccessibilityElementInfoList* elementInfoList) {
  if (g_flutterSemanticsTree.size() == 0) {
    FML_DLOG(ERROR) << "OhosAccessibilityBridge::FlutterTreeToArkuiTree "
                       "g_flutterSemanticsTree.size() = 0";
    return;
  }
  // 将flutter语义节点树传递给arkui的无障碍elementinfo
  for (const auto& item : g_flutterSemanticsTree) {
    SemanticsNodeExtent flutterNode = item.second;

    // 创建elementinfo，系统自动加入到elementinfolist
    ArkUI_AccessibilityElementInfo* elementInfo =
        OH_ArkUI_AddAndGetAccessibilityElementInfo(elementInfoList);
    if (elementInfo == nullptr) {
      FML_DLOG(INFO) << "OhosAccessibilityBridge::FlutterTreeToArkuiTree "
                        "elementInfo is null";
      return;
    }
    // 设置elementinfo的屏幕坐标范围
    int32_t left = static_cast<int32_t>(flutterNode.rect.fLeft);
    int32_t top = static_cast<int32_t>(flutterNode.rect.fTop);
    int32_t right = static_cast<int32_t>(flutterNode.rect.fRight);
    int32_t bottom = static_cast<int32_t>(flutterNode.rect.fBottom);
    ArkUI_AccessibleRect rect = {left, top, right, bottom};
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetScreenRect(elementInfo, &rect)
    );

    // 设置elementinfo的action类型
    std::string widget_type = GetNodeComponentType(flutterNode);
    FlutterSetElementInfoOperationActions(elementInfo, widget_type);

    // 设置elementid
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetElementId(elementInfo, flutterNode.id)
    );

    // 设置父节点id
    int32_t parentId = GetParentId(flutterNode.id);
    if (flutterNode.id == 0) {
      ARKUI_ACCESSIBILITY_CALL_CHECK(
          OH_ArkUI_AccessibilityElementInfoSetParentId(elementInfo, ARKUI_ACCESSIBILITY_ROOT_PARENT_ID)
      );
    } else {
      ARKUI_ACCESSIBILITY_CALL_CHECK(
          OH_ArkUI_AccessibilityElementInfoSetParentId(elementInfo, parentId)
      );
    }

    int32_t childCount = flutterNode.childrenInTraversalOrder.size();
    if (childCount > 0) {
      // 设置孩子节点
      int32_t childCount = flutterNode.childrenInTraversalOrder.size();
      auto childrenIdsVec = flutterNode.childrenInTraversalOrder;
      std::sort(childrenIdsVec.begin(), childrenIdsVec.end());
      int64_t childNodeIds[childCount];
      for (int32_t i = 0; i < childCount; i++) {
        childNodeIds[i] = static_cast<int64_t>(childrenIdsVec[i]);
        FML_DLOG(INFO) << "FlutterTreeToArkuiTree flutterNode.id= "
                      << flutterNode.id << " childCount= " << childCount
                      << " childNodeId=" << childNodeIds[i];
      }
      ARKUI_ACCESSIBILITY_CALL_CHECK(
          OH_ArkUI_AccessibilityElementInfoSetChildNodeIds(elementInfo, 
                                                          childCount,
                                                          childNodeIds)
      );
    }

    // 配置常用属性，force to true for debugging
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetCheckable(elementInfo, true));
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetFocusable(elementInfo, true));
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetVisible(elementInfo, true));
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetEnabled(elementInfo, true));
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetClickable(elementInfo, true));
    
    // 设置组件类型
    std::string componentTypeName = GetNodeComponentType(flutterNode);
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetComponentType(
            elementInfo, componentTypeName.c_str())
    );

    std::string contents = componentTypeName + "_content";
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetContents(elementInfo, contents.c_str())
    );

    // 设置无障碍相关属性
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetAccessibilityText(
            elementInfo, flutterNode.label.c_str())
    );
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetAccessibilityLevel(elementInfo, "yes"));
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetAccessibilityGroup(elementInfo, false));
  }
  FML_DLOG(INFO) << "FlutterTreeToArkuiTree is end";
}

/**
 * 获取当前elementid的父节点id
 */
int32_t OhosAccessibilityBridge::GetParentId(int64_t elementId)
{
  if (!g_parentChildIdVec.size()) {
    FML_DLOG(INFO)
        << "OhosAccessibilityBridge::GetParentId parentChildIdMap.size()=0";
    return ARKUI_ACCESSIBILITY_ROOT_PARENT_ID;
  }
  if (elementId == -1) {
    return ARKUI_ACCESSIBILITY_ROOT_PARENT_ID;
  }
  int32_t childElementId = static_cast<int32_t>(elementId);
  for (const auto& item : g_parentChildIdVec) {
    if (item.second == childElementId) {
      return item.first;
    }
  }
  return RET_ERROR_STATE_CODE;
}

/**
 * 设置并获取xcomponet上渲染的组件的屏幕绝对坐标rect
 */
void OhosAccessibilityBridge::SetAbsoluteScreenRect(SemanticsNodeExtent& flutterNode,
                                                    float left,
                                                    float top,
                                                    float right,
                                                    float bottom) {
  g_screenRectMap[flutterNode.id] = AbsoluteRect{left, top, right, bottom};
  FML_DLOG(INFO) << "SetAbsoluteScreenRect -> id=" << flutterNode.id
                  << ", {" << left << ", " << top << ", " << right << ", "<< bottom << "> }";
}

AbsoluteRect OhosAccessibilityBridge::GetAbsoluteScreenRect(SemanticsNodeExtent& flutterNode) {
  if (!g_screenRectMap.empty() && g_screenRectMap.count(flutterNode.id) > 0) {
    return g_screenRectMap.at(flutterNode.id);
  } else {
    FML_DLOG(ERROR) << "GetAbsoluteScreenRect -> flutterNodeId="
                    << flutterNode.id << " is not found !";
    return {};
  }
}

/**
 * 获取flutter相对-绝对坐标映射的真实缩放系数
 */
std::pair<float, float> OhosAccessibilityBridge::GetRealScaleFactor()
{
    auto secondNode = GetFlutterSemanticsNode(1);
    SkMatrix transform = secondNode.transform.asM33();
    auto scaleX = transform.get(SkMatrix::kMScaleX);
    auto scaleY = transform.get(SkMatrix::kMScaleY);
    return std::make_pair(scaleX, scaleY);
}

/**
 * flutter无障碍语义树的子节点相对坐标系转化为屏幕绝对坐标的映射算法
 * 目前暂未考虑旋转、透视场景，不影响屏幕朗读功能
 */
void OhosAccessibilityBridge::FlutterRelativeRectToScreenRect(
    SemanticsNodeExtent currNode) {
  // 获取当前flutter节点的相对rect
  auto currLeft = static_cast<float>(currNode.rect.fLeft);
  auto currTop = static_cast<float>(currNode.rect.fTop);
  auto currRight = static_cast<float>(currNode.rect.fRight);
  auto currBottom = static_cast<float>(currNode.rect.fBottom);

  /** 
  * 获取当前flutter节点的缩放、平移、透视等矩阵坐标转换
  * 以下矩阵坐标变换参数（如：旋转/错切、透视）场景目前暂不考虑
  * NOTE: SkMatrix::kMSkewX, SkMatrix::kMSkewY,
  * SkMatrix::kMPersp0, SkMatrix::kMPersp1, SkMatrix::kMPersp2
  */
  SkMatrix transform = currNode.transform.asM33();
  auto _kMScaleX = transform.get(SkMatrix::kMScaleX);
  auto _kMTransX = transform.get(SkMatrix::kMTransX);
  auto _kMScaleY = transform.get(SkMatrix::kMScaleY);
  auto _kMTransY = transform.get(SkMatrix::kMTransY);

  // 获取当前flutter节点的父节点的相对rect
  int32_t parentId = GetParentId(currNode.id);
  auto parentNode = GetFlutterSemanticsNode(parentId);
  if (!g_flutterSemanticsTree.count(parentId)) {
    LOGE("FlutterRelativeRectToScreenRect: GetFlutterSemanticsNode id=%{public}d null", parentId);
  }

  // 获取当前flutter节点的父节点的绝对坐标
  auto [realParentLeft, realParentTop, realParentRight, realParentBottom] = GetAbsoluteScreenRect(parentNode);

  //获取root节点的绝对坐标
  auto rootNode = GetFlutterSemanticsNode(0);
  auto _rootRect = GetAbsoluteScreenRect(rootNode);
  auto rootWidth = _rootRect.right;
  auto rootHeight = _rootRect.bottom;

  // 获取真实缩放系数
  auto [realScaleXFactor, realScaleYFactor] = GetRealScaleFactor();

  // 更新后的节点屏幕坐标
  float newLeft = 0.0;
  float newTop = 0.0;
  float newRight = 0.0;
  float newBottom = 0.0;

  if (_kMScaleX > 1 && _kMScaleY > 1) {
    // 子节点相对父节点进行变化（缩放、 平移）
    newLeft = currLeft + _kMTransX * _kMScaleX;
    newTop = currTop + _kMTransY * _kMScaleY;
    newRight = currRight * _kMScaleX;
    newBottom = currBottom * _kMScaleY;
    // 更新当前flutter节点currNode的相对坐标 -> 屏幕绝对坐标
    SetAbsoluteScreenRect(currNode, newLeft, newTop, newRight, newBottom);
  } else {
    // 子节点的屏幕绝对坐标转换，包括offset偏移值计算、缩放系数变换
    newLeft = (currLeft + _kMTransX) * realScaleXFactor + realParentLeft;
    newTop  = (currTop  + _kMTransY) * realScaleYFactor + realParentTop;
    newRight  = newLeft + currRight  * realScaleXFactor;
    newBottom = newTop  + currBottom * realScaleYFactor;

    // 若子节点rect超过父节点则跳过显示（单个屏幕显示不下，滑动再重新显示）
    const bool IS_OVER_SCREEN_AREA = newLeft < realParentLeft || newTop < realParentTop ||
                                      newRight > realParentRight || newBottom > realParentBottom ||
                                      newLeft >= newRight || newTop >= newBottom ||
                                      newRight > rootWidth || newBottom > rootHeight;

    if (IS_OVER_SCREEN_AREA) {
      FML_DLOG(ERROR) << "RelativeRectToScreenRect -> childRect exceeds parentRect {Id: "
                      << currNode.id << ", (" << newLeft << ", " << newTop
                      << ", " << newRight << ", " << newBottom << ")}";
      // 防止滑动场景下绿框坐标超出屏幕范围，进行正则化处理
      SetAbsoluteScreenRect(currNode, rootWidth, rootHeight, 0, 0);
    } else {
      SetAbsoluteScreenRect(currNode, newLeft, newTop, newRight, newBottom);
    }
  }
}

/**
 * 实现对特定id的flutter节点到arkui的elementinfo节点转化
 */
void OhosAccessibilityBridge::FlutterNodeToElementInfoById(
    ArkUI_AccessibilityElementInfo* elementInfoFromList,
    int64_t elementId) {
  if (elementInfoFromList == nullptr) {
    FML_DLOG(INFO) << "OhosAccessibilityBridge::FlutterNodeToElementInfoById "
                      "elementInfoFromList is null";
    return;
  }
  FML_DLOG(INFO) << "FlutterNodeToElementInfoById elementId = " << elementId;
  // 当elementId = -1或0时，创建root节点
  if (elementId == 0 || elementId == -1) {
    // 获取flutter的root节点
    SemanticsNodeExtent flutterNode = GetFlutterSemanticsNode(static_cast<int32_t>(0));
    if (flutterNode.isNull) {
      LOGE("FlutterNodeToElementInfoById: GetFlutterSemanticsNode id=%{public}ld null", elementId);
    }

    // 设置elementinfo的屏幕坐标范围
    int32_t left = static_cast<int32_t>(flutterNode.rect.fLeft);
    int32_t top = static_cast<int32_t>(flutterNode.rect.fTop);
    int32_t right = static_cast<int32_t>(flutterNode.rect.fRight);
    int32_t bottom = static_cast<int32_t>(flutterNode.rect.fBottom);
    ArkUI_AccessibleRect rect = {left, top, right, bottom};
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetScreenRect(
            elementInfoFromList, &rect)
    );

    // 设置root节点的屏幕绝对坐标rect
    SetAbsoluteScreenRect(flutterNode, left, top, right, bottom);

    // 设置root节点的action类型
    FlutterSetElementInfoOperationActions(elementInfoFromList, OTHER_WIDGET_NAME);

    // 根据flutternode信息配置对应的elementinfo
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetElementId(elementInfoFromList, 0)
    );

    // NOTE: arkui无障碍子系统强制设置root的父节点id = -2100000 (严禁更改)
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetParentId(
            elementInfoFromList, ARKUI_ACCESSIBILITY_ROOT_PARENT_ID)
    );

    // 设置无障碍播报文本
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetAccessibilityText(
            elementInfoFromList, flutterNode.label.empty() ? flutterNode.value.c_str() : flutterNode.label.c_str())
    );
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetAccessibilityLevel(
            elementInfoFromList, "yes")
    );
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetAccessibilityGroup(
            elementInfoFromList, false)
    );

    // 配置child节点信息
    int32_t childCount = flutterNode.childrenInTraversalOrder.size();
    if (childCount > 0) {
      auto childrenIdsVec = flutterNode.childrenInTraversalOrder;
      std::sort(childrenIdsVec.begin(), childrenIdsVec.end());
      int64_t childNodeIds[childCount];
      for (int32_t i = 0; i < childCount; i++) {
        childNodeIds[i] = static_cast<int64_t>(childrenIdsVec[i]);
        FML_DLOG(INFO)
            << "FlutterNodeToElementInfoById -> elementid=0 childCount="
            << childCount << " childNodeIds=" << childNodeIds[i];
      }
      ARKUI_ACCESSIBILITY_CALL_CHECK(
          OH_ArkUI_AccessibilityElementInfoSetChildNodeIds(
              elementInfoFromList, childCount, childNodeIds)
      );
    }
    
    // 配置root节点常用属性
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetEnabled(elementInfoFromList, true));
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetClickable(elementInfoFromList, true));
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetFocusable(elementInfoFromList, true));
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetVisible(elementInfoFromList, true));
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetComponentType(elementInfoFromList, "root"));
    ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityElementInfoSetContents(elementInfoFromList, flutterNode.label.c_str()));
  } else {
    //当elementId >= 1时，根据flutter节点信息配置elementinfo无障碍属性
    FlutterSetElementInfoProperties(elementInfoFromList, elementId);
  }
  FML_DLOG(INFO)
      << "=== OhosAccessibilityBridge::FlutterNodeToElementInfoById is end ===";
}

/**
 * 判断源字符串是否包含目标字符串
 */
bool OhosAccessibilityBridge::Contains(const std::string source,
                                       const std::string target) {
  return source.find(target) != std::string::npos;
}

/**
 * 配置arkui节点的可操作动作类型
 */
void OhosAccessibilityBridge::FlutterSetElementInfoOperationActions(
    ArkUI_AccessibilityElementInfo* elementInfoFromList,
    std::string widget_type) {
  if (Contains(widget_type, EDIT_TEXT_WIDGET_NAME) ||
      Contains(widget_type, EDIT_MULTILINE_TEXT_WIDGET_NAME)) {
    // set elementinfo action types
    int32_t actionTypeNum = 10;
    ArkUI_AccessibleAction actions[actionTypeNum];

    actions[0].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_GAIN_ACCESSIBILITY_FOCUS;
    actions[0].description = "获取焦点";

    actions[1].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLEAR_ACCESSIBILITY_FOCUS;
    actions[1].description = "清除焦点";

    actions[2].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK;
    actions[2].description = "点击操作";

    actions[3].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_LONG_CLICK;
    actions[3].description = "长按操作";

    actions[4].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_COPY;
    actions[4].description = "文本复制";

    actions[5].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_PASTE;
    actions[5].description = "文本粘贴";

    actions[6].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CUT;
    actions[6].description = "文本剪切";

    actions[7].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT;
    actions[7].description = "文本选择";

    actions[8].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_TEXT;
    actions[8].description = "文本内容设置";

    actions[9].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION;
    actions[9].description = "光标位置设置";

    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetOperationActions(
            elementInfoFromList, actionTypeNum, actions)
    );
  } else if (Contains(widget_type, SCROLL_WIDGET_NAME)) {
    // if node is a scrollable component
    int32_t actionTypeNum = 5;
    ArkUI_AccessibleAction actions[actionTypeNum];

    actions[0].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_GAIN_ACCESSIBILITY_FOCUS;
    actions[0].description = "获取焦点";

    actions[1].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLEAR_ACCESSIBILITY_FOCUS;
    actions[1].description = "清除焦点";

    actions[2].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK;
    actions[2].description = "点击动作";

    actions[3].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD;
    actions[3].description = "向上滑动";

    actions[4].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD;
    actions[4].description = "向下滑动";

    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetOperationActions(
            elementInfoFromList, actionTypeNum, actions)
    );
  } else {
    // set common component action types
    int32_t actionTypeNum = 3;
    ArkUI_AccessibleAction actions[actionTypeNum];

    actions[0].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_GAIN_ACCESSIBILITY_FOCUS;
    actions[0].description = "获取焦点";

    actions[1].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLEAR_ACCESSIBILITY_FOCUS;
    actions[1].description = "清除焦点";

    actions[2].actionType = ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK;
    actions[2].description = "点击动作";

    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetOperationActions(
            elementInfoFromList, actionTypeNum, actions)
    );
  }
}

/**
 * 根据flutter节点信息配置elementinfo无障碍属性
 */
void OhosAccessibilityBridge::FlutterSetElementInfoProperties(
    ArkUI_AccessibilityElementInfo* elementInfoFromList,
    int64_t elementId) {
  SemanticsNodeExtent flutterNode = GetFlutterSemanticsNode(static_cast<int32_t>(elementId));
  if (flutterNode.isNull) {
    LOGE("FlutterSetElementInfoProperties: GetFlutterSemanticsNode id=%{public}ld null", elementId);
  }

  // set elementinfo id
  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityElementInfoSetElementId(elementInfoFromList,
                                                    flutterNode.id)
  );

  // 相对-绝对坐标映射
  FlutterRelativeRectToScreenRect(flutterNode);
  auto [left, top, right, bottom] = GetAbsoluteScreenRect(flutterNode);
  ArkUI_AccessibleRect rect = {static_cast<int32_t>(left), static_cast<int32_t>(top), 
                                static_cast<int32_t>(right), static_cast<int32_t>(bottom)};
  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityElementInfoSetScreenRect(elementInfoFromList, &rect)
  );
  FML_DLOG(INFO) << "FlutterNodeToElementInfoById -> node.id= "
                << flutterNode.id << " SceenRect = (" << left << ", " << top
                << ", " << right << ", " << bottom << ")";
 
  // 配置arkui的elementinfo可操作动作属性
  if (IsTextField(flutterNode)) {
    // 若当前flutter节点为文本输入框组件
    std::string widgeType = flutterNode.HasFlag(FLAGS_::kIsMultiline) ? EDIT_MULTILINE_TEXT_WIDGET_NAME : EDIT_TEXT_WIDGET_NAME;
    FlutterSetElementInfoOperationActions(elementInfoFromList, widgeType);
  } else if (IsScrollableWidget(flutterNode) || IsNodeScrollable(flutterNode)) {
    // 若当前flutter节点为可滑动组件类型
    FlutterSetElementInfoOperationActions(elementInfoFromList, SCROLL_WIDGET_NAME);
  } else {
    // 若当前flutter节点为通用组件
    FlutterSetElementInfoOperationActions(elementInfoFromList, OTHER_WIDGET_NAME);
  }

  // 设置当前节点的父节点id
  int32_t parentId = GetParentId(elementId);
  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityElementInfoSetParentId(elementInfoFromList, parentId)
  );
  FML_DLOG(INFO) << "FlutterNodeToElementInfoById GetParentId = " << parentId;
  

  // 设置可朗读文本
  std::string text = flutterNode.label + flutterNode.value;
  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityElementInfoSetAccessibilityText(elementInfoFromList,
                                                            text.c_str())
  );
  FML_DLOG(INFO) << "FlutterNodeToElementInfoById SetAccessibilityText = "
                 << text;
  // 设置无障碍content文本
  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityElementInfoSetContents(elementInfoFromList, text.c_str())
  );
  // 设置hint提示文本
  std::string hint = flutterNode.hint;
  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityElementInfoSetHintText(elementInfoFromList,
                                                   hint.c_str())
  );

  // 设置当前节点的全部孩子节点
  int32_t childCount = flutterNode.childrenInTraversalOrder.size();
  if (childCount > 0) {
    auto childrenIdsVec = flutterNode.childrenInTraversalOrder;
    std::sort(childrenIdsVec.begin(), childrenIdsVec.end());
    int64_t childNodeIds[childCount];
    for (int32_t i = 0; i < childCount; i++) {
      childNodeIds[i] = static_cast<int64_t>(childrenIdsVec[i]);
      FML_DLOG(INFO) << "FlutterNodeToElementInfoById -> elementid=" << elementId
                    << " childCount=" << childCount
                    << " childNodeIds=" << childNodeIds[i];
    }
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetChildNodeIds(elementInfoFromList,
                                                        childCount, childNodeIds)
    );
  }

  /**
   * 根据当前flutter节点的SemanticsFlags特性，配置对应的elmentinfo属性
   */
  // 判断当前节点组件是否enabled
  if (IsNodeEnabled(flutterNode)) {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetEnabled(elementInfoFromList, true)
    );
    FML_DLOG(INFO) << "flutterNode.id=" << flutterNode.id
                   << " OH_ArkUI_AccessibilityElementInfoSetEnabled -> true";
  }
  // 判断当前节点是否可点击
  if (IsNodeClickable(flutterNode)) {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetClickable(elementInfoFromList, true)
    );
    FML_DLOG(INFO) << "flutterNode.id=" << flutterNode.id
                   << " OH_ArkUI_AccessibilityElementInfoSetClickable -> true";
  }
  // 判断当前节点是否可获焦点
  if (IsNodeFocusable(flutterNode)) {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetFocusable(elementInfoFromList, true)
    );
    FML_DLOG(INFO) << "flutterNode.id=" << flutterNode.id
                   << " OH_ArkUI_AccessibilityElementInfoSetFocusable -> true";
  }
  // 判断当前节点是否为密码输入框
  if (IsNodePassword(flutterNode)) {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetIsPassword(elementInfoFromList, true)
    );
    FML_DLOG(INFO) << "flutterNode.id=" << flutterNode.id
                   << " OH_ArkUI_AccessibilityElementInfoSetIsPassword -> true";
  }
  // 判断当前节点是否具备checkable状态 (如：checkbox, radio button)
  if (IsNodeCheckable(flutterNode)) {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetCheckable(elementInfoFromList, true)
    );
    FML_DLOG(INFO) << "flutterNode.id=" << flutterNode.id
                   << " OH_ArkUI_AccessibilityElementInfoSetCheckable -> true";
  }
  // 判断当前节点(check box/radio button)是否checked/unchecked
  if (IsNodeChecked(flutterNode)) {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetChecked(elementInfoFromList, true)
    );
    FML_DLOG(INFO) << "flutterNode.id=" << flutterNode.id
                   << " OH_ArkUI_AccessibilityElementInfoSetChecked -> true";
  }
  // 判断当前节点组件是否可显示
  if (IsNodeVisible(flutterNode)) {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetVisible(elementInfoFromList, true)
    );
    FML_DLOG(INFO) << "flutterNode.id=" << flutterNode.id
                   << " OH_ArkUI_AccessibilityElementInfoSetVisible -> true";
  }
  // 判断当前节点组件是否选中
  if (IsNodeSelected(flutterNode)) {
    OH_ArkUI_AccessibilityElementInfoSetSelected(elementInfoFromList, true);
    FML_DLOG(INFO) << "flutterNode.id=" << flutterNode.id
                   << " OH_ArkUI_AccessibilityElementInfoSetSelected -> true";
  }
  // 判断当前节点组件是否可滑动
  if (IsNodeScrollable(flutterNode)) {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetScrollable(elementInfoFromList, true)
    );
    FML_DLOG(INFO) << "flutterNode.id=" << flutterNode.id
                   << " OH_ArkUI_AccessibilityElementInfoSetScrollable -> true";
  }
  // 判断当前节点组件是否可编辑（文本输入框）
  if (IsTextField(flutterNode)) {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetEditable(elementInfoFromList, true)
    );
    FML_DLOG(INFO) << "flutterNode.id=" << flutterNode.id
                   << " OH_ArkUI_AccessibilityElementInfoSetEditable -> true";
  }
  // 判断当前节点组件是否为滑动条
  if (IsSlider(flutterNode)) {
    FML_DLOG(INFO) << "flutterNode.id=" << flutterNode.id
                   << " OH_ArkUI_AccessibilityElementInfoSetRangeInfo -> true";
  }
  // 判断当前节点组件是否支持长按
  if (IsNodeHasLongPress(flutterNode)) {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetLongClickable(elementInfoFromList, true)
    );
    FML_DLOG(INFO)
        << "flutterNode.id=" << flutterNode.id
        << " OH_ArkUI_AccessibilityElementInfoSetLongClickable -> true";
  }

  // 获取当前节点的组件类型
  std::string componentTypeName = GetNodeComponentType(flutterNode);
  FML_DLOG(INFO) << "FlutterNodeToElementInfoById componentTypeName = "
                 << componentTypeName;
  // flutter节点对应elementinfo所属的组件类型（如：root， button，text等）
  if (elementId == 0) {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetComponentType(elementInfoFromList, "root")
    );
  } else {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetComponentType(elementInfoFromList, 
                                                          componentTypeName.c_str())
    );
  }
  FML_DLOG(INFO) << "FlutterNodeToElementInfoById SetComponentType: "
                 << componentTypeName;

  /**
   * 无障碍重要性，用于控制某个组件是否可被无障碍辅助服务所识别。支持的值为（默认值：“auto”）：
   * “auto”：根据组件不同会转换为“yes”或者“no”
   * “yes”：当前组件可被无障碍辅助服务所识别
   * “no”：当前组件不可被无障碍辅助服务所识别
   * “no-hide-descendants”：当前组件及其所有子组件不可被无障碍辅助服务所识别
   */
  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityElementInfoSetAccessibilityLevel(elementInfoFromList, "yes");
  );
  // 无障碍组，设置为true时表示该组件及其所有子组件为一整个可以选中的组件，无障碍服务将不再关注其子组件内容。默认值：false
  ARKUI_ACCESSIBILITY_CALL_CHECK(
      OH_ArkUI_AccessibilityElementInfoSetAccessibilityGroup(elementInfoFromList, false);
  );
}

/**
 * 将flutter无障碍语义树的转化为层次遍历顺序存储，
 * 并按该顺序构建arkui语义树，以实现DevEco Testing
 * UIViewer、Hypium自动化测试工具对flutter组件树的可视化
 */
std::vector<int64_t> OhosAccessibilityBridge::GetLevelOrderTraversalTree(int32_t rootId)
{
  std::vector<int64_t> levelOrderTraversalTree;
  std::queue<SemanticsNodeExtent> semanticsQue;

  auto root = GetFlutterSemanticsNode(rootId);
  semanticsQue.push(root);

  while (!semanticsQue.empty()) {
    uint32_t queSize = semanticsQue.size();
    for (uint32_t i=0; i<queSize; i++) {
      auto currNode = semanticsQue.front();
      semanticsQue.pop();
      levelOrderTraversalTree.emplace_back(static_cast<int64_t>(currNode.id));

      std::sort(currNode.childrenInTraversalOrder.begin(), 
                currNode.childrenInTraversalOrder.end());
      for (const auto& childId: currNode.childrenInTraversalOrder) {
        auto childNode = GetFlutterSemanticsNode(childId);
        semanticsQue.push(childNode);
      }
    }
  }
  return levelOrderTraversalTree;
}

/**
 * 创建并配置完整arkui无障碍语义树
 */
void OhosAccessibilityBridge::BuildArkUISemanticsTree(
    int64_t elementId,
    ArkUI_AccessibilityElementInfo* elementInfoFromList,
    ArkUI_AccessibilityElementInfoList* elementList)
{
  //配置root节点信息
  FlutterNodeToElementInfoById(elementInfoFromList, elementId);
  //获取flutter无障碍语义树的节点总数
  auto levelOrderTreeVec = GetLevelOrderTraversalTree(0);
  int64_t elementInfoCount = levelOrderTreeVec.size();
  //创建并配置节点id >= 1的全部节点
  for (int64_t i = 1; i < elementInfoCount; i++) {
    int64_t levelOrderId = levelOrderTreeVec[i];
    auto newNode = GetFlutterSemanticsNode(levelOrderId);
    if (g_flutterSemanticsTree.count(newNode.id)) {
      LOGE("BuildArkUISemanticsTree: GetFlutterSemanticsNode id=%{public}ld null", levelOrderId);
    }
    //当节点为隐藏状态时，自动规避
    ArkUI_AccessibilityElementInfo* newElementInfo =
      OH_ArkUI_AddAndGetAccessibilityElementInfo(elementList);
    //配置当前子节点信息
    FlutterNodeToElementInfoById(newElementInfo, levelOrderId);
  }
}

/**
 * Called to obtain element information based on a specified node.
 * NOTE:该arkui接口需要在系统无障碍服务开启时，才能触发调用
 */
int32_t OhosAccessibilityBridge::FindAccessibilityNodeInfosById(
    int64_t elementId,
    ArkUI_AccessibilitySearchMode mode,
    int32_t requestId,
    ArkUI_AccessibilityElementInfoList* elementList) {
  FML_DLOG(INFO)
      << "#### FindAccessibilityNodeInfosById input-params ####: elementId = "
      << elementId << " mode=" << mode << " requestId=" << requestId
      << " elementList= " << elementList;

  if (g_flutterSemanticsTree.size() == 0) {
    FML_DLOG(INFO)
        << "FindAccessibilityNodeInfosById g_flutterSemanticsTree is null";
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
  if (elementList == nullptr) {
    FML_DLOG(INFO) << "FindAccessibilityNodeInfosById elementList is null";
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
  // 获取当前对应id的flutter节点，若为空则返回错误编码
  auto flutterNode = GetFlutterSemanticsNode(static_cast<int32_t>(elementId));
  if (!g_flutterSemanticsTree.count(flutterNode.id)) {
      LOGE("FindAccessibilityNodeInfosById: GetFlutterSemanticsNode id=%{public}ld is null", elementId);
  }

  // 开启无障碍导航功能
  if(elementId == -1 || elementId == 0) {
      accessibilityFeatures_->SetAccessibleNavigation(true, native_shell_holder_id_);
  }

  // 从elementinfolist中获取elementinfo
  ArkUI_AccessibilityElementInfo* elementInfoFromList =
      OH_ArkUI_AddAndGetAccessibilityElementInfo(elementList);
  if (elementInfoFromList == nullptr) {
    FML_DLOG(INFO)
        << "FindAccessibilityNodeInfosById elementInfoFromList is null";
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }

  if (mode == ArkUI_AccessibilitySearchMode::
                  ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_CURRENT) {
    /** Search for current nodes. (mode = 0) */
    BuildArkUISemanticsTree(elementId, elementInfoFromList, elementList);
  } else if (mode ==
             ArkUI_AccessibilitySearchMode::
                 ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_PREDECESSORS) {
    /** Search for parent nodes. (mode = 1) */
    if (IsNodeVisible(flutterNode)) {
      FlutterNodeToElementInfoById(elementInfoFromList, elementId);
    }
  } else if (mode ==
             ArkUI_AccessibilitySearchMode::
                 ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_SIBLINGS) {
    /** Search for sibling nodes. (mode = 2) */
    if (IsNodeVisible(flutterNode)) {
      FlutterNodeToElementInfoById(elementInfoFromList, elementId);
    }
  } else if (mode ==
             ArkUI_AccessibilitySearchMode::
                 ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_CHILDREN) {
    /** Search for child nodes at the next level. (mode = 4) */
    if (IsNodeVisible(flutterNode)) {
      FlutterNodeToElementInfoById(elementInfoFromList, elementId);
    }
  } else if (
      mode ==
      ArkUI_AccessibilitySearchMode::
          ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_RECURSIVE_CHILDREN) {
    /** Search for all child nodes. (mode = 8) */
    BuildArkUISemanticsTree(elementId, elementInfoFromList, elementList);
  } else {
    FlutterNodeToElementInfoById(elementInfoFromList, elementId);
  }
  FML_DLOG(INFO) << "--- FindAccessibilityNodeInfosById is end ---";
  return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
}

/**
 * 解析flutter语义动作，并通过NativAccessibilityChannel分发
 */
void OhosAccessibilityBridge::DispatchSemanticsAction(
    int32_t id,
    flutter::SemanticsAction action,
    fml::MallocMapping args)
{
  nativeAccessibilityChannel_->DispatchSemanticsAction(native_shell_holder_id_,
                                                       id,
                                                       action,
                                                       fml::MallocMapping());
}

/**
 * 执行语义动作解析，当FindAccessibilityNodeInfosById找到相应的elementinfo时才会触发该回调函数
 */
int32_t OhosAccessibilityBridge::ExecuteAccessibilityAction(
    int64_t elementId,
    ArkUI_Accessibility_ActionType action,
    ArkUI_AccessibilityActionArguments* actionArguments,
    int32_t requestId)
{
  FML_DLOG(INFO) << "ExecuteAccessibilityAction input-params-> elementId="
                 << elementId << " action=" << action
                 << " requestId=" << requestId
                 << " *actionArguments=" << actionArguments;

  if (actionArguments == nullptr) {
    FML_DLOG(ERROR) << "OhosAccessibilityBridge::ExecuteAccessibilityAction "
                       "actionArguments = null";
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }

  // 获取当前elementid对应的flutter语义节点
  auto flutterNode =
      GetFlutterSemanticsNode(static_cast<int32_t>(elementId));
  if (!g_flutterSemanticsTree.count(flutterNode.id)) {
    LOGE("ExecuteAccessibilityAction: GetFlutterSemanticsNode id=%{public}ld is null", elementId);
  }
  // 若当前节点为非屏幕显示状态，将其显示在屏幕上
  if (!IsNodeShowOnScreen(flutterNode)) {
    DispatchSemanticsAction(static_cast<int32_t>(elementId), ACTIONS_::kShowOnScreen, {});
  }

  // 根据当前elementid和无障碍动作类型，发送无障碍事件
  switch (action) {
    /** Response to a click. 16 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK: {
      /** Click event, sent after the UI component responds. 1 */
      auto clickEventType = ArkUI_AccessibilityEventType::
          ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_CLICKED;
      Flutter_SendAccessibilityAsyncEvent(elementId, clickEventType);
      FML_DLOG(INFO) << "ExecuteAccessibilityAction -> action: click(" << action
                     << ")" << " event: click(" << clickEventType << ")";
      // 解析arkui的屏幕点击 -> flutter对应节点的屏幕点击
      auto flutterTapAction = ArkuiActionsToFlutterActions(action);
      DispatchSemanticsAction(static_cast<int32_t>(elementId), flutterTapAction,
                              {});
      break;
    }
    /** Response to a long click. 32 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_LONG_CLICK: {
      /** Long click event, sent after the UI component responds. 2 */
      auto longClickEventType = ArkUI_AccessibilityEventType::
          ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_LONG_CLICKED;
      Flutter_SendAccessibilityAsyncEvent(elementId, longClickEventType);
      FML_DLOG(INFO) << "ExecuteAccessibilityAction -> action: longclick("
                     << action << ")" << " event: longclick("
                     << longClickEventType << ")";
      // 解析arkui的屏幕动作 -> flutter对应节点的屏幕动作
      auto flutterLongPressAction = ArkuiActionsToFlutterActions(action);
      DispatchSemanticsAction(static_cast<int32_t>(elementId),
                              flutterLongPressAction, {});
      break;
    }
    /** Accessibility focus acquisition. 64 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_GAIN_ACCESSIBILITY_FOCUS: {
      // 感知获焦flutter节点
      accessibilityFocusedNode = flutterNode;
      // 解析arkui的获焦 -> flutter对应节点的获焦
      auto flutterGainFocusAction = ArkuiActionsToFlutterActions(action);
      DispatchSemanticsAction(static_cast<int32_t>(elementId),
                              flutterGainFocusAction, {});
      // Accessibility focus event, sent after the UI component responds. 32768
      auto focusEventType = ArkUI_AccessibilityEventType::
          ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_ACCESSIBILITY_FOCUSED;
      Flutter_SendAccessibilityAsyncEvent(elementId, focusEventType);
      FML_DLOG(INFO) << "ExecuteAccessibilityAction -> action: focus(" << action
                     << ")" << " event: focus(" << focusEventType << ")";

      if (flutterNode.HasAction(ACTIONS_::kIncrease) ||
          flutterNode.HasAction(ACTIONS_::kDecrease)) {
        Flutter_SendAccessibilityAsyncEvent(
            elementId, ArkUI_AccessibilityEventType::
                           ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_SELECTED);
      }
      break;
    }
    /** Accessibility focus clearance. 128 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLEAR_ACCESSIBILITY_FOCUS: {
      // 解析arkui的失焦 -> flutter对应节点的失焦
      auto flutterLoseFocusAction = ArkuiActionsToFlutterActions(action);
      DispatchSemanticsAction(static_cast<int32_t>(elementId),
                              flutterLoseFocusAction, {});
      /** Accessibility focus cleared event, sent after the UI component
       * responds. 65536 */
      auto clearFocusEventType = ArkUI_AccessibilityEventType::
          ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_ACCESSIBILITY_FOCUS_CLEARED;
      Flutter_SendAccessibilityAsyncEvent(elementId, clearFocusEventType);
      FML_DLOG(INFO) << "ExecuteAccessibilityAction -> action: clearfocus("
                     << action << ")" << " event: clearfocus("
                     << clearFocusEventType << ")";
      break;
    }
    /** Forward scroll action. 256 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD: {
      // flutter scroll forward with different situations
      if (flutterNode.HasAction(ACTIONS_::kScrollUp)) {
        auto flutterScrollUpAction = ArkuiActionsToFlutterActions(action);
        DispatchSemanticsAction(static_cast<int32_t>(elementId),
                                flutterScrollUpAction, {});
      } else if (flutterNode.HasAction(ACTIONS_::kScrollLeft)) {
        DispatchSemanticsAction(static_cast<int32_t>(elementId),
                                ACTIONS_::kScrollLeft, {});
      } else if (flutterNode.HasAction(ACTIONS_::kIncrease)) {
        flutterNode.value = flutterNode.increasedValue;
        flutterNode.valueAttributes = flutterNode.increasedValueAttributes;
        
        Flutter_SendAccessibilityAsyncEvent(
            elementId, ArkUI_AccessibilityEventType::
                           ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_SELECTED);
        DispatchSemanticsAction(static_cast<int32_t>(elementId),
                                ACTIONS_::kIncrease, {});
      }
      std::string currComponetType = GetNodeComponentType(flutterNode);
      if (Contains(currComponetType, SCROLL_WIDGET_NAME)) {
        /** Scrolled event, sent when a scrollable component experiences a
          * scroll event. 4096 */
        ArkUI_AccessibilityEventType scrollEventType1 =
            ArkUI_AccessibilityEventType::ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_SCROLLED;
        Flutter_SendAccessibilityAsyncEvent(elementId, scrollEventType1);
        FML_DLOG(INFO)
            << "ExecuteAccessibilityAction -> action: scroll forward(" << action
            << ")" << " event: scroll forward(" << scrollEventType1 << ")";
      }
      break;
    }
    /** Backward scroll action. 512 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD: {
      // flutter scroll down with different situations
      if (flutterNode.HasAction(ACTIONS_::kScrollDown)) {
        auto flutterScrollDownAction = ArkuiActionsToFlutterActions(action);
        DispatchSemanticsAction(static_cast<int32_t>(elementId),
                                flutterScrollDownAction, {});
      } else if (flutterNode.HasAction(ACTIONS_::kScrollRight)) {
        DispatchSemanticsAction(static_cast<int32_t>(elementId),
                                ACTIONS_::kScrollRight, {});
      } else if (flutterNode.HasAction(ACTIONS_::kDecrease)) {
        flutterNode.value = flutterNode.decreasedValue;
        flutterNode.valueAttributes = flutterNode.decreasedValueAttributes;

        Flutter_SendAccessibilityAsyncEvent(
            elementId, ArkUI_AccessibilityEventType::
                           ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_SELECTED);
        DispatchSemanticsAction(static_cast<int32_t>(elementId),
                                ACTIONS_::kDecrease, {});
      }
      std::string currComponetType = GetNodeComponentType(flutterNode);
      if (Contains(currComponetType, SCROLL_WIDGET_NAME)) {
        /** Scrolled event, sent when a scrollable component experiences a
          * scroll event. 4096 */
        ArkUI_AccessibilityEventType scrollEventType1 =
            ArkUI_AccessibilityEventType::ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_SCROLLED;
        Flutter_SendAccessibilityAsyncEvent(elementId, scrollEventType1);
        FML_DLOG(INFO)
            << "ExecuteAccessibilityAction -> action: scroll forward(" << action
            << ")" << " event: scroll forward(" << scrollEventType1 << ")";
      }
      break;
    }
    /** Copy action for text content. 1024 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_COPY: {
      FML_DLOG(INFO) << "ExecuteAccessibilityAction -> action: copy(" << action
                     << ")";
      DispatchSemanticsAction(static_cast<int32_t>(elementId), ACTIONS_::kCopy,
                              {});
      break;
    }
    /** Paste action for text content. 2048 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_PASTE: {
      FML_DLOG(INFO) << "ExecuteAccessibilityAction -> action: paste(" << action
                     << ")";
      DispatchSemanticsAction(static_cast<int32_t>(elementId), ACTIONS_::kPaste,
                              {});
      break;
    }
    /** Cut action for text content. 4096 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CUT:
      FML_DLOG(INFO) << "ExecuteAccessibilityAction -> action: cut(" << action
                     << ")";
      DispatchSemanticsAction(static_cast<int32_t>(elementId), ACTIONS_::kCut,
                              {});
      break;
    /** Text selection action, requiring the setting of <b>selectTextBegin</b>,
     * <b>TextEnd</b>, and <b>TextInForward</b> parameters to select a text
     * segment in the text box. 8192 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT: {
      FML_DLOG(INFO) << "ExecuteAccessibilityAction -> action: select text("
                     << action << ")";
      // 输入框文本选择操作
      PerformSelectText(flutterNode, action, actionArguments);
      break;
    }
    /** Text content setting action. 16384 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_TEXT: {
      FML_DLOG(INFO) << "ExecuteAccessibilityAction -> action: set text("
                     << action << ")";
      // 输入框设置文本
      PerformSetText(flutterNode, action, actionArguments);
      break;
    }
    /** Cursor position setting action. 1048576 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION: {
      FML_DLOG(INFO)
          << "ExecuteAccessibilityAction -> action: set cursor position("
          << action << ")";
      // 当前os接口不支持该功能，不影响正常屏幕朗读
      break;
    }
    /** Invalid action. 0 */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_INVALID: {
      /** Invalid event. 0 */
      ArkUI_AccessibilityEventType invalidEventType =
          ArkUI_AccessibilityEventType::
              ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_INVALID;
      Flutter_SendAccessibilityAsyncEvent(elementId, invalidEventType);
      FML_DLOG(ERROR) << "ExecuteAccessibilityAction -> action: invalid("
                      << action << ")" << " event: innvalid("
                      << invalidEventType << ")";
      break;
    }
    default: {
      /** custom semantics action */
    }
  }

  FML_DLOG(INFO) << "--- ExecuteAccessibilityAction is end ---";
  return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
}

/**
 * Called to obtain element information based on a specified node and text
 * content.
 */
int32_t OhosAccessibilityBridge::FindAccessibilityNodeInfosByText(
    int64_t elementId,
    const char* text,
    int32_t requestId,
    ArkUI_AccessibilityElementInfoList* elementList) {
  FML_DLOG(INFO) << "=== FindAccessibilityNodeInfosByText is end ===";
  return 0;
}
int32_t OhosAccessibilityBridge::FindFocusedAccessibilityNode(
    int64_t elementId,
    ArkUI_AccessibilityFocusType focusType,
    int32_t requestId,
    ArkUI_AccessibilityElementInfo* elementinfo) {
  FML_DLOG(INFO) << "=== FindFocusedAccessibilityNode is end ===";

  return 0;
}
int32_t OhosAccessibilityBridge::FindNextFocusAccessibilityNode(
    int64_t elementId,
    ArkUI_AccessibilityFocusMoveDirection direction,
    int32_t requestId,
    ArkUI_AccessibilityElementInfo* elementList) {
  FML_DLOG(INFO) << "=== FindNextFocusAccessibilityNode is end ===";
  return 0;
}

int32_t OhosAccessibilityBridge::ClearFocusedFocusAccessibilityNode() {
  FML_DLOG(INFO) << "=== ClearFocusedFocusAccessibilityNode is end ===";
  return 0;
}
int32_t OhosAccessibilityBridge::GetAccessibilityNodeCursorPosition(
    int64_t elementId,
    int32_t requestId,
    int32_t* index) {
  FML_DLOG(INFO) << "=== GetAccessibilityNodeCursorPosition is end ===";
  return 0;
}

/**
 * 将arkui的action类型转化为flutter的action类型
 */
flutter::SemanticsAction OhosAccessibilityBridge::ArkuiActionsToFlutterActions(
    ArkUI_Accessibility_ActionType arkui_action) {
  switch (arkui_action) {
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK:
      return ACTIONS_::kTap;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_LONG_CLICK:
      return ACTIONS_::kLongPress;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD:
      return ACTIONS_::kScrollUp;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD:
      return ACTIONS_::kScrollDown;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_COPY:
      return ACTIONS_::kCopy;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CUT:
      return ACTIONS_::kCut;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_GAIN_ACCESSIBILITY_FOCUS:
      return ACTIONS_::kDidGainAccessibilityFocus;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLEAR_ACCESSIBILITY_FOCUS:
      return ACTIONS_::kDidLoseAccessibilityFocus;

    // Text selection action, requiring the setting of <b>selectTextBegin</b>,
    // <b>TextEnd</b>, and <b>TextInForward</b> parameters to select a text
    // segment in the text box. */
    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT:
      return ACTIONS_::kSetSelection;

    case ArkUI_Accessibility_ActionType::
        ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_TEXT:
      return ACTIONS_::kSetText;

    default:
      // might not match to the valid action in arkui
      return ACTIONS_::kCustomAction;
  }
}

/**
 * 自定义无障碍异步事件发送
 */
void OhosAccessibilityBridge::Flutter_SendAccessibilityAsyncEvent(
    int64_t elementId,
    ArkUI_AccessibilityEventType eventType)
{
  if (provider_ == nullptr) {
    FML_DLOG(ERROR) << "Flutter_SendAccessibilityAsyncEvent "
                       "AccessibilityProvider = nullptr";
    return;
  }
  // 1.创建eventInfo对象
  ArkUI_AccessibilityEventInfo* eventInfo =
      OH_ArkUI_CreateAccessibilityEventInfo();
  if (eventInfo == nullptr) {
    FML_DLOG(ERROR) << "Flutter_SendAccessibilityAsyncEvent "
                       "OH_ArkUI_CreateAccessibilityEventInfo eventInfo = null";
    return;
  }

  // 2.创建的elementinfo并根据对应id的flutternode进行属性初始化
  ArkUI_AccessibilityElementInfo* _elementInfo =
      OH_ArkUI_CreateAccessibilityElementInfo();
  FlutterNodeToElementInfoById(_elementInfo, elementId);

  // 若为获焦事件，则设置当前elementinfo获焦
  if (eventType ==
      ArkUI_AccessibilityEventType::
          ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_ACCESSIBILITY_FOCUSED) {
    ARKUI_ACCESSIBILITY_CALL_CHECK(
        OH_ArkUI_AccessibilityElementInfoSetAccessibilityFocused(_elementInfo,
                                                                 true)
    );
  }

  // 3.设置发送事件，如配置获焦、失焦、点击、滑动事件
  ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityEventSetEventType(eventInfo, eventType));

  // 4.将eventinfo事件和当前elementinfo进行绑定
  ARKUI_ACCESSIBILITY_CALL_CHECK(OH_ArkUI_AccessibilityEventSetElementInfo(eventInfo, _elementInfo));

  // 5.调用接口发送到ohos侧
  auto callback = [](int32_t errorCode) {
    FML_DLOG(INFO)
        << "Flutter_SendAccessibilityAsyncEvent callback-> errorCode ="
        << errorCode;
  };

  // 6.发送event到OH侧
  OH_ArkUI_SendAccessibilityAsyncEvent(provider_, eventInfo, callback);

  // 7.销毁新创建的elementinfo, eventinfo
  OH_ArkUI_DestoryAccessibilityElementInfo(_elementInfo);
  _elementInfo = nullptr;
  OH_ArkUI_DestoryAccessibilityEventInfo(eventInfo);
  eventInfo = nullptr;

  FML_DLOG(INFO)
      << "OhosAccessibilityBridge::Flutter_SendAccessibilityAsyncEvent is end";
  return;
}

/**
 * 设置输入框文本
 */
void OhosAccessibilityBridge::PerformSetText(
    SemanticsNodeExtent flutterNode,
    ArkUI_Accessibility_ActionType action,
    ArkUI_AccessibilityActionArguments* actionArguments)
{
    // 获取特殊动作参数
    auto flutterSetTextAction = ArkuiActionsToFlutterActions(action);
    DispatchSemanticsAction(flutterNode.id, flutterSetTextAction, {});
    flutterNode.value = "debug";
    flutterNode.valueAttributes = {};
}

/**
 * 设置文本选择范畴
 */
void OhosAccessibilityBridge::PerformSelectText(
    SemanticsNodeExtent flutterNode,
    ArkUI_Accessibility_ActionType action,
    ArkUI_AccessibilityActionArguments* actionArguments)
{
    return;
}

/**
 * 获取当前flutter节点的组件类型，并映射为arkui组件
 */
std::string OhosAccessibilityBridge::GetNodeComponentType(
    const SemanticsNodeExtent& node)
{
    if (node.HasFlag(FLAGS_::kIsButton)) {
        return BUTTON_WIDGET_NAME;
    }
    if (node.HasFlag(FLAGS_::kIsTextField)) {
        return EDIT_TEXT_WIDGET_NAME;
    }
    if (node.HasFlag(FLAGS_::kIsMultiline)) {
        return EDIT_MULTILINE_TEXT_WIDGET_NAME;
    }
    if (node.HasFlag(FLAGS_::kIsLink)) {
        return LINK_WIDGET_NAME;
    }
    if (node.HasFlag(FLAGS_::kIsSlider) || node.HasAction(ACTIONS_::kIncrease) ||
        node.HasAction(ACTIONS_::kDecrease)) {
        return SLIDER_WIDGET_NAME;
    }
    if (node.HasFlag(FLAGS_::kIsHeader)) {
        return HEADER_WIDGET_NAME;
    }
    if (node.HasFlag(FLAGS_::kIsImage)) {
       return IMAGE_WIDGET_NAME;
    }
    if (node.HasFlag(FLAGS_::kHasCheckedState)) {
        if (node.HasFlag(FLAGS_::kIsInMutuallyExclusiveGroup)) {
            // arkui没有RadioButton，这里透传为RadioButton
            return RADIO_BUTTON_WIDGET_NAME;
        } else {
            return CHECK_BOX_WIDGET_NAME;
        }
    }
    if (node.HasFlag(FLAGS_::kHasToggledState)) {
        return SWITCH_WIDGET_NAME;
    }
    if (node.HasAction(ACTIONS_::kIncrease) || 
        node.HasAction(ACTIONS_::kDecrease)) {
        return SEEKBAR_WIDGET_NAME;
    }
    if (node.HasFlag(FLAGS_::kHasImplicitScrolling)) {
        return SCROLL_WIDGET_NAME;
    }
    if ((!node.label.empty() || !node.tooltip.empty() || !node.hint.empty())) {
        return TEXT_WIDGET_NAME;
    }
    return OTHER_WIDGET_NAME;
}

/**
 * 判断当前节点是否为textfield文本框
 */
bool OhosAccessibilityBridge::IsTextField(SemanticsNodeExtent flutterNode)
{
    return flutterNode.HasFlag(FLAGS_::kIsTextField);
}
/**
 * 判断当前节点是否为滑动条slider类型
 */
bool OhosAccessibilityBridge::IsSlider(SemanticsNodeExtent flutterNode)
{
    return flutterNode.HasFlag(FLAGS_::kIsSlider);
}
/**
 * 判断当前flutter节点组件是否可点击
 */
bool OhosAccessibilityBridge::IsNodeClickable(
    SemanticsNodeExtent flutterNode)
{
    return flutterNode.HasAction(ACTIONS_::kTap);
}
/**
 * 判断当前flutter节点组件是否可显示
 */
bool OhosAccessibilityBridge::IsNodeVisible(
    SemanticsNodeExtent flutterNode)
{
    return !flutterNode.HasFlag(FLAGS_::kIsHidden);
}
/**
 * 判断当前flutter节点组件是否具备checkable属性
 */
bool OhosAccessibilityBridge::IsNodeCheckable(
    SemanticsNodeExtent flutterNode)
{
    return flutterNode.HasFlag(FLAGS_::kHasCheckedState) ||
           flutterNode.HasFlag(FLAGS_::kHasToggledState);
}
/**
 * 判断当前flutter节点组件是否checked/unchecked（checkbox、radio button）
 */
bool OhosAccessibilityBridge::IsNodeChecked(
    SemanticsNodeExtent flutterNode)
{
    return flutterNode.HasFlag(FLAGS_::kIsChecked) ||
           flutterNode.HasFlag(FLAGS_::kIsToggled);
}
/**
 * 判断当前flutter节点组件是否选中
 */
bool OhosAccessibilityBridge::IsNodeSelected(
    SemanticsNodeExtent flutterNode)
{
    return flutterNode.HasFlag(FLAGS_::kIsSelected);
}
/**
 * 判断当前flutter节点组件是否为密码输入框
 */
bool OhosAccessibilityBridge::IsNodePassword(
    SemanticsNodeExtent flutterNode)
{
    return flutterNode.HasFlag(FLAGS_::kIsTextField) &&
           flutterNode.HasFlag(FLAGS_::kIsObscured);
}
/**
 * 判断当前flutter节点组件是否支持长按功能
 */
bool OhosAccessibilityBridge::IsNodeHasLongPress(
    SemanticsNodeExtent flutterNode)
{
    return flutterNode.HasAction(ACTIONS_::kLongPress);
}
/**
 * 判断当前flutter节点是否enabled
 */
bool OhosAccessibilityBridge::IsNodeEnabled(
    SemanticsNodeExtent flutterNode)
{
    return !flutterNode.HasFlag(FLAGS_::kHasEnabledState) ||
           flutterNode.HasFlag(FLAGS_::kIsEnabled);
}
/**
 * 判断当前flutter节点是否在当前屏幕上显示
 */
bool OhosAccessibilityBridge::IsNodeShowOnScreen(SemanticsNodeExtent flutterNode)
{
  return flutterNode.HasAction(ACTIONS_::kShowOnScreen);
}
/**
 * 判断当前节点是否已经滑动
 */
bool OhosAccessibilityBridge::HasScrolled(
    const SemanticsNodeExtent& flutterNode)
{
    return flutterNode.scrollPosition != std::nan("") &&
           flutterNode.previousScrollPosition != std::nan("") &&
           flutterNode.previousScrollPosition != flutterNode.scrollPosition;
}
/**
 * 判断当前节点是否改变标签文本
 */
bool OhosAccessibilityBridge::HasChangedLabel(const SemanticsNodeExtent& flutterNode)
{
    if (flutterNode.label.empty() && flutterNode.previousLabel.empty()) {
        return false;
    }
    return flutterNode.label.empty() || 
           flutterNode.previousLabel.empty() ||
           flutterNode.label != flutterNode.previousLabel;
}
/**
 * 判断当前语义节点是否可获焦
 */
bool OhosAccessibilityBridge::IsNodeFocusable(
    const SemanticsNodeExtent& node) {
  if (node.HasFlag(FLAGS_::kScopesRoute)) {
    return false;
  }
  if (node.HasFlag(FLAGS_::kIsFocusable)) {
    return true;
  }
  // Always consider platform views focusable.
  if (node.IsPlatformViewNode()) {
    return true;
  }
  // Always consider actionable nodes focusable.
  if (node.actions != 0) {
    return true;
  }
  if ((node.flags & FOCUSABLE_FLAGS) != 0) {
    return true;
  }
  if ((node.actions & ~FOCUSABLE_FLAGS) != 0) {
    return true;
  }
  // Consider text nodes focusable.
  return !node.label.empty() || !node.value.empty() || !node.hint.empty();
}
/**
 * 判断当前flutter节点是否获焦
 */
bool OhosAccessibilityBridge::IsNodeFocused(const SemanticsNodeExtent& flutterNode)
{
    return flutterNode.HasFlag(FLAGS_::kIsFocused);
}
/**
 * 判断是否可滑动
 */
bool OhosAccessibilityBridge::IsNodeScrollable(
    SemanticsNodeExtent flutterNode)
{
  return flutterNode.HasAction(ACTIONS_::kScrollLeft) ||
         flutterNode.HasAction(ACTIONS_::kScrollRight) ||
         flutterNode.HasAction(ACTIONS_::kScrollUp) ||
         flutterNode.HasAction(ACTIONS_::kScrollDown);
}
/**
 * 判断当前节点组件是否是滑动组件，如: listview, gridview等
 */
bool OhosAccessibilityBridge::IsScrollableWidget(
    SemanticsNodeExtent flutterNode)
{
  return flutterNode.HasFlag(FLAGS_::kHasImplicitScrolling);
}

void OhosAccessibilityBridge::AddRouteNodes(
    std::vector<SemanticsNodeExtent> edges,
    SemanticsNodeExtent node) {
  if (node.HasFlag(FLAGS_::kScopesRoute)) {
    edges.emplace_back(node);
  }
  for (auto& childNodeId : node.childrenInTraversalOrder) {
    auto childNode = GetFlutterSemanticsNode(childNodeId);
    AddRouteNodes(edges, childNode);
  }
}

std::string OhosAccessibilityBridge::GetRouteName(SemanticsNodeExtent node) {
  if (node.HasFlag(FLAGS_::kNamesRoute) && !node.label.empty()) {
    return node.label;
  }
  for (auto& childNodeId : node.childrenInTraversalOrder) {
    auto childNode = GetFlutterSemanticsNode(childNodeId);
    std::string newName = GetRouteName(childNode);
    if (!newName.empty()) {
      return newName;
    }
  }
  return "";
}

void OhosAccessibilityBridge::onWindowNameChange(SemanticsNodeExtent route) {
  std::string routeName = GetRouteName(route);
  if (routeName.empty()) {
    routeName = " ";
  }
  Flutter_SendAccessibilityAsyncEvent(
      static_cast<int64_t>(route.id),
      ArkUI_AccessibilityEventType::
          ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_PAGE_CONTENT_UPDATE);
}

void OhosAccessibilityBridge::removeSemanticsNode(
    SemanticsNodeExtent nodeToBeRemoved) {
  if (!g_flutterSemanticsTree.size()) {
    FML_DLOG(ERROR) << "OhosAccessibilityBridge::removeSemanticsNode -> "
                       "g_flutterSemanticsTree.szie()=0";
    return;
  }
  if (g_flutterSemanticsTree.find(nodeToBeRemoved.id) ==
      g_flutterSemanticsTree.end()) {
    FML_DLOG(INFO) << "Attempted to remove a node that is not in the tree.";
  }
  int32_t nodeToBeRemovedParentId = GetParentId(nodeToBeRemoved.id);
  for (auto it = g_parentChildIdVec.begin(); it != g_parentChildIdVec.end(); it++) {
    if (it->first == nodeToBeRemovedParentId &&
        it->second == nodeToBeRemoved.id) {
      g_parentChildIdVec.erase(it);
    }
  }
}

/**
 * when the system accessibility service is shut down,
 * clear all the flutter semantics-relevant caches like maps, vectors
 */
void OhosAccessibilityBridge::ClearFlutterSemanticsCaches() {
  g_flutterSemanticsTree.clear();
  g_parentChildIdVec.clear();
  g_screenRectMap.clear();
  g_actions_mp.clear();
  g_flutterNavigationVec.clear();
}

/**
 * 更新扩展版flutter语义节点
 */
SemanticsNodeExtent OhosAccessibilityBridge::UpdatetSemanticsNodeExtent(
    flutter::SemanticsNode node)
{
  SemanticsNodeExtent nodeEx = SemanticsNodeExtent();
  // 获取更新前的flutter节点信息
  if (g_flutterSemanticsTree.size() > 0) {
    auto prevNode = GetFlutterSemanticsNode(node.id);
    nodeEx.hadPreviousConfig = true;
    nodeEx.previousFlags = prevNode.flags;
    nodeEx.previousActions = prevNode.actions;
    nodeEx.previousTextSelectionBase = prevNode.textSelectionBase;
    nodeEx.previousTextSelectionExtent = prevNode.textSelectionExtent;
    nodeEx.previousScrollPosition = prevNode.scrollPosition;
    nodeEx.previousScrollExtentMax = prevNode.scrollExtentMax;
    nodeEx.previousScrollExtentMin = prevNode.scrollExtentMin;
    nodeEx.previousValue = prevNode.value;
    nodeEx.previousLabel = prevNode.label;
  }
  // 更新当前flutter节点信息
  nodeEx.isNull = false;
  nodeEx.id = std::move(node.id);
  nodeEx.flags = std::move(node.flags);
  nodeEx.actions = std::move(node.actions);
  nodeEx.maxValueLength = std::move(node.maxValueLength);
  nodeEx.currentValueLength = std::move(node.currentValueLength);
  nodeEx.textSelectionBase = std::move(node.textSelectionBase);
  nodeEx.textSelectionExtent = std::move(node.textSelectionExtent);
  nodeEx.platformViewId = std::move(node.platformViewId);
  nodeEx.scrollChildren = std::move(node.scrollChildren);
  nodeEx.scrollIndex = std::move(node.scrollIndex);
  nodeEx.scrollPosition = std::move(node.scrollPosition);
  nodeEx.scrollExtentMax = std::move(node.scrollExtentMax);
  nodeEx.scrollExtentMin = std::move(node.scrollExtentMin);
  nodeEx.elevation = std::move(node.elevation);
  nodeEx.thickness = std::move(node.thickness);
  nodeEx.label = std::move(node.label);
  nodeEx.labelAttributes = std::move(node.labelAttributes);
  nodeEx.hint = std::move(node.hint);
  nodeEx.hintAttributes = std::move(node.hintAttributes);
  nodeEx.value = std::move(node.value);
  nodeEx.valueAttributes = std::move(node.valueAttributes);
  nodeEx.increasedValue = std::move(node.increasedValue);
  nodeEx.increasedValueAttributes = std::move(node.increasedValueAttributes);
  nodeEx.decreasedValue = std::move(node.decreasedValue);
  nodeEx.decreasedValueAttributes = std::move(node.decreasedValueAttributes);
  nodeEx.tooltip = std::move(node.tooltip);
  nodeEx.textDirection = std::move(node.textDirection);
  nodeEx.rect = std::move(node.rect);
  nodeEx.transform = std::move(node.transform);
  nodeEx.childrenInTraversalOrder = std::move(node.childrenInTraversalOrder);
  nodeEx.childrenInHitTestOrder = std::move(node.childrenInHitTestOrder);
  nodeEx.customAccessibilityActions =
      std::move(node.customAccessibilityActions);
  return nodeEx;
}

void OhosAccessibilityBridge::GetSemanticsNodeDebugInfo(
    SemanticsNodeExtent node) {
  FML_DLOG(INFO) << "-------------------SemanticsNode------------------";
  SkMatrix _transform = node.transform.asM33();
  FML_DLOG(INFO) << "node.id=" << node.id;
  FML_DLOG(INFO) << "node.label=" << node.label;
  FML_DLOG(INFO) << "node.previousLabel=" << node.previousLabel;
  FML_DLOG(INFO) << "node.tooltip=" << node.tooltip;
  FML_DLOG(INFO) << "node.hint=" << node.hint;
  FML_DLOG(INFO) << "node.flags=" << node.flags;
  FML_DLOG(INFO) << "node.previousFlags=" << node.previousFlags;
  FML_DLOG(INFO) << "node.actions=" << node.actions;
  FML_DLOG(INFO) << "node.previousActions=" << node.previousActions;
  FML_DLOG(INFO) << "node.value=" << node.value;
  FML_DLOG(INFO) << "node.previousValue=" << node.previousValue;
  FML_DLOG(INFO) << "node.rect= {" << node.rect.fLeft << ", " << node.rect.fTop
                 << ", " << node.rect.fRight << ", " << node.rect.fBottom
                 << "}";
  FML_DLOG(INFO) << "node.transform -> kMScaleX="
                 << _transform.get(SkMatrix::kMScaleX);
  FML_DLOG(INFO) << "node.transform -> kMSkewX="
                 << _transform.get(SkMatrix::kMSkewX);
  FML_DLOG(INFO) << "node.transform -> kMTransX="
                 << _transform.get(SkMatrix::kMTransX);
  FML_DLOG(INFO) << "node.transform -> kMSkewY="
                 << _transform.get(SkMatrix::kMSkewY);
  FML_DLOG(INFO) << "node.transform -> kMScaleY="
                 << _transform.get(SkMatrix::kMScaleY);
  FML_DLOG(INFO) << "node.transform -> kMTransY="
                 << _transform.get(SkMatrix::kMTransY);
  FML_DLOG(INFO) << "node.transform -> kMPersp0="
                 << _transform.get(SkMatrix::kMPersp0);
  FML_DLOG(INFO) << "node.transform -> kMPersp1="
                 << _transform.get(SkMatrix::kMPersp1);
  FML_DLOG(INFO) << "node.transform -> kMPersp2="
                 << _transform.get(SkMatrix::kMPersp2);
  FML_DLOG(INFO) << "node.maxValueLength=" << node.maxValueLength;
  FML_DLOG(INFO) << "node.currentValueLength=" << node.currentValueLength;
  FML_DLOG(INFO) << "node.textSelectionBase=" << node.textSelectionBase;
  FML_DLOG(INFO) << "node.previousTextSelectionBase=" << node.previousTextSelectionBase;
  FML_DLOG(INFO) << "node.textSelectionExtent=" << node.textSelectionExtent;
  FML_DLOG(INFO) << "node.previousTextSelectionExtent=" << node.previousTextSelectionExtent;
  FML_DLOG(INFO) << "node.platformViewId=" << node.platformViewId;
  FML_DLOG(INFO) << "node.scrollChildren=" << node.scrollChildren;
  FML_DLOG(INFO) << "node.scrollIndex=" << node.scrollIndex;
  FML_DLOG(INFO) << "node.scrollPosition=" << node.scrollPosition;
  FML_DLOG(INFO) << "node.previousScrollPosition=" << node.previousScrollPosition;
  FML_DLOG(INFO) << "node.scrollIndex=" << node.scrollIndex;
  FML_DLOG(INFO) << "node.scrollExtentMax=" << node.scrollExtentMax;
  FML_DLOG(INFO) << "node.previousScrollExtentMax=" << node.previousScrollExtentMax;
  FML_DLOG(INFO) << "node.scrollExtentMin=" << node.scrollExtentMin;
  FML_DLOG(INFO) << "node.previousScrollExtentMin=" << node.previousScrollExtentMin;
  FML_DLOG(INFO) << "node.elevation=" << node.elevation;
  FML_DLOG(INFO) << "node.thickness=" << node.thickness;
  FML_DLOG(INFO) << "node.textDirection=" << node.textDirection;
  FML_DLOG(INFO) << "node.childrenInTraversalOrder.size()="
                 << node.childrenInTraversalOrder.size();
  for (uint32_t i = 0; i < node.childrenInTraversalOrder.size(); i++) {
    FML_DLOG(INFO) << "node.childrenInTraversalOrder[" << i
                   << "]=" << node.childrenInTraversalOrder[i];
  }
  FML_DLOG(INFO) << "node.childrenInHitTestOrder.size()="
                 << node.childrenInHitTestOrder.size();
  for (uint32_t i = 0; i < node.childrenInHitTestOrder.size(); i++) {
    FML_DLOG(INFO) << "node.childrenInHitTestOrder[" << i
                   << "]=" << node.childrenInHitTestOrder[i];
  }
  FML_DLOG(INFO) << "node.customAccessibilityActions.size()="
                 << node.customAccessibilityActions.size();
  for (uint32_t i = 0; i < node.customAccessibilityActions.size(); i++) {
    FML_DLOG(INFO) << "node.customAccessibilityActions[" << i
                   << "]=" << node.customAccessibilityActions[i];
  }
  FML_DLOG(INFO) << "------------------SemanticsNode-----------------";
}

void OhosAccessibilityBridge::GetSemanticsFlagsDebugInfo(
    SemanticsNodeExtent node) {
  FML_DLOG(INFO) << "----------------SemanticsFlags-------------------------";
  FML_DLOG(INFO) << "node.id=" << node.id;
  FML_DLOG(INFO) << "node.label=" << node.label;
  FML_DLOG(INFO) << "kHasCheckedState: "
                 << node.HasFlag(FLAGS_::kHasCheckedState);
  FML_DLOG(INFO) << "kIsChecked:" << node.HasFlag(FLAGS_::kIsChecked);
  FML_DLOG(INFO) << "kIsSelected:" << node.HasFlag(FLAGS_::kIsSelected);
  FML_DLOG(INFO) << "kIsButton:" << node.HasFlag(FLAGS_::kIsButton);
  FML_DLOG(INFO) << "kIsTextField:" << node.HasFlag(FLAGS_::kIsTextField);
  FML_DLOG(INFO) << "kIsFocused:" << node.HasFlag(FLAGS_::kIsFocused);
  FML_DLOG(INFO) << "kHasEnabledState:"
                 << node.HasFlag(FLAGS_::kHasEnabledState);
  FML_DLOG(INFO) << "kIsEnabled:" << node.HasFlag(FLAGS_::kIsEnabled);
  FML_DLOG(INFO) << "kIsInMutuallyExclusiveGroup:"
                 << node.HasFlag(FLAGS_::kIsInMutuallyExclusiveGroup);
  FML_DLOG(INFO) << "kIsHeader:" << node.HasFlag(FLAGS_::kIsHeader);
  FML_DLOG(INFO) << "kIsObscured:" << node.HasFlag(FLAGS_::kIsObscured);
  FML_DLOG(INFO) << "kScopesRoute:" << node.HasFlag(FLAGS_::kScopesRoute);
  FML_DLOG(INFO) << "kNamesRoute:" << node.HasFlag(FLAGS_::kNamesRoute);
  FML_DLOG(INFO) << "kIsHidden:" << node.HasFlag(FLAGS_::kIsHidden);
  FML_DLOG(INFO) << "kIsImage:" << node.HasFlag(FLAGS_::kIsImage);
  FML_DLOG(INFO) << "kIsLiveRegion:" << node.HasFlag(FLAGS_::kIsLiveRegion);
  FML_DLOG(INFO) << "kHasToggledState:"
                 << node.HasFlag(FLAGS_::kHasToggledState);
  FML_DLOG(INFO) << "kIsToggled:" << node.HasFlag(FLAGS_::kIsToggled);
  FML_DLOG(INFO) << "kHasImplicitScrolling:"
                 << node.HasFlag(FLAGS_::kHasImplicitScrolling);
  FML_DLOG(INFO) << "kIsMultiline:" << node.HasFlag(FLAGS_::kIsMultiline);
  FML_DLOG(INFO) << "kIsReadOnly:" << node.HasFlag(FLAGS_::kIsReadOnly);
  FML_DLOG(INFO) << "kIsFocusable:" << node.HasFlag(FLAGS_::kIsFocusable);
  FML_DLOG(INFO) << "kIsLink:" << node.HasFlag(FLAGS_::kIsLink);
  FML_DLOG(INFO) << "kIsSlider:" << node.HasFlag(FLAGS_::kIsSlider);
  FML_DLOG(INFO) << "kIsKeyboardKey:" << node.HasFlag(FLAGS_::kIsKeyboardKey);
  FML_DLOG(INFO) << "kIsCheckStateMixed:"
                 << node.HasFlag(FLAGS_::kIsCheckStateMixed);
  FML_DLOG(INFO) << "----------------SemanticsFlags--------------------";
}

void OhosAccessibilityBridge::GetCustomActionDebugInfo(
    flutter::CustomAccessibilityAction customAccessibilityAction) {
  FML_DLOG(INFO) << "--------------CustomAccessibilityAction------------";
  FML_DLOG(INFO) << "customAccessibilityAction.id="
                 << customAccessibilityAction.id;
  FML_DLOG(INFO) << "customAccessibilityAction.overrideId="
                 << customAccessibilityAction.overrideId;
  FML_DLOG(INFO) << "customAccessibilityAction.label="
                 << customAccessibilityAction.label;
  FML_DLOG(INFO) << "customAccessibilityAction.hint="
                 << customAccessibilityAction.hint;
  FML_DLOG(INFO) << "------------CustomAccessibilityAction--------------";
}
}  // namespace flutter
