//
//  IRRemoteModule.h — 目标已加载镜像句柄(L1)
//
//  目标进程里一个已加载 dyld 镜像的快照:加载基址 / ASLR slide / 路径。镜像启动即定、基本不变,
//  所以 `findRemoteModule:` 结果**懒加载 + 缓存**;`dlopen` 进新库后调 `reloadModules` 刷新。
//
//  用途:调**没有导出符号**的目标内部 C 函数——在 IDA 里逆向到"某库 + 偏移",跨进程要算上 ASLR:
//    IRRemoteModule *m = [session findRemoteModule:@"AwemeCore"];
//    IRRemoteAddress fn = [m addressForOffset:0x12340];              // = base + 偏移
//    IRRemoteCFunction *f = [IRRemoteCFunction functionWithAddress:fn typeEncoding:@"i@"];
//  或直接 `[IRRemoteCFunction functionWithLibrary:@"AwemeCore" offset:0x12340 typeEncoding:@"i@"]`(内部走同一套)。
//

#import <IRemoteKit/IRTypes.h>
#import <IRemoteKit/IRSession.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRRemoteModule : NSObject

/// mach header 加载地址(ASLR slid 后的运行时基址)。
@property (nonatomic, readonly) IRRemoteAddress base;
/// ASLR 偏移(= `base` − 首选 `__TEXT` vmaddr)。
@property (nonatomic, readonly) int64_t slide;
/// __TEXT 段大小(字节数)。
@property (nonatomic, readonly) uint64_t textSize;
/// 完整路径。
@property (nonatomic, readonly, copy) NSString *path;
/// basename(如 `AwemeCore` / `libsystem_c.dylib`)。
@property (nonatomic, readonly, copy) NSString *name;
/// Mach-O 128位 UUID 格式化字符串(标准 8-4-4-4-12 大写，如 `A8B9C0D1-E2F3-4A5B-6C7D-8E9F0A1B2C3D`)，未读取到为 nil。
@property (nonatomic, readonly, nullable, copy) NSString *uuid;
/// 架构字符串(如 @"arm64" / @"arm64e" / @"x86_64")。
@property (nonatomic, readonly, copy) NSString *arch;
/// Mach-O 文件类型标志 (MH_EXECUTE / MH_DYLIB / MH_BUNDLE 等)。
@property (nonatomic, readonly) uint32_t fileType;
/// 是否为主可执行程序 (fileType == MH_EXECUTE)。
@property (nonatomic, readonly, getter=isMainExecutable) BOOL mainExecutable;
/// 是否为 App Bundle 内部二进制(主程序或包含在 .app/ 下的动态库/插件)。
@property (nonatomic, readonly, getter=isAppModule) BOOL appModule;
/// 是否位于系统 dyld 共享缓存(shared cache)内;NO 表示磁盘独立映射(App 主程序、Frameworks 或注入的插件 dylib)。
@property (nonatomic, readonly, getter=isInSharedCache) BOOL inSharedCache;

/// 镜像内偏移 → 运行时地址(= `base + offset`)。IDA 给「镜像内偏移」直接用;
/// 若 IDA 给的是虚拟地址(preferred base + N),用 `N + module.slide`。
- (IRRemoteAddress)addressForOffset:(uint64_t)offset;

/// 给定运行时内存地址是否落在该镜像的 `__TEXT` 段内(base .. base + textSize)。
- (BOOL)containsAddress:(IRRemoteAddress)address;

@end

#pragma mark - 地址符号化结果(对标 LLDB image lookup -a)

@interface IRSymbolInfo : NSObject

/// 调用方传入的原始地址。
@property (nonatomic, readonly) IRRemoteAddress inputAddress;
/// 剥除 arm64e PAC 高位后的运行时地址。
@property (nonatomic, readonly) IRRemoteAddress address;
/// 所属镜像模块。
@property (nonatomic, readonly, nullable) IRRemoteModule *module;
/// 镜像名称(如 `AwemeCore` / `libsystem_kernel.dylib`)。
@property (nonatomic, readonly, copy) NSString *moduleName;
/// 镜像完整路径。
@property (nonatomic, readonly, copy) NSString *modulePath;
/// 镜像内静态虚拟地址(= `address - module.slide`,对齐 IDA / LLDB `Module[0x...]`)。
@property (nonatomic, readonly) IRRemoteAddress unslidAddress;
/// 所属 Mach-O 段名(如 `__TEXT` / `__DATA_CONST` / `__DATA`)。
@property (nonatomic, readonly, copy, nullable) NSString *segmentName;
/// 所属 Mach-O 节名(如 `__text` / `__stubs` / `__objc_data`)。
@property (nonatomic, readonly, copy, nullable) NSString *sectionName;
/// 节内字节偏移(无节时为段内偏移)。
@property (nonatomic, readonly) uint64_t sectionOffset;

/// 符号来源类型: `@"objc_method"` / `@"symtab"` / `@"function_starts"` / `@"dladdr"` / `@"none"`。
@property (nonatomic, readonly, copy) NSString *symbolKind;
/// 符号名称(如 `-[AWEAwemeModel itemID]` / `main` / `___lldb_unnamed_symbol18420$$AwemeCore`)。
@property (nonatomic, readonly, copy, nullable) NSString *symbolName;
/// Swift 还原后的可读函数签名,或匿名函数对应的 IDA `sub_xxx` 名称。
@property (nonatomic, readonly, copy, nullable) NSString *demangledSymbolName;
/// 符号入口运行时地址(IMP / 函数起点)。
@property (nonatomic, readonly) IRRemoteAddress symbolAddress;
/// 符号入口静态虚拟地址(IDA IMG = `symbolAddress - module.slide`)。
@property (nonatomic, readonly) IRRemoteAddress symbolUnslidAddress;
/// 查询地址相对符号入口的字节偏移(= `address - symbolAddress`)。
@property (nonatomic, readonly) uint64_t offset;
/// `LC_FUNCTION_STARTS` 中的 1-based 函数序号(未知时为 0)。
@property (nonatomic, readonly) uint32_t functionIndex;

/// 单行摘要(对标 LLDB `Summary:` 行内容)。
- (NSString *)summaryText;
/// 对标 LLDB `image lookup -a` 的多行格式化输出(`Address:` + `Summary:`)。
- (NSString *)formattedDescription;
/// 结构化字典表示(供 CLI `--json` 与 iagent 消费)。
- (NSDictionary<NSString *, id> *)dictionaryRepresentation;

@end

#pragma mark - 查找(归 session 实例;懒加载 + 缓存)

@interface IRSession (IRRemoteModule)

/// 按名(`basename`,带不带 `.dylib` 后缀都行)找目标里已加载的镜像;找不到 `nil`。**懒加载 + 缓存**。
- (nullable IRRemoteModule *)findRemoteModule:(NSString *)moduleName;
/// 按运行时内存地址查找其所属的镜像模块(命中 base <= addr 且包含在该镜像内);找不到 `nil`。
- (nullable IRRemoteModule *)moduleContainingAddress:(IRRemoteAddress)address;
/// 按运行时内存地址反查所属镜像、段/节与最近符号(对标 LLDB `image lookup -a`, 100% 纯内存免 worker);不属于任何镜像返回 `nil`。
- (nullable IRSymbolInfo *)lookupSymbolAtAddress:(IRRemoteAddress)address;
/// 目标里全部已加载镜像(懒加载 + 缓存)。
- (NSArray<IRRemoteModule *> *)remoteModules;
/// 清模块缓存,下次访问重新遍历(`dlopen` 进新库后用)。
- (void)reloadModules;

@end

NS_ASSUME_NONNULL_END
