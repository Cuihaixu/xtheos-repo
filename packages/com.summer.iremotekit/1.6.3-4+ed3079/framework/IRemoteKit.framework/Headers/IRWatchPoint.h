//
//  IRWatchPoint.h — 远程数据读写监控:单个观察点(L1 原语)
//
//  对标 lldb `memory watchpoint`,但【不停目标】:命中→异步回调→自动续跑(无注入自动化)。
//  两后端(内部,见 IRWatchBackend):HW 调试寄存器(字段级、per-thread、低开销)/ 守卫页
//  mprotect(整段、task-wide、慢但全)。本类是对外的【共同面】——后端用 `backend` 选,
//  能力经 `backendUsed`/`coversAllThreads`/`exactRange` 诚实回读,不拆子类。
//
//  生命周期:构造(本类)→ `-[IRWatchPointManager addWatchPoint:error:]`(武装)
//            → `disable`/`enable`(暂停/恢复,保留注册)→ `deleteWatchPoint:`(移除)。
//  策略(条件/限频/计数)不在此层:调用方在 `onEvent` 回调里自理;命令层才抽象 -c/--max 等。
//
//  opt-in:经 IRToolkit.h 引入,不在默认伞 IRemoteKit.h。
//

#import <Foundation/Foundation.h>
#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRCPUContext;

/// 监控的访问类型(可或)。
typedef NS_OPTIONS(NSUInteger, IRWatchAccess) {
    IRWatchRead  = 1 << 0,   ///< 读
    IRWatchWrite = 1 << 1,   ///< 写
};

/// 后端选择。
typedef NS_ENUM(NSInteger, IRWatchBackendKind) {
    IRWatchBackendAuto = 0,    ///< 小 + 指定线程 → HW;大 / 全线程 / 整段 → 守卫页
    IRWatchBackendHardware,    ///< 强制 ARM 调试寄存器(小内存也可逼走 HW)
    IRWatchBackendGuardPage,   ///< 强制 vm_protect 守卫页
};

/// 一次命中事件。回调在 Manager 的监听线程(非目标线程)上派发。
@interface IRWatchEvent : NSObject

/// 实际被访问的地址(落在观察范围内)。
@property (nonatomic, readonly) IRRemoteAddress dataAddress;
/// 触发该次访问的指令地址(目标线程 PC)。
@property (nonatomic, readonly) IRRemoteAddress pc;
/// 本次触发是读还是写(单一值:`IRWatchRead` 或 `IRWatchWrite`)。
@property (nonatomic, readonly) IRWatchAccess access;
/// 触发访问的目标线程 id(mach thread)。
@property (nonatomic, readonly) uint64_t threadId;
/// 写前值(按 `length`;> 8 字节取低 8 字节)。读:等于 `newValue`。
@property (nonatomic, readonly) uint64_t oldValue;
/// 写后值(> 8 字节取低 8 字节)。
@property (nonatomic, readonly) uint64_t newValue;

#pragma mark - 命中时 CPU 上下文(调试 / 取参 / 回溯调用者)
//
// 命中那一刻(单步放行前)目标线程的寄存器快照。命中回调期间该线程仍冻结在内核等异常
// 应答,读到的是触发访问时的真实现场。arm64e 下 fp/lr/sp 已按 PAC 规则 strip 成裸地址。

/// 统一的 ARM64 CPU 寄存器上下文快照 (`event.cpu.x0.value` / `.cstrValue` / `.objectValue` / `event.cpu.pc.symbol`)。
/// 未抓到寄存器快照时为 `nil`。
@property (nonatomic, readonly, nullable) IRCPUContext *cpu;

/// 是否抓到寄存器快照(`thread_get_state` 失败则 NO,下面各访问器返回 0)。
@property (nonatomic, readonly) BOOL hasCPUState;
/// 通用寄存器 x0–x28(`index` 0–28,越界返回 0)。x0–x7 即调用参数。
- (uint64_t)generalRegister:(NSUInteger)index;
/// x29 帧指针(配 `lr` 顺链回溯调用栈)。
@property (nonatomic, readonly) uint64_t fp;
/// x30 返回地址(调用者 PC)。
@property (nonatomic, readonly) uint64_t lr;
/// 栈指针。
@property (nonatomic, readonly) uint64_t sp;
/// 程序状态寄存器(cpsr)。
@property (nonatomic, readonly) uint64_t cpsr;

