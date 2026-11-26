//
//  param-override.m
//  IRAutomationKit - 外部参数覆盖权重与兜底决策标准范式 (Param-Weight Override Idiom)
//

#import <IRAutomationKit/IRAutomationKit.h>

void ExampleParamWeightOverride(IRActionContext *context, IRPageNode *node, IRDiagnostics *diag) {
    // =========================================================================
    // 标准一行式 (Param-Weight Override Idiom):
    // 原理: NSNumber 与 IRWeightSelection 原生多态支持 .integerValue / .doubleValue / .stringValue / .boolValue
    // - 外部传参时: findParam 返回 NSNumber, 取出传入的值;
    // - 外部未传参时: findParam 回退到第二个参数 node.weightOrValue, 取出权重随机决策结果;
    // - 若权重组不存在/全被禁用: weightOrValue 回退到给定的兜底默认值 (如 @6 或 @3.0)。
    // =========================================================================

    // 1. 整型场景 (如观看作品数、滑动翻页次数) + ensure 前置断言
    NSInteger watchCount = [context.findParam(@"watch_count", node.weightOrValue(@"flow_watch_count", @6)) integerValue];
    node.ensure(watchCount > 0, @"目标观看数必须大于 0");

    // 2. 浮点型场景 (如停留秒数、延迟时间)
    IRWeightSelection *cmtSel = node.weightOrValue(@"flow_comment_interaction", nil);
    if ([cmtSel.name isEqualToString:@"view"]) {
        NSTimeInterval dwellTime = [context.findParam(@"comment_dwell_time", cmtSel) doubleValue] ?: 3.0;
        node.wait(dwellTime);
    }

    // 3. 循环批处理
    node.loop(@"watch_batch", watchCount, ^(NSUInteger index, NSUInteger total, BOOL *stop) {
        if (!node.invoke(@"next_video")) {
            *stop = YES;
        }
    });
}
