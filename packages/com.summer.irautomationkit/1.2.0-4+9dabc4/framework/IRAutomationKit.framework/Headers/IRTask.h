//
//  IRTask.h
//  IRAutomationKit
//
//  Created by cu on 2026/10/20.
//
//  任务基类 (接口参考 NSOperation)。每个 App 的任务由它自己的 IRApplication 管理。
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRApplication.h>

NS_ASSUME_NONNULL_BEGIN

/// 任务状态 (同一时间只有一种)
typedef NS_ENUM(NSInteger, IRTaskState) {
    IRTaskStatePending = 0,   ///< 未开始: 已添加, 等待可以运行的应用状态
    IRTaskStateRunning,       ///< 运行中 (间隔任务两次执行之间也算运行中)
    IRTaskStateSuspended,     ///< 已挂起
    IRTaskStateCompleted,     ///< 已完成 (终态): error 为 nil 表示成功, 否则失败 (含应用退出中止)
    IRTaskStateCancelled,     ///< 已取消 (终态): 只有 -[IRApplication removeTask:] 才会进入
};

#pragma mark - IRTask

@interface IRTask : NSObject

/// 任务名称 (日志/查看用)
@property (nonatomic, copy, readonly) NSString *name;

/// 所属应用 (addTask: 之后有值)
@property (nonatomic, weak, readonly, nullable) IRApplication *application;

/// 当前状态 (KVO)。只由所属应用写 (在应用的 queue 上), 任务只读;
/// 任务通过 main 返回 / failWithError: 上报结果, 由应用完成实际操作(停定时器/挂起线程等)后再更新状态
@property (readonly) IRTaskState state;

/// 失败原因 (Completed 时失败才有值; 业务 failWithError: / SDK 操作失败 / 应用退出)
@property (readonly, nullable) NSError *error;

/// 结束回调 (进入 Completed / Cancelled 后回调一次; 在所属应用的 queue 上执行)
/// 先回调再从应用的 tasks 中自动移除 (回调时任务仍在 tasks 中); 自动移除不改变状态和 error
@property (nonatomic, copy, nullable) void (^completionBlock)(void);

- (instancetype)initWithName:(NSString *)name NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

/// 任务内容 (子类重写)。由所属应用调用, 不要自己调用
///
/// 何时调用:
/// - 只调用一次: 添加后第一次 canRunInState: 返回 YES 时, 在任务的执行环境(线程)中调用, Pending → Running
/// - 不在应用的 queue 上执行, 可以阻塞/等待
/// - 挂起后恢复不会重新调用: main 停在当前这一行, 恢复后从这一行继续
/// - 一直未满足运行条件则不会调用; 此间被移除直接进入 Cancelled
///
/// 退出后:
/// | main 怎么退出                              | 最终状态   | error            |
/// | 正常返回                                   | Completed | nil              |
/// | failWithError: 后返回                      | Completed | 业务给出的错误     |
/// | 被 removeTask:, 下一个 SDK 操作抛出中止     | Cancelled | nil              |
/// | 应用退出, 下一个 SDK 操作抛出中止           | Completed | 应用已退出         |
/// | 其他未捕获异常 (由应用接住, 不崩宿主进程)    | Completed | 由异常包装的错误   |
/// 之后依次: 状态进入终态 → taskDidChangeStateFrom: → completionBlock(一次) → 从应用的 tasks 移除、释放执行线程;
/// 任务不可再次启动
///
/// 注意: main 必须能退出。中止依赖"下一个 SDK 操作抛出异常", 应用不会强杀线程;
/// 长循环要检查 self.state == IRTaskStateCancelled 或使用 SDK 的 sleep, 否则任务永远结束不了
- (void)main;

/// 上报失败 (调用后应尽快返回)。只是上报, state 不会立即改变; 由应用处理后以 Completed + error 结束
- (void)failWithError:(NSError *)error;


#pragma mark 生命周期 (子类重写; 均在所属应用的 queue 上调用, 必须快速返回, 不做远程调用)

/// 该应用状态下是否可以运行 (默认仅 Active 返回 YES)。
/// 应用状态变化时询问: 返回 NO 应用挂起任务, 返回 YES 应用启动/恢复任务。用掩码与 state 按位与即可:
///
///     - (BOOL)canRunInState:(IRApplicationState)state {
///         return (IRApplicationStateActive | IRApplicationStateInactive) & state;
///     }
///
/// - Parameter state: 应用的新状态 (单个状态)
- (BOOL)canRunInState:(IRApplicationState)state;

/// 任务状态已变化 (挂起/恢复/结束由应用完成, 这里只做善后, 如进后台前保存数据、结束时释放资源)
/// - Parameter oldState: 变化前的状态 (新状态读 self.state)
- (void)taskDidChangeStateFrom:(IRTaskState)oldState;

@end

NS_ASSUME_NONNULL_END
