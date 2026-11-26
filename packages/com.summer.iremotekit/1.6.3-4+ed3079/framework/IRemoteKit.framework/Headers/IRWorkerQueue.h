//
//  IRWorkerQueue.h — 远程调用队列(串行 / 并发调度层)(L0)
//
//  在 `IRWorker`(一条远程线程)之上的调度层,GCD 类比:
//    串行:背后 1 条 worker,提交的调用按顺序串行;
//    并发:背后一池 worker(至多 maxConcurrency 条),提交的调用并行。
//  提交用 `run:`(同步 / 异步由队列类型定),块内 `IRObject` 调用自动落在本次分派的那条 worker(当前 worker 栈)。
//  队列拥有并 RAII 管理名下 worker;`close` / dealloc 时全部销毁。worker 懒建(首次分派时)。
//
//  【并发安全前提】并发队列上并行的调用必须落在【互不相干的对象】上——并发时不冻结保一致性
//  (会冻死兄弟 worker);撞同一被目标改的对象 → 竞争 fault(异常守卫兜底,可重试)。由调用方负责。
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRSession;

/// 远程调用队列(串行 / 并发调度层),在 `IRWorker` 之上;`run:` 提交,同步/异步由队列类型定。
@interface IRWorkerQueue : NSObject

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;

#pragma mark - 状态

/// 所属会话。
@property (nonatomic, readonly, weak) IRSession *session;
/// 是否并发队列(串行为 `NO`)。
@property (nonatomic, readonly, getter=isConcurrent) BOOL concurrent;
/// 最大并发:串行 = 1;并发 = 实际上限(构造传 `0` 时已解析成 CPU 核心数)。
@property (nonatomic, readonly) NSUInteger maxConcurrency;

#pragma mark - 构造

/// 串行队列:背后 1 条 worker,提交的调用按顺序串行执行。
/// - Parameter session: 目标会话(worker 挂在它上)
- (instancetype)initSerialQueueForSession:(IRSession *)session;

/// 并发队列:背后至多 `maxConcurrency` 条 worker,提交的调用并行执行。
/// - Parameters:
///   - session: 目标会话
///   - maxConcurrency: 最大并发(worker 池上限);**传 `0` = CPU 核心数**
- (instancetype)initConcurrentQueueForSession:(IRSession *)session
                               maxConcurrency:(NSUInteger)maxConcurrency;

#pragma mark - 提交

/// 提交一串调用到本队列,块内 `IRObject` 调用落在本队列分派的 worker 上。
///
/// **同步 / 异步由队列类型决定**(不再分两个方法):
///   - 串行队列:**同步**——阻塞至完成,返回块结果;
///   - 并发队列:**异步**——取一条空闲 worker(满则排队)提交后立即返回 `nil`,与其它提交并行;
///     结果自行在块内消费(如写入线程安全容器)。
/// - Parameter block: 调用序列(串行时返回值带出)
/// - Returns: 串行 = 块返回值(可空;建 worker 失败见 `session.lastError`);并发 = 立即 `nil`
- (nullable id)run:(id _Nullable (^)(void))block;

#pragma mark - 收尾

/// 销毁池中所有 worker、还原线程级端口。幂等。
- (void)close;

@end

NS_ASSUME_NONNULL_END
