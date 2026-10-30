// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

// ignore_for_file: invalid_use_of_internal_member
// ignore_for_file: implementation_imports

import 'dart:convert' show utf8;
import 'dart:ffi' hide Size;
import 'dart:io';
import 'dart:ui' show Display, FlutterView;

import 'package:flutter/foundation.dart';
import 'package:flutter/rendering.dart';

import '../foundation/_features.dart';
import '_window.dart';
import '_window_positioner.dart';
import 'binding.dart';

// Do not import this file in production applications or packages published
// to pub.dev. Flutter will make breaking changes to this file, even in patch
// versions.
//
// All APIs in this file must be private or must:
//
// 1. Have the `@internal` attribute.
// 2. Throw an `UnsupportedError` if `isWindowingEnabled`
//    is `false`.

const String _kWindowingDisabledErrorMessage = '''
Windowing APIs are not enabled.

Windowing APIs are currently experimental. Do not use windowing APIs in
production applications or plugins published to pub.dev.

To try experimental windowing APIs:
1. Switch to Flutter's main release channel.
2. Turn on the windowing feature flag.

See: https://github.com/flutter/flutter/issues/30701.
''';

const int _kNoParentViewId = -1;

/// Resolves the [FlutterView] for a freshly created window's view id.
///
/// A negative viewId means native creation failed; a view missing from
/// `PlatformDispatcher.views` is also a contract failure — throw descriptive
/// errors instead of an opaque `StateError: No element`.
FlutterView _viewForCreatedWindow(int viewId, String kind) {
  if (viewId < 0) {
    throw StateError(
      '$kind: native window creation failed — no live windowing controller, '
      'or the engine refused to add the view.',
    );
  }
  return WidgetsBinding.instance.platformDispatcher.views.firstWhere(
    (FlutterView view) => view.viewId == viewId,
    orElse: () => throw StateError(
      '$kind: view $viewId was created but never appeared in '
      'PlatformDispatcher.views.',
    ),
  );
}

/// [WindowingOwner] implementation for OHOS; throws [UnsupportedError] if
/// [Platform.isOhos] is false.
///
/// {@macro flutter.widgets.windowing.experimental}
class WindowingOwnerOHOS extends WindowingOwner {
  /// Creates a new [WindowingOwnerOHOS] instance.
  ///
  /// {@macro flutter.widgets.windowing.experimental}
  @internal
  WindowingOwnerOHOS() {
    if (!isWindowingEnabled) {
      throw UnsupportedError(_kWindowingDisabledErrorMessage);
    }

    if (!Platform.isOhos) {
      throw UnsupportedError('Only available on the OHOS platform');
    }

    assert(
      WidgetsBinding.instance.platformDispatcher.engineId != null,
      'WindowingOwnerOHOS must be created after the engine has been initialized.',
    );
  }

  @override
  RegularWindowController createRegularWindowController({
    required RegularWindowControllerDelegate delegate,
    Size? preferredSize,
    BoxConstraints? preferredConstraints,
    String? title,
    bool decorated = true,
  }) {
    final controller = RegularWindowControllerOHOS(
      owner: this,
      delegate: delegate,
      preferredSize: preferredSize,
      preferredConstraints: preferredConstraints,
      title: title,
      decorated: decorated,
    );
    return controller;
  }

  @override
  DialogWindowController createDialogWindowController({
    required DialogWindowControllerDelegate delegate,
    Size? preferredSize,
    BoxConstraints? preferredConstraints,
    BaseWindowController? parent,
    String? title,
    bool decorated = true,
  }) {
    final controller = DialogWindowControllerOHOS(
      owner: this,
      delegate: delegate,
      preferredSize: preferredSize,
      preferredConstraints: preferredConstraints,
      parent: parent,
      title: title,
      decorated: decorated,
    );
    return controller;
  }

  @internal
  @override
  TooltipWindowController createTooltipWindowController({
    required TooltipWindowControllerDelegate delegate,
    required BoxConstraints preferredConstraints,
    required Rect anchorRect,
    required WindowPositioner positioner,
    required BaseWindowController parent,
  }) {
    final controller = TooltipWindowControllerOHOS(
      owner: this,
      delegate: delegate,
      contentSizeConstraints: preferredConstraints,
      anchorRect: anchorRect,
      positioner: positioner,
      parent: parent,
    );
    return controller;
  }

  @internal
  @override
  PopupWindowController createPopupWindowController({
    required PopupWindowControllerDelegate delegate,
    required BoxConstraints preferredConstraints,
    required Rect anchorRect,
    required WindowPositioner positioner,
    required BaseWindowController parent,
  }) {
    final controller = PopupWindowControllerOHOS(
      owner: this,
      delegate: delegate,
      contentSizeConstraints: preferredConstraints,
      parent: parent,
      anchorRect: anchorRect,
      positioner: positioner,
    );
    return controller;
  }

  final List<_WindowControllerMixin> _activeControllers = <_WindowControllerMixin>[];

  /// Returns the window handle for the given [view].
  static Pointer<Void> getWindowHandle(FlutterView view) {
    return _OHOSPlatformInterface.getWindowHandle(
      PlatformDispatcher.instance.engineId!,
      view.viewId,
    );
  }

  @internal
  @override
  SatelliteWindowController createSatelliteWindowController({
    required SatelliteWindowControllerDelegate delegate,
    required BaseWindowController parent,
    required WindowPositioner initialPositioner,
    Rect? initialAnchorRect,
    Size? preferredSize,
    BoxConstraints? preferredConstraints,
    String? title,
  }) {
    throw UnimplementedError('Satellite windows are not yet implemented on OHOS.');
  }
}

