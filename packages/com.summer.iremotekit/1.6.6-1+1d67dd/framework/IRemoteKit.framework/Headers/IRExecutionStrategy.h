//
//  IRExecutionStrategy.h — 执行策略(worker 怎么执行、怎么察觉返回)(L0)
//
//  术语:「调用」= 动作层(driver 发起的远程调用);「执行」= 承载层(worker 怎么把它跑起来)。
//  IRExecutionStrategy 属承载层,与【执行者 IRWorker】【执行线程 IRThread】同族,描述一条
//  `IRWorker` 被劫持跑完后"怎么察觉返回、读回状态",设给 `IRWorker.executionStrategy`:
//    [IRExecutionStrategy exceptionWait]  返回落只读守卫页 → 线程级异常端口 mach_msg 即时唤醒
//                                         (默认;µs 级 + 精确超时)
//    [IRExecutionStrategy hwbpWait]        返回落 rop_loop(设硬件断点)→ EXC_BREAKPOINT → 端口即时唤醒
//                                         (免守卫页;每 worker 设一次调试寄存器)
//    [IRExecutionStrategy spinPoll]       返回落 `b .` → 忙轮询 pc(无端口依赖,烧 CPU、超时粗糙)
//  用【对象 / 单例】而非枚举,是为将来可携带更多配置(轮询退避阈值 / 守卫页选项 / 超时细调…)时
//  不破坏接口。执行者始终是 `IRWorker`;线程级端口 / 守卫页每 worker 各一份。
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// worker 执行策略:怎么察觉返回 + 超时(`exceptionWait` / `spinPoll`);创建时固定于 worker,不可变。
@interface IRExecutionStrategy : NSObject

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;

/// 守卫页 + 线程级异常端口即时唤醒,默认超时 3000ms。
@property (class, nonatomic, readonly) IRExecutionStrategy *exceptionWait;

/// rop_loop 硬件断点 + 线程级异常端口即时唤醒(免守卫页),默认超时 3000ms。
@property (class, nonatomic, readonly) IRExecutionStrategy *hwbpWait;

/// `b .` 忙轮询,默认超时 3000ms。
@property (class, nonatomic, readonly) IRExecutionStrategy *spinPoll;

/// 自定义超时的异常端口策略。
/// - Parameter timeoutMs: 调用超时(毫秒)
- (instancetype)initExceptionWaitWithTimeoutMs:(uint32_t)timeoutMs;

/// 自定义超时的硬件断点策略。
/// - Parameter timeoutMs: 调用超时(毫秒)
- (instancetype)initHwbpWaitWithTimeoutMs:(uint32_t)timeoutMs;

/// 自定义超时的忙轮询策略。
/// - Parameter timeoutMs: 调用超时(毫秒)
- (instancetype)initSpinPollWithTimeoutMs:(uint32_t)timeoutMs;

/// 调用超时(毫秒)。**超时不强断线程**(会丢锁带崩目标),改毒化 worker。
/// 策略对象不可变;要换超时须新建策略(并配新 worker)。
@property (nonatomic, readonly) uint32_t callTimeoutMs;

@end

NS_ASSUME_NONNULL_END
