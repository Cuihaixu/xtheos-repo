# OC 接口

```objc
#import <XTCommand/XTCommand.h>
```

先用 `XTCommand` / `XTOption` 声明(属性赋值,顺序随意),最后 `XTCommandMain` 一次性解析并执行 handler。

## 命令

```objc
XTCommand *app = [XTCommand commandWithName:@"awetool" summary:@"抖音自动化"];
app.version = @"1.0.0";                 // 加 --version
// app.requireSubcommand = YES;         // 缺子命令报错;不设时没 handler 的分组直接执行打印本级帮助
app.handler = ^int(XTCommandContext *ctx) { return 0; };   // 返回值 = 退出码
return XTCommandMain(app, argc, argv);  // 只能传根命令

XTCommand *video = [app addSubcommand:@"video" summary:@"视频操作"];
XTCommand *like  = [video addSubcommand:@"like" summary:@"点赞" detail:@"给当前视频点赞,支持指定次数与模式。"];
like.aliases = @[ @"l" ];
like.tags    = @[ @"interactive", @"mutation" ];
[app addShortcut:@"lk" target:@"video like -c 2 --json" summary:@"快捷以 JSON 点赞 2 次"];
```

## 参数

```objc
[app addOption:@"-c,--count" summary:@"次数" type:XTValueInteger];   // NSInteger
[app addOption:@"--ratio" summary:@"比例" type:XTValueDouble];
[app addOption:@"--title" summary:@"标题" type:XTValueString];
[app addFlag:@"-v,--verbose" summary:@"详细,可 -vvv"];               // 开关
[app addPositional:@"file" summary:@"文件" type:XTValueInputPath];   // 位置参数

opt.required       = YES;
opt.defaultValue   = @1;                     // NSString / NSNumber;要和 type 对得上,否则"命令定义错误"
opt.envName        = @"AWE_COUNT";
opt.choices        = @[ @"fast", @"safe" ];
opt.multiple       = YES;                    // -t a -t b → arrayForOption
opt.expectedCount  = 2;                      // --point 0.5 1.5 → arrayForOption
opt.multiplePolicy = XTMultiplePolicyTakeLast;   // Error(默认) / TakeLast / TakeFirst / TakeAll(按数组读) / Join
opt.ignoreCase     = YES;
opt.helpGroup      = @"高级选项";
opt.inherited      = YES;                    // 子命令里也能用

[a needs:b];                                 // 给了 a 必须给 b(默认值不算"给了")
[a excludes:b];                              // 不能同时给

XTOptionGroup *g = [cmd addGroup:@"输出格式"];
[g addFlag:@"--plain" summary:@"纯文本"];
[g addFlag:@"--table" summary:@"表格"];
g.maxSelected = 1;                           // 互斥;minSelected = 至少几个
```

文件类型:`XTValueInputPath` / `XTValueOutputPath` / `XTValueInputDirectory` / `XTValueOutputDirectory`——handler 里永远是手机上的**真实路径**:
roothide 上会把用户在终端里输入的路径自动转换(`/x` → `<jbroot>/x`,`/rootfs/x` → `/x`,相对路径和已是 jbroot 的不变;先于校验)。
`XTValueString` 的路径不转;需要时 `XTResolveUserPath(path)`(`#import <XTCommand/XTCommand.h>` 已包含)。别用 `xtheos_jbroot()` 处理用户输入。

## 校验与转换

```objc
[opt addValidator:[XTValidator rangeFrom:1 to:100]];
[opt addValidator:[XTValidator existingFile]];        // existingDirectory / existingPath / nonexistentPath / IPv4
[opt addValidator:[[XTValidator existingFile] or:[XTValidator existingDirectory]]];   // and: / or:
[opt addValidator:[XTValidator validatorWithName:@"偶数" block:^NSString *(NSString *v) {
    return v.integerValue % 2 == 0 ? nil : @"必须是偶数";   // nil = 通过
}]];
[opt addTransform:[XTValidator mapping:@{ @"on" : @"1", @"off" : @"0" } ignoreCase:YES]];
```

## 读值(handler 里)

key = 长名去横线(`--count` → `count`);没长名用短名;位置参数用它的名字。当前命令找不到时往父命令找(同名时子命令的优先)。

```objc
[ctx stringForOption:@"title"];     // 没值 nil
[ctx integerForOption:@"count"];
[ctx doubleForOption:@"ratio"];
[ctx boolForOption:@"verbose"];     // 开关按值:--verbose=false / 环境变量 0 → NO
[ctx countForOption:@"verbose"];    // -vvv → 3
[ctx arrayForOption:@"tag"];        // multiple / expectedCount>1 / TakeAll
[ctx valueForOption:@"x"];          // NSString / NSNumber / NSArray / nil
[ctx isProvided:@"count"];          // 用户显式给了(环境变量 / 配置文件给的也算)
ctx.remainingArguments;             // 需 allowExtras;`--` 之后原样(含 `--`)
ctx.commandPath;                    // @[@"awetool", @"video", @"like"]
ctx.isJSONMode;
ctx.isAIMode;                       // --ai:面向机器,据此裁剪输出(含 isJSONMode)
```

## 输出

```objc
[ctx print:@"开始 %@", x];                 // stdout;JSON 模式不输出
[ctx printError:@"警告"];                  // stderr
[ctx output:@{ @"liked" : @3 }];           // 人看 / JSON 自动切换;NSDate → ISO8601、NSURL → 字符串、NSData → base64
return [ctx failWithMessage:@"没找到" code:2];   // 错误 + 退出码(JSON 模式输出 {"error":…,"code":2})
```

## 帮助 / 配置文件 / 回调

```objc
// 帮助内容(必须填什么见 references/help.md;排版由框架统一生成,不要自己拼)
[cmd addExample:@"awetool video like -c 3" summary:@"当前视频点赞 3 次"];   // 每一级都写
[cmd addExampleBlock:@"部署流程" commands:@[ @"awetool a", @"awetool b" ] summary:@"多步协作场景"];
cmd.detail = @"…";  cmd.defaultAction = @"…";  opt.metavar = @"<COUNT>";  sub.helpGroup = @"…";
cmd.skillTrigger = @"TRIGGER when: …";  cmd.prerequisites = @[ @"…" ];  cmd.outputSchema = @"{\"ok\":true}"; // 选填,不填省略
NSString *text = [cmd helpText];   // 只渲染帮助、不执行(自己解析参数的程序借用排版)
app.authors = @[ @"cu" ];  app.logo = @"…";  app.footer = @"文档:…";  app.helpAllEnabled = YES;     // 只在根命令
app.aiModeEnabled = YES;   // 默认 YES:自动注入 --ai、--skill(全层级)与 --export-skills(仅根命令)
// 导出 Agent Skills: [app exportSkillMarkdown] / [app exportSkillJSON] / [app exportSkillToDirectory:dir error:&err]
[app setConfigOption:@"--config" defaultPath:nil required:NO];   // TOML / INI,只在根命令;[子命令.路径] 作节
app.preParse = ^(NSUInteger n) {};  app.parseComplete = ^{};  app.finalHandler = ^{};
app.allowExtras = YES;
```

回调顺序(`app video like`):`app.preParse` → `like.parseComplete` → `app.parseComplete` → `like.handler` → `like.final` → `app.final`;handler 失败也会调 final。

## 接入 iagent(挂载命令)

用 `mountGroup:summary:` 把命令按组挂到 iagent,见 [iagent.md](iagent.md)。
