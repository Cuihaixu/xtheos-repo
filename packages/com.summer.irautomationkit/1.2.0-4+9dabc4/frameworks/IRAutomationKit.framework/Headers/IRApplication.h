//
//  IRApplication.h
//  IRAutomationKit
//
//  Created by cu on 2026/10/12.
//

#import <Foundation/Foundation.h>
#import <IRemoteKit/IRSession.h>
#import <IRAutomationKit/IRPageAgent.h>
#import <IRAutomationKit/IRScreenState.h>

NS_ASSUME_NONNULL_BEGIN

@class IRApplication;
@class IRTask;
@class IRAutomationTask;

/// 应用运行状态 (与 CHRTouchKit 状态一一对应)
/// 每个状态占一位: `state` 属性永远只有一位; 组合多个状态用作掩码 (如 IRTask -canRunInState:)
/// 位从低到高按"越来越可用"排列, 可用大小比较 (state >= Inactive 即在前台)
typedef NS_OPTIONS(NSUInteger, IRApplicationState) {
    IRApplicationStateNotRunning = 1 << 0,   ///< 未运行(无会话)
    IRApplicationStateSuspended  = 1 << 1,   ///< 已挂起(进程在内存但被冻结)
    IRApplicationStateBackground = 1 << 2,   ///< 后台运行
    IRApplicationStateInactive   = 1 << 3,   ///< 前台但失活(下拉通知/控制中心/系统弹窗覆盖)
    IRApplicationStateActive     = 1 << 4,   ///< 前台活跃可交互
};

/// 应用生命周期代理 (对标 UIApplicationDelegate)
/// - 所有回调都在应用主队列 (`application.queue`) 上执行
/// - 外部事后观测: 除 WillResignActive 外均为事件已发生后回调
/// - 边沿触发: 设置代理前已处于的状态不补发回调, 读 `state` 获取当前状态
@protocol IRApplicationDelegate <NSObject>
@optional
/// 启动: 检出新进程(冷启动), 已尝试建立会话 (非"启动完成")
- (void)applicationDidLaunch:(IRApplication *)application;
/// 进前台: 从后台回到前台 (热启动); 前台内失活恢复不触发
- (void)applicationDidEnterForeground:(IRApplication *)application;
/// 激活: 前台且可交互
- (void)applicationDidBecomeActive:(IRApplication *)application;
/// 失活: 前台失去交互 (下拉通知/控制中心/系统弹窗)
- (void)applicationWillResignActive:(IRApplication *)application;
/// 进后台
- (void)applicationDidEnterBackground:(IRApplication *)application;
/// 挂起: 后台被冻结
- (void)applicationDidSuspend:(IRApplication *)application;
/// 终止: 进程已退出, 会话已关闭
/// - Parameter reason: 退出原因 (可辨正常退出/崩溃/被杀), 可能为 nil
- (void)applicationDidTerminate:(IRApplication *)application reason:(nullable NSString *)reason;
@end

@interface IRApplication : NSObject

/// 应用标识符
@property (nonatomic, copy, readonly) NSString *bundleID;

/// 应用运行状态
@property (atomic, readonly) IRApplicationState state;

/// 进程标识 (未运行时为 0; 重启后更换)
@property (atomic, readonly) pid_t processIdentifier;

/// 远程进程会话对象 (未运行时为 nil; 应用重启只更换此会话, 实例不变)
@property (atomic, readonly, nullable) IRSession *session;

/// 生命周期代理
@property (atomic, weak, nullable) id<IRApplicationDelegate> delegate;

/// 应用主队列 (串行): 生命周期回调、状态更新都在这里执行。
/// 可直接使用 dispatch API (async/after/group/source); 禁止在上面长时间阻塞或等待
@property (nonatomic, readonly) dispatch_queue_t queue;

/// 当前是否运行在本应用的主队列上
@property (nonatomic, readonly, getter=isOnQueue) BOOL onQueue;

