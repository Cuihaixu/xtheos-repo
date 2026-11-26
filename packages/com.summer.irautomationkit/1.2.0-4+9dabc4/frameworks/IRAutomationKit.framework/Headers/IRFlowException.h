//
//  IRFlowException.h
//  IRAutomationKit
//
//  流程里的一次失败 (FLOW.md "流程会话"; ↔ CHRFlowException): 交给 onException, 回调选重试 / 跳过 / 终止。
//  不处理 (不调 retry / skip / abort) = 终止。只在 onException 里有效
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class IRFlowSession;

/// 失败类型
typedef NS_ENUM(NSInteger, IRFlowExceptionType) {
    IRFlowExceptionTypeUnknown      = 0,    ///< require 取不到值
    IRFlowExceptionTypeActionFailed = -1,   ///< 页面不对 / 动作失败 (节点的 invoke / loop / repeat)
    IRFlowExceptionTypeManualRaise  = 1,    ///< session.raise 手动终止
    IRFlowExceptionTypeUnexpected   = 2,    ///< 流程里抛出了别的异常 (代码 bug / 任务中止): 只能终止
};

/// 回调的选择
typedef NS_ENUM(NSInteger, IRFlowExceptionResult) {
    IRFlowExceptionResultAbort = 0,   ///< 终止 (默认): 抛异常退出 execute, 后面的代码都不执行
    IRFlowExceptionResultRetry,       ///< 就地再执行这一步 (最多 IRFlowMaxRetryCount 次)
    IRFlowExceptionResultSkip,        ///< 跳过: 这一步返回 NO (require 返回 nil), 接着往下
};

/// 同一步最多重试几次: retryCount 到这个数后 canRetry 为 NO
FOUNDATION_EXPORT const NSInteger IRFlowMaxRetryCount;

@interface IRFlowException : NSObject

/// 所属会话 (可读会话数据)
@property (nonatomic, weak, readonly, nullable) IRFlowSession *session;

/// 类型
@property (nonatomic, readonly) IRFlowExceptionType type;

/// 错误码 (-1 为通用错误码; raise 时为传入的 code)
@property (nonatomic, readonly) NSInteger code;

/// 名称: 动作名 / "loop 名字" / "repeat 名字" / require 的名字; raise 时为空; Unexpected 时为异常名
@property (nonatomic, copy, readonly) NSString *name;

/// 期望的页面 (节点的 pageID; nil = 任意页面或不是节点发出的)
@property (nonatomic, copy, readonly, nullable) NSString *pageID;

/// 原因
@property (nonatomic, copy, readonly) NSString *reason;

/// 这一步已经重试了几次 (第一次失败为 0)
@property (nonatomic, readonly) NSInteger retryCount;

/// 能否重试 (Unexpected / raise 不能; 到重试上限也不能)
@property (nonatomic, readonly) BOOL canRetry;

/// 能否跳过 (Unexpected / raise 不能)
@property (nonatomic, readonly) BOOL canSkip;

/// Unexpected 时: 原始的异常
@property (nonatomic, strong, readonly, nullable) NSException *exception;

/// 回调的选择 (默认终止)
@property (nonatomic, readonly) IRFlowExceptionResult result;

/// 跳过时给这一步填的结果 (skipWith 传入; 节点上由 node.result 拿到, require 返回它)
@property (nonatomic, strong, readonly, nullable) id skipResult;

/// 重试 (不能重试时按终止处理)
@property (nonatomic, copy, readonly) void (^retry)(void);

/// 跳过 (不能跳过时按终止处理)
@property (nonatomic, copy, readonly) void (^skip)(void);

/// 跳过, 并给这一步填结果
@property (nonatomic, copy, readonly) void (^skipWith)(id _Nullable result);

/// 终止
@property (nonatomic, copy, readonly) void (^abort)(void);

@end

NS_ASSUME_NONNULL_END
