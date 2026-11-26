//
//  IRViewAgent.h
//  IRAutomationKit
//
//  视图 Agent ↔ UIView: 控件 (控件感知找到的) / 视图型弹窗; 可以注册动作 (如视图型弹窗的"关闭", ACTION.md 第 23 项)
//
//      @implementation AwemeCommentButtonAgent     // : AwemeViewAgent
//      + (void)load {
//          [self registerAgent:^(id<IRViewAgentRegistration> registration) {
//              registration.bindClass = @"AWEFeedVideoButton";
//              registration.tag       = @"feed.comment";
//              registration.comment   = @"Feed 评论按钮";
//          }];
//          [self registerAction:IRIdentityMake(@"tap", @"点评论") block:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
//              ...
//          }];
//      }
//      @end
//

#import <IRAutomationKit/IRAgent.h>
#import <IRAutomationKit/IRAgentRegistration.h>

NS_ASSUME_NONNULL_BEGIN

/// 视图 Agent 的注册配置 (目前只有公共字段)
@protocol IRViewAgentRegistration <IRAgentRegistration>
@end

@interface IRViewAgent : IRAgent

@end

@interface IRViewAgent (IRRegistration)

/// 注册 (在 +load 里调用): 在 block 里填写配置, block 结束后检查完备性
/// - Parameter block: 配置
/// - Returns: 是否注册成功
+ (BOOL)registerAgent:(NS_NOESCAPE void (^)(id<IRViewAgentRegistration> registration))block;

@end

NS_ASSUME_NONNULL_END
