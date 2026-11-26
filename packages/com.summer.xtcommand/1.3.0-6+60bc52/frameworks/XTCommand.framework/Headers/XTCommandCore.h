//
//  XTCommandCore.h
//  XTCommand —— 命令(子命令树)与入口
//

#import <Foundation/Foundation.h>
#import <XTCommand/XTCommandTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class XTCommand, XTOption, XTOptionGroup, XTCommandContext, XTMountGroup;

/// 返回值即退出码
typedef int (^XTCommandHandler)(XTCommandContext *ctx);
/// 自定义帮助:拿到默认帮助文本,返回最终要显示的
typedef NSString *_Nonnull (^XTHelpBuilder)(XTCommand *command, NSString *defaultHelp);

@interface XTCommand : NSObject

+ (instancetype)commandWithName:(NSString *)name summary:(NSString *)summary;
+ (instancetype)commandWithName:(NSString *)name summary:(NSString *)summary detail:(nullable NSString *)detail;

@property (nonatomic, copy, readonly) NSString *name;
@property (nonatomic, copy, readonly) NSString *summary;
@property (nonatomic, weak, readonly, nullable) XTCommand *parent;

/// 执行本命令;没设 handler 的命令被直接执行时显示帮助
@property (nonatomic, copy, nullable) XTCommandHandler handler;

// 子命令
- (XTCommand *)addSubcommand:(NSString *)name summary:(NSString *)summary;
- (XTCommand *)addSubcommand:(NSString *)name summary:(NSString *)summary detail:(nullable NSString *)detail;
@property (nonatomic, copy, nullable) NSArray<NSString *> *aliases;
/// 在父命令帮助里归到哪一节(不设归 "Commands");同名的子命令放同一节,节按首次出现的顺序
@property (nonatomic, copy, nullable) NSString *helpGroup;
/// 必须带子命令:不带时报错(106)。不设时,没 handler 的分组直接执行会打印本级帮助(退出码 0)
@property (nonatomic) BOOL requireSubcommand;

// 参数;names 用 CLI11 写法:@"-c,--count"、位置参数直接写名字 @"file"
- (XTOption *)addOption:(NSString *)names summary:(NSString *)summary type:(XTValueType)type;
- (XTOption *)addFlag:(NSString *)names summary:(NSString *)summary;
- (XTOption *)addPositional:(NSString *)name summary:(NSString *)summary type:(XTValueType)type;
- (XTOptionGroup *)addGroup:(NSString *)name;

// 帮助(格式与要填什么见 skill references/help.md;排版由框架统一生成)
/// 版本号(根命令必填):加 --version,并显示在根帮助的简介后面
@property (nonatomic, copy, nullable) NSString *version;
/// 作者(只在根命令生效):帮助简介行显示 (version 1.2.0 · by cu, alice, bob, ...)(最多 3 个);--version 显示全部
@property (nonatomic, copy, nullable) NSArray<NSString *> *authors;
/// 详细说明(选填,推荐 ≤ 5 行,用 \n 分行;反引号 `…` 包住的内容会高亮)
@property (nonatomic, copy, nullable) NSString *detail;
/// 默认动作:不带子命令时做什么(既有 handler 又有子命令时必填;纯分组自动显示"show this help…")
@property (nonatomic, copy, nullable) NSString *defaultAction;
/// 品牌字符画(只在根命令、终端框线版显示;可含颜色码,原样输出;宽于终端就不显示)
@property (nonatomic, copy, nullable) NSString *logo;
/// 示例(每一级都要写,至少 1 条):命令从程序名写起,summary 一句话
- (void)addExample:(NSString *)command summary:(NSString *)summary;
/// 场景代码块示例: 表达需要多条命令连续协作的复杂场景（在帮助与 Skill 中成组展示）
- (void)addExampleBlock:(NSString *)title commands:(NSArray<NSString *> *)commands summary:(nullable NSString *)summary;
/// 别名宏: 执行 `cmd <name> [args...]` 时透明宏展开为 `<target> [args...]`, 并在帮助中展示 Aliases 节
- (void)addShortcut:(NSString *)name target:(NSString *)target summary:(nullable NSString *)summary;
/// 页脚(只在根命令显示:源码地址、反馈方式)
@property (nonatomic, copy, nullable) NSString *footer;
/// 覆盖自动生成的用法行(一般不要设)
@property (nonatomic, copy, nullable) NSString *usage;
@property (nonatomic) BOOL helpAllEnabled; ///< 加 --help-all(打印完整命令树)
/// 在框架生成的帮助上再加工;子命令没设时用最近祖先的(一般不需要,示例用 addExample:summary:)
@property (nonatomic, copy, nullable) XTHelpBuilder helpBuilder;
/// 渲染本命令的帮助文本(不执行任何命令;整棵树从根编译一遍)。外观按当前终端自动选(框线版 / 纯文本版)。
/// 给"自己解析参数、只借用 XTCommand 出帮助"的程序用(如 iagent)。
- (NSString *)helpText;
/// 同上,可强制无色 / AI 模式(纯文本版)
- (NSString *)helpTextWithOptions:(XTHelpOptions)options;
/// 已不再生效(1.1.0 起左列宽度由框架按内容自动计算),保留只为兼容
@property (nonatomic) NSInteger helpColumnWidth;

