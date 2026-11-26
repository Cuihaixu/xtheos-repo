//
//  ensure-raise.m
//  IRAutomationKit - 前置断言 (ensure/ensureNamed) 与主动抛错 (raise/raiseNamed) 范例
//
//  核心设计哲学:
//  1. ensure (Guard Clause 前置守护): condition == NO 时立即中断流程，免写繁琐 if (!cond) return;
//  2. raise (深层主动抛错): 在 switch 状态机 default 或严重非法分支中主动抛出;
//  3. 命名采用 ensure 而非 assert: 避免与 <assert.h> 中 Foundation 引入的单参宏碰撞;
//  4. 致命不可恢复性 (Fatal Invariants): 抛出的异常定性为 ManualRaise，严格不可重试 (canRetry = NO)，
//     不可跳过 (canSkip = NO)，在 Session 中强制立即展开到 onFinally 执行收尾。
//

#import <IRAutomationKit/IRAutomationKit.h>

void ExampleEnsureAndRaise(IRActionContext *context, IRDiagnostics *diag) {
    // =========================================================================
    // 场景 1: 在 IRFlowSession 中使用 (会话事务模式，统一由 onException 拦截并展开)
    // =========================================================================
    [IRFlowSession execute:^(IRFlowSession *session) {
        IRPageNode *feed = session.page(@"feed.recommend");

        // 1. 匿名断言 (ensure): 校验入参，条件不满足直接中断
        NSInteger watchCount = [context.findParam(@"watch_count", @(5)) integerValue];
        feed.ensure(watchCount > 0, @"目标观看数必须大于 0");

        // 2. 带标识断言 (ensureNamed): 区分多个检查点，报错带明确的 e.name
        NSString *targetAuthor = [context.findParam(@"author", @"") stringValue];
        feed.ensureNamed(@"valid_author", targetAuthor.length > 0, @"作者名不能为空");

        // 3. 业务深层分支中的主动抛错 (raise / raiseNamed)
        NSInteger accountStatus = 2; // 假设 2 代表被风控封禁
        switch (accountStatus) {
            case 0: // 正常
                break;
            case 1: // 需验证码
                feed.raise(@"当前账号触发了安全验证码");
                break;
            case 2: // 封禁
                feed.raiseNamed(@"account_banned", @"当前账号已被封禁，流程无法继续");
                break;
            default:
                feed.raiseNamed(@"unknown_status", @"遇到未知的账号状态");
                break;
        }

        // 正常业务主线
        feed.invoke(@"watch");

    } onException:^(IRFlowException *exception) {
        // 关键特性核对:
        // - exception.type == IRFlowExceptionTypeManualRaise
        // - exception.canRetry == NO (严禁且无法重试)
        // - exception.canSkip == NO (严禁且无法跳过)
        // - exception.name == @"valid_author" 或 @"account_banned"
        NSLog(@"[Session] 捕获到致命异常: name=%@, page=%@, reason=%@", exception.name, exception.pageID, exception.reason);

        // 任何调用 exception.retry() 或 exception.skip() 都会被框架忽略，默认按终止处理

    } onFinally:^(IRFlowSession *session, IRDiagnostics *flowDiag) {
        NSLog(@"[Session] 收尾清理完成");
    } diag:diag];

    // =========================================================================
    // 场景 2: 在非 Session 普通裸 Node 中使用 (直接写入绑定的 diag)
    // =========================================================================
    IRPageNode *player = IRPageNode(nil, diag);

    player.ensure(player.available(@"watch"), @"当前界面不支持看播操作");
    if (player.hasError) {
        NSLog(@"[Standalone] 断言失败: %@", player.errorMessage);
        return;
    }

    player.raiseNamed(@"state_invalid", @"非法播放状态");
    if (player.hasError) {
        NSLog(@"[Standalone] 主动抛错: code=%ld, msg=%@", (long)player.errorCode, player.errorMessage);
        return;
    }
}