mixin _WindowControllerMixin {
  void _initController(WindowingOwnerOHOS owner) {
    if (!isWindowingEnabled) {
      throw UnsupportedError(_kWindowingDisabledErrorMessage);
    }

    _onShouldClose = NativeCallable<Void Function()>.isolateLocal(_handleOnShouldClose);
    _onWillClose = NativeCallable<Void Function()>.isolateLocal(_handleOnWillClose);
    _onNotifyListeners = NativeCallable<Void Function()>.isolateLocal(_handleOnNotifyListeners);
    _onGetWindowPosition =
        NativeCallable<
          Pointer<_Rect> Function(
            Pointer<_Size> childSize,
            Pointer<_Rect> parentRect,
            Pointer<_Rect> outputRect,
          )
        >.isolateLocal(_handleOnGetWindowPosition);
    _owner = owner;
    _owner._activeControllers.add(this);
  }

  void _handleOnShouldClose();

  void _handleOnNotifyListeners();

  @mustCallSuper
  void _handleOnWillClose() {
    _release();
  }

  void _release() {
    _onWillClose.close();
    _onShouldClose.close();
    _onNotifyListeners.close();
    _onGetWindowPosition.close();
    _destroyed = true;
    _owner._activeControllers.remove(this);
  }

  /// Tears down a window that native created but whose Dart-side setup then
  /// failed; a negative [viewId] means native creation itself failed.
  void _destroyCreatedWindow(int viewId) {
    if (viewId < 0) {
      return;
    }
    _OHOSPlatformInterface.destroyWindow(
      _OHOSPlatformInterface.getWindowHandle(
        WidgetsBinding.instance.platformDispatcher.engineId!,
        viewId,
      ),
    );
  }

  @mustCallSuper
  Pointer<_Rect> _handleOnGetWindowPosition(
    Pointer<_Size> childSize,
    Pointer<_Rect> parentRect,
    Pointer<_Rect> outputRect,
  ) {
    return Pointer<_Rect>.fromAddress(0);
  }

  void _ensureNotDestroyed() {
    if (_destroyed) {
      throw StateError('Window has been destroyed.');
    }
  }

  FlutterView get rootView;

  /// Returns window handle for the current window.
  Pointer<Void> getWindowHandle() {
    _ensureNotDestroyed();
    return WindowingOwnerOHOS.getWindowHandle(rootView);
  }

  Size get contentSize {
    _ensureNotDestroyed();
    return _OHOSPlatformInterface.getWindowContentSize(getWindowHandle());
  }

  void destroy() {
    if (_destroyed) {
      return;
    }
    final Pointer<Void> handle = getWindowHandle();
    _OHOSPlatformInterface.destroyWindow(handle);
  }

  bool get destroyed => _destroyed;

  bool _destroyed = false;

  late final NativeCallable<Void Function()> _onShouldClose;
  late final NativeCallable<Void Function()> _onWillClose;
  late final NativeCallable<Void Function()> _onNotifyListeners;
  late final NativeCallable<
    Pointer<_Rect> Function(
      Pointer<_Size> childSize,
      Pointer<_Rect> parentRect,
      Pointer<_Rect> outputRect,
    )
  >
  _onGetWindowPosition;

  late final WindowingOwnerOHOS _owner;
}

/// OHOS specific implementation of [RegularWindowController].
///
/// {@macro flutter.widgets.windowing.experimental}
class RegularWindowControllerOHOS extends RegularWindowController with _WindowControllerMixin {
  /// Creates a new regular window controller for OHOS.
  ///
  /// When this constructor completes the native window has been created and
  /// has a view associated with it.
  RegularWindowControllerOHOS({
    required WindowingOwnerOHOS owner,
    required RegularWindowControllerDelegate delegate,
    required Size? preferredSize,
    BoxConstraints? preferredConstraints,
    String? title,
    bool decorated = true, // ignore: avoid_unused_constructor_parameters
  }) : _delegate = delegate,
       super.empty() {
    _initController(owner);
    final Pointer<NativeFunction<Void Function()>> osc = _onShouldClose.nativeFunction;
    final Pointer<NativeFunction<Void Function()>> owc = _onWillClose.nativeFunction;
    final Pointer<NativeFunction<Void Function()>> onl = _onNotifyListeners.nativeFunction;

    final FlutterView flutterView;
    var viewId = -1;
    try {
      viewId = _OHOSPlatformInterface.createRegularWindow(
        preferredSize: preferredSize,
        preferredConstraints: preferredConstraints,
        title: title,
        onShouldClose: osc,
        onWillClose: owc,
        onNotifyListeners: onl,
      );
      flutterView = _viewForCreatedWindow(viewId, 'RegularWindowControllerOHOS');
    } catch (_) {
      _destroyCreatedWindow(viewId);
      _release();
      rethrow;
    }
    rootView = flutterView;
  }

  @override
  void _handleOnShouldClose() {
    try {
      _delegate.onWindowCloseRequested(this);
    } catch (error, stackTrace) {
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: error,
          stack: stackTrace,
          library: 'windowing',
          context: ErrorDescription('onWindowCloseRequested threw; close request not granted'),
        ),
      );
    }
  }

  @override
  void _handleOnWillClose() {
    super._handleOnWillClose();
    _delegate.onWindowDestroyed();
  }

  @override
  void _handleOnNotifyListeners() {
    notifyListeners();
  }

  @override
  @internal
  void setSize(Size size) {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.setWindowContentSize(getWindowHandle(), size);
  }

  @override
  @internal
  void setConstraints(BoxConstraints constraints) {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.setWindowConstraints(getWindowHandle(), constraints);
  }

  @override
  void setTitle(String title) {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.setWindowTitle(getWindowHandle(), title);
    notifyListeners();
  }

  @override
  Size get contentSize {
    _ensureNotDestroyed();
    return _OHOSPlatformInterface.getWindowContentSize(getWindowHandle());
  }

  @override
  void activate() {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.activate(getWindowHandle());
  }

  @override
  void setMaximized(bool maximized) {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.setWindowMaximized(getWindowHandle(), maximized);
    _isMaximized = maximized;
    if (maximized) {
      // Maximize clears minimized (mutually exclusive window states).
      _isMinimized = false;
    }
    notifyListeners();
  }

  @override
  bool get isMaximized => _isMaximized;

  @override
  void setMinimized(bool minimized) {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.setWindowMinimized(getWindowHandle(), minimized);
    _isMinimized = minimized;
    if (minimized) {
      _isMaximized = false;
    }
    notifyListeners();
  }

  @override
  bool get isMinimized => _isMinimized;

  @override
  void setFullscreen(bool fullscreen, {Display? display}) {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.setWindowFullscreen(getWindowHandle(), fullscreen);
    _isFullscreen = fullscreen;
    notifyListeners();
  }

  @override
  bool get isFullscreen => _isFullscreen;

  // Mirror of the OS window state: OHOS has no synchronous state query
  // reachable from Dart FFI, so track the last value set.
  bool _isMaximized = false;
  bool _isMinimized = false;
  bool _isFullscreen = false;

  final RegularWindowControllerDelegate _delegate;

  // Native-cached focus flag the ArkTS host pushes (windowStageEvent
  // ACTIVE↔INACTIVE → nativeNotifyWindowActivated); a change also fires the
  // notify_listeners chain so dependents rebuild and re-query this.
  @override
  bool get isActivated => _OHOSPlatformInterface.getWindowActivated(rootView.viewId);

  // The SetTitle echo from the native cache (the chain's only title writer,
  // so the cache is the platform truth); setTitle also notifies listeners.
  @override
  String get title => _OHOSPlatformInterface.getWindowTitle(getWindowHandle());
}

