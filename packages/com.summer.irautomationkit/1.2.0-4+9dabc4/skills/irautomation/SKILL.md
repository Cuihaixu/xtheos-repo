---
name: irautomation
description: IRAutomationKit——在越狱 iOS 设备上从进程外自动化别的 App(远程读视图树 / 响应链、合成触摸、输入文字)。当用户在 IRAutomationKit 工程或业务工程(如 AwemeBrush)里写或调试自动化: 识别页面(registerAgent / pageIDs / IRPageResolving)、找控件(app.query / views / texts / axLabels)、点击输入(element.tap / typeText / app.tapAt / playback)、等页面(waitPage / waitPageChange / query.wait)、写页面动作(registerAction / invokeAction)、串流程(IRFlowSession / IRPageNode / onException)、列表浏览(ListBrowse / browse.view / browse.slide / browse.exit)、拟人权重(registerWeight:)、处理弹窗(IRInterruptHandling), 或用 mpa 调试命令(hitchain / pvc / pviews / axlabels / classchain)时用。按需读 references/ 下对应子文档。
---

# irautomation

IRAutomationKit 在**我们自己的进程**里远程读目标 App(如抖音)的视图树和响应链，用 CHRTouchKit 合成触摸 / 键盘输入。
不注入目标、不改它的代码。看到的界面是**拍下来的快照**，点击是**按坐标合成的触摸**，一切跨进程读写都有远程代价。

## 四层架构 (写代码前先分清放哪)
| 层 | 在哪 | 管什么 |
|---|---|---|
| **坐标操作** | `IRApplication` 分类(`tapAt` / `swipeFrom:to:` / `inputText` / `playback`) | 怎么摸屏幕；写操作的检查(执行权 / 前台)都在这层 |
| **元素** | `app.query…` → `IRUIElement`(`tap` / `typeText` / 取值) | 找控件、操作一个控件(内部调坐标操作) |
| **Agent** | 页面 / 弹窗 Agent(`registerAgent` + `registerAction`) | 业务: 认页面、页面能做什么(内部用元素) |
| **流程** | `IRFlowSession` / `IRPageNode` / `ListBrowse` 场景状态机 | 把动作串成业务流程；出错统一处理(重试 / 跳过 / 终止) |

## 编号体系与 `qmd` 混合向量检索规范（写代码前必查铁律）

22 份参考文档（`DOC-01` ~ `DOC-22`）已建本地 `qmd` 混合向量索引集合 **`irauto-docs`**（SQLite `fts5` BM25 + `sqlite-vec` 向量）。
**写代码或改事件前严禁整篇盲读 `DOC-xx.md`，必须先用 `qmd`「两步检索法」定位并只拉取对应 `[DOC-XX.Y]` 小节切片**：

```bash
# 第一步：检索定位目标章节与行号（选 A 或 B）
# 方式 A — 查开发意图/架构规范（BM25 + 300M 向量 RRF 融合，必须带 lex:/vec: 与 --no-rerank，耗时 ~1.0s）：
qmd query $'lex: <核心API或术语>\nvec: <自然语言描述的开发意图或规范问题>' -c irauto-docs --no-rerank --line-numbers -n 5

# 方式 B — 查已知红线编号 / 文档章节号 / 精确 API 名（纯 BM25，耗时 ~0.05s）：
qmd search "<如 R-FLOW-05 或 DOC-03.4.4 或 registerDecisionPoint>" -c irauto-docs --line-numbers -n 5

# 第二步：按行号定点拉取小节切片（只读命中的 20~50 行，耗时 ~0.03s）
qmd get qmd://irauto-docs/<文档名>.md:<起始行>:<行数>

# 备选：已知精确章节 ID（如 DOC-03.4.4）也可直接用 sed 按闭合标签截取整个章节
sed -n '/<a id="DOC-03.4.4">/,/<!-- \[\/DOC-03.4.4\] -->/p' skills/irautomation/references/DOC-03-page-actions.md
```

> **文档修改同步铁律**：每次修改或新增 `skills/irautomation/references/*.md` 后，必须执行 `qmd update && qmd embed` 增量更新向量库。

