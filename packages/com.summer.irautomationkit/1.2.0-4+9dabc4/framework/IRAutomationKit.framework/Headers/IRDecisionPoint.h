//
//  IRDecisionPoint.h
//  IRAutomationKit
//
//  页面级决策点 (Decision Point) 与小范围动态判断队列:
//  - 在 PageAgent 的 +load 中通过 registerDecisionPoint:builder: 静态声明决策点 (带唯一标识 identity);
//  - 支持通过 [point setTypeEncodingBlock:^(IRTypeEncoding *encoding) { ... }] 注册该决策点支持的入参签名;
//  - 自动将决策点的时长权重与各事件的 [询问进度分段权重 / 执行概率权重 / 最小冷却间隔权重] 注册到当前 PageAgent 的权重空间;
//  - 执行时通过 node.decisionPoint(@"id").params(...).before(...).after(...).execute() 触发:
//    1) 校验/补齐决策点入参 decisionParams 并透传到 before / after 回调;
//    2) 动态调用 contextAction / contextBlock 获取当前页面上下文;
//    3) 提取当前决策点下可用事件及其参数签名 (IRActionSignature);
//    4) 询问远程决策器 (IRDecisionRemote), 按 [服务器 > 本地指定(传入参数/回调) > 权重随机] 决定时长与子事件执行;
//    5) 驱动 IRWatchTimeline 动态判断队列，每个事件执行前后触发 before / after 回调 (支持 addParams: / skip / stop)。
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRPageAgent.h>
#import <IRAutomationKit/IRPageNode.h>
#import <IRAutomationKit/IRActionBuilder.h>
#import <IRAutomationKit/IRActionContext.h>
#import <IRAutomationKit/IRIdentity.h>
#import <IRAutomationKit/IRWeightChain.h>
#import <IRAutomationKit/IRTimeRange.h>

NS_ASSUME_NONNULL_BEGIN

#pragma mark - 远程决策协议与全局入口

@protocol IRDecisionRemoteDecider <NSObject>

/// 同步询问远程服务器当前决策点该如何执行 (超时或出错返回 nil, 自动降级走本地权重)
/// - Parameters:
///   - pointID: 决策点标识 (如 @"watch_preview")
///   - scene: 当前显示的页面 pageID (context.page.pageID, 如 @"feed.recommend")
///   - context: 当前上下文数据 (由 point.contextAction / contextBlock 实时产出)
///   - actions: 当前决策点关联的可用事件及参数签名列表 (@[ @{ @"name": ..., @"comment": ..., @"signature": ... } ])
///   - timeout: 超时秒数
/// - Returns: 服务器返回的决策字典 (支持 data.result 或顶层包含 action/skip, duration/dwell_seconds, actions: @[@{ action/name, percent/at_percent, enabled, params/plan }])
- (nullable NSDictionary<NSString *, id> *)decidePoint:(NSString *)pointID
                                                 scene:(NSString *)scene
                                               context:(NSDictionary<NSString *, id> *)context
                                               actions:(NSArray<NSDictionary<NSString *, id> *> *)actions
                                               timeout:(NSTimeInterval)timeout;

@end

@interface IRDecisionRemote : NSObject

@property (class, nonatomic, readonly) IRDecisionRemote *sharedRemote;

/// 全局远程决策器 (由业务层如 AWBDecisionManager 挂载); nil = 纯本地权重决策
@property (atomic, strong, nullable) id<IRDecisionRemoteDecider> decider;

/// 远程请求超时时间 (秒, 默认 1.5s)
@property (atomic, assign) NSTimeInterval timeout;

@end

#pragma mark - 执行期事件回调对象

/// 决策点执行期事件对象 (在 before / after 回调中传入, 代表决策点自身或某个候选子事件)
@interface IRDecisionEvent : NSObject

/// 事件名称 (决策点自身如 @"watch_preview", 或候选子事件如 @"try_search")
@property (nonatomic, copy, readonly) NSString *name;

/// 事件完整路径 (决策点自身为 @"watch_preview", 候选子事件为 @"watch_preview.try_search")
@property (nonatomic, copy, readonly) NSString *eventPath;

/// 决策点采集的页面上下文信息 (如 live_info / video_info 产出的字典)
@property (nonatomic, copy, readonly) NSDictionary<NSString *, id> *context;

/// 观看总时长 (秒; 当 event 为决策点自身时可在 before 回调中读取或修改)
@property (nonatomic, assign) NSInteger duration;

/// 当前事件将要使用的参数字典 (只读快照; 修改请调 -addParams:)
@property (nonatomic, copy, readonly) NSDictionary<NSString *, id> *params;

/// 事件执行完成后的返回值 (在 after 回调中可读)
@property (nonatomic, strong, readonly, nullable) id result;

/// 事件是否执行成功 (在 after 回调中可读)
@property (nonatomic, assign, readonly) BOOL succeeded;

/// 是否已标记跳过当前事件
@property (nonatomic, assign, readonly) BOOL skipped;

/// 是否已标记终止整个决策点流程
@property (nonatomic, assign, readonly) BOOL stopped;

/// 为当前事件追加/覆盖参数 (若为决策点自身且包含 @"duration" 也会同步更新 duration)
- (void)addParams:(NSDictionary<NSString *, id> *)params;

/// 跳过当前事件 (不执行当前事件, 但决策点时间线继续推进后续事件)
- (void)skip;

/// 立即终止整个决策点流程 (后续时间线与未触发事件不再继续执行)
- (void)stop;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

