//
//  IRAutomationTask.h
//  IRAutomationKit
//
//  自动化任务: 操作 UI 的业务流程。同一应用同一时间最多一个 (槽位), 只能通过 -[IRApplication setAutomation:] 提交。
//
//  - 线程 / 挂起 / 检查点 / 不可挂起区见 IRUITask
//  - 挂起原因有两个, 取或: 应用不在可运行状态 (canRunInState:) / 有中断在处理
//  - 取消 (被新的自动化替换 / removeTask:) 与应用退出、弹窗关不掉: 下一个检查点中止, 等 main 退出后进入终态
//

#import <IRAutomationKit/IRUITask.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRAutomationTask : IRUITask

/// block 任务 (不想写子类时用)
/// - Parameters:
///   - name: 任务名称
///   - block: 任务内容, 在自动化线程上执行
+ (instancetype)automationWithName:(NSString *)name block:(void (^)(IRAutomationTask *task))block;

/// 当前线程所属的自动化任务 (不在自动化线程上返回 nil)
@property (class, readonly, nullable) IRAutomationTask *currentAutomation;

@end

NS_ASSUME_NONNULL_END
