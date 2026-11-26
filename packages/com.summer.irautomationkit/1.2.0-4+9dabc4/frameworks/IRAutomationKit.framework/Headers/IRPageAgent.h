//
//  IRPageAgent.h
//  IRAutomationKit
//
//  控制器 Agent ↔ UIViewController。
//  实例有 pageID 的是页面级控制器, 参与"当前页面"判断 (页面识别只靠 pageID); 没有的是非页面控制器。
//  pageID 在创建实例时确定一次, 之后不变: 遵守 IRPageResolving → 问它; 否则 → 注册时填的 pageID
//
//      @implementation AwemeFeedAgent              // : AwemePageAgent (+bundleIDs 写在 App 基类上)
//      + (void)load {
//          [self registerAgent:^(id<IRPageAgentRegistration> registration) {
//              registration.bindClass = @"AWEFeedTableViewController";
//              registration.pageIDs   = @[ @"feed" ];
//              registration.comment   = @"首页推荐 Feed";
//          }];
//      }
//      @end
//
//  控制器复用 (同一个类表示多个页面) 见 IRPageResolving.h
//

#import <IRAutomationKit/IRAgent.h>
#import <IRAutomationKit/IRAgentRegistration.h>
#import <IRAutomationKit/IRPageResolving.h>

NS_ASSUME_NONNULL_BEGIN

/// 通用页面的页面标识 (ACTION.md 第 26 项): 不是真实页面, 只用来承载通用动作 (如"返回") 给子类继承。
/// 注册时 pageIDs = @[IRPageIDCommon] 的类: 不要求 bindClass、不进全局注册表、不参与页面识别、永远不算页面; 可以注册动作。
/// 允许多层 (如 抖音基类 → 视频类页面基类)。
FOUNDATION_EXPORT NSString *const IRPageIDCommon;

/// 控制器 Agent 的注册配置。用 pageIDs 声明这个类可能是哪些页面:
///
///     registration.pageIDs = @[ @"search.middle" ];                                   // 固定页面
///     registration.pageIDs = @[ @"feed.recommend", @"feed.follow", @"feed.nearby" ];  // 复用: 必须遵守 IRPageResolving
///     registration.pageIDs = @[ IRPageIDCommon ];                                     // 通用页面基类: 不要求 bindClass
///     (不设)                                                                          // 非页面控制器: 只提供数据, 不能注册动作
@protocol IRPageAgentRegistration <IRAgentRegistration>

/// 这个类可能是哪些页面 (ACTION.md 第 25 项):
/// - 一个: 实例直接用它; 若遵守 IRPageResolving, 由协议决定这次是不是页面 (返回它或 nil)
/// - 多个: 必须遵守 IRPageResolving, 每个实例由协议从中选一个 (或 nil); 返回不在列表里的值记日志并按 nil 处理
/// - IRPageIDCommon: 通用页面基类, 只能单独出现
@property (nonatomic, copy, nullable) NSArray<NSString *> *pageIDs;

@end

@interface IRPageAgent : IRAgent

/// 这个实例的页面标识 (第一次出现在识别结果里时确定); 非页面控制器为 nil。永远不会是 IRPageIDCommon
@property (nonatomic, copy, readonly, nullable) NSString *pageID;

@end

@interface IRPageAgent (IRRegistration)

/// 注册 (在 +load 里调用): 在 block 里填写配置, block 结束后检查完备性
/// - Parameter block: 配置
/// - Returns: 是否注册成功
+ (BOOL)registerAgent:(NS_NOESCAPE void (^)(id<IRPageAgentRegistration> registration))block;

@end

NS_ASSUME_NONNULL_END