### 1. 文档编号（`DOC-01` ~ `DOC-22`）
| 编号 | 文档 | 领域 |
|---|---|---|
| `DOC-01` | [references/DOC-01-structure.md](references/DOC-01-structure.md) | 代码组织、命名与基础编码红线 |
| `DOC-02` | [references/DOC-02-page-agent.md](references/DOC-02-page-agent.md) | 控制器绑定、`+load` 四步顺序、属性作用域 |
| `DOC-03` | [references/DOC-03-page-actions.md](references/DOC-03-page-actions.md) | Action 签名、三种模式、归属决策 |
| `DOC-04` | [references/DOC-04-page-flow.md](references/DOC-04-page-flow.md) | 控制流压栈、`IRFlowSession`、裸用 `IRPageNode` |
| `DOC-05` | [references/DOC-05-weight.md](references/DOC-05-weight.md) | 拟人权重系统与参数覆盖 |
| `DOC-06` | [references/DOC-06-waiting.md](references/DOC-06-waiting.md) | 等待机制与执行权调度 |
| `DOC-07` | [references/DOC-07-pitfalls.md](references/DOC-07-pitfalls.md) | 静默失败目录与高频 IPC/爬链避坑 |
| `DOC-08` | [references/DOC-08-actions.md](references/DOC-08-actions.md) | 触摸输入、手势回放与元素动作 |
| `DOC-09` | [references/DOC-09-browse.md](references/DOC-09-browse.md) | ListBrowse 列表浏览状态机与防死循环 |
| `DOC-10` | [references/DOC-10-debug-tools.md](references/DOC-10-debug-tools.md) | `mpa` 调试命令与远程画板 |
| `DOC-11` | [references/DOC-11-flow.md](references/DOC-11-flow.md) | 流程会话、`IRFlowSession`、`IRWatchTimeline` 与页面级决策点 `IRDecisionPoint` |
| `DOC-12` | [references/DOC-12-interrupt.md](references/DOC-12-interrupt.md) | `<IRInterruptHandling>` 弹窗自愈与中断 |
| `DOC-13` | [references/DOC-13-lifecycle-task.md](references/DOC-13-lifecycle-task.md) | 应用生命周期与 `IRAutomationTask` |
| `DOC-14` | [references/DOC-14-pages.md](references/DOC-14-pages.md) | 页面识别与复用控制器解析 |
| `DOC-15` | [references/DOC-15-query.md](references/DOC-15-query.md) | 视图树快照与链式查询 |
| `DOC-16` | [references/DOC-16-quickstart.md](references/DOC-16-quickstart.md) | 快速开始与工程接入 |
| `DOC-17` | [references/DOC-17-remote-models.md](references/DOC-17-remote-models.md) | 实例扫描与远程模型映射 |
| `DOC-18` | [references/DOC-18-scroll.md](references/DOC-18-scroll.md) | 滚动引擎与 `IRListScroller` 对齐 |
| `DOC-19` | [references/DOC-19-support.md](references/DOC-19-support.md) | 诊断、计时与三级缓存 |
| `DOC-20` | [references/DOC-20-workflow.md](references/DOC-20-workflow.md) | 标准开发工作流 |
| `DOC-21` | [references/DOC-21-flow-action.md](references/DOC-21-flow-action.md) | `flow` 事件开发指南（`IRFlowSession` + `IRPageNode` + `loop`） |
| `DOC-22` | [references/DOC-22-page-node.md](references/DOC-22-page-node.md) | `IRPageNode` 全量 API 与诊断（`diag` / `clearError`）使用指南 |

### 2. 问题规则编号（`R-<DIM>-xx`）
* **语法与格式（`R-FMT-01` ~ `R-FMT-06`）**：`static` C 函数禁用、作废 Builder 语法、单行无大括号 `if`、禁用缩写、局部 Block 批量注册、直接读写 `_ivar`
* **结构与边界（`R-ARCH-01` ~ `R-ARCH-06`）**：`+load` 四步顺序、`.h` 私有化与跨 Agent 隔离、分类前缀、强类型模型与 `IRObject` 注册、动作归属与 `back`、业务动作收口到 Action（禁私有方法替代事件复用）
* **Action 与权重（`R-ACT-01` ~ `R-ACT-05`）**：禁用 `arc4random`、权重覆盖写法、`getParam` 判错、`isAvailable` 性能红线、查询 `NSNull` 语义
* **控制流与性能（`R-FLOW-01` ~ `R-FLOW-04`）**：`IRFlowSession`/`timeline` 边界、裸 `IRPageNode` 判返回值、禁手写轮询与盲等、禁手爬 `_parentViewController` 与重复 IPC

