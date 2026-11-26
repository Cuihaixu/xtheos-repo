//
//  02-list-scroller.m
//  IRAutomationKit 示例 [EX-LIST-02] —— IRListScroller 对齐滚动与停稳等待
//

#import <IRAutomationKit/IRAutomationKit.h>

@interface AWBListScrollerDemoAgent : IRPageAgent
@end

@implementation AWBListScrollerDemoAgent

+ (void)load {
    [self registerAction:IRIdentityMake(@"scroll_item_demo", @"列表对齐滚动示例")
                 builder:^(IRActionBuilder *action) {
        [action setTypeEncodingBlock:^(IRTypeEncoding *encoding) {
            encoding.argWith(@"section", @0);
            encoding.argWith(@"item", @0);
        }];

        [action setPerformBlock:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
            do {
                IRUIElement *listElement = context.agent.query.views(@"UICollectionView").visible().first().element;
                IRUINode *node = listElement.node(diag);
                if (node == nil) {
                    break;
                }
                IRUICollectionView *list = [IRUICollectionView viewWithObject:[context.agent.remoteObject.session objectAtAddress:node.address]];

                // 1. 等列表停稳（替代自写轮询）
                [list waitStillWithTimeout:5.0];

                // 2. 推算中心项与相邻项（跨 section）
                NSIndexPath *current = [list centerIndexPathWithDiag:diag];
                NSIndexPath *next = current ? [list adjacentIndexPath:current offset:1 diag:nil] : nil;

                // 3. 用 IRListScroller 滚到屏幕中心指定 Y 线（rect 为 CGRectNull 表示整 cell）
                NSIndexPath *target = next ?: [NSIndexPath indexPathForItem:[context.findParam(@"item", @0) integerValue]
                                                                  inSection:[context.findParam(@"section", @0) integerValue]];
                IRListScroller *scroller = listElement.scroller(diag);
                [scroller scrollToIndexPath:target rect:CGRectNull atPosition:IRListPositionMiddle offset:0];
            } while (NO);
            return diag.noError;
        }];
    }];
}

@end
