//
//  01-list-browse.m
//  IRAutomationKit 示例 [EX-LIST-01] —— ListBrowse 三阶段与防死循环熔断
//

#import <IRAutomationKit/IRAutomationKit.h>

@interface AWBListBrowseDemoAgent : IRPageAgent
@end

@implementation AWBListBrowseDemoAgent

+ (void)load {
    [self registerListBrowse:IRIdentityMake(@"browse_demo", @"列表浏览示例")
                contextClass:nil
                     builder:^(IRListBrowseBuilder *flow) {
        flow.arg(@"max_items", IRSchemaTypeInteger).setDefaultValue(@10).setComment(@"最多看几条");

        flow.onStep(^(IRListBrowseStep *step, IRListBrowseContext *list) {
            // 1. 防死循环硬上限：到达 maxItems、列表到底或连续滑动未前移时，非 Exit 阶段直接 skip
            BOOL reachedLimit = (list.computed.exposed.integerValue >= list.intParam(@"max_items")) || list.listState.reachedEnd;
            if (reachedLimit && !step.at(IRListBrowseStageExit)) {
                step.skip();
                return;
            }

            // 2. View 阶段：看当前条目（未看过才看）
            if (step.at(IRListBrowseStageView)) {
                if (step.item.watched == 0 && step.available(@"watch_item")) {
                    step.invoke(@"watch_item");
                    return;
                }
                step.skip();
                return;
            }

            // 3. Slide 阶段：滑到下一条（合并后的 step_item）
            if (step.at(IRListBrowseStageSlide)) {
                if (step.available(@"step_item")) {
                    step.invoke(@"step_item");
                } else {
                    step.skip();
                }
                return;
            }

            // 4. Exit 阶段：退出当前列表页
            if (step.at(IRListBrowseStageExit)) {
                step.invoke(@"back");
            }
        }, nil);
    }];
}

@end