/// OHOS specific implementation of [DialogWindowController].
///
/// {@macro flutter.widgets.windowing.experimental}
class DialogWindowControllerOHOS extends DialogWindowController with _WindowControllerMixin {
  /// Creates a new dialog window controller for OHOS.
  ///
  /// When this constructor completes the native window has been created and
  /// has a view associated with it.
  DialogWindowControllerOHOS({
    required WindowingOwnerOHOS owner,
    required DialogWindowControllerDelegate delegate,
    required Size? preferredSize,
    this.parent,
    BoxConstraints? preferredConstraints,
    String? title,
    bool decorated = true, // ignore: avoid_unused_constructor_parameters
  }) : _delegate = delegate,
       super.empty() {
    _initController(owner);

    final FlutterView flutterView;
    var viewId = -1;
    try {
      viewId = _OHOSPlatformInterface.createDialogWindow(
        preferredSize: preferredSize,
        preferredConstraints: preferredConstraints,
        parentViewId: parent?.rootView.viewId,
        title: title,
        onShouldClose: _onShouldClose.nativeFunction,
        onWillClose: _onWillClose.nativeFunction,
        onNotifyListeners: _onNotifyListeners.nativeFunction,
      );
      flutterView = _viewForCreatedWindow(viewId, 'DialogWindowControllerOHOS');
    } catch (_) {
      // Same creation-failure containment as RegularWindowControllerOHOS.
      _destroyCreatedWindow(viewId);
      _release();
      rethrow;
    }
    rootView = flutterView;
  }

  @override
  final BaseWindowController? parent;

  @override
  void _handleOnShouldClose() {
    try {
      _delegate.onWindowCloseRequested(this);
    } catch (error, stackTrace) {
      // Same FFI-boundary containment as RegularWindowControllerOHOS.
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: error,
          stack: stackTrace,
          library: 'windowing',
          context: ErrorDescription('onWindowCloseRequested threw; close request not granted'),
        ),
      );
    }
  }

  @override
  void _handleOnWillClose() {
    super._handleOnWillClose();
    _delegate.onWindowDestroyed();
  }

  @override
  void _handleOnNotifyListeners() {
    notifyListeners();
  }

  @override
  @internal
  void setSize(Size size) {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.setWindowContentSize(getWindowHandle(), size);
  }

  @override
  @internal
  void setConstraints(BoxConstraints constraints) {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.setWindowConstraints(getWindowHandle(), constraints);
  }

  @override
  void setTitle(String title) {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.setWindowTitle(getWindowHandle(), title);
    notifyListeners();
  }

  @override
  Size get contentSize {
    _ensureNotDestroyed();
    return _OHOSPlatformInterface.getWindowContentSize(getWindowHandle());
  }

  @override
  void activate() {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.activate(getWindowHandle());
  }

  // Same native focus-flag query as RegularWindowControllerOHOS; sub-window
  // hosts push via windowEvent WINDOW_ACTIVE/WINDOW_INACTIVE.
  @override
  bool get isActivated => _OHOSPlatformInterface.getWindowActivated(rootView.viewId);

  @override
  bool get isMinimized => _isMinimized;

  @override
  void setMinimized(bool minimized) {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.setWindowMinimized(getWindowHandle(), minimized);
    _isMinimized = minimized;
    notifyListeners();
  }

  bool _isMinimized = false;

  // Same SetTitle-echo cache as RegularWindowControllerOHOS.title.
  @override
  String get title => _OHOSPlatformInterface.getWindowTitle(getWindowHandle());

  final DialogWindowControllerDelegate _delegate;
}

/// OHOS specific implementation of [TooltipWindowController].
///
/// {@macro flutter.widgets.windowing.experimental}
class TooltipWindowControllerOHOS extends TooltipWindowController with _WindowControllerMixin {
  /// Creates a new tooltip window controller for OHOS.
  TooltipWindowControllerOHOS({
    required WindowingOwnerOHOS owner,
    required TooltipWindowControllerDelegate delegate,
    required BoxConstraints contentSizeConstraints,
    required BaseWindowController parent,
    required Rect anchorRect,
    required WindowPositioner positioner,
  }) : _delegate = delegate,
       _parent = parent,
       _anchorRect = anchorRect,
       _positioner = positioner,
       super.empty() {
    _initController(owner);

    final FlutterView flutterView;
    var viewId = -1;
    try {
      viewId = _OHOSPlatformInterface.createTooltipWindow(
        preferredConstraints: contentSizeConstraints,
        onShouldClose: _onShouldClose.nativeFunction,
        onWillClose: _onWillClose.nativeFunction,
        onNotifyListeners: _onNotifyListeners.nativeFunction,
        onGetWindowPosition: _onGetWindowPosition.nativeFunction,
        parentViewId: parent.rootView.viewId,
      );
      flutterView = _viewForCreatedWindow(viewId, 'TooltipWindowControllerOHOS');
    } catch (_) {
      // Same creation-failure containment as RegularWindowControllerOHOS.
      _destroyCreatedWindow(viewId);
      _release();
      rethrow;
    }
    rootView = flutterView;
  }

  final TooltipWindowControllerDelegate _delegate;
  final BaseWindowController _parent;
  Rect _anchorRect;
  WindowPositioner _positioner;

  @override
  BaseWindowController get parent => _parent;

  @override
  @internal
  void setConstraints(BoxConstraints constraints) {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.setWindowConstraints(getWindowHandle(), constraints);
  }

  @override
  void updatePosition({Rect? anchorRect, WindowPositioner? positioner}) {
    if (anchorRect != null) {
      _anchorRect = anchorRect;
    }
    if (positioner != null) {
      _positioner = positioner;
    }
    // Store first, push after (win32 parity): the ETS side re-runs the
    // positioner, whose callback reads the values stored above.
    _OHOSPlatformInterface.updateTooltipWindowPosition(getWindowHandle());
  }

