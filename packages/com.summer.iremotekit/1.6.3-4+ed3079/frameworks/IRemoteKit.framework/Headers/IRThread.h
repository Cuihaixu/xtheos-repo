//
//  IRThread.h — 三层远程线程继承体系与免 worker 调用栈回溯(L0)
//
//  三层继承结构:
//    1. IRThread (L1, type = IRThreadTypeDescriptor):
//       抽象线程基类与执行目标描述符 (+mainThread / +childThread 单例)，100% 兼容既有调用接口。
//    2. IRRemoteThread : IRThread (L2, type = IRThreadTypeRemote):
//       目标进程内的真实业务线程实例，承载线程身份(tid/name)、调度状态(runState/suspended)、
//       CPU 寄存器快照(thread.cpu)、幂等布尔挂起/恢复(-suspend:/-resume:)、安全寄存器回写(-flushCPU:)
//       以及 100% 免 worker 的跨进程 FP 链调用栈回溯(-backtraceWithMaxFrames:error:)。
//    3. IRWorkerThread : IRRemoteThread (L3, type = IRThreadTypeWorker):
//       IRemoteKit 在目标进程内引导驻留的常驻工作线程。继承全部只读观测能力(可读 cpu/backtrace)，
//       但通过多态重写拦截 -suspend: / -resume: / -flushCPU:，实现强制只读保护。
//

#import <Foundation/Foundation.h>
#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRSession;
@class IRCPUContext;
@class IRSymbolInfo;

/// 线程对象层级类型。
typedef NS_ENUM(NSInteger, IRThreadType) {
    IRThreadTypeDescriptor = 0, ///< 抽象执行目标描述符 (+mainThread / +childThread，不绑定具体线程实例)
    IRThreadTypeRemote     = 1, ///< 目标进程内的真实业务线程 (IRRemoteThread)
    IRThreadTypeWorker     = 2, ///< IRemoteKit 驻留的远程工作线程 (IRWorkerThread)
};

/// 线程内核调度状态 (对齐 Mach `TH_STATE_*`)。
typedef NS_ENUM(NSInteger, IRThreadRunState) {
    IRThreadRunStateUnknown         = 0,
    IRThreadRunStateRunning         = 1, ///< TH_STATE_RUNNING (正在 CPU 上运行或就绪)
    IRThreadRunStateStopped         = 2, ///< TH_STATE_STOPPED (已停止)
    IRThreadRunStateWaiting         = 3, ///< TH_STATE_WAITING (正常阻塞等待事件/锁/RunLoop)
    IRThreadRunStateUninterruptible = 4, ///< TH_STATE_UNINTERRUPTIBLE (不可中断等待)
    IRThreadRunStateHalted          = 5, ///< TH_STATE_HALTED (已干净暂停)
};

#pragma mark - 调用栈单帧 (IRStackFrame)

/// 免 worker 跨进程调用栈回溯的单帧信息。
@interface IRStackFrame : NSObject

/// 帧序号 (`0` = 当前 `PC`, `1` = `LR` / 上一层调用者, ...)。
@property (nonatomic, readonly) NSUInteger index;
/// 该帧的指令地址 (`arm64e` 下已自动剥除 PAC 签名)。
@property (nonatomic, readonly) IRRemoteAddress pc;
/// 该帧对应的栈帧指针 `FP` (`x29`)；第 0 帧为当前 `FP`。
@property (nonatomic, readonly) IRRemoteAddress fp;
/// 该帧指令地址对应的符号化结果 (`Module!Symbol + offset [IMG]`)。
@property (nonatomic, readonly, nullable) IRSymbolInfo *symbol;

/// 对标 LLDB `bt` 的单行格式化输出 (如 `frame #0: 0x00000001da8c3b48 libsystem_kernel.dylib`mach_msg2_trap + 8`)。
- (NSString *)formattedDescription;
/// 转为 JSON 友好的字典表示。
- (NSDictionary<NSString *, id> *)dictionaryRepresentation;

@end

#pragma mark - L1: 线程抽象基类 (IRThread)

/// L1 线程抽象基类：既提供 `+mainThread` / `+childThread` 抽象执行目标描述符，又作为 `IRRemoteThread` 的多态基类。
@interface IRThread : NSObject

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;

/// 目标主线程抽象描述符：供 `IRCallOptions.thread` / `IRWorker` 指定将调用投递到目标主线程执行。
@property (class, nonatomic, readonly) IRThread *mainThread;

/// 子线程抽象描述符：供 `IRCallOptions.thread` / `IRWorker` 指定在 worker 自身子线程上执行 (默认)。
@property (class, nonatomic, readonly) IRThread *childThread;

/// 当前线程对象的层级类型 (`Descriptor` / `Remote` / `Worker`)。
@property (nonatomic, readonly) IRThreadType type;

/// 是否代表目标主线程 (`+mainThread` 或 1 号主线程实例均为 `YES`)。
@property (nonatomic, readonly) BOOL isMainThread;

@end

#pragma mark - L2: 目标进程真实线程 (IRRemoteThread)

/// L2 目标进程内的真实远程线程实例 (`type == IRThreadTypeRemote`)。
/// 生命周期内持有目标线程的 Mach send right，`dealloc` 时自动 `mach_port_deallocate` 释放。
@interface IRRemoteThread : IRThread

/// 所属目标进程会话。
@property (nonatomic, weak, readonly, nullable) IRSession *session;

