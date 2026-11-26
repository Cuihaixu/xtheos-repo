//
//  IRResponderAgent.h
//  IRAutomationKit
//
//  通用 Responder Agent ↔ UIResponder: 响应链上既不是视图、控制器、窗口、应用的节点 (如 UIWindowScene) 用它兜底
//  (ACTION.md 第 28 项: 只要会出现在响应链上就给一个 Agent)。架构与其它 Agent 相同: 有自己的元信息 / 注册配置, 可以注册动作
//

#import <IRAutomationKit/IRAgent.h>
#import <IRAutomationKit/IRAgentRegistration.h>

NS_ASSUME_NONNULL_BEGIN

/// Responder Agent 的注册配置 (目前只有公共字段)
@protocol IRResponderAgentRegistration <IRAgentRegistration>
@end

@interface IRResponderAgent : IRAgent
@end

@interface IRResponderAgent (IRRegistration)

/// 注册 (在 +load 里调用): 在 block 里填写配置, block 结束后检查完备性
/// - Parameter block: 配置
/// - Returns: 是否注册成功
+ (BOOL)registerAgent:(NS_NOESCAPE void (^)(id<IRResponderAgentRegistration> registration))block;

@end

NS_ASSUME_NONNULL_END