  @override
  Pointer<_Rect> _handleOnGetWindowPosition(
    Pointer<_Size> childSize,
    Pointer<_Rect> parentRect,
    Pointer<_Rect> outputRect,
  ) {
    super._handleOnGetWindowPosition(childSize, parentRect, outputRect);
    final Pointer<_Rect> result = _allocator.allocate<_Rect>(sizeOf<_Rect>());
    try {
      final double scale = rootView.devicePixelRatio == 0 ? 1.0 : rootView.devicePixelRatio;
      final Rect scaledAnchorRect = Rect.fromLTWH(
        _anchorRect.left * scale,
        _anchorRect.top * scale,
        _anchorRect.width * scale,
        _anchorRect.height * scale,
      );
      final WindowPositioner scaledPositioner = _positioner.copyWith(
        offset: _positioner.offset * scale,
      );
      final Rect targetRect = scaledPositioner.placeWindow(
        childSize: childSize.ref.toSize(),
        anchorRect: scaledAnchorRect.translate(parentRect.ref.left, parentRect.ref.top),
        parentRect: parentRect.ref.toRect(),
        displayRect: outputRect.ref.toRect(),
      );
      result.ref.left = targetRect.left;
      result.ref.top = targetRect.top;
      result.ref.width = targetRect.width;
      result.ref.height = targetRect.height;
    } catch (error, stackTrace) {
      _allocator.free(result);
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: error,
          stack: stackTrace,
          library: 'windowing',
          context: ErrorDescription(
            'WindowPositioner.placeWindow threw; keeping default placement',
          ),
        ),
      );
      return Pointer<_Rect>.fromAddress(0);
    }
    return result;
  }

  @override
  void _handleOnShouldClose() {
    try {
      _delegate.onWindowCloseRequested(this);
    } catch (error, stackTrace) {
      // Called synchronously from C++ via an isolateLocal NativeCallable;
      // an escaping exception would cross the FFI boundary as undefined
      // behavior. Report and keep the window open (request NOT granted).
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: error,
          stack: stackTrace,
          library: 'windowing',
          context: ErrorDescription('onWindowCloseRequested threw; close request not granted'),
        ),
      );
    }
  }

  @override
  void _handleOnWillClose() {
    super._handleOnWillClose();
    _delegate.onWindowDestroyed();
  }

  @override
  void _handleOnNotifyListeners() {
    notifyListeners();
  }
}

/// OHOS specific implementation of [PopupWindowController].
///
/// {@macro flutter.widgets.windowing.experimental}
class PopupWindowControllerOHOS extends PopupWindowController with _WindowControllerMixin {
  /// Creates a new popup window controller for OHOS.
  PopupWindowControllerOHOS({
    required WindowingOwnerOHOS owner,
    required PopupWindowControllerDelegate delegate,
    required BoxConstraints contentSizeConstraints,
    required BaseWindowController parent,
    required Rect anchorRect,
    required WindowPositioner positioner,
  }) : _delegate = delegate,
       _parent = parent,
       _anchorRect = anchorRect,
       _positioner = positioner,
       super.empty() {
    _initController(owner);

    final FlutterView flutterView;
    var viewId = -1;
    try {
      viewId = _OHOSPlatformInterface.createPopupWindow(
        preferredConstraints: contentSizeConstraints,
        onShouldClose: _onShouldClose.nativeFunction,
        onWillClose: _onWillClose.nativeFunction,
        onNotifyListeners: _onNotifyListeners.nativeFunction,
        onGetWindowPosition: _onGetWindowPosition.nativeFunction,
        parentViewId: parent.rootView.viewId,
      );
      flutterView = _viewForCreatedWindow(viewId, 'PopupWindowControllerOHOS');
    } catch (_) {
      // Same creation-failure containment as RegularWindowControllerOHOS.
      _destroyCreatedWindow(viewId);
      _release();
      rethrow;
    }
    rootView = flutterView;
  }

  final PopupWindowControllerDelegate _delegate;
  final BaseWindowController _parent;
  Rect _anchorRect;
  WindowPositioner _positioner;

  // isActivated/activate() deliberately keep the BASE PopupWindowController
  // semantics (walk to the closest regular/dialog parent, _window.dart): the
  // OHOS popup host is permanently non-focusable
  // (AnchoredWindowHost.prepareShow → setWindowFocusable(false)), so the
  // embedding never updates the popup's own activation flag — reading it
  // would pin isActivated to false — and showWindow() cannot hand keyboard
  // focus to a non-focusable window. The contract's keyboard-input path
  // ("parent active + focus node inside the popup focused") runs through
  // the active parent, which activate() raises. macOS's popup controller
  // keeps the base semantics for the same reason.

  @override
  BaseWindowController get parent => _parent;

  // Anchored content offset inside the parked host window (native memo,
  // PHYSICAL px) → logical via the view's DPR. Zero for a not-yet-laid-out
  // or non-anchored window.
  @override
  Offset get offsetFromParent =>
      _OHOSPlatformInterface.getWindowOffsetFromParent(getWindowHandle()) /
      (rootView.devicePixelRatio == 0 ? 1.0 : rootView.devicePixelRatio);

  @override
  @internal
  void setConstraints(BoxConstraints constraints) {
    _ensureNotDestroyed();
    _OHOSPlatformInterface.setWindowConstraints(getWindowHandle(), constraints);
  }

  @override
  void updatePosition({Rect? anchorRect, WindowPositioner? positioner}) {
    if (anchorRect != null) {
      _anchorRect = anchorRect;
    }
    if (positioner != null) {
      _positioner = positioner;
    }
    _OHOSPlatformInterface.updatePopupWindowPosition(getWindowHandle());
  }

  @override
  Pointer<_Rect> _handleOnGetWindowPosition(
    Pointer<_Size> childSize,
    Pointer<_Rect> parentRect,
    Pointer<_Rect> outputRect,
  ) {
    super._handleOnGetWindowPosition(childSize, parentRect, outputRect);
    final Pointer<_Rect> result = _allocator.allocate<_Rect>(sizeOf<_Rect>());
    try {
      final double scale = rootView.devicePixelRatio == 0 ? 1.0 : rootView.devicePixelRatio;
      final Rect scaledAnchorRect = Rect.fromLTWH(
        _anchorRect.left * scale,
        _anchorRect.top * scale,
        _anchorRect.width * scale,
        _anchorRect.height * scale,
      );
      final WindowPositioner scaledPositioner = _positioner.copyWith(
        offset: _positioner.offset * scale,
      );
      final Rect targetRect = scaledPositioner.placeWindow(
        childSize: childSize.ref.toSize(),
        anchorRect: scaledAnchorRect.translate(parentRect.ref.left, parentRect.ref.top),
        parentRect: parentRect.ref.toRect(),
        displayRect: outputRect.ref.toRect(),
      );
      result.ref.left = targetRect.left;
      result.ref.top = targetRect.top;
      result.ref.width = targetRect.width;
      result.ref.height = targetRect.height;
    } catch (error, stackTrace) {
      _allocator.free(result);
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: error,
          stack: stackTrace,
          library: 'windowing',
          context: ErrorDescription(
            'WindowPositioner.placeWindow threw; keeping default placement',
          ),
        ),
      );
      return Pointer<_Rect>.fromAddress(0);
    }
    return result;
  }

  @override
  void _handleOnShouldClose() {
    try {
      _delegate.onWindowCloseRequested(this);
    } catch (error, stackTrace) {
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: error,
          stack: stackTrace,
          library: 'windowing',
          context: ErrorDescription('onWindowCloseRequested threw; close request not granted'),
        ),
      );
    }
  }

  @override
  void _handleOnWillClose() {
    super._handleOnWillClose();
    _delegate.onWindowDestroyed();
  }

  @override
  void _handleOnNotifyListeners() {
    notifyListeners();
  }
}

