//
//  01-query-sources.m
//  IRAutomationKit 示例 [EX-QUERY-01] —— 从哪能获取到元素（适用边界）
//

#import <IRAutomationKit/IRAutomationKit.h>

@interface AWBQuerySourcesDemoAgent : IRPageAgent
@end

@implementation AWBQuerySourcesDemoAgent

+ (void)load {
    [self registerAction:IRIdentityMake(@"query_sources_demo", @"元素获取入口示例")
                 builder:^(IRActionBuilder *action) {
        [action setPerformBlock:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
            do {
                // 1. 首选：context.agent.query —— 限定在当前 PageAgent / 子控制器视图子树内
                //    不会扫到背后被盖住的旧页面
                IRUIElement *insideButton = context.agent.query
                    .views(@"UIButton")
                    .axLabels(@"搜索*")
                    .visible()
                    .first()
                    .element;
                (void)insideButton;

                // 2. 外层壳/独立弹窗：app.query —— 从全屏根窗口开始查
                //    仅用于顶部导航栏、底部 TabBar、独立 Window 弹窗
                IRUIElement *navBackButton = app.query
                    .views(@"AWESearchBarButton")
                    .axLabels(@"返回*")
                    .visible()
                    .first()
                    .element;
                (void)navBackButton;

                // 3. 已拿到父元素/Cell 后继续往下找：element.query（复用同一次快照，不重拍全屏）
                NSArray<IRUIElement *> *cells = context.agent.query
                    .views(@"AWESearchSugTableViewCell")
                    .visible()
                    .elements(diag);
                if (diag.hasError) {
                    break;
                }
                for (IRUIElement *cell in cells) {
                    NSString *word = cell.query.views(@"UILabel").first().element.text(nil) ?: @"";
                    (void)word;
                }

                // 4. 从已有 IRUINode 包装成元素：node.element（复用 node 所在那份快照）
                IRUINode *firstCellNode = cells.firstObject.node(nil);
                IRUIElement *reusedElement = firstCellNode.element;
                (void)reusedElement;
            } while (NO);
            return diag.noError;
        }];
    }];
}

@end
