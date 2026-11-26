//
//  IRCallOptions.h — 单次远程调用的可扩展配置(L1)
//
//  单次调用的**派发配置**:传给 `-[IRWorker invoke:options:]`(以及 `-[IRObject call:arguments:options:]`
//  / 语法糖 `.remote`/`.main`/`.child`)。装的是"这次怎么调"里**不绑 worker 资源、逐调用可变**的轴——
//  线程(child/main)、是否冻结其它线程。**worker 只持不可变的 `executionStrategy`;per-call 的都在这里。**
//  底层兜底容器:将来 per-call 新需求(超时 / 返回持有档…)只往这里加字段,invoke/兜底/语法糖入口都不变,
//  加属性不影响接口整洁与适配性。`nil` options 一律等价于全默认(子线程 + **不冻**其它线程)。
//

#import <IRemoteKit/IRTypes.h>

@class IRWorker;
@class IRObject;
@class IRThread;

NS_ASSUME_NONNULL_BEGIN

/// 单次远程调用配置。`nil` = 全默认。
@interface IRCallOptions : NSObject <NSCopying>

/// 【底层真身】本次调用的目标执行线程（默认 `[IRThread childThread]`，即在当前 worker 子线程上执行）。
/// 可传抽象标签（`[IRThread mainThread]` / `[IRThread childThread]`）或具体远程线程实例（`IRRemoteThread *`）。
/// 传 `nil` 时自动重置为 `[IRThread childThread]`。
@property (nonatomic, strong, null_resettable) IRThread *thread;

/// 【便利开关】是否在目标**主线程**执行（等价于读/写 `self.thread`）：
/// - 读取时返回 `self.thread.isMainThread`；
/// - 设置为 `YES` 时将 `self.thread` 置为 `[IRThread mainThread]`，设置为 `NO` 时置为 `[IRThread childThread]`。
@property (nonatomic) BOOL mainThread;

/// 调用期是否冻结目标其它线程(默认 `NO`)。
/// **默认不冻**:多数远程方法自身要取锁,冻住持锁的主线程会死锁,且冻结奇贵(task_threads + 逐条 suspend,~6x)。
/// 仅当"这次是**无锁**操作、又怕对象图被并发释放/修改"才逐调用显式 `YES` 取一致性快照。
/// 这是**逐调用**的决定(不该固定在 worker 上)——同一条 worker 可以这次冻、下次不冻;主线程执行强制不冻。
@property (nonatomic) BOOL suspendOthers;

/// 本次调用超时(毫秒)。`0` = 继承会话默认(`IRSession.defaultTimeoutMs`)。
/// `>0` 仅覆盖这一次(重对象 / 慢方法临时放宽;超时仍不强断线程,只毒化本 worker)。
@property (nonatomic) uint32_t timeoutMs;

/// 调用前回调:已在借到的那条 worker 的**同一作用域**里执行,可先做别的远程调用。`nil` = 不执行。
@property (nonatomic, copy, nullable) void (^beforeCall)(IRWorker *worker);
/// 调用后回调:在**同一作用域**退出前执行,拿到本次结果(`result.error` 带失败原因,`+0` 返回值此刻仍有效),
/// 可接着读 / 调。调用本身失败时也照调。`nil` = 不执行。
@property (nonatomic, copy, nullable) void (^afterCall)(IRWorker *worker, IRObject *result);

// 将来:@property (nonatomic) IRObjectOwnership returnOwnership; // 对象返回持有档
// —— 加字段即可,invoke:options: / call:arguments:options: / .remote/.main/.child 都不动。

@end

NS_ASSUME_NONNULL_END