// Native platform interface: binds the `InternalFlutter_Window*` C symbols
// exported by `windowing/ohos_window_controller.cpp`. `libflutter.so` is
// loaded RTLD_LOCAL, so the default `@Native` resolution cannot find its
// symbols — `lookupFunction` against a direct `DynamicLibrary` handle.

// Native (C) signatures for the windowing entry points; Dart requires
// top-level typedefs.
typedef _GetWindowHandleNative = Pointer<Void> Function(Int64, Int64);
typedef _DestroyWindowNative = Void Function(Int64, Pointer<Void>);
typedef _GetContentSizeNative = _Size Function(Pointer<Void>);
typedef _SetContentSizeNative = Void Function(Pointer<Void>, Pointer<_Size>);
typedef _SetConstraintsNative = Void Function(Pointer<Void>, Pointer<_Constraints>);
typedef _SetTitleNative = Void Function(Pointer<Void>, Pointer<_Utf8>);
typedef _ActivateNative = Void Function(Pointer<Void>);
typedef _UpdateTooltipPositionNative = Void Function(Pointer<Void>);
typedef _UpdatePopupPositionNative = Void Function(Pointer<Void>);
typedef _SetMaximizedNative = Void Function(Pointer<Void>, Bool);
typedef _SetMinimizedNative = Void Function(Pointer<Void>, Bool);
typedef _SetFullscreenNative = Void Function(Pointer<Void>, Bool);
typedef _GetActivatedNative = Bool Function(Int64, Int64);
typedef _GetOffsetFromParentNative = Void Function(Pointer<Void>, Pointer<_Size>);
typedef _GetTitleNative = Void Function(Pointer<Void>, Pointer<_Utf8>, Int64);
typedef _WindowingSupportedNative = Bool Function();

typedef _CreateRegularWindowNative = Int64 Function(Int64, Pointer<_RegularWindowCreationRequest>);
typedef _CreateDialogWindowNative = Int64 Function(Int64, Pointer<_DialogWindowCreationRequest>);
typedef _CreateTooltipWindowNative = Int64 Function(Int64, Pointer<_TooltipWindowCreationRequest>);
typedef _CreatePopupWindowNative = Int64 Function(Int64, Pointer<_PopupWindowCreationRequest>);

/// Whether the OHOS multi-window owner can be created on this device: true
/// only on HarmonyOS PC (`2in1`); elsewhere the factories throw like any
/// unsupported platform.
@internal
bool get ohosWindowingSupported => _OHOSPlatformInterface.windowingSupported;

class _OHOSPlatformInterface {
  static DynamicLibrary get _flutterLib {
    return _lib ??= DynamicLibrary.open('libflutter.so');
  }

  static DynamicLibrary? _lib;

  /// Cached lookup of `OHOS_WindowingSupported`; true only on PC form factors.
  static final bool Function() _windowingSupportedPtr = _flutterLib
      .lookupFunction<_WindowingSupportedNative, bool Function()>('OHOS_WindowingSupported');

  static bool get windowingSupported => _windowingSupportedPtr();

  static final Pointer<Void> Function(int, int) _getWindowHandlePtr = _flutterLib
      .lookupFunction<_GetWindowHandleNative, Pointer<Void> Function(int, int)>(
        'InternalFlutter_Window_GetHandle',
      );

  static Pointer<Void> getWindowHandle(int engineId, int viewId) =>
      _getWindowHandlePtr(engineId, viewId);

  static final int Function(int, Pointer<_RegularWindowCreationRequest>) _createRegularWindowPtr =
      _flutterLib.lookupFunction<
        _CreateRegularWindowNative,
        int Function(int, Pointer<_RegularWindowCreationRequest>)
      >('InternalFlutter_WindowController_CreateRegularWindow');

  static int createRegularWindow({
    required Size? preferredSize,
    BoxConstraints? preferredConstraints,
    String? title,
    required Pointer<NativeFunction<Void Function()>> onShouldClose,
    required Pointer<NativeFunction<Void Function()>> onWillClose,
    required Pointer<NativeFunction<Void Function()>> onNotifyListeners,
  }) {
    final Pointer<_Utf8> titlePointer = title?.toNativeUtf8() ?? nullptr;
    final Pointer<_RegularWindowCreationRequest> request =
        _allocator<_RegularWindowCreationRequest>();
    request.ref
      ..preferredSize.from(preferredSize)
      ..preferredConstraints.from(preferredConstraints)
      ..title = titlePointer
      ..callbacks.onShouldClose = onShouldClose
      ..callbacks.onWillClose = onWillClose
      ..callbacks.onNotifyListeners = onNotifyListeners;

    try {
      final int viewId = _createRegularWindowPtr(
        WidgetsBinding.instance.platformDispatcher.engineId!,
        request,
      );
      return viewId;
    } finally {
      _free(request);
      if (titlePointer != nullptr) {
        _free(titlePointer);
      }
    }
  }

  static final int Function(int, Pointer<_DialogWindowCreationRequest>) _createDialogWindowPtr =
      _flutterLib.lookupFunction<
        _CreateDialogWindowNative,
        int Function(int, Pointer<_DialogWindowCreationRequest>)
      >('InternalFlutter_WindowController_CreateDialogWindow');