/// 决策点事件执行前/后回调
/// - Parameters:
///   - eventPath: 当前事件路径 (如 @"watch_preview" 或 @"watch_preview.try_search")
///   - decisionParams: 传入决策点的参数字典 (经决策点 setTypeEncodingBlock 校验与默认值补齐后透传)
///   - event: 当前事件控制对象 (可调用 [event addParams:] / [event skip] / [event stop])
typedef void (^IRDecisionEventCallback)(NSString *eventPath,
                                        NSDictionary<NSString *, id> *decisionParams,
                                        IRDecisionEvent *event);

#pragma mark - 决策点事件与构建器

/// 单个决策点事件配置 (固定三要素: 询问进度分段权重 / 执行概率 / 最小冷却间隔范围)
@interface IRDecisionEventBuilder : NSObject

/// 事件名 (对应要调用的纯动作 Action 名)
@property (nonatomic, copy, readonly) NSString *actionName;

/// 1. 询问进度分段权重: 数组长度 N 将 0~100% 均分为 N 段, 某段不需要触发则传 @0 (如 @[@0, @30, @70])
@property (nonatomic, readonly) IRDecisionEventBuilder *(^progress)(NSArray<NSNumber *> *segmentWeights);

/// 2. 执行事件概率 (0~100%, 支持小数如 0.5、15.0; 底层按万分比 10000 自动注册 yes/no 权重)
@property (nonatomic, readonly) IRDecisionEventBuilder *(^probability)(double percent);

/// 3. 与上一已执行动作的最小冷却间隔范围 (秒, 如 .minIntervalRange(1.5, 3.5))
@property (nonatomic, readonly) IRDecisionEventBuilder *(^minIntervalRange)(NSTimeInterval minSeconds, NSTimeInterval maxSeconds);

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

/// 决策点构建器
@interface IRDecisionPointBuilder : NSObject

/// 0. 集中声明该决策点支持的入参签名 (同 IRActionBuilder, 入参将自动补齐默认值并透传至 before / after 回调)
- (void)setTypeEncodingBlock:(NS_NOESCAPE void (^)(IRTypeEncoding *encoding))block;

/// setTypeEncodingBlock: 的点语法写法
@property (nonatomic, readonly) void (^typeEncoding)(NS_NOESCAPE void (^block)(IRTypeEncoding *encoding));

/// 是否开启网络远程决策 (YES = 执行前询问 IRDecisionRemote 服务器决策; NO = 纯本地权重决策, 不发起网络请求; 默认 NO)
@property (nonatomic, readonly) void (^remote)(BOOL enabled);

/// 1. 绑定上下文来源 (方式 1, 执行时动态调用): 传只读查询事件名 (如 @"live_info" / @"video_info")
@property (nonatomic, readonly) void (^contextAction)(NSString *queryActionName);

/// 1b. 或绑定上下文回调 Block (执行时实时回调获取字典)
@property (nonatomic, readonly) void (^contextBlock)(NSDictionary<NSString *, id> *_Nullable (^block)(IRActionContext *context));

/// 2. 观看总时长 (可选: 设置后按时间轴进度推进; 不设则按注册顺序直接串行判断推进)
@property (nonatomic, readonly) void (^duration)(NSInteger defaultSeconds,
                                                 void (NS_NOESCAPE ^ _Nullable)(id<IRWeightGroupChain> group));

/// 2b. 观看总时长单区间简写 (如 point.durationRange(5, 15))
@property (nonatomic, readonly) void (^durationRange)(NSTimeInterval minSeconds, NSTimeInterval maxSeconds);

/// 3. 添加一个决策点候选事件
@property (nonatomic, readonly) IRDecisionEventBuilder *(^addEvent)(NSString *actionName);

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

#pragma mark - PageAgent 注册与 PageNode 调用分类

@interface IRPageAgent (IRDecisionPoint)

/// 在 PageAgent 的 +load 中注册一个决策点 (自动注册所属权重组与同名可执行事件)
+ (void)registerDecisionPoint:(IRIdentity *)identity
                      builder:(void (NS_NOESCAPE ^)(IRDecisionPointBuilder *point))builder;

@end

/// PageNode 上的决策点执行句柄
@interface IRDecisionPointRunner : NSObject

/// 传入决策点参数 (对应决策点 setTypeEncodingBlock 声明的参数, 透传至 before / after 回调)
@property (nonatomic, readonly) IRDecisionPointRunner *(^params)(NSDictionary<NSString *, id> *_Nullable params);

/// 每个事件 (含决策点自身与各候选子事件) 执行前回调
@property (nonatomic, readonly) IRDecisionPointRunner *(^before)(IRDecisionEventCallback _Nullable callback);

/// 每个事件 (含决策点自身与各候选子事件) 执行后回调
@property (nonatomic, readonly) IRDecisionPointRunner *(^after)(IRDecisionEventCallback _Nullable callback);

/// 执行该决策点 (驱动远程/本地综合计算与动态判断队列, 返回是否执行成功; 结果写入 node.result)
@property (nonatomic, readonly) BOOL (^execute)(void);

/// 带附加参数执行该决策点 (等价于 .params(params).execute())
@property (nonatomic, readonly) BOOL (^executeWith)(NSDictionary<NSString *, id> *_Nullable params);

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

#pragma push_macro("IRPageNode")
#undef IRPageNode
@interface IRPageNode (IRDecisionPoint)

/// 获取当前节点上的指定决策点执行器: node.decisionPoint(@"watch_preview").execute()
@property (nonatomic, readonly) IRDecisionPointRunner *(^decisionPoint)(NSString *pointID);

@end
#pragma pop_macro("IRPageNode")

NS_ASSUME_NONNULL_END
