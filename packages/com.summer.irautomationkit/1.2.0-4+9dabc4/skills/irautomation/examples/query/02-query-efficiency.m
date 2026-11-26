//
//  02-query-efficiency.m
//  IRAutomationKit 示例 [元素模块] —— 怎么查找效率最高（减少跨进程 IPC 与快照重拍）
//

#import <IRAutomationKit/IRAutomationKit.h>

@interface AWBQueryEfficiencyDemoAgent : IRPageAgent
@end

@implementation AWBQueryEfficiencyDemoAgent

+ (void)load {
    [self registerAction:IRIdentityMake(@"query_efficiency_demo", @"高效查找示例")
                 builder:^(IRActionBuilder *action) {
        [action setPerformBlock:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
            do {
                // 法则 1：先用本地免费条件收窄，再做远程过滤
                // - views(类名)：纯内存沿类链匹配，免费
                // - visible()：纯内存几何可见性判断，免费
                // - axLabel / text / hittable()：涉及远程补读或 hitTest，只对前面剩下的少量候选执行
                IRUIElement *button = context.agent.query
                    .views(@"AWEFeedVideoButton")   // 先按类收窄（免费）
                    .visible()                      // 再筛屏幕内可见（免费）
                    .axLabel(@"评论")                // 最后只对候选查属性
                    .first()
                    .element;
                (void)button;

                // 法则 2：只重拍局部子树（.refresh()），避免整屏重拍
                // 适合弹窗刚出来、或只关心某个面板内部更新的场景
                IRUIElement *panelConfirm = context.agent.query
                    .views(@"AWESharePanelView")
                    .visible()
                    .first()
                    .refresh()                      // 只对 Panel 做 detach + refresh，不重拍整屏
                    .texts(@"确认")
                    .first()
                    .element;
                (void)panelConfirm;

                // 法则 3：批量读取用一次 .elements(diag) + 节点本地属性，禁止循环内重复 context.agent.query
                NSArray<IRUIElement *> *cards = context.agent.query
                    .views(@"CachalotCommonCollectionViewCell")
                    .visible()
                    .elements(diag);
                if (diag.hasError) {
                    break;
                }
                for (IRUIElement *card in cards) {
                    // node(nil).frame 读本次快照数据，不触发全屏重拍
                    CGRect frame = card.node(nil).frame;
                    if (frame.size.height < 40) {
                        continue;
                    }
                    NSString *title = card.query.views(@"UILabel").first().element.text(nil) ?: @"";
                    (void)title;
                }

                // 法则 4：filter block 里只做轻量判断（读 frame / 子树本地节点），严禁在 filter 里发耗时远程调用或 app.query
                IRUIElement *smallIcon = app.query
                    .views(@"AWENormalModeTabBarFeedView")
                    .descendants()
                    .views(@"UIImageView")
                    .visible()
                    .filter(^BOOL(IRUIElement *element) {
                        CGSize size = element.node(nil).frame.size;
                        return fabs(size.width - 8) < 2 && fabs(size.height - 8) < 2;
                    })
                    .first()
                    .element;
                (void)smallIcon;

                // 法则 5：isAvailable 分支内严禁调用会拍整屏的 query；优先读内存属性 / pageID
            } while (NO);
            return diag.noError;
        }];
    }];
}

@end
