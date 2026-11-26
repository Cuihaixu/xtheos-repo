//
//  IRInterruptTask.h
//  IRAutomationKit
//
//  中断任务: UI 执行权按感知结果的 interruptAgent 创建, 同一时间最多一个; 业务不直接创建。
//  main 在自己的 UITask 线程上调用 [agent handleInterrupt:], 返回即结束; 运行期间自动化被挂起。
//

#import <IRAutomationKit/IRUITask.h>
#import <IRAutomationKit/IRInterruptHandling.h>

NS_ASSUME_NONNULL_BEGIN

@class IRAgent;

@interface IRInterruptTask : IRUITask

/// 处理弹窗的 Agent (绑定弹窗自己)
@property (nonatomic, readonly) IRAgent<IRInterruptHandling> *agent;

/// 发现弹窗时的链; 那一轮走 pvc (键盘打开) 时为 nil
@property (nonatomic, readonly, nullable) IRResponderChain *chain;

/// 发现弹窗时的 VC 层级树; 那一轮走 hitTest 时为 nil
@property (nonatomic, readonly, nullable) IRViewControllerHierarchy *pvc;

/// 同一 Agent 连续第几次被派发 (1 开始)
@property (nonatomic, readonly) NSUInteger attempt;

/// 取消原因 (仅调试日志用)
@property (atomic, readonly) IRInterruptCancelReason cancelReason;

/// handleInterrupt: 的返回值 (结束后有效)
@property (atomic, readonly) BOOL handled;

- (instancetype)initWithName:(NSString *)name NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
