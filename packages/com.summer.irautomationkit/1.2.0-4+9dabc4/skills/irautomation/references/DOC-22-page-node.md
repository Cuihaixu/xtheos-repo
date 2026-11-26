---
title: IRPageNode 全量 API 与诊断使用指南
tags: [IRPageNode, IRDiagnostics, diag, clearError, isVisible, backToSelf, loop, repeatUntil, waitUntil, weight, decision, IRDecisionPointRunner]
keywords: [IRPageNode, IRDiagnostics, diag, clearError, isVisible, backToSelf, invoke, invokeQuery, available, loop, repeat, repeatUntil, waitUntil, timeline, decision, IRDecisionPointRunner, weight, weightOrValue, ensure, raise]
description: IRPageNode 全量公开 API 速查、两种构造方式（session.page vs 裸节点）、diag 诊断绑定与 clearError() 铁律、node.decision 决策点调用、Block 属性带括号规则及全部注意事项。
---

# `IRPageNode` 全量 API 与诊断使用指南（`DOC-22`）

> 文档编号：`DOC-22`（可在 `skills/` 下用 `rg "DOC-22"` 定位本文档）

`IRPageNode` 是流程层操作页面的唯一标准句柄，封装了「校验当前页面 → 沿响应链找接收者（本页 → 子控制器 → 外层容器）→ 调动作 → 记录 `diag` 与 `result`」。

---

## 目录索引

