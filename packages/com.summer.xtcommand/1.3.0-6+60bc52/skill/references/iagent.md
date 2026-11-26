# 让命令被 iagent / MCP 发现

> `iagent` = 设备端总入口。库这一侧已就绪:写好命令、声明挂载,打包即自动注册;设备上没有 iagent 时这些闲置,不影响单独使用。

## 挂载组(推荐):mountGroup

把命令按组挂到 iagent,组带说明和标签。**在哪个命令上开组,组里 `mount:` 的源路径就相对那个命令**(在根上开 = 从根算);组名是 iagent 里的绝对路径。

```objc
// 自定义组:summary 生效,tags 是总体介绍(iagent 帮助里显示)
XTMountGroup *aweme = [app mountGroup:@"aweme" summary:@"抖音自动化"];
aweme.tags = @[ @"点赞", @"关注", @"评论" ];
[aweme mount:@"like"];            // 本程序 like 子树 → iagent aweme like
[aweme mount:@"follow"];
[aweme mount:@"like" as:@"zan"];  // 改名:iagent aweme zan(= like)

// 多层子组:从父组派生
XTMountGroup *debug = [aweme groupNamed:@"debug" summary:@"调试"];
[debug mount:@"trace"];           // iagent aweme debug trace

// 一命令多挂:同一命令两个入口
[[app mountGroup:@"net" summary:@"网络"] mount:@"vpn"];
[[app mountGroup:@"tunnel" summary:@"隧道"] mount:@"vpn"];
```

- `[组 mount:@"源"]`:iagent 路径 = 组路径 + 源(挂整棵子树);`mount:as:` 改名。
- **挂进预设公共组**(apps/screen/ui/log/fs/net/device/proc):直接 `mountGroup:@"apps"`,它的说明由 iagent 内置清单给(权威),你设的 `summary` 被忽略,但 `tags` 会汇总显示。
- **自定义组**(你自己起的名):`summary` 生效;多个插件挂同名自定义组时,显示最后注册的那个的 `summary`。
- 组路径每段只能用**小写字母、数字、`-`**;首段不能是保留名:`refresh` `help` `version` `test` `device` `mount` `doctor`。

## 在子命令上开组(源相对、层级可改)

```objc
XTCommand *obj = [app addSubcommand:@"obj" summary:@"对象自省"];   // obj 下有 p / po / A B C …
XTMountGroup *debug = [obj mountGroup:@"debug" summary:@"远程调试"];
[debug mount:@"p"];                    // iagent debug p     → 程序 obj p
[debug mount:@"A B" as:@"A"];          // iagent debug A     → 程序 obj A B(去掉一层)
[debug mount:@"A B C" as:@"E F"];      // iagent debug E F   → 程序 obj A B C(三层挂成两层)
[[debug groupNamed:@"ui" summary:@"界面"] mount:@"view tree" as:@"tree"];   // 子组沿用基准 obj
```
- `as:` 是相对组的目标路径,可多段;源不存在报 `mount/source-not-found`(报错里是拼好的完整路径),目标重叠报 `mount/overlap`。

## 底层 mount:to:(需要完全控制目标路径时)

```objc
[app mount:@"like" to:@"aweme like"];   // 直接指定目标路径;mountGroup 是它的糖
```

一般用 mountGroup。`mount:to:` 不带组说明 / 标签。

## 给 AI 的信息与 Agent Skills 元数据

> 遵循 `帮助 (summary) → 详情 (detail) → Skills` 渐进式信息链：能用 `summary` + `example` 说清的命令无需强填扩展字段，未填项导出时自动省略、不留 `-` 占位。

```objc
cmd.agentVisible  = NO;               // 危险命令不暴露(连子树);手机上仍可手动执行
cmd.agentSummary  = @"给当前视频点赞";  // 不填用 summary
cmd.tags          = @[ @"mutation", @"interactive" ];
cmd.skillTrigger  = @"TRIGGER when: 需要点赞; DO NOT TRIGGER when: 仅查信息"; // 选填:根命令自动并入 Frontmatter description
cmd.prerequisites = @[ @"目标设备已连接且在线" ];                                // 选填:渲染为 Constraints 列表
cmd.outputSchema  = @"{\"liked\": 2, \"dryRun\": false}";                      // 选填:JSON 自动包裹代码块
[cmd addExampleBlock:@"部署与校验" commands:@[ @"cmd a", @"cmd b" ] summary:@"多步连招"]; // 选填:场景代码块
opt.agentSummary  = @"次数,1-100";
opt.choices / rangeFrom:to:           // 导出成 enum / minimum / maximum,AI 才知道能填什么
```

- 目录导出 `--export-skills-dir <DIR>` 自动将主 `SKILL.md` 控制在 **100 行以内**，子命令详情与参数表下沉至 `references/<cmd>.md`；参数表按需展示 `Default` / `Constraints` / `Location` 列（全空列自动裁剪）。

## 文件参数

```objc
XTOption *out = [cmd addOption:@"-o,--output" summary:@"输出" type:XTValueOutputPath];
out.preferredLocation = XTLocationMac;   // 建议放 Mac(只写进说明)
```

- 文件 / 目录类型(`XTValueInputPath`/`OutputPath`/`…Directory`):MCP 调用时 AI 可选放手机还是 Mac;**handler 里永远是手机真实路径**。
- `XTValueString` 声明的路径不被当文件,roothide 上也不转换。

## 自查

```bash
<程序> --xt-describe     # 看 commands / groups / errors
```

导出:`commands`(每条含 `path`/`source`/`runnable`/`options`/`inputSchema`)、`groups`(每组 `path`/`summary`/`tags`)、`errors`。
自查错误码:`mount/syntax`、`mount/reserved`、`mount/overlap`、`mount/source-not-found`、`option/reserved`;有错退出码 100,出错那条不导出、其它照常。

## 全局模式(iagent / MCP 调用时自动带)

- `--ai`:AI 模式 = JSON + 无色,`ctx.isAIMode` 为真,命令可据此裁剪输出。MCP 调用时带它。
- `--json`:只要 JSON。`--ai` 已隐含。
- `--skill`:全层级动态在线探针(父级输出路由表、叶子级输出参数与示例)。
- `--export-skills` / `--export-skills-dir <DIR>`:仅根命令具备,导出单文件 Markdown 或完整 `SKILL.md` + `references/` 目录。
- `--no-color` / `NO_COLOR` / 非终端:自动无色。
- 这些是保留选项,**命令不能自定义** `--ai` / `--json` / `--no-color` / `--skill` / `--export-skills` / `--xt-*`。可通过 `app.aiModeEnabled = NO` 关闭 AI 增强开关。
