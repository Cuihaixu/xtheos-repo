# 退出码与报错

## 退出码

| 码 | 含义 | 常见原因 |
|---|---|---|
| 0 | 成功 / `--help` / `--version` | |
| 1~99 | **留给你的 handler**(`return 2;` / `failWithMessage:code:`) | |
| 1 | 也用于:handler / `preParse` / `parseComplete` 抛 `NSException`、C++ 抛 `std::exception` | 看 stderr 的异常名 |
| 100 | **命令定义错误**(编译命令树失败)/ 保留名 / `--xt-describe` 自查有错 | 见下表 |
| 103 | 文件错误 | 配置文件不存在 / 读不了 |
| 104 | 类型转换失败 | 整数参数给 `abc`:`Could not convert: --i = abc` |
| 105 | 校验失败 | 范围、可选值、文件存在性、自定义校验(含校验器抛异常);**带范围 / 校验器的参数给错类型也是 105**(校验先于转换) |
| 106 | 缺必填 / 缺子命令(仅 `requireSubcommand = YES`;分组默认打印帮助)/ 选项组数量不对 | `--bundle is required`、`A subcommand is required`、`Exactly 1 option …` |
| 107 | needs 不满足 | `--dry-run requires --count` |
| 108 | excludes 冲突 | `--quiet excludes --loud` |
| 109 | 多余参数 | 不认识的选项、一层给了两个子命令 |
| 114 | 值的个数不对 | `--point 0.5`(要 2 个) |

## 退出码 100:命令定义错误

stderr 是 `命令定义错误:…`(`--xt-describe` 时为 `{"code":"definition"}`)。不会崩,但程序什么都执行不了:

| stderr 里看到 | 原因 |
|---|---|
| `added option matched existing option name: dup is already added` | 同一命令里两个选项名字重复(含短名) |
| `subcommand name or alias matches existing subcommand: hello is already added` | 同层两个子命令同名(或和别名撞) |
| `Bad long name: bad name` | 选项名写法不对(如带空格 `@"--bad name"`) |
| `… given default value("abc") produces an error : Could not convert` | `defaultValue` 和 `type` 对不上(整数给了 `@"abc"`) |
| `使用了保留名` | 定义了 `--json` / `--ai` / `--no-color` / `--xt-` 开头的选项 |

## 异常

| 在哪抛 `NSException` | 结果 |
|---|---|
| handler | 打印异常,退出码 1 |
| 校验器 block | 当成校验失败(105),提示"校验器异常" |
| `preParse` / `parseComplete` | 打印,退出码 1 |
| `helpBuilder` | 退回默认帮助 |
| `finalHandler` | 打印,退出码变成非 0 |

## 排查

- 先看 **stderr**:CLI11 的报错都在 stderr(JSON 模式下也是纯文本),stdout 为空。
- 读不到值:key 写错(要 `@"count"` 不是 `@"--count"`)、选项定义在别的不相关命令上、或该选项不是 `inherited`。
- 开关读成 NO:用户写了 `--flag=false` 或环境变量是 `0`。
- 导出里没有某命令:没挂载(写了任何挂载就只导出挂载的)、`agentVisible = NO`(含祖先)、或挂载自查出错(看 `errors`)。
- roothide 上文件"明明在却说不存在"(`File does not exist`、配置 `was not readable`):参数声明成了 `XTValueString`(不转换路径)→ 改成文件类型,或 `XTResolveUserPath()`。
- 运行时 `Library not loaded: @rpath/XTCommand.dylib`:手机没装 `com.summer.xtcommand`;把它写进 deb 依赖。
