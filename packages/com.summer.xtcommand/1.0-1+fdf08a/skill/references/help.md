# 帮助信息:作者必须填什么

> 需要 XTCommand ≥ 1.1.0。帮助的**分节、顺序、排版全部由框架生成**,作者只负责把下面这些内容填对、填全。
> 框架**不做强制检查**:缺了照样能编译运行,只是那一节不显示。所以下面标"必须"的,**写命令时一律写上,不要省**。

## 必须填(每个命令都要)

| 内容 | 接口 | 要求 |
|---|---|---|
| **简介** | `commandWithName:summary:` / `addSubcommand:summary:` 的 `summary` | **每一级都写**(根、分组、叶子)。一句话说清做什么,动词开头,不加句号。推荐 ≤ 30 个汉字(英文 ≤ 60 字符),超了也能显示,但会折行 |
| **示例** | `[cmd addExample:@"完整命令" summary:@"做什么"]` | **每一级都至少写 1 条**,分组和根命令也要写。推荐 1~6 条。命令**从程序名开始写完整路径**(`awetool video like -c 3`,不要只写 `like -c 3`);说明推荐 ≤ 20 个汉字。挑最常用、最能说明用法的写,覆盖必填参数 |
| **版本** | 根命令 `app.version = @"1.2.0"` | **根命令必须写**,语义化版本号。会显示在根帮助的简介后面,并提供 `--version` |
| **选项 / 参数说明** | `addOption:summary:type:` 等的 `summary` | 每个都写。说明它是什么,推荐 ≤ 20 个汉字。**不要在说明里写默认值、范围、可选值、环境变量**——框架会自动生成 `[default: …]` 这类注解,写了就重复 |
| **默认动作**(只在一种情况下) | `cmd.defaultAction = @"…"` | 命令**既有 handler 又有子命令**时必须写:不带子命令时它做什么。纯分组(没 handler)不用写,框架自动显示"show this help when no command is given" |

## 建议填

| 内容 | 接口 | 什么时候写 |
|---|---|---|
| 详细说明 | `cmd.detail = @"…"` | 行为有细节要交代(会跳过什么、有什么副作用、和别的命令的区别)。推荐 ≤ 5 行,用 `\n` 分行;父命令未设时自动合成子命令概览 |
| 场景代码块示例 | `[cmd addExampleBlock:@"标题" commands:@[…] summary:@"…"]` | 多条命令连续协作的复杂场景;帮助里独立成 `Example: <标题>` 分区连续展示,Skill 里输出多行 `bash` 块 |
| 别名宏(Aliases) | `[cmd addShortcut:@"lk" target:@"video like -c 2" summary:@"…"]` | 预置常用参数组合,执行时原地宏展开并透传参数;终端用户也可用内置系统子命令 `alias` / `al` 动态增删 |
| 占位名 | `opt.metavar = @"<次数>"` | 默认按类型显示 `<TEXT>` `<INT>` `<FLOAT>` `<PATH>` `<DIR>`;能写得更具体就写(`<COUNT>`、`<BUNDLE_ID>`、`<次数>`),用尖括号包起来 |
| 可选值 | `opt.choices = @[…]` | 取值是固定几个时**一定用它**,不要只写在说明里:帮助里会显示 `{fast|safe}`,也会校验输入,iagent / MCP 也能拿到 |
| 范围 | `[opt rangeFrom:to:]` | 数值有上下限时用它,理由同上 |
| 作者 | 根命令 `app.authors = @[@"cu", @"alice"]` | 显示在根帮助简介行版本号后:`(version 1.2.0 · by cu, alice)`,最多显示 3 个、超过加 `...`;`--version` 显示全部。只写名字,邮箱 / 反馈放页脚 |
| 页脚 | 根命令 `app.footer = @"…"` | 源码地址、反馈方式。只在根命令显示,推荐 ≤ 2 行 |
| logo | 根命令 `app.logo = @"…"` | 品牌字符画(多行)。只在根命令、终端框线版显示,位于用法行和简介之间;宽于终端就整幅不显示。可以直接粘贴带颜色码的生成结果(渐变工具如 `cfonts --gradient`、`toilet`、`lolcat`),框架原样输出。建议高度 ≤ 6 行。注意 24 位真彩色在部分终端(老版 macOS 终端 App)显示不对,要兼容就生成 256 色 |
| 描述里的高亮 | 在 `summary` / `detail` / 说明里用反引号 `` `illdb stop` `` | 框线版会去掉反引号并把内容染色,纯文本版原样保留。只支持反引号这一种标记 |