  static int createDialogWindow({
    required Size? preferredSize,
    BoxConstraints? preferredConstraints,
    int? parentViewId,
    String? title,
    required Pointer<NativeFunction<Void Function()>> onShouldClose,
    required Pointer<NativeFunction<Void Function()>> onWillClose,
    required Pointer<NativeFunction<Void Function()>> onNotifyListeners,
  }) {
    final Pointer<_Utf8> titlePointer = title?.toNativeUtf8() ?? nullptr;
    final Pointer<_DialogWindowCreationRequest> request =
        _allocator<_DialogWindowCreationRequest>();
    request.ref
      ..preferredSize.from(preferredSize)
      ..preferredConstraints.from(preferredConstraints)
      ..parentViewId = parentViewId ?? _kNoParentViewId
      ..title = titlePointer
      ..callbacks.onShouldClose = onShouldClose
      ..callbacks.onWillClose = onWillClose
      ..callbacks.onNotifyListeners = onNotifyListeners;

    try {
      final int viewId = _createDialogWindowPtr(
        WidgetsBinding.instance.platformDispatcher.engineId!,
        request,
      );
      return viewId;
    } finally {
      _free(request);
      if (titlePointer != nullptr) {
        _free(titlePointer);
      }
    }
  }

  static final int Function(int, Pointer<_TooltipWindowCreationRequest>) _createTooltipWindowPtr =
      _flutterLib.lookupFunction<
        _CreateTooltipWindowNative,
        int Function(int, Pointer<_TooltipWindowCreationRequest>)
      >('InternalFlutter_WindowController_CreateTooltipWindow');
  static final int Function(int, Pointer<_PopupWindowCreationRequest>) _createPopupWindowPtr =
      _flutterLib.lookupFunction<
        _CreatePopupWindowNative,
        int Function(int, Pointer<_PopupWindowCreationRequest>)
      >('InternalFlutter_WindowController_CreatePopupWindow');

  static int createTooltipWindow({
    BoxConstraints? preferredConstraints,
    required Pointer<NativeFunction<Void Function()>> onShouldClose,
    required Pointer<NativeFunction<Void Function()>> onWillClose,
    required Pointer<NativeFunction<Void Function()>> onNotifyListeners,
    required Pointer<
      NativeFunction<Pointer<_Rect> Function(Pointer<_Size>, Pointer<_Rect>, Pointer<_Rect>)>
    >
    onGetWindowPosition,
    required int parentViewId,
  }) {
    final Pointer<_TooltipWindowCreationRequest> request =
        _allocator<_TooltipWindowCreationRequest>();
    request.ref
      ..preferredConstraints.from(preferredConstraints)
      ..parentViewId = parentViewId
      ..onGetWindowPosition = onGetWindowPosition
      ..callbacks.onShouldClose = onShouldClose
      ..callbacks.onWillClose = onWillClose
      ..callbacks.onNotifyListeners = onNotifyListeners;

    try {
      final int viewId = _createTooltipWindowPtr(
        WidgetsBinding.instance.platformDispatcher.engineId!,
        request,
      );
      return viewId;
    } finally {
      _free(request);
    }
  }

  static int createPopupWindow({
    BoxConstraints? preferredConstraints,
    required Pointer<NativeFunction<Void Function()>> onShouldClose,
    required Pointer<NativeFunction<Void Function()>> onWillClose,
    required Pointer<NativeFunction<Void Function()>> onNotifyListeners,
    required Pointer<
      NativeFunction<Pointer<_Rect> Function(Pointer<_Size>, Pointer<_Rect>, Pointer<_Rect>)>
    >
    onGetWindowPosition,
    required int parentViewId,
  }) {
    final Pointer<_PopupWindowCreationRequest> request = _allocator<_PopupWindowCreationRequest>();
    request.ref
      ..preferredConstraints.from(preferredConstraints)
      ..parentViewId = parentViewId
      ..onGetWindowPosition = onGetWindowPosition
      ..callbacks.onShouldClose = onShouldClose
      ..callbacks.onWillClose = onWillClose
      ..callbacks.onNotifyListeners = onNotifyListeners;

    try {
      final int viewId = _createPopupWindowPtr(
        WidgetsBinding.instance.platformDispatcher.engineId!,
        request,
      );
      return viewId;
    } finally {
      _free(request);
    }
  }

  static final void Function(int, Pointer<Void>) _destroyWindowPtr = _flutterLib
      .lookupFunction<_DestroyWindowNative, void Function(int, Pointer<Void>)>(
        'InternalFlutter_Window_Destroy',
      );

  static void destroyWindow(Pointer<Void> windowHandle) {
    _destroyWindowPtr(WidgetsBinding.instance.platformDispatcher.engineId!, windowHandle);
  }

  static final _GetContentSizeNative _getWindowContentSizePtr = _flutterLib
      .lookupFunction<_GetContentSizeNative, _Size Function(Pointer<Void>)>(
        'InternalFlutter_Window_GetContentSize',
      );

  static Size getWindowContentSize(Pointer<Void> windowHandle) {
    final _Size size = _getWindowContentSizePtr(windowHandle);
    return Size(size.width, size.height);
  }

  static final void Function(Pointer<Void>, Pointer<_Size>) _setWindowContentSizePtr = _flutterLib
      .lookupFunction<_SetContentSizeNative, void Function(Pointer<Void>, Pointer<_Size>)>(
        'InternalFlutter_Window_SetContentSize',
      );

  static void setWindowContentSize(Pointer<Void> windowHandle, Size size) {
    final Pointer<_Size> ffiSize = _allocator<_Size>()
      ..ref.width = size.width
      ..ref.height = size.height;
    _setWindowContentSizePtr(windowHandle, ffiSize);
    _free(ffiSize);
  }

  static final void Function(Pointer<Void>, Pointer<_Constraints>) _setWindowConstraintsPtr =
      _flutterLib.lookupFunction<
        _SetConstraintsNative,
        void Function(Pointer<Void>, Pointer<_Constraints>)
      >('InternalFlutter_Window_SetConstraints');

  static void setWindowConstraints(Pointer<Void> windowHandle, BoxConstraints constraints) {
    final Pointer<_Constraints> ffiConstraints = _allocator<_Constraints>()
      ..ref.hasViewConstraints = true
      ..ref.minWidth = constraints.minWidth
      ..ref.minHeight = constraints.minHeight
      ..ref.maxWidth = constraints.maxWidth
      ..ref.maxHeight = constraints.maxHeight;
    _setWindowConstraintsPtr(windowHandle, ffiConstraints);
    _free(ffiConstraints);
  }

