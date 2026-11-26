---
name: irautomation-write
description: 用 IRAutomationKit 照需求写一个自动化任务(模拟用户在目标 App 里看视频、搜索、点赞、填表…),并在设备上跑通。触发词:写自动化、自动化脚本、模拟用户操作、模拟一个人刷、帮我自动点 / 自动搜、加一个自动化命令 / 测试。
---

# irautomation-write

照需求写一个自动化任务并在设备上跑通。**事实都在 `irautomation` skill 的 `references/`**,本 skill 只列步骤和自查,不复制。

## 1. 先摸清现场(不写代码)
1. 设备:`idm list --ai`;目标进程在跑:`issh exec <设备> "mpa <进程> hitchain" -t 60` 有输出。
2. 把需求拆成"页面 → 操作 → 等什么"的步骤,每个页面先**截图 + hitchain** 认出来;已有页面 Agent 的看它注册的 pageID,
   没有的按 `references/DOC-14-pages.md` 写注册。
3. 每个要操作的控件:到那个页面上 `mpa <进程> pviews`(必要时 `axlabels`)找依据(文字 → label → 类名 → 位置)。
4. 需要像人的滑动 / 复杂手势:向用户要回放标识(`app.playback`)。

## 2. 写
- **开发前先跑公式匹配样板**：`bd mol pour mol-page-dev --var target="<类名>" --var path="<目录>" --var features="page,query,..."`，先读对应的 `examples/` 样板和 `DOC-xx` 规范再动笔。
- **先读 `references/DOC-01-structure.md`**(代码组织规范):页面类以 `PageAgent` 结尾、识别在 `+load` 注册、复杂页面按关注点拆分类、一个页面一个文件夹。
- 流程放进 `IRAutomationTask`,`[app setAutomation:task]`。
- 页面上的业务操作(返回、看视频、打开评论、搜索)写成**页面 Agent 的动作**(内部:找控件 → 操作 → 等结果),流程只调动作。
- **业务控制流（`+Flow.m`、多步串联交互）强制使用流程会话 `IRFlowSession`**（原子事件视情况而定，不强制）：`[IRFlowSession execute:onException:onFinally:diag:]`，`session.page(@"页面").invoke(@"动作")` 一路写下去；多步/多视频循环**必须使用 `node.loop(name, count, body)`**（内置每轮页面断言）；失败统一交给 `onException`（回根页面后 `skip()` / `retry()` / 默认终止），`onFinally` 里回根页面。**严禁手写裸 `for` 循环绕过每步页面检测，严禁在 Session 内开临时 sub-diag 手动屏蔽错误**。少量独立步骤或原子操作要自己按返回值判断时才用 `IRPageNode(pageID, diag)` 或底层 `app.currentAgent` + `invokeAction`。写法和规则见 `references/DOC-04-page-flow.md` 与 `references/DOC-11-flow.md`。
- 开头确认当前页面,不对先回到起点(根页面:不在就一直 back);**每次操作后等结果**(`waitPageChange` / `query.wait`),不用固定 sleep 等页面。
- 找控件从当前页面查:动作里 `context.agent.query…`,流程里 `app.currentAgent.query…`。
- 每步传 `diag`(流程会话用会话的 diag),失败打 `diag.errorMessage` 并说明停在哪一步。
- 人的节奏和选择(看还是划走、看几秒、想一想再输入、滑几屏)用权重系统:页面 Agent 里注册,`node.weight(@"组").doubleValue` 取,见 `references/DOC-05-weight.md`;这是"节奏"不是"等待"(等页面用等待接口)。
- 打开 `app.debugEnabled = YES`,便于用户在手机上看点了哪里。
- 写成命令或测试:`references/DOC-10-debug-tools.md` 里的 mpa 命令形式;跑:`issh exec <设备> "mpa …" -t 300`。

## 3. 跑通后自查
| 查 | 核对依据(irautomation skill) |
|---|---|
| 代码组织:类名 / 前缀 / `+load` 注册 / 分类拆分 / 文件夹 | `references/DOC-01-structure.md` 自查清单 |
| 每一步读到 / 点到的是不是当前页面上的东西(没读到下面那层) | `references/DOC-07-pitfalls.md` 第 3 条 |
| 等待是不是等的"新"页面 / 新内容 | `references/DOC-06-waiting.md`、`DOC-07-pitfalls.md` 第 4、5 条 |
| 文字查询用的是 `texts`(向下询问)不是 `text`(过滤) | `references/DOC-15-query.md`、`DOC-07-pitfalls.md` 第 6 条 |
| 多个候选时有没有 `visible()` 再挑 | `DOC-07-pitfalls.md` 第 7 条 |
| 有没有强杀应用、`issh` 超时够不够 | `DOC-07-pitfalls.md` 第 11、12 条 |
| 用了流程会话:工程加了 `-fobjc-arc-exceptions`;`onFinally` 回根页面;回调 / 收尾里用的是原始写法的节点 | `references/DOC-11-flow.md` 使用规则、`DOC-07-pitfalls.md` 第 17、18 条 |

跑一次不够:至少跑 2~3 次,起始页面换一换(从首页 / 从上次停下的页面),都能走完才算通。

参考实现:IRAutomationKit 的 `mpa/Tests/IRSceneTests.m`(页面注册 + 页面动作 + 流程会话,推荐页看视频 → 搜索 → 读结果 → 回推荐页)。
