## 架构

### 废弃 VNDK

### 启用 16KB 页面大小

Android 15 及更高版本支持构建具有 16KB 页面大小的 Android.

配置为使用 16 KB 页面大小的设备平均会使用略多一些的内存，但系统和应用的性能也会得到各种提升：

- 缩短了系统内存压力时的应用启动时间：平均降低了 3.16%；对于我们测试的某些应用而言，改进幅度更大（最高可达 30%）
- 应用启动期间的功耗降低：平均降低了 4.56%
- 相机启动更快：热启动速度平均提高了 4.48%，冷启动速度平均提高了 6.60%
- 缩短了系统启动时间：平均缩短了 8%（约 950 毫秒）

## 相机

### Android camera feature combination query API

从 Android 15 开始，Android 平台提供用于查询相机功能组合的 API。借助此 API，相机客户端可以查询设备是否可支持指定的相机功能组合。

如需了解详情，请参阅[用于查询功能组合的 API](https://source.android.google.cn/docs/core/camera/stream-config?hl=zh-cn#feature-combinations-api)。

### 弱光增强

Android 15 引入了弱光增强功能，这是一种新的自动曝光模式，可在 HAL 的 Camera2 中实现，也可在[相机扩展](https://source.android.google.cn/docs/core/camera/camerax-vendor-extensions?hl=zh-cn)（例如夜间模式）中实现。弱光增强功能会在弱光条件下自动调整预览流的亮度。

## 字体

### 对可变字体的支持

从 Android 15 开始，可变字体会在运行时呈现，并且效率和精细度更高。字体配置文件 [`fonts.xml`](https://cs.android.com/android/platform/superproject/main/+/main:frameworks/base/data/fonts/fonts.xml?hl=zh-cn) 已废弃。

### 废弃 Neural Networks API

从 Android 15 开始，Neural Networks API (NNAPI NDK API) 已被废弃。神经网络 HAL 接口继续受到支持

## 权限

### 平台签名的共享 UID 许可名单

Android 15 引入了明确的许可名单，以便平台签名的非系统应用加入（平台签名的）共享 UID。

## 升级

### Virtual A/B version 3

引入了新版本的 Android 虚拟 A/B 更新机制。这项新功能可实现更快、更小且性能更高的 OTA 更新。

[私密空间](https://developer.android.google.cn/about/versions/15/features?hl=zh-cn#private-space)是 Android 15 中推出的一项新功能，可让用户在设备上创建一个单独的空间，在额外的身份验证层保护下，防止敏感应用遭到窥探。

## 移除了基于 PNG 的表情符号字体

我们移除了基于 PNG 的旧版表情符号字体文件 (`NotoColorEmojiLegacy.ttf`)，只保留了基于矢量的文件。

## 将最低目标 SDK 版本从 23 提高到 24

Android 15 基于 [在 Android 14 中进行的更改](https://developer.android.google.cn/about/versions/14/behavior-changes-all?hl=zh-cn#minimum-target-api-level)，并扩展了 安全性。在 Android 15 中， 无法安装低于 24 的 [`targetSdkVersion`](https://developer.android.google.cn/guide/topics/manifest/uses-sdk-element?hl=zh-cn)。 要求应用符合现代 API 级别有助于确保更好的安全性和 保护隐私。

## 后台网络访问权限限制

在 Android 15 中，如果应用在有效的[进程生命周期](https://developer.android.google.cn/guide/components/activities/process-lifecycle?hl=zh-cn)之外启动网络请求，则会收到异常。通常是 [`UnknownHostException`](https://developer.android.google.cn/reference/java/net/UnknownHostException?hl=zh-cn) 或其他与套接字相关的 `IOException`。在有效生命周期之外发生的网络请求通常是因为应用在不再活跃后，不知不觉地继续发出网络请求。

## Android 15 融入Gemini 模型