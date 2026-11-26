//
//  IRListBrowseRemote.h
//  IRAutomationKit
//
//  列表浏览的远程决策 (BROWSE.md 双层决策): 每一步先问服务器, 服务器不管 / 出错 / 超时再调本地 onStep。
//  SDK 只定接口, 怎么请求服务器由业务实现。用哪个: 场景上指定的 (flow.remoteDecider) → 全局单例的 (sharedRemote.decider) → 都没有就全部本地;
//  flow.localOnly(YES) 的场景一律不问。一次浏览里连续 3 次超时 / 出错, 这次浏览后面就不问了 (全部本地), 下次浏览重新问。
//  给服务器的和本地一样: @{ context: 上下文 (history 只带最近 N 步), step: 当前这一步 }; 服务器的回答只有三种:
//
//      { "decision": "local" }                                             交给本地
//      { "decision": "action", "action": "open_ad", "params": { … } }      做这个 (不在这一阶段的可用事件里 → 交给异常 block)
//      { "decision": "skip" }                                              这一阶段跳过
//
//      [IRListBrowseRemote sharedRemote].decider = [AWBListBrowseServer new];   // 全局默认 (业务实现 IRListBrowseRemoteDecider)
//      [IRListBrowseRemote sharedRemote].timeout = 0.3;
//      flow.remoteDecider = AWBSearchManager.sharedManager;                     // 某个场景单独指定
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// 服务器的回答
typedef NS_ENUM(NSInteger, IRListBrowseDecisionKind) {
    IRListBrowseDecisionLocal = 0,   ///< 交给本地 (出错 / 超时 / 认不出的回答也按它)
    IRListBrowseDecisionAction,      ///< 做 action
    IRListBrowseDecisionSkip,        ///< 跳过这一阶段
};

@interface IRListBrowseDecision : NSObject
@property (nonatomic, readonly) IRListBrowseDecisionKind kind;
@property (nonatomic, copy, readonly, nullable) NSString *action;
@property (nonatomic, copy, readonly, nullable) NSDictionary<NSString *, id> *params;
@property (nonatomic, copy, readonly, nullable) NSString *reason;   ///< 交给本地时: 服务器说的 / 出错的原因 (记进步骤和日志)
/// 从服务器的 JSON 解析; 认不出 → local, reason 写明
+ (instancetype)decisionWithJSON:(nullable id)json;
+ (instancetype)local:(nullable NSString *)reason;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
@end

/// 远程决策者 (业务实现)
@protocol IRListBrowseRemoteDecider <NSObject>
/// 同步问一次 (框架在后台队列上调, 最多等 sharedRemote.timeout, 超时当 local)。
/// 返回 nil = 出错 / 没回 (计入连续失败; 连续 3 次这次浏览后面不再问); 服务器有意交给本地返回 [IRListBrowseDecision local:]
/// - Parameters:
///   - context: @{ context: 上下文.dictionaryValue, step: 当前这一步.dictionaryValue } (和本地一样)
///   - timeout: 最多等几秒
- (nullable IRListBrowseDecision *)decideWithContext:(NSDictionary<NSString *, id> *)context timeout:(NSTimeInterval)timeout;
@end

/// 全局配置 (单例)
@interface IRListBrowseRemote : NSObject
@property (class, nonatomic, readonly) IRListBrowseRemote *sharedRemote;
/// 默认的远程决策者 (场景没单独指定时用); nil = 不问服务器
@property (atomic, strong, nullable) id<IRListBrowseRemoteDecider> decider;
/// 每一步等服务器的超时 (秒), 默认 0.3
@property (atomic) NSTimeInterval timeout;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END