  static final void Function(Pointer<Void>, Pointer<_Utf8>) _setWindowTitlePtr = _flutterLib
      .lookupFunction<_SetTitleNative, void Function(Pointer<Void>, Pointer<_Utf8>)>(
        'InternalFlutter_Window_SetTitle',
      );

  static void setWindowTitle(Pointer<Void> windowHandle, String title) {
    final Pointer<_Utf8> titlePointer = title.toNativeUtf8();
    _setWindowTitlePtr(windowHandle, titlePointer);
    _free(titlePointer);
  }

  static final void Function(Pointer<Void>) _activatePtr = _flutterLib
      .lookupFunction<_ActivateNative, void Function(Pointer<Void>)>(
        'InternalFlutter_Window_Activate',
      );

  static void activate(Pointer<Void> windowHandle) => _activatePtr(windowHandle);

  // Dart updatePosition push (win32 parity symbol
  // InternalFlutterWindows_WindowManager_UpdateTooltipPosition): native
  // re-runs the Dart positioner (which reads the anchorRect/positioner
  // stored just before) and repositions the ArkTS sub-window.
  static final void Function(Pointer<Void>) _updateTooltipPositionPtr = _flutterLib
      .lookupFunction<_UpdateTooltipPositionNative, void Function(Pointer<Void>)>(
        'InternalFlutter_Window_UpdateTooltipPosition',
      );

  static void updateTooltipWindowPosition(Pointer<Void> windowHandle) =>
      _updateTooltipPositionPtr(windowHandle);

  static final void Function(Pointer<Void>) _updatePopupPositionPtr = _flutterLib
      .lookupFunction<_UpdatePopupPositionNative, void Function(Pointer<Void>)>(
        'InternalFlutter_Window_UpdatePopupPosition',
      );

  static void updatePopupWindowPosition(Pointer<Void> windowHandle) =>
      _updatePopupPositionPtr(windowHandle);

  // Route to the InternalFlutter_Window_Set{Maximized,Minimized,Fullscreen}
  // FFI symbols, which dispatch to the ETS host window APIs.
  static final void Function(Pointer<Void>, bool) _setWindowMaximizedPtr = _flutterLib
      .lookupFunction<_SetMaximizedNative, void Function(Pointer<Void>, bool)>(
        'InternalFlutter_Window_SetMaximized',
      );

  static void setWindowMaximized(Pointer<Void> windowHandle, bool maximized) =>
      _setWindowMaximizedPtr(windowHandle, maximized);

  static final void Function(Pointer<Void>, bool) _setWindowMinimizedPtr = _flutterLib
      .lookupFunction<_SetMinimizedNative, void Function(Pointer<Void>, bool)>(
        'InternalFlutter_Window_SetMinimized',
      );

  static void setWindowMinimized(Pointer<Void> windowHandle, bool minimized) =>
      _setWindowMinimizedPtr(windowHandle, minimized);

  static final void Function(Pointer<Void>, bool) _setWindowFullscreenPtr = _flutterLib
      .lookupFunction<_SetFullscreenNative, void Function(Pointer<Void>, bool)>(
        'InternalFlutter_Window_SetFullscreen',
      );

  static void setWindowFullscreen(Pointer<Void> windowHandle, bool fullscreen) =>
      _setWindowFullscreenPtr(windowHandle, fullscreen);

  static final bool Function(int, int) _getWindowActivatedPtr = _flutterLib
      .lookupFunction<_GetActivatedNative, bool Function(int, int)>(
        'InternalFlutter_Window_GetActivated',
      );

  static bool getWindowActivated(int viewId) =>
      _getWindowActivatedPtr(WidgetsBinding.instance.platformDispatcher.engineId!, viewId);

  // Anchored (tooltip/popup) content offset inside its host window. The FFI
  // writes PHYSICAL px; callers divide by the view's devicePixelRatio for
  // the logical Offset the framework reports.
  static final void Function(Pointer<Void>, Pointer<_Size>) _getOffsetFromParentPtr = _flutterLib
      .lookupFunction<_GetOffsetFromParentNative, void Function(Pointer<Void>, Pointer<_Size>)>(
        'InternalFlutter_Window_GetOffsetFromParent',
      );

  static Offset getWindowOffsetFromParent(Pointer<Void> windowHandle) {
    final Pointer<_Size> ffiOffset = _allocator<_Size>();
    _getOffsetFromParentPtr(windowHandle, ffiOffset);
    final result = Offset(ffiOffset.ref.width, ffiOffset.ref.height);
    _free(ffiOffset);
    return result;
  }

  // Cached window title — the SetTitle echo (SetTitle is the only writer in
  // the chain, so the native cache is the platform truth). Native writes at
  // most `capacity - 1` bytes + NUL; longer titles truncate.
  static const int _kTitleCapacity = 512;

  static final void Function(Pointer<Void>, Pointer<_Utf8>, int) _getTitlePtr = _flutterLib
      .lookupFunction<_GetTitleNative, void Function(Pointer<Void>, Pointer<_Utf8>, int)>(
        'InternalFlutter_Window_GetTitle',
      );

  static String getWindowTitle(Pointer<Void> windowHandle) {
    final Pointer<_Utf8> buffer = _allocator<Uint8>(_kTitleCapacity).cast<_Utf8>();
    try {
      _getTitlePtr(windowHandle, buffer, _kTitleCapacity);
      return buffer.toDartString();
    } finally {
      _free(buffer);
    }
  }
}

// FFI structs: field order and types must match the C structs in
// `windowing/ohos_window_types.h` byte-for-byte.

final class _Size extends Struct {
  @Double()
  external double width;

  @Double()
  external double height;

  @override
  String toString() {
    return 'Size(width: $width, height: $height)';
  }

  Size toSize() {
    return Size(width, height);
  }
}

final class _Rect extends Struct {
  @Double()
  external double left;

  @Double()
  external double top;

  @Double()
  external double width;

  @Double()
  external double height;

  Rect toRect() {
    return Rect.fromLTWH(left, top, width, height);
  }

  @override
  String toString() {
    return 'Rect(left: $left, top: $top, width: $width, height: $height)';
  }
}

final class _Constraints extends Struct {
  // Mirrors C++ `WindowConstraints::has_view_constraints` (leading bool +
  // 7 bytes padding); the runtime SetConstraints path always applies
  // constraints, so the flag is written as true.
  @Bool()
  external bool hasViewConstraints;

  @Double()
  external double minWidth;

  @Double()
  external double minHeight;

  @Double()
  external double maxWidth;

  @Double()
  external double maxHeight;
}

