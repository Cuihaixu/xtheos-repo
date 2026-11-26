//
//  IRApplication+IRWait.h
//  IRAutomationKit
//
//  等待: 等界面变成某个样子 (OPERATION.md 4.2)。都是读操作, 任何线程都能调, 主队列 (app.queue) 上除外 (不允许等待)。
//  - 每一轮拿一次识别结果 (currentScreen), 两轮之间隔 0.15 秒; 切换中 (transitioning) 的那几轮不算结果
//  - 在 UI 任务线程上: 等待期间过检查点 (取消 / 应用退出抛中止), 来了中断会被挂起, 恢复后接着等
//  - 超时: 返回 nil / NO, 原因写进 diag; 传 nil = 不关心原因, 照样打日志
//
//      IRPageAgent *before = app.currentAgent;
//      app.tap(380, 520, diag);
//      IRPageAgent *page = app.waitPageChange(before, 5, diag);       // 离开操作前的页面
//      app.waitPage(@"comment", 5, diag);                              // 等评论页
//      app.waitUntil(^BOOL(IRScreenState *s) { return !s.chain.contains(@"DUXAlertDialog"); }, 5, diag);
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRApplication.h>
#import <IRAutomationKit/IRDiagnostics.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRApplication (IRWait)

#pragma mark 底层

/// 等当前页面变成指定页面 (pageID 相等)
/// - Parameters:
///   - pageID: 页面标识
///   - timeout: 最长等多久 (秒)
///   - diag: 诊断, 可为 nil
/// - Returns: 那个页面的 Agent; 超时 nil
- (nullable __kindof IRPageAgent *)waitPage:(NSString *)pageID timeout:(NSTimeInterval)timeout diag:(nullable IRDiagnostics *)diag;

/// 等页面变化: 当前页面不再是 page 这个实例 (比较 Agent 实例, 不比较 pageID: 进到另一个同类页面也算变了; 变成未知页面也算)。
/// page 要在操作之前取 (如 app.currentAgent / 动作里的 context.agent): 操作后才取, 取到的可能已经是新页面
/// - Parameters:
///   - page: 操作之前的当前页面; nil = 等到任意一个页面就算
///   - timeout: 最长等多久 (秒)
///   - diag: 诊断, 可为 nil
/// - Returns: 新页面的 Agent (未知页面时是离叶子最近的 VC 的 Agent, pageID 为 nil); 超时 nil
- (nullable __kindof IRPageAgent *)waitPageChangeFrom:(nullable IRPageAgent *)page
                                              timeout:(NSTimeInterval)timeout
                                                 diag:(nullable IRDiagnostics *)diag;

/// 等任意条件成立: 每一轮把识别结果交给 condition, 返回 YES 就结束
/// - Parameters:
///   - condition: 条件 (在调用方线程上执行)
///   - timeout: 最长等多久 (秒)
///   - diag: 诊断, 可为 nil
/// - Returns: 条件是否在超时前成立
- (BOOL)waitUntil:(BOOL (NS_NOESCAPE ^)(IRScreenState *screen))condition
          timeout:(NSTimeInterval)timeout
             diag:(nullable IRDiagnostics *)diag;

#pragma mark block 型 (转到上面的方法)

/// app.waitPage(pageID, timeout, diag)
@property (nonatomic, readonly) IRPageAgent *_Nullable (^waitPage)(NSString *pageID, NSTimeInterval timeout, IRDiagnostics *_Nullable diag);

/// app.waitPageChange(page, timeout, diag)
@property (nonatomic, readonly) IRPageAgent *_Nullable (^waitPageChange)(IRPageAgent *_Nullable page, NSTimeInterval timeout,
                                                                        IRDiagnostics *_Nullable diag);

/// app.waitUntil(condition, timeout, diag)
@property (nonatomic, readonly) BOOL (^waitUntil)(BOOL (^condition)(IRScreenState *screen), NSTimeInterval timeout,
                                                 IRDiagnostics *_Nullable diag);

@end

NS_ASSUME_NONNULL_END
