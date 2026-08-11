# flutter3.41 到 flutter3.44 版本差异总结

从 Flutter 3.41 到 Flutter 3.44 的版本差异主要集中在 API 弃用与重命名、类可见性调整、滚动控件属性变更以及 Android 构建工具链迁移上。开发者需要注意适配。

## 详细如下表所示：

| 序号 | 标题 | 变更来源版本 | 是否需要适配 | 变更指导 |
| :--: | --- | :----------: | :----------: | --- |
| 1 | 调整 RawMenuAnchor 关闭顺序 | 3.44 | 否 | [https://docs.flutter.dev/release/breaking-changes/raw-menu-anchor-close-order](https://docs.flutter.dev/release/breaking-changes/raw-menu-anchor-close-order) |
| 2 | 弃用 onReorder 回调，改用 onReorderItem | 3.44 | 否 | [https://docs.flutter.dev/release/breaking-changes/deprecate-onreorder-callback](https://docs.flutter.dev/release/breaking-changes/deprecate-onreorder-callback) |
| 3 | 弃用 TextInputConnection.setStyle，改用 TextInputConnection.updateStyle | 3.44 | 否 | [https://docs.flutter.dev/release/breaking-changes/deprecate-text-input-connection-set-style](https://docs.flutter.dev/release/breaking-changes/deprecate-text-input-connection-set-style) |
| 4 | 弃用 cacheExtent 和 cacheExtentStyle，改用 scrollCacheExtent、ScrollCacheExtent | 3.44 | 否 | [https://docs.flutter.dev/release/breaking-changes/scroll-cache-extent](https://docs.flutter.dev/release/breaking-changes/scroll-cache-extent) |
| 5 | IconData 类标记为 final | 3.44 | 否 | [https://docs.flutter.dev/release/breaking-changes/icondata-class-marked-final](https://docs.flutter.dev/release/breaking-changes/icondata-class-marked-final) |
| 6 | ListTile 被带颜色的 Widget 包裹时在 debug 模式下报错 | 3.44 | 否 | [https://docs.flutter.dev/release/breaking-changes/list-tile-color-warning](https://docs.flutter.dev/release/breaking-changes/list-tile-color-warning) |
| 7 | Flutter Android 项目迁移到内置 Kotlin | 3.44 | 否 | [https://docs.flutter.dev/release/breaking-changes/migrate-to-built-in-kotlin](https://docs.flutter.dev/release/breaking-changes/migrate-to-built-in-kotlin) |
| 8 | 页面过渡构建器重新组织 | 3.44 | 否 | [https://docs.flutter.dev/release/breaking-changes/decouple-page-transition-builders](https://docs.flutter.dev/release/breaking-changes/decouple-page-transition-builders) |
