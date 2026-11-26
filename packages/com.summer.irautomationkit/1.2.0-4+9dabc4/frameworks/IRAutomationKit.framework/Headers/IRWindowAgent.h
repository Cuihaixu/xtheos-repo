//
//  IRWindowAgent.h
//  IRAutomationKit
//
//  Window Agent ↔ UIWindow: 独立窗口 (如登录窗口 AWELoginWindow) / 窗口型弹窗。基本用不到, 架构与其它 Agent 相同 (ACTION.md 第 23 项):
//  有自己的元信息 / 注册配置, 可以注册动作。没注册的 UIWindow 节点用基础 IRWindowAgent 兜底
//

#import <IRAutomationKit/IRAgent.h>
#import <IRAutomationKit/IRAgentRegistration.h>

NS_ASSUME_NONNULL_BEGIN

/// Window Agent 的注册配置 (目前只有公共字段)
@protocol IRWindowAgentRegistration <IRAgentRegistration>
@end

@interface IRWindowAgent : IRAgent
@end

@interface IRWindowAgent (IRRegistration)

/// 注册 (在 +load 里调用): 在 block 里填写配置, block 结束后检查完备性
/// - Parameter block: 配置
/// - Returns: 是否注册成功
+ (BOOL)registerAgent:(NS_NOESCAPE void (^)(id<IRWindowAgentRegistration> registration))block;

@end

NS_ASSUME_NONNULL_END
