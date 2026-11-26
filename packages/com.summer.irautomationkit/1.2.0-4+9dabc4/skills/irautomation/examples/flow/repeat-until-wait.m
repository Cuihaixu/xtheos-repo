//
//  repeat-until-wait.m
//  IRAutomationKit - 页面节点条件循环、重试与等待示例 (非 Session 常用)
//

#import <IRAutomationKit/IRAutomationKit.h>

void ExampleRepeatUntilAndWait(IRDiagnostics *diag) {
    // 1. 获取目标页面节点 (自动绑定诊断)
    IRPageNode *commentNode = IRPageNode(@"comment", diag);

    // 2. repeatUntil: 带重试次数和间隔的条件轮询 (支持链式 .onExhausted)
    // - 每一轮自动检查当前是否在 comment 页面 (不在则中止并在 diag 记错误);
    // - 自动在 RemoteCanvas 画板显示 "退出评论 1/3" -> 失败重试时自动显示 "重试 退出评论 2/3";
    // - 每次重试前自动清空 commentNode 上的上一轮临时错误;
    // - 间隔走 [IRUITask sleep:]，天然支持任务取消与弹窗中断挂起;
    // - *stop = YES 提前终止循环 (无论是达标正常退出还是中途报致命错);
    // - 链式 .onExhausted 仅在耗尽且从未 *stop 时触发; 不链式则自动兜底写默认错误。
    commentNode.repeatUntil(@"退出评论", 3, 0.5, ^(NSUInteger attempt, NSUInteger total, BOOL *stop) {
        // 发现致命异常提前退出: 记错误并停下 (不会被后续的 onExhausted 覆盖)
        if (commentNode.errorCode == -99) {
            *stop = YES;
            return;
        }

        commentNode.invoke(@"back");
        if (!commentNode.isVisible) {
            *stop = YES; // 达标正常退出
            return;
        }
    }).onExhausted(^{
        commentNode.diag.error(-1, @"重试 3 次退出评论页均未成功");
    });

    if (commentNode.hasError) {
        NSLog(@"退出评论页失败: %@", commentNode.errorMessage);
        commentNode.clearError(); // 之后继续使用该节点
        return;
    }

    // 3. waitUntil: 带超时时间与轮询间隔的条件等待 (支持链式 .onExhausted)
    // - 自动在画板上屏轮询等待时长 (如 "1.2/5.0s");
    // - *stop = YES 提前退出; 超时且未提前 *stop 时走兜底或 .onExhausted。
    IRPageNode *feedNode = IRPageNode(@"feed.recommend", diag);
    feedNode.waitUntil(@"等待视频就绪", 5.0, 0.2, ^(BOOL *stop) {
        if (feedNode.available(@"watch")) {
            *stop = YES;
        }
    }).onExhausted(^{
        feedNode.diag.error(-1, @"等待视频就绪超时 (5.0s)");
    });

    if (feedNode.noError) {
        feedNode.invoke(@"watch");
    }
}
