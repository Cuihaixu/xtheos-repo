//
//  IRFlowSession.h
//  IRAutomationKit
//
//  流程会话 (FLOW.md "流程会话"; 一比一复刻 CHRFlowSession 的语法糖): block 里写任意代码, 不用每步判断。
//  - session.page(pageID) 建页面节点: 共用会话的 diag; 页面不对 / 动作失败先交给 onException
//  - onException 选重试 (最多 10 次) / 跳过 (这一步返回 NO, 接着往下) / 终止 (默认; 抛异常退出 execute)
//  - onException 里节点再失败: 不进回调, 直接终止 (回调里要自己处理失败, 用 IRPageNode(pageID, diag))
//  - 流程里抛出的别的异常 (代码 bug 等) 也报给 onException (Unexpected, 只能终止); 任务中止在 onFinally 后原样再抛
//  - onFinally 一定执行; execute 返回 = 没终止, 终止原因在 diag (处理掉的失败转成 warning)
//  使用规则 (FLOW.md "修不了的问题"): 业务工程加 -fobjc-arc-exceptions 终止时才不漏局部变量;
//  block 里拿锁 / 手动管资源要 @try/@finally; 节点不能放进 dispatch_sync 等别的线程的 block
//
//      BOOL ok = [IRFlowSession execute:^(IRFlowSession *session) {
//          IRPageNode *feed = session.page(@"feed.recommend");
//          feed.invoke(@"watch_video");
//          feed.invoke(@"next_video");
//      } onException:^(IRFlowException *exception) {
//          if (backToFeed()) exception.skip();
//      } onFinally:^(IRFlowSession *session, IRDiagnostics *diag) { ... } diag:diag];
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRDiagnostics.h>
#import <IRAutomationKit/IRFlowException.h>
#import <IRAutomationKit/IRPageNode.h>

NS_ASSUME_NONNULL_BEGIN

/// require 的现场 (↔ CHRRequireInfo): block 取不到值时写原因 (reason 或 diag.error)
@interface IRRequireInfo : NSObject
@property (nonatomic, copy, readonly) NSString *name;
@property (nonatomic, copy, nullable) NSString *reason;
@property (nonatomic, strong, nullable) NSDictionary *userInfo;
@property (nonatomic, strong, readonly) IRDiagnostics *diag;
@end

@interface IRFlowSession : NSObject

/// 执行流程
/// - Parameters:
///   - execute: 流程 (在调用方线程上同步执行; 节点要在 UI 任务线程上用)
///   - onException: 失败回调 (nil = 一律终止)
///   - onFinally: 收尾, 一定执行; 它的 diag 是单独的, 有错误时并进会话的 diag
///   - diag: 会话的诊断 (节点共用); 传 nil 则自己建
/// - Returns: YES = 没终止
+ (BOOL)execute:(void (NS_NOESCAPE ^)(IRFlowSession *session))execute
    onException:(nullable void (^)(IRFlowException *exception))onException
      onFinally:(nullable void (^)(IRFlowSession *session, IRDiagnostics *diag))onFinally
           diag:(nullable IRDiagnostics *)diag;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

/// 会话的诊断 (会话建的节点共用)
@property (nonatomic, strong, readonly) IRDiagnostics *diag;

/// 建页面节点: pageID 为 nil / @"" = 任意页面; 失败交给 onException
@property (nonatomic, copy, readonly) IRPageNode *(^page)(NSString *_Nullable pageID);

/// 手动终止: 报给 onException (不能重试 / 跳过), 然后终止
@property (nonatomic, copy, readonly) void (^raise)(NSInteger code, NSString *reason);

- (void)raiseWithCode:(NSInteger)code reason:(NSString *)reason;
- (void)raiseWithCode:(NSInteger)code pageID:(nullable NSString *)pageID reason:(NSString *)reason;
- (void)raiseWithCode:(NSInteger)code name:(nullable NSString *)name pageID:(nullable NSString *)pageID reason:(NSString *)reason;

/// 取依赖值: block 返回 nil = 取不到 (原因写 info.reason 或 info.diag), 报给 onException;
/// 重试 = 再调一次 block, 跳过 = 返回 skipWith 的结果 (默认 nil)
@property (nonatomic, copy, readonly) id _Nullable (^require)(NSString *_Nullable name, id _Nullable (^NS_NOESCAPE block)(IRRequireInfo *info));

/// 自定义数据
@property (nonatomic, copy, readonly) NSDictionary<NSString *, id> *userInfo;
- (nullable id)objectForKeyedSubscript:(NSString *)key;
- (void)setObject:(nullable id)object forKeyedSubscript:(NSString *)key;

@end

NS_ASSUME_NONNULL_END
