//
//  IRUITask.h
//  IRAutomationKit
//
//  UI 任务: 会操作 UI 的任务基类。自动化 (IRAutomationTask) 和中断 (IRInterruptTask) 都继承它,
//  由应用分配 UI 执行权, 业务不直接创建 IRUITask。
//
//  - main 在 SDK 自建的任务线程上执行 (每个任务一条线程)
//  - 挂起: 直接挂起线程, main 停在当前这一行, 恢复后从这一行继续; SDK 的原子操作 (如一个手势) 内则延后到它结束
//  - 取消 / 中止 (应用退出等): 下一个检查点 (checkpoint / sleep: / SDK 操作) 抛出 IRAutomationAbortException,
//    由 SDK 接住; 等 main 真正退出后才进入终态。也可以随时读 isCancelled 自己退出
//

#import <IRAutomationKit/IRTask.h>

@class IRScreenState;
@class IRPageAgent;

NS_ASSUME_NONNULL_BEGIN

/// 中止异常: 任务被取消 / 中止时由检查点抛出 (SDK 接住, 业务不要吞掉)
FOUNDATION_EXPORT NSExceptionName const IRAutomationAbortException;

/// 自动化错误域
FOUNDATION_EXPORT NSErrorDomain const IRAutomationErrorDomain;

typedef NS_ERROR_ENUM(IRAutomationErrorDomain, IRAutomationErrorCode) {
    IRAutomationErrorApplicationTerminated = 1,   ///< 应用已退出
    IRAutomationErrorInterruptUnresolved   = 2,   ///< 弹窗关不掉 (同一弹窗连续处理失败, 已放弃)
};

@interface IRUITask : IRTask

/// 当前线程所属的 UI 任务 (不在任务线程上返回 nil)
@property (class, readonly, nullable) IRUITask *currentTask;

/// 已被取消 / 中止 (任何线程可读): 两步操作之间可以检查它, 自己提前退出
@property (readonly, getter=isCancelled) BOOL cancelled;

/// 检查点: 已取消 / 中止时抛出 IRAutomationAbortException。
/// 只在本任务的线程上有效; 不调用任何 SDK 操作的长循环里要主动调用
- (void)checkpoint;

/// 睡眠 (分段, 每段都过检查点, 可被中止)
/// - Parameter seconds: 秒
- (void)sleep:(NSTimeInterval)seconds;

#pragma mark 共同能力 (自动化 / 中断两个 UITask 相同)

/// 当前感知结果: 正在扫描就等这一轮, 上一轮扫完不到 0.4 秒就直接用, 否则立刻扫一轮 (最多 2 秒, 超时 nil); 等待期间过检查点
- (nullable IRScreenState *)currentScreen;

/// 当前显示的页面 (= currentScreen.pageAgent)
- (nullable IRPageAgent *)currentAgent;

// 操作层 (下一步): tapAtPoint: / swipeFrom:to:duration: / inputText: / waitUntil:timeout:

@end

NS_ASSUME_NONNULL_END
