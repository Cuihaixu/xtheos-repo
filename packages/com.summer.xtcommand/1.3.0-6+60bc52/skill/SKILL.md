---
name: xtcommand
description: XTCommand——越狱设备上的命令行解析库(OC 接口 + 公开的改版 CLI11)。当用户在越狱命令行 / 守护进程工程里解析命令行参数、加子命令 / 选项 / 开关 / 位置参数、写 handler、做参数校验 / 取值范围 / 可选值、读环境变量或配置文件、输出 JSON(--json)、让命令被设备端 iagent / MCP 发现(挂载 mount、--xt-describe、agentVisible、文件参数),或遇到 XTCommand / CLI11 / xt_run / XTCommandMain 相关报错(退出码 100~114、命令定义错误)时用。写帮助(简介 / 示例 / 版本 / 详细说明 / 占位名)时按 references/help.md 填。按需读 references/:oc / cpp / iagent / errors / help。
---

# XTCommand

越狱设备上的**通用命令行解析库**,基于 CLI11 v2.7.2。

- **OC**:`#import <XTCommand/XTCommand.h>`,纯 Objective-C,写 `.m` 即可。
- **C++**:`#include <XTCommand/CLI11.hpp>`(只能在 `.cpp` / `.mm` 里),原生 CLI11 写法 + 我们的扩展方法(可链式)。
- 附带:隐藏选项 `--xt-describe` 导出命令描述,给设备端总入口 `iagent` 和 MCP 用;设备上没有它们时闲置,不影响单独使用。

## 接入(xtheos 工程)

| 步骤 | 做法 |
|---|---|
| 链接 | Other Linker Flags 加 `-framework XTCommand`(SDK 壳在 `$(XTHEOS)/vendor/frameworks`,模板已配搜索路径) |
| deb 依赖 | `XTHEOS_PACKAGE_DEPENDS` 加 `com.summer.xtcommand`(手机没装库时安装阶段就拦下,而不是运行时 `Library not loaded`) |
| 入口 | OC:`return XTCommandMain(app, argc, argv);`;C++:`return CLI::xt_run(app, argc, argv);` |

## 先读这张表(常见先验在这里是错的)

| 你可能会这么写 | 在 XTCommand 里 | 后果 |
|---|---|---|
| 自己解析 `argv` / 用 `getopt` / `NSUserDefaults` 读参数 | 用 `XTCommand` 声明命令和参数,`XTCommandMain` 解析 | 没帮助、没校验、iagent / MCP 发现不了 |
| C++ 用 `CLI11_PARSE(app, argc, argv)` | 用 `CLI::xt_run(app, argc, argv)` | `--xt-describe` / `--json` / 保留名检查全没了(不报错) |
| C++ 另外 `#include` 一份 CLI11 | 只用 `<XTCommand/CLI11.hpp>` | 两份都在 `CLI` 命名空间,符号冲突 |
| 在 `.m` 里 `#include <XTCommand/CLI11.hpp>` | OC 只 `#import <XTCommand/XTCommand.h>`;要用 C++ 改成 `.mm` | C++ 头文件在 `.m` 里编译失败 |
| OC 里把值绑定到变量 | handler 里从 `ctx` 读:`[ctx integerForOption:@"count"]` | OC 接口没有变量绑定 |
| 读值时用 `@"--count"` / `@"-c"` 当 key | key 是长名去掉横线:`@"count"`(没长名用短名,位置参数用它的名字) | 读到 nil / 0,不报错 |
| 自己定义 `--json`/`--ai`/`--no-color`/`--skill`/`--export-skills`/`--xt-*` 选项 | 都是保留名,入口统一处理;handler 用 `ctx.isJSONMode` / `ctx.isAIMode` | 程序直接退出码 100 |
| JSON 模式下用 `printf` / `NSLog` 输出 | 用 `[ctx output:]` 输出结果、`[ctx print:]` 输出给人看的文字 | stdout 混进非 JSON,iagent / MCP 解析失败 |
| C++ `check(CLI::IsMember({...}))` / `check(CLI::Range(a,b))` | 用 `->choices({...})` / `->range(a,b)`(OC:`choices` / `rangeFrom:to:`) | 能校验,但导出里没有 enum / minimum / maximum,AI 不知道取值范围 |
| 文件参数用 `XTValueString` | 用 `XTValueInputPath` / `XTValueOutputPath`(目录用 `…Directory`) | MCP 不知道它是文件,不能放 Mac / 自动拷回;**roothide 上用户在终端里给的路径也不会被转成真实路径,文件找不到** |
| 用 `xtheos_jbroot()` 处理用户输入的路径 | 文件类型参数已自动转换;字符串参数用 `XTResolveUserPath()` | rootless 上会多加 `/var/jb`、相对路径被拼坏 |
| 危险命令(清数据、重启…)照常暴露 | `cmd.agentVisible = NO` | AI 能直接调用 |
| C++ `add_flag("--x", desc)`,`desc` 是 `std::string` 变量 | `add_flag("--x")->description(desc)` 或传字面量 | 左值 string 被当成结果变量,说明丢失(不报错) |
| 只写顶层简介、不写示例 / 分组也不写简介 | **每一级**都写 `summary` 和至少 1 条 `addExample:summary:`,根命令写 `version`(见 references/help.md) | 帮助缺"做什么、怎么用",框架不报错,人和 AI 都只能猜 |
| 挂到 iagent 用一堆 `mount:to:` | 用 `mountGroup:summary:`(带组说明/标签,可多层);可在任意命令上开组,源路径相对开组的命令 | 帮助里没组说明、没分层 |
| 自定义退出码用 100 以上 | 用 1~99;100~114 是 CLI11 / XTCommand 的(见 errors) | 和"命令定义错误""参数错误"分不清 |

