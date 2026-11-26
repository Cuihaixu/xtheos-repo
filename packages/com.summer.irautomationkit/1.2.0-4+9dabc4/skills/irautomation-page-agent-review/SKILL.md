---
name: irautomation-page-agent-review
description: IRAutomationKit 自动化代码审查流程规范——只定义 Review 方法论与文档路由（先读总览 → 按模块定位 references/ → 读完细则再审源码 → 分级输出报告与验证）。用于对现有或新写的自动化守护进程代码（如 AwemeBrush automated）进行系统性代码评审。触发词: "review"、"代码审查"、"自查"、"检查自动化"、"对齐规范"、"重构审查"。
---

# irautomation-page-agent-review：Review 流程与文档定位规范

> **定位原则**：本 skill **不存放具体编码细则与红线条文**。所有规范的唯一事实源都在 `../irautomation/references/`。
> 审查时**严禁凭上下文记忆直接看代码下结论**，必须先按本流程把对应文档读进上下文，再逐行对拍源码。

---

## 第一步：强制启用 `bd` 公式 `mol-page-review`（禁止自由发挥跳步）

1. 收到 review 请求后，**立即灌入公式**建立追踪流：
   ```bash
   bd mol pour mol-page-review --var target="<类名或模块名>" --var path="<绝对路径>"
   ```
   （若当前工作区不在 `IRAutomationKit` 下，先切换或指定 `IRAutomationKit` 的 `bd` 仓库；若为全量多模块排查，按模块分别 pour）。
2. 严格按公式生成的 4 个步骤顺序 `bd update <id> --claim` → 执行 → `bd close <id>`，**严禁跳过 `static-scan` 直接做语义审查**：
   `spec-load` → `static-scan` → `semantic-review` → `fix-and-verify`。

---

## 第二步：先读总览文档，建立全局边界

在看任何业务源码前，**先读取**：
1. [irautomation/SKILL.md](../irautomation/SKILL.md) — 了解四层架构分层与 `references/` 目录索引。
2. [references/DOC-01-structure.md](../irautomation/references/DOC-01-structure.md) — 建立目录组织、命名、分层与代码规范基线。
3. [references/DOC-07-pitfalls.md](../irautomation/references/DOC-07-pitfalls.md) — 了解高频踩坑项与跨进程代价边界。

---

## 第三步：按模块/文件类型定位必读 `references/`（优先用 `qmd` 精准拉取章节切片）

根据当前正在审查的文件路径与职责，命中下表哪几类，**优先使用 `qmd` 混合检索 + `qmd get` 定点拉取命中的章节切片**（或对必查文档读取对应 `[DOC-xx.x]` 章节），严禁凭空猜测红线：

```bash
# 1. 按红线号或领域语义检索定位行号：
qmd search "<如 R-ARCH-04 或 R-FLOW-05>" -c irauto-docs --line-numbers -n 5
qmd query $'lex: <核心API>\nvec: <审查关注点描述>' -c irauto-docs --no-rerank --line-numbers -n 5

# 2. 按行号拉取目标规范切片：
qmd get qmd://irauto-docs/<文档名>.md:<起始行>:<行数>
```

