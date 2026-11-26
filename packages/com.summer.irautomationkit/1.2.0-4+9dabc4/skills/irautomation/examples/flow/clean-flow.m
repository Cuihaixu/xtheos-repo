//
//  clean-flow.m
//  IRAutomationKit - 控制流纯剧本编排标准范式 (Pure Orchestrator Flow)
//
//  核心设计思想:
//  1. 控制流是「剧本导演/业务编排者」，而不是「搬砖打杂的执行者」，严控在 10~25 行；
//  2. 严禁在 Flow 内部写微观逻辑 (计算时长、画板倒计时、拼装字典、就地 fallback 假数据、多层 if-else)；
//  3. 纯粹表达人类刷视频主线:
//     - 观看当前内容 (watch_content: 由 Feed 页面容器自决分流: 支持则调 watch，不支持走兜底权重)
//     - 拟人切换视频 (switch_video: 由 Feed 页面容器漫游自决: 大概率下滑，偶发回看 1~N 条)
//

#import <IRAutomationKit/IRAutomationKit.h>

void RegisterCleanFeedFlow(Class agentClass) {
    [agentClass registerAction:IRIdentityMake(@"flow", @"刷推荐流") builder:^(IRActionBuilder *action) {
        [action setTypeEncodingBlock:^(IRTypeEncoding *encoding) {
            encoding.result(IRSchemaTypeInteger).setComment(@"最终观看的作品数");
        }];

        [action setPerformBlock:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
            __block NSInteger watchedCount = 0;

            // 直接 return [IRFlowSession execute:... diag:diag];
            // 框架内部会将 session 状态及异常自动写入传入的 diag，execute 返回值即为最终执行状态 (YES=未终止/已自愈, NO=终止)
            return [IRFlowSession execute:^(IRFlowSession *session) {
                IRPageNode *feed = session.page(@"feed.recommend");

                // 1. 确定观看总数 (外部传参优先，缺省走拟人权重，配合 ensure 致命断言守护)
                NSInteger total = [context.findParam(@"watch_count", feed.weightOrValue(@"flow_watch_count", @(6))) integerValue];
                feed.ensure(total > 0, @"目标观看数必须大于 0");

                // 2. 纯粹的剧本骨架: 看内容 -> 切视频 (两句话讲完故事)
                feed.loop(@"watch_batch", total, ^(NSUInteger index, NSUInteger totalCount, BOOL *stop) {
                    // A. 观看当前内容 (Feed 内部识别 Cell 类型: 支持则调 watch, 不支持则兜底停留)
                    feed.invokeWith(@"watch_content", @{ @"index": @(index + 1), @"total": @(totalCount) });

                    // B. 拟人切换视频 (Feed 内部处理漫游: 大概率下滑, 偶发回看 1~N 个作品)
                    feed.invoke(@"switch_video");

                    watchedCount++;
                });

            } onException:^(IRFlowException *exception) {
                // 统一异常自愈: 先尝试 backToSelf() 退回 Feed，回到原页且可跳过才 skip
                if (IRPageNode(@"feed.recommend", diag).backToSelf() && exception.canSkip) {
                    exception.skip();
                }
            } onFinally:^(IRFlowSession *session, IRDiagnostics *flowDiag) {
                // onFinally 无论成功还是异常终止均必定执行: 沉淀产出与退回清理
                context.result = @(watchedCount);
                IRPageNode(@"feed.recommend", flowDiag).backToSelf();
            } diag:diag];
        }];
    }];
}
