//
//  IRObject+Debug.h — 对象/类调试 dump(L1,对标 Facebook Chisel)
//
//  把远程对象/类"自己长什么样"序列化成可读串(给人看 / 日志 / mpx 命令),格式对齐 Chisel/class-dump:
//    pivars   —— 全部 ivar(按定义类分段)+ 解码后的值   (Chisel pinternals/pivar)
//    pmethods —— 类方法 + 实例方法(带类型签名)         (Chisel pmethods)
//    pprops   —— 属性(@property 声明)                   (Chisel pproperties)
//    pclass   —— 继承链                                   (Chisel pclass)
//    p        —— `(类名 *) 0x地址`:纯内存读、免 worker、无副作用(lldb `p` 轻量版)
//    po       —— 调 `-debugDescription`/`-description` 取文本(需 worker;lldb `po`)
//  数据来源经 `IRDebugOptions.source` 选:静态自解析(默认,免 worker)/ 远程 runtime(含 category/swizzle)。
//  程序化按字段读走 `-[IRObject ivars]`/`IRIvar`;本分类是"便利序列化层"。
//

#import <IRemoteKit/IRObject.h>
#import <IRemoteKit/IRSession.h>
#import <IRemoteKit/IRThread.h>
#import <IRemoteKit/IRDebugOptions.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRObject (Debug)

/// 全部 ivar(含继承链,按定义类分段)+ 值,序列化成串。默认 static、免 worker。
- (NSString *)pivars;
- (NSString *)pivarsWithOptions:(IRDebugOptions *)options;

/// 类方法 + 实例方法(带类型签名)。默认 static。
- (NSString *)pmethods;
- (NSString *)pmethodsWithOptions:(IRDebugOptions *)options;

/// 属性(`@property` 声明)。默认 static、**只本类**(对标 Chisel);`options.inherited=YES` 沿继承链到根。
- (NSString *)pprops;
- (NSString *)ppropsWithOptions:(IRDebugOptions *)options;

/// 继承链(从本对象的类往上到根)。默认 static。
- (NSString *)pclass;
- (NSString *)pclassWithOptions:(IRDebugOptions *)options;

/// `(类名 *) 0x地址`:纯内存读定类,**免 worker、无副作用**(lldb `p` 的轻量版:看到地址先确认是啥)。
- (NSString *)p;

/// 调目标对象的 `-debugDescription`(无则退回 `-description`)取文本(lldb `po`)。
/// 需 worker(目标前台);是一次真方法调用,受 arb_call 超时 + 自退出 gadget 保护——
/// 撞阻塞/死锁退化成超时串,不带崩目标,但**别对会走锁/主线程专属的方法用**(如 UIView)。
- (NSString *)po;

/// 导出本对象所属类(或类对象自身)的完整 `.h` 头文件声明(对标 class-dump,附带 IMP/IMG/ivar 偏移注释)。
- (NSString *)classdump;
- (NSString *)classdumpWithOptions:(IRDebugOptions *)options;

@end

@class IRRemoteModule;
@class IRValue;

#pragma mark - 表达式求值结果(对标 LLDB p / po)

@interface IRExpressionResult : NSObject

/// 原始表达式文本。
@property (nonatomic, readonly, copy) NSString *expression;
/// 求值结果类型编码(`@encode`,如 `@"@"`, `@"q"`, `@"d"`, `@"B"`, `@"*"`, `@"v"` 等)。
@property (nonatomic, readonly, copy) NSString *typeEncoding;
/// 可读 C/ObjC 类型名(如 `AWEAwemeModel *`, `int`, `BOOL`, `const char *`, `void`)。
@property (nonatomic, readonly, copy) NSString *typeName;
/// 原始 64 位寄存器/对象地址值(若为对象则为对象运行时地址,供 REPL 记入 `$`)。
@property (nonatomic, readonly) uint64_t rawValue;
/// 若求值结果为有效远程对象(或类对象),对应 `IRObject` 句柄;否则为 `nil`。
@property (nonatomic, readonly, nullable) IRObject *objectValue;
/// 若求值结果为 C 字符串(`char *`)或 `NSString`,对应解码后的字符串内容。
@property (nonatomic, readonly, copy, nullable) NSString *stringValue;
/// 底层 `IRValue` 载体(void 时为 nil)。
@property (nonatomic, readonly, nullable) IRValue *value;
/// 面向终端的格式化输出文本(`p` 模式为 `(Type) value`,`po` 模式为对象 `-description` 或标量值)。
@property (nonatomic, readonly, copy) NSString *formattedOutput;

/// 结构化字典表示(供 CLI `--json` 与 iagent 消费)。
- (NSDictionary<NSString *, id> *)dictionaryRepresentation;

@end

