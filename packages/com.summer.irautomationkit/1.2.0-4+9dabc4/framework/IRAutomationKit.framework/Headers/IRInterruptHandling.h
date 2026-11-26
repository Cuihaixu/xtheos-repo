//
//  IRInterruptHandling.h
//  IRAutomationKit
//
//  自动处理协议: 遵守它的 Agent 就是"会被自动处理"的 Agent。弹窗自己最知道怎么关, 所以交给绑定它的 Agent。
//  - 控制器型弹窗: IRPageAgent 子类绑定弹窗 VC; 视图型弹窗: IRViewAgent 子类绑定弹窗视图
//  - Agent 只负责"怎么处理自己", 不保存中断状态; 派发 / 抢占 / 放弃由 UI 执行权维护
//  - 注册: 和普通页面 Agent 一样, 在 +load 里 +registerAgent: 全局注册
//  - 识别: 页面感知沿链从叶子往根, 第一个对应的 Agent 遵守本协议且 shouldInterruptInContext: 为 YES 的就是本轮的 interruptAgent
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRChainContext.h>

NS_ASSUME_NONNULL_BEGIN

@class IRApplication;
@class IRAgent;
@class IRInterruptTask;
@class IRInterruptContext;

/// 中断被取消的原因 (仅调试日志用, 不作为业务分支依据)
typedef NS_ENUM(NSInteger, IRInterruptCancelReason) {
    IRInterruptCancelReasonNone = 0,
    IRInterruptCancelReasonPreempted,       ///< 链上出现了更靠前 (更上层) 的需要处理的 Agent
    IRInterruptCancelReasonAutomationOff,   ///< 自动化已关闭 (槽位空)
    IRInterruptCancelReasonAborted,         ///< 中止 (应用退出)
};

/// 原因名称 (日志用)
FOUNDATION_EXPORT NSString *IRInterruptCancelReasonName(IRInterruptCancelReason reason);

@protocol IRInterruptHandling <NSObject>

@optional
/// 这一次是否需要自动处理 (不实现 = YES): 同一个类有时并不需要处理, 看链判断。
/// 每轮识别时在识别队列上调用 (只对候选); 可以只读远程 (读 ivar), 优先读内存、少调方法; 抛异常按 NO 处理
/// - Parameter context: 本轮的响应链 + 这个 Agent 绑定对象的节点
- (BOOL)shouldInterruptInContext:(IRChainContext *)context;

@required
/// 处理自己。在中断任务自己的 UITask 线程上调用 (不是识别队列, 也不是 app.queue), 期间自动化已挂起;
/// 与自动化线程能力相同 (currentScreen / currentAgent / sleep / 以后的点击), 感知结果照常更新。
/// 两步之间可读 context.isCancelled 提前退出; 不检查也行, 下一个 SDK 操作会因取消而中止
/// - Parameter context: 中断上下文
/// - Returns: 返回即结束。YES = 处理完; NO = 没处理好 (日志)。同一 Agent 连续被派发达到上限后放弃
- (BOOL)handleInterrupt:(IRInterruptContext *)context;

@end

/// 中断上下文: 处理期间的状态
@interface IRInterruptContext : NSObject

/// 所属应用
@property (nonatomic, weak, readonly, nullable) IRApplication *application;

/// 所在的中断任务 (用它调用 UITask 共同能力: currentScreen / currentAgent / sleep)
@property (nonatomic, weak, readonly, nullable) IRInterruptTask *task;

/// 被处理的 Agent (== 发现时那一轮的 interruptAgent, 来自 Agent 池)
@property (nonatomic, readonly) IRAgent<IRInterruptHandling> *agent;

/// 发现时的链 (正式接口: 可直接取遮罩等节点); 那一轮走 pvc (键盘打开) 时为 nil
@property (nonatomic, readonly, nullable) IRResponderChain *chain;

/// 发现时的 VC 层级树; 那一轮走 hitTest 时为 nil
@property (nonatomic, readonly, nullable) IRViewControllerHierarchy *pvc;

/// 同一 Agent 连续第几次被派发 (1 开始)
@property (nonatomic, readonly) NSUInteger attempt;

/// 已被取消 (任何时候可读, 原子)
@property (readonly, getter=isCancelled) BOOL cancelled;

/// 取消原因 (仅调试日志用)
@property (readonly) IRInterruptCancelReason cancelReason;

/// 检查点: 已取消就抛出中止异常
- (void)checkpoint;

/// 睡眠 (分段, 每段过检查点)
- (void)sleep:(NSTimeInterval)seconds;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