## 最小示例(OC)

```objc
#import <XTCommand/XTCommand.h>

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        XTCommand *app = [XTCommand commandWithName:@"awetool" summary:@"抖音自动化"];
        XTCommand *like = [app addSubcommand:@"like" summary:@"点赞"];
        XTOption *count = [like addOption:@"-c,--count" summary:@"次数" type:XTValueInteger];
        count.defaultValue = @1;
        [count addValidator:[XTValidator rangeFrom:1 to:100]];
        like.handler = ^int(XTCommandContext *ctx) {
            NSInteger n = [ctx integerForOption:@"count"];
            [ctx output:@{ @"liked" : @(n) }];   // 人看:liked: 3;--json:{"liked":3}
            return 0;
        };
        XTMountGroup *g = [app mountGroup:@"aweme" summary:@"抖音自动化"];
        g.tags = @[ @"点赞" ];
        [g mount:@"like"];                         // 设备端:iagent aweme like -c 3
        return XTCommandMain(app, argc, argv);
    }
}
```

## 行为要点(不看会踩)

- 每层**最多一个**子命令;没 handler 只有子命令的是**分组**,直接执行(含根命令)**打印本级帮助**,退出码 0;想让缺子命令报错(106)才设 `requireSubcommand = YES`。
- 只执行**最深的那一层**的 handler;父命令的 handler 不会顺带执行。
- `inherited = YES` 的选项在子命令里也能用(前后都行);但 `allowExtras` 的命令后面写的继承选项会进 `remainingArguments`。
- **roothide**:文件类型参数和 `--config` 里用户给的路径会自动转成真实路径(`/x` → `<jbroot>/x`,`/rootfs/x` → `/x`,相对路径不变),handler 直接用;rootful / rootless 不变。
- 开关按值计:`--flag=false`、环境变量 `0` 都是"关";`-vvv` 计数为 3。
- `needs:` 看用户是否**显式给了**,默认值不算。
- 优先级:命令行 > 环境变量 > 配置文件 > 默认值;配置文件里的未知键被忽略。
- 定义写错(重复选项名、重名子命令、非法名字、默认值和类型不符)**不会崩**:报"命令定义错误"、退出码 100——看 stderr。
- handler / 校验器 / 回调里抛 `NSException` 会被接住,不会崩(见 errors)。

## 按需读(references/)

| 场景 | 读 |
|---|---|
| OC 接口完整用法(类型 / 约束 / 校验 / 子命令 / 读值 / 输出 / 帮助 / 配置文件 / 回调) | [references/oc.md](references/oc.md) |
| C++ 写法、扩展方法、`xt_run` | [references/cpp.md](references/cpp.md) |
| 帮助与场景示例编写规范(简介 / 详情 / 示例 / 场景代码块 / 别名宏 / 版本 / 占位名) | [references/help.md](references/help.md) |
| 让命令被 iagent / MCP / Agent Skills 发现:挂载组 mountGroup、`--xt-describe`、`--skill`、`--export-skills-dir`、Trigger/Prerequisites/OutputSchema | [references/iagent.md](references/iagent.md) |
| 退出码、报错信息、异常怎么处理、怎么排查 | [references/errors.md](references/errors.md) |