// FFI request structs: field order and types must match the C structs in
// `windowing/ohos_window_types.h` byte-for-byte. The data fields mirror
// win32's layouts; `callbacks` trails them as the OHOS-specific addition
// (no per-window OS message pump here), keeping the create entry points'
// signatures identical to win32's `(engineId, request)`.

// Payload for the creation method used by
// [_OHOSPlatformInterface.createRegularWindow].
final class _RegularWindowCreationRequest extends Struct {
  external _WindowSizeRequest preferredSize;
  external _WindowConstraintsRequest preferredConstraints;
  external Pointer<_Utf8> title;
  external _WindowLifecycleCallbacks callbacks;
}

// Payload for the creation method used by
// [_OHOSPlatformInterface.createDialogWindow].
final class _DialogWindowCreationRequest extends Struct {
  external _WindowSizeRequest preferredSize;
  external _WindowConstraintsRequest preferredConstraints;
  external Pointer<_Utf8> title;

  @Int64()
  external int parentViewId;

  external _WindowLifecycleCallbacks callbacks;
}

// Payload for the creation method used by
// [_OHOSPlatformInterface.createTooltipWindow]. No preferred size (win32
// parity): tooltips are sized to their content constraints.
final class _TooltipWindowCreationRequest extends Struct {
  external _WindowConstraintsRequest preferredConstraints;

  @Int64()
  external int parentViewId;

  external Pointer<
    NativeFunction<
      Pointer<_Rect> Function(
        Pointer<_Size> childSize,
        Pointer<_Rect> parentRect,
        Pointer<_Rect> outputRect,
      )
    >
  >
  onGetWindowPosition;

  external _WindowLifecycleCallbacks callbacks;
}

// Payload for the creation method used by
// [_OHOSPlatformInterface.createPopupWindow]. No preferred size (mirrors the
// tooltip layout; win32 has no popup counterpart).
final class _PopupWindowCreationRequest extends Struct {
  external _WindowConstraintsRequest preferredConstraints;

  @Int64()
  external int parentViewId;

  external Pointer<
    NativeFunction<
      Pointer<_Rect> Function(
        Pointer<_Size> childSize,
        Pointer<_Rect> parentRect,
        Pointer<_Rect> outputRect,
      )
    >
  >
  onGetWindowPosition;

  external _WindowLifecycleCallbacks callbacks;
}

final class _WindowSizeRequest extends Struct {
  @Bool()
  external bool hasSize;

  @Double()
  external double width;

  @Double()
  external double height;

  void from(Size? size) {
    hasSize = size != null;
    width = size?.width ?? 0;
    height = size?.height ?? 0;
  }
}

final class _WindowConstraintsRequest extends Struct {
  @Bool()
  external bool hasConstraints;

  @Double()
  external double minWidth;

  @Double()
  external double minHeight;

  @Double()
  external double maxWidth;

  @Double()
  external double maxHeight;

  void from(BoxConstraints? constraints) {
    hasConstraints = constraints != null;
    minWidth = constraints?.minWidth ?? 0;
    minHeight = constraints?.minHeight ?? 0;
    maxWidth = constraints?.maxWidth ?? double.maxFinite;
    maxHeight = constraints?.maxHeight ?? double.maxFinite;
  }
}

final class _WindowLifecycleCallbacks extends Struct {
  external Pointer<NativeFunction<Void Function()>> onShouldClose;
  external Pointer<NativeFunction<Void Function()>> onWillClose;
  external Pointer<NativeFunction<Void Function()>> onNotifyListeners;
}

// FFI utilities: libc calloc/free allocator and `_Utf8` string conversion.

// libc calloc/free is globally visible in the process image, so resolve it
// against DynamicLibrary.process() rather than a named library.
typedef _PosixCallocNative = Pointer<Void> Function(IntPtr num, IntPtr size);

final Pointer<Void> Function(int, int) _posixCalloc = DynamicLibrary.process()
    .lookupFunction<_PosixCallocNative, Pointer<Void> Function(int, int)>('calloc');

typedef _PosixFreeNative = Void Function(Pointer<NativeType>);

final void Function(Pointer<NativeType>) _posixFree = DynamicLibrary.process()
    .lookupFunction<_PosixFreeNative, void Function(Pointer<NativeType>)>('free');

const _CallocAllocator _allocator = _CallocAllocator._();

final class _CallocAllocator implements Allocator {
  const _CallocAllocator._();

  /// Allocates [byteCount] bytes of zeroed memory on the native heap.
  @override
  Pointer<T> allocate<T extends NativeType>(int byteCount, {int? alignment}) {
    final Pointer<T> result = _posixCalloc(byteCount, 1).cast();

    if (result.address == 0) {
      throw ArgumentError('Could not allocate $byteCount bytes.');
    }
    return result;
  }

  /// Releases memory allocated on the native heap.
  @override
  void free(Pointer<NativeType> pointer) {
    _posixFree(pointer);
  }
}

// Wrapper kept for call-site readability inside `_OHOSPlatformInterface`.
void _free(Pointer<NativeType> pointer) {
  _allocator.free(pointer);
}

/// The contents of a native zero-terminated array of UTF-8 code units.
final class _Utf8 extends Opaque {}

/// Extension method for converting a `Pointer<Utf8>` to a [String].
extension _Utf8Pointer on Pointer<_Utf8> {
  String toDartString({int? length}) {
    _ensureNotNullptr('toDartString');
    final Pointer<Uint8> codeUnits = cast<Uint8>();
    if (length != null) {
      RangeError.checkNotNegative(length, 'length');
    } else {
      length = _length(codeUnits);
    }
    return utf8.decode(codeUnits.asTypedList(length));
  }

  static int _length(Pointer<Uint8> codeUnits) {
    var length = 0;
    while (codeUnits[length] != 0) {
      length++;
    }
    return length;
  }

  void _ensureNotNullptr(String operation) {
    if (this == nullptr) {
      throw UnsupportedError("Operation '$operation' not allowed on a 'nullptr'.");
    }
  }
}

/// Extension method for converting a [String] to a `Pointer<Utf8>`.
extension _StringUtf8Pointer on String {
  Pointer<_Utf8> toNativeUtf8({Allocator allocator = _allocator}) {
    final Uint8List units = utf8.encode(this);
    final Pointer<Uint8> result = allocator<Uint8>(units.length + 1);
    final Uint8List nativeString = result.asTypedList(units.length + 1);
    nativeString.setAll(0, units);
    nativeString[units.length] = 0;
    return result.cast();
  }
}