/// 共享缓存 (`dyld_shared_cache`) 类元数据解析位置。
typedef NS_ENUM(NSInteger, IRSharedCacheSource) {
    /// 共享进程直读（默认）：校验目标与宿主共享缓存基址一致后，零 syscall 本进程内存直读；不匹配时自动回退目标进程。
    IRSharedCacheSourceLocal = 0,
    /// 目标进程跨进程读：强制走 `mach_vm_read_overwrite` 从目标进程读取。
    IRSharedCacheSourceRemote = 1,
};

@interface IRSession (IRClassDump)

/// 遍历指定镜像模块内 `__objc_classlist` 的所有类(纯内存读,免 worker; 共享缓存镜像默认优先本进程零 syscall 直读)。
- (void)enumerateClassesInModule:(IRRemoteModule *)module
                      usingBlock:(void (^)(NSString *className, IRRemoteAddress classAddress, BOOL *stop))block;

/// 遍历指定镜像模块内 `__objc_classlist` 的所有类，并显式指定共享缓存解析位置 (`Local` 共享进程直读 / `Remote` 目标进程跨进程读)。
- (void)enumerateClassesInModule:(IRRemoteModule *)module
               sharedCacheSource:(IRSharedCacheSource)cacheSource
                      usingBlock:(void (^)(NSString *className, IRRemoteAddress classAddress, BOOL *stop))block;

/// 读取指定类地址的类名 / 父类名(纯内存读,免 worker)。
- (nullable NSString *)classNameForClassAddress:(IRRemoteAddress)classAddress;
- (nullable NSString *)superclassNameForClassAddress:(IRRemoteAddress)classAddress;

/// 判断 `classAddress` 是否是 `superclassName` 的子类(不含自身;沿 superclass 链上溯,带 `cache` 记忆化加速全库扫描,纯内存免 worker)。
- (BOOL)classAddress:(IRRemoteAddress)classAddress
isSubclassOfClassNamed:(NSString *)superclassName
               cache:(nullable NSMutableDictionary<NSNumber *, NSNumber *> *)cache;

/// 按 `includeSuper`(非系统父类继承链)与 `depsDepth`(属性/ivar 关联非系统类深度)展开类地址列表。
- (NSArray<NSNumber *> *)expandClassAddresses:(NSArray<NSNumber *> *)seedAddresses
                                 includeSuper:(BOOL)includeSuper
                                    depsDepth:(NSInteger)depsDepth;

/// 为一组类地址(NSNumber<uint64_t>)生成完整的 `.h` 头文件文本(支持 options.inherited 展开非系统父类与 options.depsDepth 展开关联类)。
- (NSString *)classdumpForClassAddresses:(NSArray<NSNumber *> *)classAddresses
                           primaryModule:(nullable IRRemoteModule *)primaryModule
                                 options:(nullable IRDebugOptions *)options;

/// 为单个类地址生成结构化字典(供 `--json` / AI 使用)。
- (nullable NSDictionary *)classdumpDictionaryForClassAddress:(IRRemoteAddress)classAddress
                                                      options:(nullable IRDebugOptions *)options;

/// 对标 LLDB `p` / `po` 评估表达式(基于 SDK `IRRemoteMethod` / `IRRemoteInvocation` / `IRWorker`):
/// 支持 ① 裸对象地址、寄存器名(`$x0..$x28/$fp/$lr/$sp/$pc/$cpsr`)或类名 (`0x...` / `ClassName`, `p` 走纯内存免 worker, `po` 调 `-description`);
///      ② Objective-C 方法调用 `[recv sel:arg1 ...]` (支持实例地址、类名、`$reg` 寄存器引用、嵌套 `[...]` / `func(...)` 表达式);
///      ③ C 函数调用 `[(retType)]func(arg1, arg2, ...)` (支持导出符号名、`Module+0xOffset`、IDA 静态地址或运行时地址);
///      ④ 空格简写 `0x... sel arg1 ...`。
- (nullable IRExpressionResult *)evaluateExpression:(NSString *)expression
                                        printObject:(BOOL)printObject
                                       onMainThread:(BOOL)onMainThread
                                          imageName:(nullable NSString *)imageName
                                              error:(NSError **)error;

/// 指定目标线程上下文(`IRThread.mainThread` / `IRThread.childThread` / 具体 `IRRemoteThread *`)评估表达式。
/// 当传入具体 `IRRemoteThread *` 时，表达式内的 `$x0..$x28/$fp/$lr/$sp/$pc` 自动绑定该线程的 `cpu` 寄存器快照。
- (nullable IRExpressionResult *)evaluateExpression:(NSString *)expression
                                        printObject:(BOOL)printObject
                                             thread:(nullable IRThread *)thread
                                          imageName:(nullable NSString *)imageName
                                              error:(NSError **)error;

@end

NS_ASSUME_NONNULL_END
