---
title: IRAutomationKit 快速开始教程
tags: [快速开始, Hello World, 接入指南, 工程搭建, 自动化任务]
keywords: [IRAutomationKit, IRApplication, IRPageAgent, IRAutomationTask, setAutomation, registerAction, registerAgent, 快速开始, 极简示例, 接入步骤]
description: 从零新建工程、引入 IRAutomationKit、连接目标 App、绑定控制器与注册 Hello World 动作，并在独立 UI 自动化任务中跑通的端到端快速上手指南。
---

# 快速开始：从零建工程到跑通 `hello` 操作（`DOC-16`）

> 文档编号：`DOC-16`（可在 `skills/` 下用 `rg "DOC-16"` 定位本文档）

本指南用最精简的 5 步，演示如何新建工程、接入 `IRAutomationKit`、连接目标应用、绑定控制器并执行第一个 `hello` 自动化操作。
*(若要搭长期运行的完整守护进程架构，另见 **`irautomation-process`** skill。)*

---


## 目录索引

- [`[DOC-16.1]` 第 1 步：创建一个工程](#DOC-16.1)
- [`[DOC-16.2]` 第 2 步：引入自动化库](#DOC-16.2)
- [`[DOC-16.3]` 第 3 步：绑定目标应用与控制器，注册 `hello` 事件](#DOC-16.3)
- [`[DOC-16.4]` 第 4 步：连接目标应用并提交 `hello` 任务](#DOC-16.4)
- [`[DOC-16.5]` 第 5 步：编译安装到设备运行](#DOC-16.5)

---

<a id="DOC-16.1"></a>
## `[DOC-16.1]` 第 1 步：创建一个工程
使用 `xtheos` 创建一个越狱端 CLI 或守护进程 Target，并在 Target 的 `xtheos.entitlements` 中配置自动化所需权限（模板见 `irautomation-process/resources/automation.entitlements`）。

---

<!-- [/DOC-16.1] -->
<a id="DOC-16.2"></a>
## `[DOC-16.2]` 第 2 步：引入自动化库
在工程 Target 的 Build Settings 中完成两项配置：
- **Framework Search Paths**：`~/xtheos/vendor/frameworks`
- **Other Linker Flags**：`-framework IRAutomationKit -framework IRemoteKit -framework CHRTouchKit`
- **Other C Flags / Objective-C ARC**：开启 **`-fobjc-arc-exceptions`**（`IRFlowSession` 异常退出需要）

在源码中引入主头文件：
```objc
#import <IRAutomationKit/IRAutomationKit.h>
```

---

<!-- [/DOC-16.2] -->
<a id="DOC-16.3"></a>
## `[DOC-16.3]` 第 3 步：绑定目标应用与控制器，注册 `hello` 事件
先用 `mpa <目标进程> hitchain` 查看当前页面的原生控制器类名（例如 `HomeViewController`），然后创建一个 `PageAgent` 绑定它并注册 `hello` 原子事件：

```objc
// 1. 声明 PageAgent（正式工程建议先建 App 基类统一写 +bundleIDs）
@interface DemoHomePageAgent : IRPageAgent
@end

@implementation DemoHomePageAgent

+ (NSArray<NSString *> *)bundleIDs {
    return @[ @"com.example.target" ];   // 替换为目标应用 Bundle ID
}

#pragma mark - 识别与注册

+ (void)load {
    // 1. 绑定控制器
    [self registerAgent:^(id<IRPageAgentRegistration> registration) {
        registration.bindClass = @"HomeViewController";   // hitchain 看到的控制器类名
        registration.pageIDs   = @[ @"home" ];
        registration.comment   = @"首页";
    }];

    // 2. 注册 hello 事件
    [self registerPageActions];
}

#pragma mark - 原子事件

+ (void)registerPageActions {
    [self registerAction:IRIdentityMake(@"hello", @"打招呼：点击屏幕中心按钮")
                 builder:^(IRActionBuilder *action) {
        action.perform(^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
            do {
                NSLog(@"[Demo] Hello IRAutomationKit!");
                // 示例：找当前页可见的“确定”按钮并点击
                context.agent.query.axLabels(@"确定").visible().first().element.tap(diag);
                if (diag.hasError) {
                    break;
                }
            } while (NO);
            return diag.noError;
        });
    }];
}

@end
```

---

<!-- [/DOC-16.3] -->
<a id="DOC-16.4"></a>
## `[DOC-16.4]` 第 4 步：连接目标应用并提交 `hello` 任务
在 `main` 或应用入口里获取目标应用对象 `IRApplication`，提交一个 `IRAutomationTask` 触发 `hello`：

```objc
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // 1. 连接目标应用
        IRApplication *app = [IRApplication applicationWithBundleID:@"com.example.target"];

        // 2. 提交自动化任务
        dispatch_semaphore_t done = dispatch_semaphore_create(0);
        IRAutomationTask *task = [IRAutomationTask automationWithName:@"hello_demo"
                                                                block:^(IRAutomationTask *task) {
            IRDiagnostics *diag = [IRDiagnostics diagnosticsWithDomain:@"HelloDemo"];
            [IRFlowSession execute:^(IRFlowSession *session) {
                IRPageNode *home = session.page(@"home");
                home.invoke(@"hello");
            } onException:^(IRFlowException *exception) {
                NSLog(@"[Demo] 执行异常: %@", exception.reason);
                exception.abort();
            } onFinally:^(IRFlowSession *session, IRDiagnostics *flowDiag) {
                if (flowDiag.hasError) {
                    NSLog(@"[Demo] 失败详情: %@", flowDiag.errorDescription());
                }
            } diag:diag];
        }];
        task.completionBlock = ^{
            dispatch_semaphore_signal(done);
        };

        [app setAutomation:task];
        dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
    }
    return 0;
}
```

---

<!-- [/DOC-16.4] -->
<a id="DOC-16.5"></a>
## `[DOC-16.5]` 第 5 步：编译安装到设备运行
确保目标应用处于前台活跃状态后，编译打包并运行即可看到日志输出和点击效果：
- 如果遇到 `hello` 报找不到页面，用 `mpa <目标进程> hitchain` 确认当前控制器类名是否与 `bindClass` 一致。
- 如果想在控制台直接看到 `diag` 过程日志，运行时加环境变量 `IR_DIAG_STDERR=1`（见 [DOC-19-support.md](DOC-19-support.md)）。
<!-- [/DOC-16.5] -->