@end

/// 命中回调。在 Manager 监听线程(非目标线程)派发。
///
/// 【同步 hold 语义】回调执行期间,触发访问的那条目标线程**一直冻结在内核**(等异常应答),
/// 回调返回才放行续跑 —— 相当于 lldb 断点停住。期间你可以:
///   · 读现场:`event` 的寄存器(`generalRegister:`/`lr`/`fp`…)、`mach_vm_read` —— 无条件安全;
///   · 调函数:用 **worker**(独立线程)`-[IRWorker invoke:…]` —— 实测与观察点互不干扰(arb_call
///     异常端口是线程级,观察点是任务级,优先级分离);碰被盯区域也不死锁。
/// 【唯一禁区】**别把调用投递到观察点可能冻住的线程**——典型即:盯着主线程时在回调里用
///   `options.mainThread=YES`。主线程正被你冻住 → 派发永远跑不了 → 卡满超时(默认 3s)调用端拿 nil,
///   并泄漏一条卡死 worker(目标不崩,但白等 3 秒)。
/// 回调别太长:被冻线程(及排队的其它 fault)全程挂起。过滤/计数/条件在回调内自理。
typedef void (^IRWatchHandler)(IRWatchEvent *event);

/// 单个观察点(L1 原语)。标准构造(A)本类 + `addWatchPoint:`;便捷构造(B)见
/// `IRWatchPointManager (Convenience)` 的 `watchAddress:…`。
@interface IRWatchPoint : NSObject

/// 标准构造。构造只记配置,未武装;交 `-[IRWatchPointManager addWatchPoint:error:]` 才生效。
/// - Parameters:
///   - address: 起始地址(堆/栈/镜像/全局均可)
///   - length: 字节数(HW 后端 ≤ 8;守卫页任意)
///   - access: 读 / 写 / 读写
///   - backend: 后端选择(`Auto` / 强制 `Hardware` / `GuardPage`)
///   - threadIds: 只武装这些目标线程(`nil` = 全部);HW 据此省开销,守卫页 task-wide 忽略之
///   - onEvent: 命中回调(在监听线程派发,勿在内做重阻塞;过滤 / 计数 / 条件在此自理)
- (instancetype)initWithAddress:(IRRemoteAddress)address
                         length:(size_t)length
                         access:(IRWatchAccess)access
                        backend:(IRWatchBackendKind)backend
                      threadIds:(nullable NSArray<NSNumber *> *)threadIds
                        onEvent:(IRWatchHandler)onEvent;

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;

#pragma mark - 配置(只读)

@property (nonatomic, readonly) IRRemoteAddress    address;
@property (nonatomic, readonly) size_t             length;
@property (nonatomic, readonly) IRWatchAccess      access;
@property (nonatomic, readonly) IRWatchBackendKind requestedBackend;   ///< 请求的后端(可能是 Auto)
/// 请求只武装的线程(`nil` = 全部)。
@property (nonatomic, readonly, nullable) NSArray<NSNumber *> *threadIds;

#pragma mark - 状态 / 能力回读(`addWatchPoint:` 之后有效)

/// 实际采用的后端(`Auto` 会落到 `Hardware` 或 `GuardPage`)。
@property (nonatomic, readonly) IRWatchBackendKind backendUsed;
/// 是否覆盖所有线程。HW = 仅已武装线程(NO);守卫页 = YES。
@property (nonatomic, readonly) BOOL coversAllThreads;
/// 命中是否精确到被盯范围。HW = YES;守卫页 = 页级、按范围过滤(NO)。
@property (nonatomic, readonly) BOOL exactRange;
/// 累计命中次数。
@property (nonatomic, readonly) uint64_t hitCount;
/// 当前是否在武装态(`disable` 后为 NO,`deleteWatchPoint:` 后亦为 NO)。
@property (nonatomic, readonly, getter=isActive) BOOL active;

#pragma mark - 暂停 / 恢复武装(保留注册与配置)

/// 恢复武装(清→设寄存器 / 去页权限)。幂等。
- (void)enable;
/// 暂停武装(保留注册,可 `enable` 再开)。幂等。`active` 置 NO。
- (void)disable;

@end

NS_ASSUME_NONNULL_END
