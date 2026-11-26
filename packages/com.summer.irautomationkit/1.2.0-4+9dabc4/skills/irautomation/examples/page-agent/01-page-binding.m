//
//  01-page-binding.m
//  IRAutomationKit 示例 [EX-PAGE-01] —— 页面绑定、+load 四步顺序与复用识别
//

#import <IRAutomationKit/IRAutomationKit.h>

@interface AWBPageBindingDemoAgent : IRPageAgent <IRPageResolving>
@end

@implementation AWBPageBindingDemoAgent

+ (void)load {
    // 1. 绑定页面（固定页面直接填 pageIDs；子控制器/容器不设 pageIDs；复用页面必须遵守 <IRPageResolving>）
    [self registerAgent:^(id<IRPageAgentRegistration> registration) {
        registration.bindClass = @"AWEFeedTableViewController";
        registration.pageIDs = @[ @"feed.recommend" ];
        registration.comment = @"推荐页";
    }];

    // 2. 注册权重（见 examples/weight/param-override.m）
    // 3. 注册原子事件（如 back / next_video）
    // 4. 注册控制流事件（复杂页面放到 +Flow.m 分类）
}

// 复用控制器时：纯内存从响应链判断当前实例属于哪个 pageID，不触发耗时 IPC
+ (nullable NSString *)pageIDInContext:(IRChainContext *)context {
    if ([context.node.parentViewController.className isEqualToString:@"AWEHPChannelPageViewController"]) {
        return @"feed.recommend";
    }
    return nil;
}

@end