## 框架自动生成(不要自己写)

- **示例的颜色**框架按命令树自动上:程序名、子命令、选项各有颜色,作者只写纯文本命令。
- **选项放哪一节**:只属于本命令的在 Options;设了 `inherited = YES` 的进 Global Options(每一级都显示),和 `--help` / `--json` / `--ai` / `--no-color` 放在一起。

用法行、选项列表及注解、选项组、子命令列表、全局选项(从上层继承来的)、末尾"Run '… <command> --help' …"提示,都由框架按定义生成。

- **不要**用 `helpBuilder` 往帮助里硬拼示例或章节,用 `addExample:summary:`。
- **不要**自己设 `usage` 覆盖用法行,除非框架生成的确实表达不了(极少见)。
- 子命令多、想分节显示:给子命令设 `helpGroup = @"…"`,同组的放一节(不设归 Commands)。
- 示例写**本程序**的完整路径即可;经 iagent 调用时框架自动换成 iagent 路径。

## 显示出来是什么样

框架文字固定英文(`Usage` / `Options` / `Commands` / `Examples` …),作者的内容用什么语言都行。终端里是框线版,管道 / 窄屏 / `--ai` 时是纯文本版,内容一样。

```
 Usage: awetool video like -c <COUNT> [OPTIONS] <VIDEO_ID>...

 给视频点赞
 按视频 ID 点赞;不给 ID 时点当前播放的视频。

╭─ Arguments ────────────────────────────────────────────────────────╮
│ <VIDEO_ID>...  要点赞的视频  [multiple]                            │
╰────────────────────────────────────────────────────────────────────╯
╭─ Options ──────────────────────────────────────────────────────────╮
│ -c, --count <COUNT>     每个视频点赞次数  [required]               │
│                         [range: 1–100]                             │
│     --mode {fast|safe}  点击节奏  [default: fast]                  │
╰────────────────────────────────────────────────────────────────────╯
╭─ Global Options ───────────────────────────────────────────────────╮
│ -h, --help  Show this help and exit                                │
╰────────────────────────────────────────────────────────────────────╯
╭─ Examples ─────────────────────────────────────────────────────────╮
│ awetool video like -c 3         当前视频点赞 3 次                  │
│ awetool video like -c 1 7301 7302                                  │
│                                 给两个视频各点 1 次                │
╰────────────────────────────────────────────────────────────────────╯
```

对应的写法:

```objc
XTCommand *like = [video addSubcommand:@"like" summary:@"给视频点赞"];
like.detail = @"按视频 ID 点赞;不给 ID 时点当前播放的视频。";
[like addPositional:@"video-id" summary:@"要点赞的视频" type:XTValueString].multiple = YES;

XTOption *count = [like addOption:@"-c,--count" summary:@"每个视频点赞次数" type:XTValueInt];
count.required = YES;
count.metavar = @"<COUNT>";
[count rangeFrom:1 to:100];

XTOption *mode = [like addOption:@"--mode" summary:@"点击节奏" type:XTValueString];
mode.choices = @[ @"fast", @"safe" ];
mode.defaultValue = @"fast";

[like addExample:@"awetool video like -c 3"           summary:@"当前视频点赞 3 次"];
[like addExample:@"awetool video like -c 1 7301 7302" summary:@"给两个视频各点 1 次"];
```

## 多语言

要随设备语言显示,给 `summary` / `detail` / 示例说明传本地化字符串(`NSLocalizedString`)。注意:命令行进程读的是**运行它的那个用户**的语言设置——以 root 运行(`XTHEOS_CLI_USER=root`)时读 root 的偏好,不一定等于手机界面上的语言,要实测。

## 写完自查

- [ ] 根、每个分组、每个叶子命令都有简介和至少 1 条示例
- [ ] 根命令有版本号
- [ ] 示例都从程序名写起,能直接复制运行
- [ ] 选项说明里没有重复写默认值 / 范围 / 可选值
- [ ] 取值固定的选项用了 `choices`,有上下限的用了 `rangeFrom:to:`
- [ ] 既有 handler 又有子命令的命令写了 `defaultAction`
- [ ] 跑一遍 `<程序> --help` 和每个分组的 `--help`,看一眼
