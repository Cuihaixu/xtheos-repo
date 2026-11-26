//
//  IRAgent+IRAction.h
//  IRAutomationKit
//
//  动作: 这个 Agent 能做哪些动作 (ACTION.md)。动作 = 标识 (名字 + 说明) + 一个执行 block。
//  - 四种 Agent 都能注册: IRPageAgent / IRViewAgent / IRWindowAgent / IRApplicationAgent (第 23 项); IRAgent 本身不能
//  - 控制器 (IRPageAgent) 额外规则 (第 25、26 项):
//      注册: 这个类必须先 registerAgent——页面 (pageIDs)、通用页面 (pageIDs = @[IRPageIDCommon]), 或子控制器 (只绑 bindClass); 否则断言
//  - 调用 (底层): 拿到哪个 Agent 对象就调它自己注册的动作 (含继承), 不判断是不是页面 / 当前页面, 不沿链找
//  - 页面逻辑在流程层的 IRPageNode (第 24 项): 检查当前页面 → 页面自己有就调页面的 → 没有沿当前链 (叶子 → 页面) 找子控制器
//    例: Feed 的视频 cell 注册 open_comment, IRPageNode(推荐 / 关注 / 视频详情).invoke(@"open_comment") 都能用
//  - 在 +load 里注册, 新版本 / 新动作可用分类追加; 通用动作注册在通用页面基类上, 子类继承
//  - 在调用方的 UI 线程上同步执行, 不切线程; 只能在持有执行权的 UI 任务上调用 (第 4、19 项)
//  - 没有"能不能做"的预判: 哪一步不对就在哪一步记错误、返回 NO (第 5 项)
//
//      @implementation AwemeBasePageAgent          // 通用页面基类
//      + (void)load {
//          [self registerAgent:^(IRPageAgentRegistration *registration) { registration.pageIDs = @[ IRPageIDCommon ]; }];
//          [self registerAction:IRIdentityMake(@"back", @"返回上一页") block:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
//              ...
//          }];
//      }
//      @end
//
//      @implementation AwemeFeedAgent              // : AwemeBasePageAgent, 继承到 back; 自己再注册 back 就覆盖
//      + (void)load {
//          [self registerAgent:^(IRPageAgentRegistration *registration) { ... }];
//          [self registerAction:IRIdentityMake(@"watch_comment", @"打开评论")
//                         block:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
//              diag.verbose(@"找评论按钮");
//              ...   // app 上的操作, 都传 diag; 失败就 return NO
//              return YES;
//          }];
//      }
//      @end
//
//      // 调用 (UI 任务上)
//      IRDiagnostics *diag = [IRDiagnostics diagnosticsWithDomain:@"浏览流程"];
//      [feed invokeAction:@"watch_comment" params:nil result:NULL diag:diag];
//

#import <IRAutomationKit/IRAgent.h>
#import <IRAutomationKit/IRActionContext.h>
#import <IRAutomationKit/IRIdentity.h>
#import <IRAutomationKit/IRActionBuilder.h>

NS_ASSUME_NONNULL_BEGIN

@class IRApplication;


@interface IRAgent (IRAction)

#pragma mark 注册

/// 注册动作 (在 +load 里调用; 四种 Agent 的子类都能调, 控制器见上面的额外规则)
/// - 同一个类同名动作注册两次: Debug 断言, Release 日志 + 拒绝第二次
/// - 子类注册与父类同名的动作 = 覆盖 (调用时从实例的类开始沿继承链找, 距离最近的先命中)
/// - Parameters:
///   - identity: 动作名 + 说明, IRIdentityMake(@"watch_comment", @"打开评论"); 名字用字符串, 不定义常量 (写错由诊断报"找不到动作")
///   - block: 执行体
+ (void)registerAction:(IRIdentity *)identity block:(IRActionBlock)block;

/// 用构建器注册 (ACTION.md 第 29 项): 声明签名 (入参 / 返回值)、支持的模式, 再给执行体; 见 IRActionBuilder.h。
/// 没给 perform、参数名重复等: 同 registerAction:block: 的规则 (Debug 断言, Release 日志 + 拒绝)
+ (void)registerAction:(IRIdentity *)identity builder:(NS_NOESCAPE void (^)(IRActionBuilder *action))builder;

#pragma mark 调用

/// 执行这个 Agent 的动作
/// 步骤: 校验调用线程是持有执行权的 UI 任务 → 从实例的类开始沿继承链找动作 (最近的先命中, 找不到就失败)
///      → 日志 "→ 动作名 开始" → 建上下文、在调用方线程执行 block → 日志 "← 动作名 成功 / 失败 (耗时)"
/// - Parameters:
///   - actionID: 动作名
///   - params: 参数 (可选)
///   - result: 拿回 context.result (不需要传 NULL)
///   - diag: 调用方的诊断, 动作和动作里的操作都记在它上面; 传 nil 则框架临时建一个 (照样打日志)
/// - Returns: 成功 / 失败
- (BOOL)invokeAction:(NSString *)actionID params:(nullable NSDictionary<NSString *, id> *)params
              result:(id _Nullable * _Nullable)result diag:(nullable IRDiagnostics *)diag;

/// 简写: 无参数、不要结果和诊断
- (BOOL)invokeAction:(NSString *)actionID;

/// 简写: 带参数、不要结果和诊断
- (BOOL)invokeAction:(NSString *)actionID params:(nullable NSDictionary<NSString *, id> *)params;

/// 这个 Agent 类 (含继承来的) 是否注册了这个动作 (不沿链找)
- (BOOL)hasAction:(NSString *)actionID;

#pragma mark 活动面板

/// 事件执行时显示在远程画板的活动面板上 (docs/CANVAS.md): 自动化一行、中断一行; 开始显示事件说明, 结束显示成功 / 警告 / 错误。
/// 默认 YES; 关掉后不上屏, 其它行为不变。画板服务不可用时自动跳过
@property (class, nonatomic) BOOL activityPanelEnabled;

#pragma mark 查询

/// 这个 Agent 类能执行的全部动作 (含继承来的; 同名的只留最近的那个, 即子类覆盖父类的), 按名字排序。
/// 只看类, 不沿链找; 当前页面能调的 (含链上子控制器的) 用 IRPageNode availableActions(tags) / +availableActionsInApplication:tags:
+ (NSArray<IRIdentity *> *)registeredActions;

@end

NS_ASSUME_NONNULL_END
