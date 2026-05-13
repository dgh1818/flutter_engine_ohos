# flutter3.22 到 flutter3.27 版本差异总结

从 Flutter 3.22 到 Flutter 3.27 的版本差异主要集中在性能优化、新功能引入以及一些框架改进上。开发者需要注意适配。

## 详细如下表所示：

| 序号 | 标题                                          | 变更来源版本 | 是否需要适配 | 变更指导                                                                                                                                                                 |
| :--: | --------------------------------------------- | :----------: | :----------: | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
|  1   | Navigator 页面 API 非兼容性变更               |     3.24     |      是      | [https://docs.flutter.cn/release/breaking-changes/navigator-and-page-api](https://docs.flutter.cn/release/breaking-changes/navigator-and-page-api)                       |
|  2   | 泛型类型 PopScope                             |     3.24     |      是      | [https://docs.flutter.cn/release/breaking-changes/popscope-with-result](https://docs.flutter.cn/release/breaking-changes/popscope-with-result)                           |
|  3   | 弃用 ButtonBar 并支持 OverflowBar             |     3.24     |      是      | [https://docs.flutter.cn/release/breaking-changes/deprecate-buttonbar](https://docs.flutter.cn/release/breaking-changes/deprecate-buttonbar)                             |
|  4   | 为 Android 插件提供新的 API，用于渲染 Surface |     3.24     |      否      | [https://docs.flutter.cn/release/breaking-changes/android-surface-plugins](https://docs.flutter.cn/release/breaking-changes/android-surface-plugins)                     |
|  5   | Color 宽色域支持                              |     3.27     |      否      | [https://docs.flutter.cn/release/breaking-changes/wide-gamut-framework](https://docs.flutter.cn/release/breaking-changes/wide-gamut-framework)                           |
|  6   | 组件主题规范化                                |     3.27     |      是      | [https://docs.flutter.cn/release/breaking-changes/component-theme-normalization](https://docs.flutter.cn/release/breaking-changes/component-theme-normalization)         |
|  7   | 深层链接标志更改                              |     3.27     |      是      | [https://docs.flutter.cn/release/breaking-changes/deep-links-flag-change](https://docs.flutter.cn/release/breaking-changes/deep-links-flag-change)                       |
|  8   | Flutter 中的 Material 3 令牌更新              |     3.27     |      是      | [https://docs.flutter.cn/release/breaking-changes/material-design-3-token-update](https://docs.flutter.cn/release/breaking-changes/material-design-3-token-update)       |
|  9   | 删除无效参数 InputDecoration.collapsed        |     3.27     |      是      | [https://docs.flutter.cn/release/breaking-changes/input-decoration-collapsed](https://docs.flutter.cn/release/breaking-changes/input-decoration-collapsed)               |
|  10  | 将 SystemUiMode 的默认值设置为 Edge-to-Edge   |     3.27     |      否      | [https://docs.flutter.cn/release/breaking-changes/default-systemuimode-edge-to-edge](https://docs.flutter.cn/release/breaking-changes/default-systemuimode-edge-to-edge) |
