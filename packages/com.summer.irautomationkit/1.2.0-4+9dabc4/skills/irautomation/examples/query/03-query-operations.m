//
//  03-query-operations.m
//  IRAutomationKit 示例 [元素模块] —— 唯一性防歧义、动态等待与安全点击/输入
//

#import <IRAutomationKit/IRAutomationKit.h>

@interface AWBQueryOperationsDemoAgent : IRPageAgent
@end

@implementation AWBQueryOperationsDemoAgent

+ (void)load {
    [self registerAction:IRIdentityMake(@"query_operations_demo", @"元素等待与安全操作示例")
                 builder:^(IRActionBuilder *action) {
        [action setPerformBlock:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
            do {
                // 1. 唯一性约束：tap / text / typeText 要求命中正好 1 个控件（0 个或多个都会报错）
                //    预期可能存在多个时，必须显式写 .first() / .last() / .at(index)
                IRUIElement *searchField = context.agent.query
                    .views(@"UITextField")
                    .visible()
                    .first()
                    .element;

                // 2. 动态等待元素出现：用 .wait(timeout, diag) 替代手写 while + sleep
                IRUIElement *readyField = context.agent.query
                    .views(@"UITextField")
                    .visible()
                    .first()
                    .wait(5.0, diag);
                if (diag.hasError) {
                    break;
                }

                // 3. 输入文字并检查 diag.hasError
                readyField.clearAndTypeText(@"测试关键词", diag);
                if (diag.hasError) {
                    break;
                }
                (void)searchField;

                // 4. 点击按钮并等待页面响应（返回 YES 只代表手势已投递，必须 waitPageChange / waitUntil 校验结果）
                context.agent.query.views(@"UIButton").axLabels(@"搜索*").visible().first().element.tap(diag);
                if (diag.hasError) {
                    break;
                }
                app.waitPageChange(context.page, 5.0, diag);
            } while (NO);
            return diag.noError;
        }];
    }];
}

@end