## 按需读 (references/ 与 examples/)
| 场景 | 描述 | 文档 | 示例目录 |
|---|---|---|---|
| **快速开始** | 建工程、引库、连目标应用、绑控制器、跑通 `hello` 操作 | [references/DOC-16-quickstart.md](references/DOC-16-quickstart.md) | — |
| **标准工作流** | 截图认页面 → `hitchain` → 页面注册 → `pviews` 找控件 → 写 → 调试跑 | [references/DOC-20-workflow.md](references/DOC-20-workflow.md) | — |
| **PageAgent 结构** | 创建、`+load` 四步顺序、控制器绑定、代码 `#pragma mark` 分区 | [references/DOC-02-page-agent.md](references/DOC-02-page-agent.md) | [examples/page-agent/](examples/page-agent/) |
| **Agent与动作体系** | 四大 Agent 实体分层（Page/View/Window/App）、动作签名与构建器、三种调用模式、归属决策 | [references/DOC-03-page-actions.md](references/DOC-03-page-actions.md) | [examples/page-agent/](examples/page-agent/) |
| **PageAgent 控制流** | 单线程压栈交接（`A → B → C` 时序图）、`IRFlowSession`/`ListBrowse` 异常封装、`IRPageNode.loop` | [references/DOC-04-page-flow.md](references/DOC-04-page-flow.md) | [examples/flow/](examples/flow/) |
| **代码与命名规范** | 页面类 `PageAgent` 结尾、应用前缀、复杂页面拆分类、一页一目录、强类型模型 | [references/DOC-01-structure.md](references/DOC-01-structure.md) | [examples/page-agent/](examples/page-agent/) |
| **流程会话与节点** | `IRFlowSession`（异常分级、不可重试错误必须终止/跳过、严禁无脑重试）、`IRPageNode`、`IRDecisionPoint`（`registerDecisionPoint:` / `point.remote(BOOL)` / `node.decision`） | [references/DOC-11-flow.md](references/DOC-11-flow.md) | [examples/flow/](examples/flow/) |
| **列表浏览状态机** | `ListBrowse`（View / Slide / Exit 三阶段）、防死循环熔断（最大步数上限、连续未前移退出） | [references/DOC-09-browse.md](references/DOC-09-browse.md) | [examples/list/](examples/list/) |
| **拟人权重系统** | 链式注册 `registerWeight:`、环境因子与禁用项，严禁私写 `arc4random` | [references/DOC-05-weight.md](references/DOC-05-weight.md) | [examples/weight/](examples/weight/) |
| **页面识别与复用** | `registerAgent` / `pageIDs` / `IRPageResolving`、从响应链区分复用的控制器 | [references/DOC-14-pages.md](references/DOC-14-pages.md) | [examples/page-agent/](examples/page-agent/) |
| **视图树查询** | 向下询问 / 过滤 / 跳转 / 切片 / 刷新点、个数要求、从当前页面查 | [references/DOC-15-query.md](references/DOC-15-query.md) | [examples/query/](examples/query/) |
| **元素与坐标操作** | 坐标触摸（`tap`/`swipe`）、手势回放（`playback`）、拖动（`dragGesture`）、文字输入与元素动作 | [references/DOC-08-actions.md](references/DOC-08-actions.md) | [examples/query/](examples/query/) |
| **滚动引擎与对齐** | 闭环滚动策略（`IRScrollOperation`）、120Hz 停稳检测（`IRScrollMonitor`）、IndexPath 对齐（`IRListScroller`） | [references/DOC-18-scroll.md](references/DOC-18-scroll.md) | [examples/list/](examples/list/) |
| **等待与执行权** | 页面到达/变更（`waitPage`/`Change`）、条件/元素等待（`waitUntil`/`query.wait`）、检查点与防盲等 | [references/DOC-06-waiting.md](references/DOC-06-waiting.md) | [examples/query/](examples/query/) |
| **应用生命周期与任务** | `IRApplication` 实例、独占 UI 自动化槽位、线程整线挂起、检查点与中断守护 | [references/DOC-13-lifecycle-task.md](references/DOC-13-lifecycle-task.md) | — |
| **弹窗自愈与中断** | `<IRInterruptHandling>` 弹窗代理、识别寻址、自动化挂起与唤醒、多层抢占与熔断 | [references/DOC-12-interrupt.md](references/DOC-12-interrupt.md) | [examples/interrupt/](examples/interrupt/) |
| **实例扫描与类映射** | `IRInstanceScanner` 堆实例跟踪、`IRObject` 业务模型映射、`IRUIView`/`IRUIScrollView` 包装 | [references/DOC-17-remote-models.md](references/DOC-17-remote-models.md) | [examples/models/](examples/models/) |
| **诊断、计时与缓存** | `IRDiagnostics` 诊断、`IRTimeRecorder` 计时、`IRCache` 三级缓存、`IR_TRACE` | [references/DOC-19-support.md](references/DOC-19-support.md) | — |
| **调试命令与画板** | `mpa`（`hitchain` / `pvc` / `pviews` / `axlabels` / `tests`）、远程画板调试开关 | [references/DOC-10-debug-tools.md](references/DOC-10-debug-tools.md) | — |
| **避坑清单** | 读错/点错/死循环/高频 IPC 诱发看门狗 Panic 重启避坑清单——写完对一遍 | [references/DOC-07-pitfalls.md](references/DOC-07-pitfalls.md) | — |

## 功能类 skills (遇到对应场景切过去)
本 skill 是**知识库与总导览**；具体专项任务与收尾审查拆成独立 skill：
| 场景 | skill |
|---|---|
| **改完代码收尾自查**(流程重试受控、严禁裸随机、列表防死循环、命名规范) | **`irautomation-page-agent-review`** |
| **快速开始**: 做一个自动化守护进程(建 target、接库、生命周期、部署) | **`irautomation-process`** |
| **照需求写一个自动化任务** / 模拟用户交互 | **`irautomation-write`** |

## 参考文档 (docs/)
[DESIGN](../../docs/DESIGN.md) · [FLOW](../../docs/FLOW.md) · [BROWSE](../../docs/BROWSE.md) · [WEIGHT](../../docs/WEIGHT.md) · [ACTION](../../docs/ACTION.md) · [GESTURE](../../docs/GESTURE.md) · [OPERATION](../../docs/OPERATION.md) · [SCROLL](../../docs/SCROLL.md) · [CANVAS](../../docs/CANVAS.md)