/// 1-based 线程序号 (`#1` 恒为主线程，与 LLDB `thread list` 对齐)。
@property (nonatomic, readonly) NSUInteger index;

/// 全局唯一线程 ID (`THREAD_IDENTIFIER_INFO.thread_id`，对应 LLDB `tid`)。
@property (nonatomic, readonly) uint64_t threadID;

/// 本进程空间内的 Mach `thread_act_t` 端口名 (`dealloc` 自动释放)。
@property (nonatomic, readonly) uint32_t machThreadPort;

/// pthread 线程名 (`pth_name`)；未命名线程为 `nil` 或空串。
@property (nonatomic, readonly, nullable) NSString *name;

/// 是否为 `IRemoteKit` 内部驻留的 worker 线程 (`IRWorkerThread` 实例恒为 `YES`，普通业务线程为 `NO`)。
@property (nonatomic, readonly) BOOL isWorkerThread;

#pragma mark 状态与 CPU 寄存器快照 (免 worker)

/// 内核调度状态 (`Running` / `Waiting` / `Stopped` / `Uninterruptible` / `Halted`)。
@property (nonatomic, readonly) IRThreadRunState runState;

/// 内核调度状态的可读名称 (`"RUNNING"` / `"WAITING"` / `"STOPPED"` ...)。
@property (nonatomic, readonly) NSString *runStateName;

/// 该线程自身的内核挂起计数 (`THREAD_BASIC_INFO.suspend_count`)。
@property (nonatomic, readonly) NSInteger suspendCount;

/// 该线程当前是否处于挂起冻结状态 (`self.suspendCount > 0` 或所属 `session.isSuspended == YES`)。
@property (nonatomic, readonly, getter=isSuspended) BOOL suspended;

/// 该线程的 CPU 寄存器上下文静态快照 (`thread.cpu.x0.value` / `.cstrValue` / `.objectValue` / `thread.cpu.pc.symbol`)。
/// - Note: 首次访问时懒加载采样并缓存在本实例上（若线程处于运行态，内部自动执行 ~10µs 单线程瞬时 pause 采样以保证寄存器快照一致）；
///   多次访问 `.cpu.x0` / `.cpu.pc` 均读取同一份定格快照。需获取最新状态时请调用 `-refresh`。
@property (nonatomic, readonly, nullable) IRCPUContext *cpu;

/// 重新从内核采样刷新本线程的调度状态 (`runState` / `suspended` / `name`) 并重置 `cpu` 快照缓存。
- (BOOL)refresh;

/// 将对 `self.cpu` 寄存器的修改写回内核线程状态 (`thread_set_state`)。
/// - Warning: 仅允许在 `self.isSuspended == YES` (线程或进程已挂起) 时调用；运行态或 `IRWorkerThread` 调用将直接拒绝并置 `error`。
///   内部保留 `thread_get_state` 原始 `__opaque_*` PAC 签名字段，支持在 `arm64e` 下安全修改 `x0..x28` 与 `v0..v31`。
- (BOOL)flushCPU:(NSError **)error;

#pragma mark 线程级幂等挂起与恢复 (布尔开关语义)

/// 挂起当前线程 (幂等布尔开关: 若该线程自身 `suspendCount > 0` 已处于挂起态则直接返回 `YES`，绝不叠加计数)。
- (BOOL)suspend:(NSError **)error;

/// 恢复当前线程 (幂等布尔开关: 循环 `thread_resume` 将该线程自身的 `suspendCount` 彻底清零为 `0`)。
- (BOOL)resume:(NSError **)error;

#pragma mark 免 worker 调用栈回溯

/// 沿帧指针链 (`FP Chain`, `[fp, fp+8]`) 跨进程免 worker 回溯当前线程调用栈，自动剥除 `arm64e` PAC 签名并符号化。
/// - Parameters:
///   - maxFrames: 最大回溯帧数 (传 `0` 默认 `64` 帧)
///   - error: 失败时置错误
/// - Returns: 从第 0 帧 (`PC`) 开始的调用栈帧数组
- (nullable NSArray<IRStackFrame *> *)backtraceWithMaxFrames:(NSUInteger)maxFrames
                                                       error:(NSError **)error;

/// 对标 LLDB `thread list` 的单行摘要描述。
- (NSString *)formattedDescription;
/// 转为 JSON 友好的字典表示。
- (NSDictionary<NSString *, id> *)dictionaryRepresentation;

@end

#pragma mark - L3: 框架驻留工作线程 (IRWorkerThread)

/// L3 `IRemoteKit` 在目标进程内引导驻留的常驻工作线程 (`type == IRThreadTypeWorker`)。
/// 可正常读取 `.cpu` 寄存器快照与 `-backtraceWithMaxFrames:error:` 进行自省诊断，
/// 但重写拦截了 `-suspend:` / `-resume:` / `-flushCPU:` (只读保护，防止破坏 worker 状态机)。
@interface IRWorkerThread : IRRemoteThread

/// 该 worker 线程当前是否安静驻留在 `rop_loop` (`b .` gadget) 挂起待命 (0 CPU)。
@property (nonatomic, readonly, getter=isParked) BOOL parked;

/// 该 worker 驻留的 `b .` gadget 远程地址。
@property (nonatomic, readonly) IRRemoteAddress ropLoopAddress;

@end

NS_ASSUME_NONNULL_END
