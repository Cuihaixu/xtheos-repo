//
//  IRPageResolving.h
//  IRAutomationKit
//
//  页面判断协议: 一个控制器类被复用成多个页面时, 控制器 Agent 遵守它。
//  同一个类只注册一次 (只对应一个 Agent 类); 一个实例到底是哪个页面, 由响应链和运行时状态决定:
//  实例第一次出现在响应链上时调用一次, 结果存在实例的 pageID 上, 之后不变。
//
//      @implementation AwemeFeedListAgent          // : AwemePageAgent <IRPageResolving>
//      + (void)load {
//          [self registerAgent:^(id<IRPageAgentRegistration> registration) {
//              registration.bindClass = @"AWEFeedListViewController";
//              registration.pageIDs   = @[ @"feed.follow", @"feed.recommend" ];   // 可能出现的页面
//          }];
//      }
//      + (nullable NSString *)pageIDInContext:(IRChainContext *)context {
//          return context.node.under(@"AWEFollowTabView") ? @"feed.follow" : @"feed.recommend";
//      }
//      @end
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRChainContext.h>

NS_ASSUME_NONNULL_BEGIN

@protocol IRPageResolving <NSObject>

/// 这个实例是哪个页面 (实例第一次出现在响应链上时调用一次; 判断的是类的复用, 所以是类方法)
/// 可以只读远程 (读 ivar), 优先读内存、少调方法; 抛异常按 nil 处理并打日志
/// - Parameter context: 响应链 + 这个控制器的节点
/// - Returns: 注册时 pageIDs 里的一个; nil = 这次不是页面 (返回不在列表里的值: 记日志并按 nil 处理)
+ (nullable NSString *)pageIDInContext:(IRChainContext *)context;

@end

NS_ASSUME_NONNULL_END