| 被审查模块 / 文件特征 | 定位到的领域 | 必读 `references/` 文档 |
|---|---|---|
| `main.m`、`Core/`、`*Application*` | 守护进程、生命周期、开关、日志/诊断/缓存 | [DOC-13-lifecycle-task.md](../irautomation/references/DOC-13-lifecycle-task.md)<br/>[DOC-19-support.md](../irautomation/references/DOC-19-support.md) |
| `Shared/`、`Models/`、`*Scan*` | 远程对象映射、实例扫描、本地模型封装 | [DOC-17-remote-models.md](../irautomation/references/DOC-17-remote-models.md)<br/>[DOC-01-structure.md](../irautomation/references/DOC-01-structure.md) |
| `*PageIDs*`、`*PageAgent*`（绑定与识别） | 页面注册、响应链识别、控制器复用区分 | [DOC-02-page-agent.md](../irautomation/references/DOC-02-page-agent.md)<br/>[DOC-14-pages.md](../irautomation/references/DOC-14-pages.md) |
| `*PageAgent*`（动作与参数） | 动作构建、参数校验、查询/执行协议、归属边界、子控制器 `context.page.pageID` | [DOC-03-page-actions.md](../irautomation/references/DOC-03-page-actions.md)（含 `[DOC-03.4.3]`、`[DOC-03.4.4]`） |
| `Flows/`、`*+Flow.m`、多步骤串联动作 | 控制流编排、`IRFlowSession`、`IRPageNode` 初始化（`session.page(context.page.pageID)`）、异常重试/跳过 | [DOC-04-page-flow.md](../irautomation/references/DOC-04-page-flow.md)<br/>[DOC-11-flow.md](../irautomation/references/DOC-11-flow.md)<br/>[DOC-22-page-node.md](../irautomation/references/DOC-22-page-node.md)（含 `[DOC-22.1.1]` / `[R-FLOW-05]`） |
| `watch`、看播/看图集/看直播、决策点（`registerDecisionPoint:` / `point.remote` / `node.decision`）、进度停留、伴生定时触发（`timeline`） | 页面级决策点 `IRDecisionPoint`、声明式时间轴 `IRWatchTimeline`、`IRFlowSession` 会话编排 | [DOC-11-flow.md](../irautomation/references/DOC-11-flow.md)（含 `[DOC-11.5]`、`[DOC-11.5.1]`）<br/>[DOC-04-page-flow.md](../irautomation/references/DOC-04-page-flow.md) |
| `Lists/`、`*+ListBrowse.m`、列表滚动 | `ListBrowse` 状态机、防死循环、滚动对齐 | [DOC-09-browse.md](../irautomation/references/DOC-09-browse.md)<br/>[DOC-18-scroll.md](../irautomation/references/DOC-18-scroll.md) |
| `Alert/`、`Login/`、视图型弹窗 | `<IRInterruptHandling>`、抢占与熔断、检查点 | [DOC-12-interrupt.md](../irautomation/references/DOC-12-interrupt.md) |
| 视图查询、手势、等待、延时 | 视图树查询范围、触摸/输入、安全等待 | [DOC-15-query.md](../irautomation/references/DOC-15-query.md)<br/>[DOC-08-actions.md](../irautomation/references/DOC-08-actions.md)<br/>[DOC-06-waiting.md](../irautomation/references/DOC-06-waiting.md) |
| 概率决策、停留时长、分支选择 | 权重注册、环境因子、随机控制 | [DOC-05-weight.md](../irautomation/references/DOC-05-weight.md) |

---

## 第四步：对照已读文档逐项核对源码

带着刚读过的 `references/*.md`，**禁止只靠关键词扫一遍就下结论**，必须按以下粒度逐项核对：
1. **按每个 `registerAction:` / `registerListBrowse:` 逐个过筛子**（先列清单，全部过完再动手改）：
   - **归类与体量**：按 [DOC-04-page-flow.md](../irautomation/references/DOC-04-page-flow.md) 1.5 判定是原子还是控制流；非 Flow 动作超过 ~25 行或含多步串联/分支/计时，必须核查是否应拆原子 + `IRFlowSession` / `timeline`；
   - **参数与权重**：按 [DOC-05-weight.md](../irautomation/references/DOC-05-weight.md) 5.4 核对是否用了哨兵值（`@0` / `@""`）写 `if-else` / 三元兜底，而非 `findParam(..., weightOrValue(...))`；
   - **跨 Agent 耦合**：是否直接 `#import` 别的 Agent 头文件、读别的 Agent 属性、或调用别的 Agent 导出的 C 函数；
   - **等待与前置校验**：是否存在 `while` / `for` + `sleep` 手写轮询（应改 `waitUntil`），是否在写操作前缺失前置存在性检查。
2. **逐个 `static` 辅助函数与 `ivarNamed:` 审计**：
   - 文件里每出现一个 `static` C 函数，必须追问：**为什么不写成 `PageAgent` 属性/方法，或是不是在重复造轮子（SDK `IRResponderChain` / `IRPageAgent` / `AWBAwemeModel` 已有同类能力）？**
   - 每出现一次 `ivarNamed:`，必须追问：**是否在跨进程手爬框架已有的结构（如 `_parentViewController` 应改用 `self.application.chain` / `IRResponderNode.parentViewController`），或重复读取可缓存的远程对象（如 `_model` 应缓存）？**
3. **结构、职责归属与协议规范**：类/文件放置是否正确；数据转换与业务判断有没有下沉到对应 Model；返回值、错误处理、`+load` 四步顺序是否对齐文档。
4. **跨进程与性能边界**：是否存在重复 IPC、循环内高开销查询、重复 JSON 解析。
5. **若发现新规范缺口**：将具体规则沉淀到 `../irautomation/references/` 对应文档中，不在本 review skill 里堆砌条文。

---

## 第五步：输出报告、修复并验证

1. **分级输出问题清单**（每条必须附 `文件路径:行号` 与依据的 `references/*.md` 章节）：
   - **Fatal**：违反生命周期/线程/中断安全、死循环、裸阻塞等待、严重跨层破坏。
   - **Warning**：违反动作签名/返回值/参数读取/权重/模型封装等规范。
   - **Notice**：命名、代码结构、重复逻辑、可简化写法。
2. **修复后必须编译验证**（对应 target build 通过），有条件时在目标设备上做真机回归；每完成一个区域即更新并关闭对应 `bd` 任务。