- [`[DOC-22.1]` 1. 两种构造方式与适用边界](#DOC-22.1)
  - [`[DOC-22.1.1]` 1.1 `[R-FLOW-05]` 子控制器初始化当前页面节点规范：`session.page(context.page.pageID)`](#DOC-22.1.1)
- [`[DOC-22.2]` 2. 必须掌握的 5 个关键问题（含 `diag` 使用铁律）](#DOC-22.2)
- [`[DOC-22.3]` 3. 全量 API 速查表](#DOC-22.3)

---

<a id="DOC-22.1"></a>
## `[DOC-22.1]` 1. 两种构造方式与适用边界

| 写法 | 适用位置 | 失败行为 |
|---|---|---|
| `session.page(pageID)` | `IRFlowSession` 的 `execute:` 主体块内 | 动作/查询/断言失败时自动抛出 `IRFlowException`，交由 `onException` 统一处理 |
| `IRPageNode(pageID, diag)` | `onException` / `onFinally` 自愈、单步测试、非 Session 场景 | 失败返回 `NO` / `nil`，原因写入绑定的 `diag`（`diag` 传 `nil` 则节点内部自建） |

- **`pageID` 规则**：完全相等匹配；传 `nil` 或 `@""` 表示“任意已识别页面”（若当前屏幕认不出任何已注册页面，一律失败）。
- **页面不对立刻失败，不会盲等**：需要等待页面出现请先用 `app.waitPage(...)`。

<a id="DOC-22.1.1"></a>
### `[DOC-22.1.1]` 1.1 `[R-FLOW-05]` 子控制器初始化当前页面节点规范：`session.page(context.page.pageID)`

在**子控制器 Agent**（如 `AWBPlayerPageAgent`、`AWBLivePreStreamPageAgent`，自身不声明 `pageIDs`）或通用动作的 `setPerformBlock:` 中，初始化当前宿主页面的 `IRPageNode` 时，**必须传入 `context.page.pageID`**，严禁写死 `session.page(nil)`：

```objc
[action setPerformBlock:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
    return [IRFlowSession execute:^(IRFlowSession *session) {
        IRPageNode *node = session.page(context.page.pageID);
        ...
    } onException:nil onFinally:nil diag:diag];
}];
```

- 如果当前宿主页面带有页面标识（`context.page.pageID`），请求与节点校验会自动带上该标识；如果没带页面标识则自然为 `nil`。
- 这样子控制器不管嵌入在哪个父页面（推荐流、关注流、搜索视频详情等）中，都能自动、准确地识别当前所处页面位置。

<!-- [/DOC-22.1.1] -->
<!-- [/DOC-22.1] -->
<a id="DOC-22.2"></a>
## `[DOC-22.2]` 2. 必须掌握的 5 个关键问题（含 `diag` 使用铁律）

1. **`diag` 归调用方管，出错后必须 `clearError()` 才能复用**
   - `IRPageNode` **不会自动清空 `diag` 上的错误**（仅 `repeatUntil` 和 `waitUntil` 内部每轮尝试前会自动清错误）。
   - 同一 `diag` / `IRPageNode` 上一步失败后（`node.hasError == YES`），如果处理完错误还要继续往下执行或试探下一个元素，**必须先调 `node.clearError()`**（或换独立局部 `diag`），否则会触发 `IRDiagnostics` 防呆断言（`errorCode == 0`）或导致外层误判失败。
2. **`onException` / `onFinally` 内严禁使用 `session.page(...)`**
   - 异常回调与收尾块内必须用裸节点 `IRPageNode(pageID, diag)` 或 `IRPageNode(nil, flowDiag)`，否则再次失败会递归触发会话异常。
3. **所有 Block 属性调用必须带括号 `()`**
   - `node.isVisible()`、`node.backToSelf()`、`node.clearError()` 都是 Objective-C Block 属性；
   - **漏掉 `()` 会把非空 Block 指针当成 `YES`**（例如 `if (node.isVisible)` 永远为真，`if (!node.isVisible)` 永远为假），这是最隐蔽的线上 Bug 来源。
4. **裸用 `IRPageNode` 必须检查返回值 `[R-FLOW-02]`**
   - 非 `IRFlowSession` 场景下直接用 `IRPageNode(pageID, diag)` 调动作时，必须检查 `BOOL` 返回值或 `node.noError` / `diag.noError`，严禁忽略失败继续执行。
5. **线程限制**
   - 实例 API 只能在持有 UI 执行权的任务线程上调用（`+queryAction:params:inScreen:diag:` 和 `+availableActionsInApplication:tags:` 除外）。

<!-- [/DOC-22.2] -->
<a id="DOC-22.3"></a>
## `[DOC-22.3]` 3. 全量 API 速查表

### 3.1 页面与状态
| API | 返回值 / 类型 | 说明 |
|---|---|---|
| `node.pageID` | `NSString *`（可空） | 期望页面标识（`nil` 代表任意已识别页面） |
| `node.isVisible()` | `BOOL` | 当前页面是否正在显示（只读，不写 `diag`） |
| `node.backToSelf()` | `BOOL` | 最多执行 5 次 `back`（间隔 `0.5s`）退回 `node.pageID`；不走 `onException` 防递归 |
| `[node backToSelfWithMaxAttempts:interval:]` | `BOOL` | 自定义最大退回次数与间隔 |

### 3.2 动作执行与只读查询
| API | 返回值 / 类型 | 说明 |
|---|---|---|
| `node.invoke(actionID)` | `BOOL` | 执行动作；页面不对 / 链上无接收者 / 动作失败均返回 `NO` |
| `node.invokeWith(actionID, params)` | `BOOL` | 带参执行动作 |
| `node.result` | `id`（可空） | 最近一次 `invoke` / `invokeWith` 写入的 `context.result`（每次调用前清零） |
| `node.invokeQuery(actionID)` | `id`（可空） | 查询模式调用（只读不动手），失败返回 `nil`（签名未声明返回值则不支持查询） |
| `node.invokeQueryWith(actionID, params)` | `id`（可空） | 带参查询模式调用 |
| `node.available(actionID)` | `BOOL` | 检查当前页面及动作可用性；不可用原因记入 `diag.warnings`（不算错误，不进 `onException`） |
| `node.availableWith(actionID, params)` | `BOOL` | 带参可用性探测 |
| `node.availableActions(tags)` | `NSArray<IRAvailableAction *> *` | 按标签列出当前页面可用动作（带返回值声明的会自动查询填充 `userInfo`） |

### 3.3 循环、轮询与等待
| API | 返回值 / 类型 | 说明 |
|---|---|---|
| `node.loop(name, count, ^(index, total, *stop){...})` | `void` | 循环 `count` 次；每轮开始前自动检查页面是否在场；`count <= 0` 安全跳过 |
| `node.repeat(name, ^(iteration, *stop){...})` | `void` | 无限循环直到 `*stop = YES`；每轮开始前自动检查页面 |
| `node.repeatUntil(name, count, interval, ^(attempt, total, *stop){...})` | `IRPageNodeLoopResult *` | 带次数与间隔的条件重试；每轮前自动 `clearError()`；支持 `.onExhausted(^{...})` |
| `node.waitUntil(name, timeout, interval, ^(*stop){...})` | `IRPageNodeLoopResult *` | 带超时与间隔的条件等待；每轮前自动 `clearError()`；支持 `.onExhausted(^{...})` |
| `node.wait(seconds)` | `void` | 当前 UI 任务等待（过检查点，响应取消与中断）；`seconds <= 0` 安全跳过；不检查页面 |
| `node.timeline(name, duration)` | `IRWatchTimeline *` | 创建绑定本节点 `diag` 的看播时间轴（`.on(percent, name, desc, handler)...start()`，支持 `[tl stop]` 中途停止） |
| `node.decision(pointID)` | `IRDecisionPointRunner *` | 启动页面级决策点（`.params(dict).before(^(eventPath, decisionParams, event){...}).after(^(eventPath, decisionParams, event){...}).execute()`，详见 `[DOC-11.5.1]`） |

### 3.4 拟人权重语法糖（`IRPageNode+IRWeight`）
| API | 返回值 / 类型 | 说明 |
|---|---|---|
| `node.weight(groupID)` | `IRWeightSelection *` | 按节点页面权重空间抽奖；页面或组不存在时会报错（`session` 中交 `onException`） |
| `node.weightOrValue(groupID, defaultValue)` | `IRWeightSelection *` | 安全兜底版：找不到页面/组或选项全禁用时直接返回 `defaultValue`，仅记 `warning` |

### 3.5 诊断与断言抛错
| API | 返回值 / 类型 | 说明 |
|---|---|---|
| `node.diag` | `IRDiagnostics *` | 绑定的诊断对象 |
| `node.hasError` / `node.noError` | `BOOL` | 最近一次调用是否出错 / 无错（等价于 `diag.hasError` / `diag.noError`） |
| `node.errorMessage` / `node.errorCode` | `NSString *` / `NSInteger` | 错误文本与错误码 |
| `node.clearError()` | `void` | 清空 `diag` 上的错误（失败后复用同一节点前必调） |
| `node.ensure(cond, reason)` / `node.ensureNamed(name, cond, reason)` | `void` | 前置断言；`cond == NO` 触发 `ManualRaise`（致命，不可重试/不可跳过） |
| `node.raise(reason)` / `node.raiseNamed(name, reason)` | `void` | 主动终止并报错（`ManualRaise`，不可重试/不可跳过） |
<!-- [/DOC-22.3] -->