/// 获取应用实例 (按 bundleID 缓存, 多次获取返回同一实例; 应用重启只更换内部会话, 实例不变)
/// 应用未运行时同样返回实例, 状态为 IRApplicationStateNotRunning
/// - Parameter bundleID: 应用标识符
+ (instancetype)applicationWithBundleID:(NSString *)bundleID;

#pragma mark UI 自动化

/// 当前感知结果: 正在扫描就等这一轮, 上一轮扫完不到 0.4 秒就直接用, 否则立刻扫一轮 (最多 2 秒, 超时 nil); 应用不在前台返回 nil。
/// 在 UITask 线程上 = [IRUITask.currentTask currentScreen]; 在 queue 上不等待, 返回最近一轮
- (nullable IRScreenState *)currentScreen;

/// 当前显示的页面 (= currentScreen.pageAgent)
- (nullable IRPageAgent *)currentAgent;

/// 最近页面: 页面识别最近一次认出这个 pageID 时的 Agent (不要求正在显示; 没认出过 / 应用重启后为 nil)。
/// 页面可能已经关掉 (控制器释放), 需要时自己判断 agent.remoteObject.isValidObject; 只读, 不扫描, 任何线程都能调
- (nullable IRPageAgent *)recentPageAgent:(NSString *)pageID;


#pragma mark 任务

/// 未结束的任务
@property (nonatomic, readonly) NSArray<__kindof IRTask *> *tasks;

/// 添加任务: 由本应用管理执行; 结束(完成/取消/应用退出)后自动移除
/// 只用于不操作 UI 的任务 (观测 / 后台处理); IRAutomationTask 要走 setAutomation:, 这里传入无效
- (void)addTask:(IRTask *)task;

/// 移除任务 (任务进入 Cancelled, 终态不可回退; 重复调用无效)
/// - 应用不再调用其 start / resume / canRunInState:, 任务在当前这一步之后结束, completionBlock 只回调一次
/// - 挂起中被移除: 应用先恢复它, 使其在下一个检查点立即退出 (只为结束, 不继续执行业务;
///   会操作界面的任务退出过程中也不再操作, 下一个 SDK 操作直接因取消而中止)
/// - 任务可在自己的 main 中移除自己 (等本次 main 返回后生效)
/// - 需要再次执行请新建任务, 不可复用
- (void)removeTask:(IRTask *)task;

#pragma mark 自动化

/// 当前自动化 (同一时间最多一个; 含已被替换、正在退出的那个)
@property (atomic, readonly, nullable) IRAutomationTask *currentAutomation;

/// 提交自动化: 取消当前的, 等它真正退出后再开始新的 (等待期间再次提交, 只保留最后一个)
/// - 提交后出现在 tasks 中, 随应用生命周期 启动 / 挂起 / 恢复; 结束后自动移除
/// - 已添加过 / 已结束的任务传入无效
- (void)setAutomation:(IRAutomationTask *)task;

/// 自动化是否挂起 (对应 NSOperationQueue.suspended, 由生命周期驱动: 非 Active 时为 YES)
@property (atomic, readonly, getter=isAutomationSuspended) BOOL automationSuspended;

/// 独立中断守护开关 (默认 NO)
/// - 当 currentAutomation == nil 时:
///   - YES: 保持低功耗扫描 (2.0s 间隔) 并自动响应处理弹窗中断 (IRInterruptHandling)
///   - NO: 不定时扫描, 不处理中断 (完全休眠待机)
/// - 当 currentAutomation != nil 时:
///   - 无论本开关为何值, 自动化任务运行期间均始终保持全速扫描并处理中断
@property (atomic, assign, getter=isInterruptGuardEnabled) BOOL interruptGuardEnabled;

@end

/// 状态名称 (调试/日志用)
FOUNDATION_EXPORT NSString *IRApplicationStateName(IRApplicationState state);

NS_ASSUME_NONNULL_END
