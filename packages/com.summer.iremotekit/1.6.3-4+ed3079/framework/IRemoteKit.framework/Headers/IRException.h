//
//  IRException.h — 远程调用异常(L1)
//
//  一个 worker 作用域(`runWorker:`)= 一串远程调用的合集。**失败就是失败**——不修复、不重试
//  (重试要重建 marshal、worker 可能已废,对本模块太复杂)。任一远程调用失败即**抛 `IRException`**,
//  unwind 出作用域(后面的调用不再执行),`onException:` 收到"在哪失败了",`onFinally:` 恒执行收尾。
//
//    [session runWorker:^{
//        UIWindow *win = [app.remote keyWindow];
//        id vc = [win.remote rootViewController];   // 若失败 → 抛,下面不执行
//        [vc.remote doSomething];
//    } onException:^(IRException *e){
//        NSLog(@"失败于 -%@:%@", e.selector, e.reason);   // 通知在哪失败
//    } onFinally:^{
//        // 收尾(恒执行)
//    }];
//

#import <IRemoteKit/IRTypes.h>
#import <IRemoteKit/IRSession.h>   // 下方在 IRSession 上定义 runWorker: 分类,需完整接口

NS_ASSUME_NONNULL_BEGIN

@class IRObject;

/// 异常来源域(`domain` 取值,标明"从哪个子系统抛的"):
FOUNDATION_EXPORT NSErrorDomain const IRRemoteCallErrorDomain;   ///< 远程调用执行失败(invoke / call / proxy / callObject)
FOUNDATION_EXPORT NSErrorDomain const IRSessionErrorDomain;      ///< 会话/worker 层(起 worker 失败、目标退出)
FOUNDATION_EXPORT NSErrorDomain const IRUnexpectedErrorDomain;   ///< 目标 / 意外 ObjC 异常兜底(非本 SDK 主动抛)

/// 远程调用失败时抛出(`NSException` 子类,可直接 `@catch`)。NSError 式四元组 + 失败点。
@interface IRException : NSException

/// 来源域:知道"从哪抛的"(上面三个之一)。
@property (nonatomic, readonly, copy) NSErrorDomain domain;
/// 错误码(`IRError`:Fault / Timeout / Poisoned / NotFound / TargetGone …)。
@property (nonatomic, readonly) NSInteger code;
/// 详细原因(= `NSException.reason`,含 rc / 上下文)。
@property (nonatomic, readonly, nullable) NSString *reason;
/// 附加信息(= `NSException.userInfo`;含 `selector` / `object` / 底层 `NSError` 等键)。
@property (nonatomic, readonly, nullable, copy) NSDictionary *userInfo;

/// 所属会话。
@property (nonatomic, weak, readonly, nullable) IRSession *session;
/// 失败发生在哪个远程对象(OC 调用有;C 函数 / 非对象操作为 `nil`)。
@property (nonatomic, readonly, nullable) IRObject *object;
/// 失败的选择子(OC)或 C 函数名 / 操作名;`nil` 表示非具体调用点。
@property (nonatomic, readonly, nullable) NSString *selector;

// `name`(继承)固定 `IRRemoteCallException`。

@end

#pragma mark - 异常作用域(归 session 实例)

@interface IRSession (IRException)

/// 像 `withWorker:` 一样起一条常驻 worker 跑 `block`,但**作用域内任一远程调用失败即抛 `IRException`**
/// (unwind,后续调用不执行),经 `onException:` 通知失败点,`onFinally:` 恒收尾。原生 `@try/@catch/@finally`,
/// 无额外抽象。`withWorker:` 不受影响(仍返回 nil + lastError)。
/// - Returns: 无异常 `YES`;抛了(并被 `onException:` 接住)`NO`。
- (BOOL)runWorker:(void (^)(void))block
      onException:(nullable void (^)(IRException *exception))onException
        onFinally:(nullable void (^)(void))onFinally;

@end

NS_ASSUME_NONNULL_END
