---
title: flow 事件开发指南：IRFlowSession 配合 IRPageNode 标准写法
tags: [flow, IRFlowSession, IRPageNode, loop, invoke, onException, onFinally, 事件流]
keywords: [IRFlowSession, IRPageNode, session.page, loop, invoke, onException, onFinally, flow事件]
description: 专门针对 flow 事件（控制流 Action）的实战开发指南——IRFlowSession 配合 IRPageNode 的标准结构、loop 自动页面校验机制、子动作直接 invoke 原则，以及 onException / onFinally 的收尾与防退穿规范。
---

# `flow` 事件开发指南：`IRFlowSession` 配合 `IRPageNode` 标准写法（`DOC-21`）

> 文档编号：`DOC-21`（可在 `skills/` 下用 `rg "DOC-21"` 定位本文档）

本文档聚焦**如何在页面 Agent 中编写一个标准 `flow` 事件**：核心原则是 **`IRFlowSession` 负责异常与事务边界，`IRPageNode` 负责页面守门、循环与子事件调度**，业务层只写清晰易读的“剧本”，绝不越界搬砖。

---

## 目录索引

- [`[DOC-21.1]` 1. 标准 `flow` 事件结构模板](#DOC-21.1)
- [`[DOC-21.2]` 2. `IRPageNode` 在 `flow` 中的调用铁律](#DOC-21.2)
- [`[DOC-21.3]` 3. `loop` / `repeat` 的自动校验机制](#DOC-21.3)
- [`[DOC-21.4]` 4. `onException` 与 `onFinally` 收尾规范](#DOC-21.4)

---

<a id="DOC-21.1"></a>
## `[DOC-21.1]` 1. 标准 `flow` 事件结构模板

```objc
[self registerAction:IRIdentityMake(@"flow", @"在视频详情页连续观看后退出") builder:^(IRActionBuilder *action) {
    [action setTypeEncodingBlock:^(IRTypeEncoding *encoding) {
        encoding.arg(@"watch_count", IRSchemaTypeInteger).setRequired(NO).setComment(@"观看数量; 默认走权重 detail_watch_count");
        encoding.result(IRSchemaTypeInteger).setComment(@"实际观看的视频数");
    }];

    [action setPerformBlock:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
        NSString *detailPageID = context.page.pageID;
        if (detailPageID.length == 0) {
            diag.error(-1, @"detail flow: 未识别出子页面 ID");
            return NO;
        }
        __block NSInteger watched = 0;

        return [IRFlowSession execute:^(IRFlowSession *session) {
            IRPageNode *detail = session.page(detailPageID);
            NSInteger targetCount = [context.findParam(@"watch_count", detail.weightOrValue(@"detail_watch_count", @4)) integerValue];
            detail.ensure(targetCount > 0, @"detail flow: 观看数量必须大于 0");

            detail.loop(@"watch_detail_batch", targetCount, ^(NSUInteger index, NSUInteger total, BOOL *stop) {
                detail.invoke(@"watch");
                watched++;
                if (index + 1 < total) {
                    detail.invoke(@"next_video");
                    detail.wait(0.8);
                }
            });
        } onException:^(IRFlowException *exception) {
            // 1. 优先按调用名称 (exception.name) 判断：关键步骤（如 load / next_video）失败直接终止，交由上级处理
            if (![exception.name isEqualToString:@"watch"]) {
                return;
            }
            // 2. 判断当前是否在本页面：不在当前页才尝试 backToSelf() 返回本页，确认回到后再 skip()
            IRPageNode *currentDetail = IRPageNode(detailPageID, diag);
            if ((currentDetail.isVisible() || currentDetail.backToSelf()) && exception.canSkip) {
                exception.skip();
            }
        } onFinally:^(IRFlowSession *session, IRDiagnostics *flowDiag) {
            context.result = @(watched);
            if (IRPageNode(detailPageID, nil).isVisible()) {
                IRPageNode(detailPageID, flowDiag).invoke(@"back");
            }
        } diag:diag];
    }];
}];
```

<!-- [/DOC-21.1] -->
<a id="DOC-21.2"></a>
## `[DOC-21.2]` 2. `IRPageNode` 在 `flow` 中的调用铁律

1. **必须用 `session.page(pageID)` 获取节点**：只有通过 `session.page(...)` 得到的节点才会把失败自动转换成 `IRFlowException`，交由 `onException` 统一处理。
2. **直接 `invoke`，严禁 `available` 静默兜底**：
   - 正确：`detail.invoke(@"watch");`
   - 错误：`if (detail.available(@"watch")) { ... } else { detail.wait(2.0); }` —— 缺少必要动作应直接报错进入 `onException`，不能静默吞掉。
3. **不要透传无用的展示可选参数**：子动作如果只定义了用于面板展示的可选参数（如 `watch` 的 `index` / `total`），且业务不需要展示覆盖，直接 `invoke` 即可。
4. **严禁越界代劳底层动作的职责**：例如切换视频是否成功、是否滑到底，必须由 `next_video` 自己保证并在失败时报错；严禁在 `flow` 循环里手写 `video_info` 查询和 `item_id` 判重。

<!-- [/DOC-21.2] -->
<a id="DOC-21.3"></a>
## `[DOC-21.3]` 3. `loop` / `repeat` 的自动校验机制

- `node.loop(name, count, block)` 在**每一轮开始前**会自动校验当前页面是否仍匹配 `node.pageID`；若页面已被弹窗遮挡或意外切走，会立即中断循环并触发 `onException`。
- `node.invoke(...)` 每次执行时同样会校验当前页面是否在场。
- **因此严禁在 `loop` 内部重复手写 `node.ensure(node.isVisible(), ...)`**。
- 当 `count <= 0` 时 `loop` 会安全跳过，不需要额外包一层 `if (count > 0)`（若要求参数必须 > 0，直接用前置断言 `node.ensure(count > 0, ...)`）。

<!-- [/DOC-21.3] -->
<a id="DOC-21.4"></a>
## `[DOC-21.4]` 4. `onException` 与 `onFinally` 收尾规范

1. **默认即终止，严禁显式调用 `exception.abort()`**：在 `onException` 中只要没有调用 `exception.retry()` 或 `exception.skip()`，流程就会自动终止并进入 `onFinally`，随后 `[IRFlowSession execute:...]` 返回 `NO` 且保留错误 `diag`。
2. **按 `exception.name` + 当前页面 `isVisible()` 决策**：不要硬编码穷举所有外层上游页面 ID；先用 `exception.name`（调用名称，如 `load`、`watch`、`next_video`）区分错误来源，再结合当前页面节点的 `.isVisible()` 判断是否还在本页（不在且允许自愈时才调 `backToSelf()`）。**注意调用 `.isVisible()` / `.backToSelf()` 必须带括号 `()`**。
3. **异常恢复用独立节点**：在 `onException` / `onFinally` 中执行自愈或退页时，使用独立节点 `IRPageNode(pageID, diag)`，不要复用 `session.page(...)`。
<!-- [/DOC-21.4] -->
