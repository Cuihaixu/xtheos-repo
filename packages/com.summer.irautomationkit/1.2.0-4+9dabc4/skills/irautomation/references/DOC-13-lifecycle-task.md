---
title: 应用生命周期与任务调度规范
tags: [IRApplication, IRTask, IRAutomationTask, IRTimerTask, 生命周期, 自动化槽位, 线程挂起, 检查点]
keywords: [IRApplication, IRTask, IRAutomationTask, IRTimerTask, setAutomation, addTask, removeTask, interruptGuardEnabled, canRunInState, IRApplicationDelegate, checkpoint, sleep, 线程挂起, 独占槽位, 检查点, 生命周期]
description: IRAutomationKit 目标应用实例连接 (IRApplication)、运行生命周期监听、普通后台任务池、独占 UI 自动化槽位调度 (IRAutomationTask)、整线程挂起/恢复与检查点机制使用规范。
---

# 应用生命周期与任务调度（`IRApplication` / `IRTask` / `IRAutomationTask`）（`DOC-13`）

> 文档编号：`DOC-13`（可在 `skills/` 下用 `rg "DOC-13"` 定位本文档）

本文档说明目标 App 实例连接、生命周期监听、任务体系（普通任务 `IRTask`、定时任务 `IRTimerTask`、UI 自动化任务 `IRAutomationTask`）以及 UI 执行权调度的规范用法。

> [!IMPORTANT]
> **核心原则：UI 自动化任务同一时间、同一应用只有一个独占槽位！**
> - 会操作 UI 的自动化任务（`IRAutomationTask`）**严禁调用 `addTask:`**（会被直接忽略拒收），必须通过 `[app setAutomation:task]` 提交；
> - 提交新自动化会**自动取消旧的**，并等待旧任务真正退出后才启动新任务；连续提交只执行最后一个；
> - 任务随 App 前后台生命周期自动**整线程挂起/恢复**，任务线程退出必须配合检查点（`checkpoint` / `sleep:` / SDK 原子操作）。

---


## 目录索引

