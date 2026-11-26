---
name: chrtouchkit
description: "CHRTouchKit 客户端/业务接入指南（iOS 越狱环境触摸合成、键盘输入、手势回放、App 生命周期观测与启动拦截）。当需要在自己的 Daemon / Tweak / App 中接入 CHRTouchKit.framework，实现点击滑动、文字输入、监听 App 前后台/启动退出通知，或拦截 App 启动（如启动前开 VPN、防误触反悔确认）时使用。"
---

# CHRTouchKit 客户端接入指南

`CHRTouchKit` 为越狱设备上的守护进程、Tweak 与应用提供统一的进程外交互与系统控制 SDK（`#import <CHRTouchKit/CHRTouchKit.h>`）。

## 核心能力模块

| 模块 | 定位与职责 | 核心接口 | 详细文档 |
|---|---|---|---|
| **App 启动拦截器**<br/>(Launch Interceptor) | **强控制面守卫**：在 App 真正启动前拦截决策（前置开 VPN、GUI 弹窗反悔、API 错误透传、责任链一票否决） | `CHRTouchRegisterLaunchInterceptor`<br/>`CHRTouchLaunchResult`<br/>`CHRTouchLaunchApplicationWithError` | [references/launch-interceptor.md](references/launch-interceptor.md) |
| **App 生命周期观测**<br/>(Observer 通知) | **只读状态流**：旁路监听 `didLaunch` / `didBecomeActive` / `didEnterBackground` / `didTerminate` 等 8 个原子事件，零副作用 | `CHRTouchApplicationLifecycleNotification` (`CDTNotificationCenter`)<br/>`CHRTouchRunningApplications` | 见 `CHRTouchManual.h` |
| **触控与按键输入** | 屏幕坐标触摸、滑动、长按、系统键盘文字键入与状态快照 | `CHRTouchTap` / `CHRTouchSwipe`<br/>`CHRTouchTypeText` | 见 `CHRTouchManual.h` |
| **手势录制与回放** | 录制用户手势轨迹、按方案与标签拟人回放、网络库同步 | `CHRTouchPlayback` / `CHRTouchNetworkLibrary` | 见 `CHRTouchPlayback.h` |

## 选型原则：启动拦截 vs 生命周期通知

1. **只看不干预**（埋点、日志、状态同步）→ 订阅 **`CHRTouchApplicationLifecycleNotification`**（广播通知，一对多、无回执、永远不会阻塞系统）。
2. **要拦住或改变启动行为**（启动前必开 VPN、未达标禁止启动、给真人弹反悔确认框）→ 注册 **`CHRTouchRegisterLaunchInterceptor`**（点对点可靠 RPC、支持责任链一票否决与 `return CHRTouchLaunchResult`）。

## 最小接入（xtheos 工程）

- 链接：`-framework CHRTouchKit`（或链接静态库 `libCHRTouchKit.a`）
- deb 依赖：`XTHEOS_PACKAGE_DEPENDS` 加 `com.summer.chrtouchkit`
- 头文件：`#import <CHRTouchKit/CHRTouchKit.h>`
