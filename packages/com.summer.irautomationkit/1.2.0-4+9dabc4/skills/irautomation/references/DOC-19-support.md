---
title: 基础支撑诊断、计时与缓存体系规范
tags: [IRDiagnostics, IRTimeRecorder, IRCache, IRTrace, 诊断, 耗时分析, 三级缓存, 调试追踪]
keywords: [IRDiagnostics, IRTimeRecorder, IRCache, IRTrace, recordStep, printSummary, cache, trace, 诊断链, 耗时统计, 跨进程缓存, 调试开关]
description: IRAutomationKit 诊断上下文体系 (IRDiagnostics)、毫秒级步骤耗时记录器 (IRTimeRecorder)、三级缓存 (IRCache) 与环境追踪开关 (IR_TRACE) 的规范与使用场景。
---

# 基础支撑：诊断（IRDiagnostics）、计时与三级缓存（IRCache）（`DOC-19`）

> 文档编号：`DOC-19`（可在 `skills/` 下用 `rg "DOC-19"` 定位本文档）

本文档聚焦 `Support/` 与调试环境变量的**对外接口用法**及**常见使用约束**；命令行调试（`mpa`）与画板开关见 [DOC-10-debug-tools.md](DOC-10-debug-tools.md)。

---


## 目录索引

- [`[DOC-19.1]` 1. 诊断对象：`IRDiagnostics`](#DOC-19.1)
  - [`[DOC-19.1.1]` 1.1 怎么用](#DOC-19.1.1)
  - [`[DOC-19.1.2]` 1.2 遇到什么情况该怎么做](#DOC-19.1.2)
- [`[DOC-19.2]` 2. 分段计时：`IRTimeRecorder`](#DOC-19.2)
- [`[DOC-19.3]` 3. 三级缓存：`IRCache`](#DOC-19.3)
  - [`[DOC-19.3.1]` 3.1 怎么选择级别](#DOC-19.3.1)
  - [`[DOC-19.3.2]` 3.2 怎么用与约束](#DOC-19.3.2)
- [`[DOC-19.4]` 4. 排查卡死 / 被杀现场：`IR_TRACE=1`](#DOC-19.4)

---

<a id="DOC-19.1"></a>
## `[DOC-19.1]` 1. 诊断对象：`IRDiagnostics`

一次流程或动作调用配一个 `IRDiagnostics`，由调用方显式传入并贯穿 `流程 → 动作 → app 操作`。

<a id="DOC-19.1.1"></a>
### `[DOC-19.1.1]` 1.1 怎么用
```objc
IRDiagnostics *diag = [IRDiagnostics diagnosticsWithDomain:@"浏览流程"];
// 或函数内快捷创建：IRDiagnostics *diag = IRFunctionDiagnostics();

diag.verbose(@"开始执行");
diag.warning(@"未读到附加字段，走降级逻辑");
diag.error(-1, @"找不到目标按钮");

if (diag.hasError) {
    NSLog(@"%@", diag.errorDescription());   // 或 diag.toError() 转 NSError
}
```

<!-- [/DOC-19.1.1] -->
<a id="DOC-19.1.2"></a>
### `[DOC-19.1.2]` 1.2 遇到什么情况该怎么做
| 情况 | 怎么处理 | 原因（强相关约束） |
|---|---|---|
| **同一流程里多处可能报错** | 用 `do { ... break; } while (NO);` 并在每步后判断 `if (diag.hasError) break;`；或写错误前先检查 `if (diag.noError)` | **一个 `diag` 只能记一个错误**，未清除前第二次调用 `diag.error(...)` 会触发断言 |
| **一步失败处理完，要继续复用同一个 `diag` / `IRPageNode`** | 先调 `diag.clearError()`（或 `node.clearError()`） | 否则 `diag.hasError` 一直为 `YES`，后续 `return diag.noError;` 或新动作都会被当成失败 |
| **在事件 `perform` 中返回状态** | 结尾统一 `return diag.noError;` | 保证返回值与 `diag` 错误状态严格一致（见 [DOC-03-page-actions.md](DOC-03-page-actions.md)） |
| **控制台看不到 `diag` 日志** | 启动时设置环境变量 `IR_DIAG_STDERR=1` | 默认输出走 `os_log`（`subsystem: com.summer.irautomation.diag`，`category: domain`），设环境变量才同步打到 `stderr` |

---

<!-- [/DOC-19.1.2] -->
<!-- [/DOC-19.1] -->
<a id="DOC-19.2"></a>
## `[DOC-19.2]` 2. 分段计时：`IRTimeRecorder`

需要排查某段自动化流程或感知调用耗时时使用：

```objc
IRTimeRecorder *rec = [IRTimeRecorder timeRecorderWithFormater:nil];
rec.pushTimedSection();
rec.recordTime(@"查询视图树");
rec.recordTime(@"点击评论按钮");
rec.popTimedSection();
NSLog(@"%@", rec.formatAndClearTimings());
```

---

<!-- [/DOC-19.2] -->
<a id="DOC-19.3"></a>
## `[DOC-19.3]` 3. 三级缓存：`IRCache`

用于缓存耗时较长的解析结果（如扫共享缓存符号表、类名列表），线程安全且对同一个 `key` 的 `compute:` 不会并发重复计算。

<a id="DOC-19.3.1"></a>
### `[DOC-19.3.1]` 3.1 怎么选择级别
| 级别 (`IRCacheScope`) | 存储位置 | 何时自动失效 | 适用场景 |
|---|---|---|---|
| `IRCacheScopeProcess` | 内存 | 进程退出 | 当前会话/进程内的临时解析结果 |
| `IRCacheScopeBoot` | 磁盘文件 | 设备重启（按开机时间校验） | 依赖本次开机状态的量（如共享缓存 slide） |
| `IRCacheScopeSystem` | 磁盘文件 | 系统升级（build 号变化）或换机型 | 跨重启不变的系统固定偏移/符号表 |

<!-- [/DOC-19.3.1] -->
<a id="DOC-19.3.2"></a>
### `[DOC-19.3.2]` 3.2 怎么用与约束
```objc
NSArray *symbols = [IRCache objectForKey:@"ax.symbols"
                                   scope:IRCacheScopeSystem
                                 compute:^id _Nullable {
    return /* 耗时解析，返回 nil 则不缓存 */;
}];
```
- **值类型约束**：`Boot` / `System` 级别会立即写盘（路径 `/var/mobile/Library/Caches/IRAutomationKit/`，`root` 与 `mobile` 进程共用），**存入的值必须是 plist 类型**（`NSString` / `NSNumber` / `NSData` / `NSDate` / `NSArray` / `NSDictionary`）。
- **手动删除 / 清理**：`[IRCache setObject:nil forKey:key scope:scope]` 删单个；`[IRCache removeAllObjectsInScope:scope]` 清整级（常用于测试）。

---

<!-- [/DOC-19.3.2] -->
<!-- [/DOC-19.3] -->
<a id="DOC-19.4"></a>
## `[DOC-19.4]` 4. 排查卡死 / 被杀现场：`IR_TRACE=1`

- 遇到进程被 `SIGKILL` 强杀、或者卡死在某一步看不到缓冲日志时，启动时加环境变量 **`IR_TRACE=1`**。
- 内部会绕过标准输出缓冲直接 `write(2)` 到 `stderr`（带时间戳和线程名），能直接看到每条线程最后停在哪一步。
<!-- [/DOC-19.4] -->
