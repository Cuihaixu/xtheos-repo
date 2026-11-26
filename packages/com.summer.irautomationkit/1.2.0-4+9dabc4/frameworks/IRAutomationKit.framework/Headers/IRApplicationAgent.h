//
//  IRApplicationAgent.h
//  IRAutomationKit
//
//  Application Agent ↔ UIApplication: 应用本身 (响应链的根); 应用级动作。基本用不到, 架构与其它 Agent 相同 (ACTION.md 第 23 项):
//  有自己的元信息 / 注册配置, 可以注册动作。没注册的 UIApplication 节点用基础 IRApplicationAgent 兜底
//

#import <IRAutomationKit/IRAgent.h>
#import <IRAutomationKit/IRAgentRegistration.h>

NS_ASSUME_NONNULL_BEGIN

/// Application Agent 的注册配置 (目前只有公共字段)
@protocol IRApplicationAgentRegistration <IRAgentRegistration>
@end

@interface IRApplicationAgent : IRAgent
@end

@interface IRApplicationAgent (IRRegistration)

/// 注册 (在 +load 里调用): 在 block 里填写配置, block 结束后检查完备性
/// - Parameter block: 配置
/// - Returns: 是否注册成功
+ (BOOL)registerAgent:(NS_NOESCAPE void (^)(id<IRApplicationAgentRegistration> registration))block;

@end

NS_ASSUME_NONNULL_END