// 解析过程中的回调
/// 开始解析本命令前(参数个数)
@property (nonatomic, copy, nullable) void (^preParse)(NSUInteger argumentCount);
/// 本命令参数解析完、handler 执行前(适合做初始化)
@property (nonatomic, copy, nullable) void (^parseComplete)(void);
/// handler 执行完后(不论成败;从实际执行的命令往根依次调用,适合清理)
@property (nonatomic, copy, nullable) void (^finalHandler)(void);

/// 配置文件:加一个选项(如 @"--config")读 TOML / INI;键名就是选项长名,子命令用 [节]。
/// 优先级:命令行 > 环境变量 > 配置文件 > 默认值。只在根命令上调用
- (void)setConfigOption:(NSString *)name defaultPath:(nullable NSString *)path required:(BOOL)required;

/// 不认识的参数不报错,放进 remainingArguments。
/// `--` 之后的参数原样进 remainingArguments(含 `--` 本身)。
/// 注意:这类命令后面写的父命令继承选项(inherited)也会进 remainingArguments,要写在命令前面
@property (nonatomic) BOOL allowExtras;

/// 是否为系统级命令 (归入 System Commands 分区，默认 NO)
@property (nonatomic, assign, getter=isSystem) BOOL system;

// iagent / MCP / Agent Skills
/// 是否开启面向 AI 的增强特性 (默认 YES)。
/// 开启后自动注入 --ai、--skill（全命令层级）以及 --export-skills（根命令 app 层级）。
@property (nonatomic) BOOL aiModeEnabled;
/// 是否暴露给 AI(默认 YES;危险命令设 NO,手机上照样能手动执行)
@property (nonatomic) BOOL agentVisible;
/// 给 AI 看的说明(不填用 summary)
@property (nonatomic, copy, nullable) NSString *agentSummary;
/// 命令标签(面向 AI Agent 意图匹配、安全权限与分类;如 @[@"ui", @"touch", @"read-only"])
@property (nonatomic, copy, nullable) NSArray<NSString *> *tags;
/// Agent Skill 触发与不触发边界（如 @"TRIGGER when: ... DO NOT TRIGGER when: ..."）
@property (nonatomic, copy, nullable) NSString *skillTrigger;
/// 执行前置条件（如 @[@"设备已越狱且 SSH 在线", @"目标 App 已安装"]）
@property (nonatomic, copy, nullable) NSArray<NSString *> *prerequisites;
/// 返回值结构契约（--json 输出样例或字段说明）
@property (nonatomic, copy, nullable) NSString *outputSchema;
/// 挂载组(推荐):把命令按组挂到 iagent(可多层,带组说明 / 标签)。可在任意命令上调用:
/// 在哪个命令上开组,组里 mount: / mount:as: 的 source 就相对那个命令(在根上开组 = 从根算)。
/// 组名是 iagent 里的绝对路径;summary 仅自定义组生效(预设公共组说明由 iagent 内置)。
- (XTMountGroup *)mountGroup:(NSString *)name summary:(nullable NSString *)summary;
/// 底层挂载:把 source 子树(相对本命令,空格分隔)挂到 iagent 的 target 路径(绝对路径)。mountGroup 是它的糖;需要完全控制目标路径时用。
- (void)mount:(NSString *)source to:(NSString *)target;

// Agent Skills 导出体系 (面向 AI Agent)
/// 导出为单文件 Agent Skill (Markdown 格式)
- (nullable NSString *)exportSkillMarkdown;

/// 导出为 Agent Skill JSON (符合 JSON Schema 规范)
- (nullable NSString *)exportSkillJSON;

/// 导出为符合业界规范的完整 Agent Skill 目录结构 (含 SKILL.md 与 references/<cmd>.md)
- (BOOL)exportSkillToDirectory:(NSString *)directoryPath error:(NSError **)error;

- (instancetype)init NS_UNAVAILABLE;

@end

/// 入口:解析参数、帮助、报错、--json、--xt-describe,执行对应 handler,返回退出码
FOUNDATION_EXPORT int XTCommandMain(XTCommand *command, int argc, const char *_Nonnull argv[_Nonnull]);

NS_ASSUME_NONNULL_END
