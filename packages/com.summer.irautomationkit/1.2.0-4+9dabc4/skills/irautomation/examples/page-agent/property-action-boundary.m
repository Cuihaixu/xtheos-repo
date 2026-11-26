//
//  property-action-boundary.m
//  IRAutomationKit - PageAgent 属性边界与 Action 解耦标准范式
//
//  核心设计思想:
//  1. PageAgent 属性 (@property readonly) 的唯一用途: 包装和只读访问目标 ViewController 的 ivar/状态；
//  2. 内部通过 context.agent (借力 __kindof 强类型) 优雅取用属性，免强制类型转换；
//  3. 跨 Agent 严禁直接读属性！跨 Agent 必须 100% 走事件总线 (invoke / invokeQuery)；
//  4. 业务逻辑与操作必须注册为 Action，享受 available 探测、独立单测与诊断隔离；
//  5. 非 Session 的 Action 内部编排子动作时，严格遵循 do-while(NO) 与 hasError 检查。
//

#import <IRAutomationKit/IRAutomationKit.h>

// =============================================================================
// 1. 公开头文件 (AWBPlayerPageAgent.h): 保持绝对纯净，零控制器私有属性暴露！
// =============================================================================
@interface AWBPlayerPageAgent : IRPageAgent
// 公开头文件只保留类声明与继承关系，从物理上杜绝外界直接通过点语法读取控制器内部属性
@end

// =============================================================================
// 2. 属性访问作用域 (AWBPlayerPageAgent.m 类扩展):
//    属性只负责包装控制器 ivar / 状态，作用域严格收敛在当前 .m 内部！
// =============================================================================
@interface AWBPlayerPageAgent ()

/// 视频总时长 (秒, 包装读取播放器内部状态)
@property (nonatomic, readonly) double duration;

/// 当前播放进度 (秒)
@property (nonatomic, readonly) double currentPlaybackTime;

/// 作品类型 (video / image / live)
@property (nonatomic, copy, readonly) NSString *itemType;

/// 是否允许评论
@property (nonatomic, readonly) BOOL canComment;

@end

@implementation AWBPlayerPageAgent

#pragma mark - 控制器数据包装实现

- (double)duration {
    return [[self.remoteObject ivarNamed:@"_duration"].value doubleValue];
}

- (double)currentPlaybackTime {
    return [[self.remoteObject ivarNamed:@"_currentPlaybackTime"].value doubleValue];
}

- (NSString *)itemType {
    NSInteger rawType = [[self.remoteObject ivarNamed:@"_awemeType"].value integerValue];
    return rawType == 101 ? @"live" : (rawType == 68 ? @"image" : @"video");
}

- (BOOL)canComment {
    return [[self.remoteObject ivarNamed:@"_canComment"].value boolValue];
}

#pragma mark - 动作注册 (行为必须且只能是 Action)

+ (void)load {
    [self registerAgent:^(id<IRPageAgentRegistration> registration) {
        registration.bindClass = @"AWEPlayInteractionViewController";
        registration.comment   = @"视频播放交互子控制器";
    }];

    // 1. 伴生评论动作 (带 isAvailable 探测)
    [self registerAction:IRIdentityMake(@"try_comment", @"伴生评论互动") builder:^(IRActionBuilder *action) {
        action.available(YES);

        [action setPerformBlock:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
            // 利用 __kindof 直接强类型赋值，无需强制类型转换
            AWBPlayerPageAgent *agent = context.agent;

            // 分支 A: 可用性检查 (只读，根据自身属性探测是否允许评论)
            if (context.isAvailable) {
                return agent.canComment;
            }

            // 分支 B: 执行评论互动 (打开 -> 停留 -> 退出)
            do {
                IRPageNode *node = IRPageNode(nil, diag);
                if (!node.invoke(@"open_comment")) {
                    break;
                }

                IRPageNode *commentNode = IRPageNode(@"feed.comment", diag);
                commentNode.wait(node.weightOrValue(@"comment_dwell", @(3.0)).doubleValue);
                commentNode.invoke(@"back");
            } while (NO);

            return diag.noError;
        }];
    }];

    // 2. 作品消费闭环复合动作 (串联子 Action，非 Session 模式下严格采用 do-while 保护)
    [self registerAction:IRIdentityMake(@"watch", @"作品消费闭环") builder:^(IRActionBuilder *action) {
        [action setPerformBlock:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
            IRPageNode *player = IRPageNode(nil, diag);

            do {
                // 第一步: 核心必经主干: 看播倒计时 (失败立即中断 break)
                if (!player.invokeWith(@"watch_dwell", context.params) || player.hasError) {
                    break;
                }

                // 第二步: 伴生评论互动 (先判可用性，执行失败需清理错误，防止污染主流程)
                if (player.available(@"try_comment")) {
                    player.invoke(@"try_comment");
                    if (player.hasError) {
                        player.clearError(); // 故障隔离
                    }
                }

                // 第三步: 伴生关联搜索 (先判可用性，执行失败同样隔离错误)
                if (player.available(@"try_search")) {
                    player.invoke(@"try_search");
                    if (player.hasError) {
                        player.clearError();
                    }
                }

            } while (NO);

            return diag.noError;
        }];
    }];

    // 3. 跨 Agent 数据查询动作 (Query 模式: 声明 result 支持查询模式，在 perform 中处理 context.isQuery)
    [self registerAction:IRIdentityMake(@"video_info", @"获取当前视频元信息") builder:^(IRActionBuilder *action) {
        [action setTypeEncodingBlock:^(IRTypeEncoding *encoding) {
            encoding.result(IRSchemaTypeObject).setComment(@"视频元数据字典");
        }];

        [action setPerformBlock:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
            if (context.isQuery) {
                AWBPlayerPageAgent *agent = context.agent;
                context.result = @{
                    @"duration": @(agent.duration),
                    @"item_type": agent.itemType ?: @"video",
                    @"can_comment": @(agent.canComment)
                };
                return diag.noError;
            }
            return diag.noError;
        }];
    }];
}

@end
