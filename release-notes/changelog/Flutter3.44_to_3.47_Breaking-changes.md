# flutter3.44 到 flutter3.47 版本差异总结

从 Flutter 3.44 到 Flutter 3.47 的版本差异主要集中在 OpenGL ES 纹理存储行为、iOS 与 Android 语义节点标题行为以及 foundation 库 API 移除上。开发者需要注意适配。

## 详细如下表所示：

| 序号 | 标题 | 变更来源版本 | 是否需要适配 | 变更指导 |
| :--: | --- | :----------: | :----------: | --- |
| 1 | OpenGL ES render-to-texture 内容改为自顶向下存储 | 3.47 | 否 | [https://docs.flutter.dev/release/breaking-changes/opengles-render-to-texture-top-down](https://docs.flutter.dev/release/breaking-changes/opengles-render-to-texture-top-down) |
| 2 | 更新 iOS 和 Android 的 semantics header 与 headingLevel 行为 | 3.47 | 否 | [https://docs.flutter.dev/release/breaking-changes/semantics-header-heading-level](https://docs.flutter.dev/release/breaking-changes/semantics-header-heading-level) |
| 3 | 移除 describeEnum | 3.47 | 否 | [https://docs.flutter.dev/release/breaking-changes/remove-describeEnum](https://docs.flutter.dev/release/breaking-changes/remove-describeEnum) |