- [`[DOC-13.1]` 核心架构图：生命周期与执行权调度流转](#DOC-13.1)
- [`[DOC-13.2]` 一、应用实例与生命周期监听 (`IRApplication`)](#DOC-13.2)
  - [`[DOC-13.2.1]` 1. 怎么拿：单例化获取](#DOC-13.2.1)
  - [`[DOC-13.2.2]` 2. 生命周期代理 (`IRApplicationDelegate`)](#DOC-13.2.2)
- [`[DOC-13.3]` 二、普通任务与定时任务 (`IRTask` / `IRTimerTask`)](#DOC-13.3)
  - [`[DOC-13.3.1]` 1. 普通异步任务 (`IRTask`)](#DOC-13.3.1)
  - [`[DOC-13.3.2]` 2. 定时重复任务 (`IRTimerTask`)](#DOC-13.3.2)
- [`[DOC-13.4]` 三、UI 自动化任务与执行权调度 (`IRAutomationTask`)](#DOC-13.4)
  - [`[DOC-13.4.1]` 1. 提交自动化（独占槽位）](#DOC-13.4.1)
  - [`[DOC-13.4.2]` 2. 独占槽位的调度与替换行为（遇到什么情况该怎么做？）](#DOC-13.4.2)

---

<a id="DOC-13.1"></a>
## `[DOC-13.1]` 核心架构图：生命周期与执行权调度流转

```mermaid
flowchart TD
    subgraph TargetApp["目标 App 进程与生命周期 (IRApplication)"]
        State["app.state:<br>NotRunning / Suspended / Background / Inactive / Active"]
        Queue["app.queue (主串行队列，禁止长时间阻塞)"]
    end

    subgraph TaskDispatcher["任务调度与执行权分层"]
        subgraph BackgroundTasks["普通后台任务池 (app.tasks)"]
            T_Normal["IRTask (通过 addTask: 添加)"]
            T_Timer["IRTimerTask (定时轮询任务)"]
        end

        subgraph ExclusiveSlot["独占 UI 自动化槽位 (app.currentAutomation)"]
            T_Auto["IRAutomationTask (通过 setAutomation: 提交)"]
            AutoThread["自建专用自动化子线程 (独立于 app.queue)"]
            UIRight{"持有 UI 执行权<br>(Active 且无弹窗中断)"}
        end
    end

    subgraph ThreadControl["线程控制与检查点"]
        RunState{"App 处于 Active?"}
        RegionCheck{"在不可挂起区<br>performWithoutSuspension?"}
        SusThread["直接挂起线程 (停在当前行)"]
        ResumeThread["恢复线程 (原地继续)"]
        AbortCheck["检查点抛出 IRAutomationAbortException 正常退出"]
    end

    State -->|状态驱动| RunState
    RunState -->|否 (进后台/被中断)| RegionCheck
    RegionCheck -->|是| AutoThread
    RegionCheck -->|否| SusThread
    RunState -->|是 (回前台)| ResumeThread
    ResumeThread --> AutoThread

    T_Auto --> AutoThread
    AutoThread --> UIRight
    UIRight -->|被取消/替换/App退出| AbortCheck
```

---

<!-- [/DOC-13.1] -->
<a id="DOC-13.2"></a>
## `[DOC-13.2]` 一、应用实例与生命周期监听 (`IRApplication`)

<a id="DOC-13.2.1"></a>
### `[DOC-13.2.1]` 1. 怎么拿：单例化获取

```objc
// 按 BundleID 获取应用实例（同一 BundleID 全局单例；进程重启时实例不变，仅更换内部 session）
IRApplication *app = [IRApplication applicationWithBundleID:@"com.ss.iphone.ugc.Aweme"];

// 常用属性
IRApplicationState state = app.state;          // 当前运行状态 (Active / Inactive / Background...)
pid_t pid                = app.processIdentifier; // 目标进程 PID（未运行为 0）
BOOL isFront             = (state >= IRApplicationStateInactive); // 是否在前台
```

<!-- [/DOC-13.2.1] -->
<a id="DOC-13.2.2"></a>
### `[DOC-13.2.2]` 2. 生命周期代理 (`IRApplicationDelegate`)

回调全部在 `application.queue` 上执行，**严禁在回调中执行长时间阻塞操作或远程调用**：

```objc
@interface MyLifecycleListener : NSObject <IRApplicationDelegate>
@end

@implementation MyLifecycleListener

- (void)applicationDidBecomeActive:(IRApplication *)application {
    // 进入前台可交互状态
}

- (void)applicationDidEnterBackground:(IRApplication *)application {
    // 目标进入后台
}

- (void)applicationDidTerminate:(IRApplication *)application reason:(nullable NSString *)reason {
    // 目标进程退出或崩溃
}

@end

// 注册代理（边沿触发：注册前已存在的状态不补发）
app.delegate = listener;
```

---

<!-- [/DOC-13.2.2] -->
<!-- [/DOC-13.2] -->
<a id="DOC-13.3"></a>
## `[DOC-13.3]` 二、普通任务与定时任务 (`IRTask` / `IRTimerTask`)

用于不操作 UI 的后台观测、数据统计或定时拉取。

<a id="DOC-13.3.1"></a>
### `[DOC-13.3.1]` 1. 普通异步任务 (`IRTask`)

```objc
// 自定义任务子类
@interface MyObserverTask : IRTask
@end

@implementation MyObserverTask

- (BOOL)canRunInState:(IRApplicationState)state {
    // 声明可在哪些 App 状态下运行（默认仅 Active；这里允许前台和后台都跑）
    return (state & (IRApplicationStateActive | IRApplicationStateBackground)) != 0;
}

- (void)main {
    // 在独立任务线程执行，可阻塞；长循环中需检查取消状态
    while (self.state == IRTaskStateRunning) {
        // 执行纯数据处理...
        [NSThread sleepForTimeInterval:1.0];
    }
}

@end

// 添加任务：由应用接管生命周期，结束后自动从 app.tasks 移除
MyObserverTask *task = [[MyObserverTask alloc] initWithName:@"observer"];
task.completionBlock = ^{
    NSLog(@"任务结束");
};
[app addTask:task];

// 取消任务：进入 Cancelled 终态
[app removeTask:task];
```

<!-- [/DOC-13.3.1] -->
<a id="DOC-13.3.2"></a>
### `[DOC-13.3.2]` 2. 定时重复任务 (`IRTimerTask`)

```objc
@interface MyPollTask : IRTimerTask
@end

@implementation MyPollTask

- (void)timerDidFire {
    // 定时触发回调（内部防重叠：上一轮未执行完时会自动合并触发）
    NSLog(@"执行一次后台检查...");
}

@end

// 创建 5 秒一次的重复定时器
MyPollTask *poll = [[MyPollTask alloc] initWithName:@"poll" interval:5.0 repeats:YES];
[app addTask:poll];
```

---

<!-- [/DOC-13.3.2] -->
<!-- [/DOC-13.3] -->
<a id="DOC-13.4"></a>
## `[DOC-13.4]` 三、UI 自动化任务与执行权调度 (`IRAutomationTask`)

所有需要点击、滑动、输入或流转页面的自动化业务，均必须打包成 `IRAutomationTask` 并由独占槽位调度。

<a id="DOC-13.4.1"></a>
### `[DOC-13.4.1]` 1. 提交自动化（独占槽位）

```objc
IRAutomationTask *task = [IRAutomationTask automationWithName:@"feed_flow" block:^(IRAutomationTask *t) {
    // 运行在专用的自动化子线程上（非主线程，非 app.queue，随时可阻塞）
    [app.query.texts(@"推荐").element tap:nil];
    
    // 业务循环：必须使用 t.sleep:，让检查点生效
    for (int i = 0; i < 100; i++) {
        [t sleep:1.5]; // 分段休眠，随时响应取消/中断
        // ... 执行滑动和点击 ...
    }
}];

// 核心规则：必须通过 setAutomation: 提交，严禁调 addTask:！
[app setAutomation:task];
```

<!-- [/DOC-13.4.1] -->
<a id="DOC-13.4.2"></a>
### `[DOC-13.4.2]` 2. 独占槽位的调度与替换行为（遇到什么情况该怎么做？）

- **情况 1：上一个自动化还在跑，我又提交了一个新的自动化？**
  - **框架行为**：框架会自动将当前正在运行的旧任务标记为 `Cancelled`，并在后台等待旧任务真正安全退出；
  - **旧任务退出后**：新任务立即自动启动；
  - **如果在等待旧任务退出期间，又连续提交了多个任务**：框架会自动直接 Cancel 掉中间所有任务，**只保留并启动最后提交的那一个**。
- **情况 2：自动化跑着跑着，用户把 App 切到了后台或拉下了控制中心？**
  - **框架行为**：由于 App 状态变为非 `Active`，框架会**直接在内核层面挂起整个自动化线程**，代码原地冻结；
  - **恢复行为**：当 App 重新回到前台活跃（Active）时，线程原地恢复，任务从暂停的代码行无缝继续往下跑；
  - **特殊保护（不可挂起区）**：如果此时代码正在执行不可中断的底层原子操作（如正在发射一个完整的手势），该操作受到内部保护，会执行完毕后再挂起。
- **情况 3：长循环业务中如何安全响应外部停止/取消？**
  - **不要用原生的 `[NSThread sleepForTimeInterval:]`**，它无法响应取消中断；
  - **正确做法**：一律调用 **`[task sleep:seconds]`**，它内部会按细时间粒度检查取消状态；若没有休眠的长循环，显式调用 **`[task checkpoint]`**；当任务被替换、取消或应用退出时，检查点会自动抛出中止异常安全跳出 `main`。
- **情况 4：当没有自动化任务在跑时，想要让框架自动帮我关弹窗？**
  - **正确做法**：开启独立中断守护开关：
    ```objc
    app.interruptGuardEnabled = YES; // 即使 currentAutomation == nil，依然以 2s 间隔低功耗扫并关系统/应用弹窗
    ```
  - 当有自动化任务运行时，无论该开关为何值，自动化均全速处理中断。
<!-- [/DOC-13.4.2] -->
<!-- [/DOC-13.4] -->
