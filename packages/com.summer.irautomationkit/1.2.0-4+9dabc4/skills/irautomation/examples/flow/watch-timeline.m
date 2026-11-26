//
//  watch-timeline.m
//  IRAutomationKit - 声明式看播时间轴 (IRPageNode.timeline + IRWatchTimeline) 标准范式
//
//  核心原则:
//  1. 声明式先注册后启动: node.timeline(name, duration) 返回绑定 diag 的时间线对象,
//     通过 .on(percent, name, desc, handler) 链式注册全部时间节点, 最后 .start() 立即启动 (或存变量延时 .start());
//  2. 冷却间隔一行收口: 仅当确定真执行时调用 timeline.waitMinInterval(action, seconds);
//  3. 异常隔离: 伴生动作失败时先 node.backToSelf() 确认回到原宿主页，再 exception.skip()。
//

#import <IRAutomationKit/IRAutomationKit.h>

BOOL RunWatchWithTimeline(NSString *hostPageID, NSInteger duration, IRDiagnostics *diag) {
    return [IRFlowSession execute:^(IRFlowSession *session) {
        IRPageNode *player = session.page(nil);

        // 1. 链式声明并立即 .start()
        player.timeline(@"watch_video", duration)
            .on(40, @"try_comment", @"伴生评论互动", ^BOOL(IRWatchTimeline *timeline, IRTimelineAction *action) {
                if (!player.available(@"try_comment")) {
                    return NO; // 判定跳过, 不更新 previousActionExecuteTime
                }
                timeline.waitMinInterval(action, 2.0);
                player.invoke(@"try_comment");
                return YES;
            })
            .on(75, @"try_search", @"伴生搜索回流", ^BOOL(IRWatchTimeline *timeline, IRTimelineAction *action) {
                if (!player.available(@"try_search")) {
                    return NO;
                }
                timeline.waitMinInterval(action, 2.0);
                player.invoke(@"try_search");
                return YES;
            })
            .start();

    } onException:^(IRFlowException *exception) {
        IRPageNode *host = IRPageNode(hostPageID, nil);
        if ([@[@"try_comment", @"try_search"] containsObject:exception.name] && host.backToSelf()) {
            exception.skip();
        }
    } onFinally:nil diag:diag];
}
