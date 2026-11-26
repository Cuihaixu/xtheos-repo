//
//  IRWatchTimeline.h
//  IRAutomationKit
//
//  声明式看播时间轴: 按总时长百分比调度伴生动作 (评论 / 搜索 / 详情等),
//  须在 startWithDiagnostics: 前完成全部事件注册, 启动后禁止动态添加。
//

#import <Foundation/Foundation.h>

@class IRDiagnostics;

NS_ASSUME_NONNULL_BEGIN

@class IRWatchTimeline, IRTimelineAction;

/// 时间轴事件处理回调
/// - Returns: YES 表示本次回调实际执行了动作; NO 表示判定后跳过 (例如权重选择不执行)。
///            timeline 仅在返回 YES 时更新 `previousActionExecuteTime`, 供后续 handler 判断间隔。
typedef BOOL(^IRWatchTimelineActionHandler)(IRWatchTimeline *timeline, IRTimelineAction *action);

@interface IRTimelineAction : NSObject

/// 事件名称 (调试用, 可重复)
@property (nonatomic, copy, readonly) NSString *name;

/// 事件描述 (调试日志用, 可选)
@property (nonatomic, copy, readonly, nullable) NSString *desc;

/// 执行百分比 (0~100)
@property (nonatomic, assign, readonly) int percent;

/// 计划执行时刻 (秒) = timeline.duration * percent / 100, 注册时算出
@property (nonatomic, assign, readonly) NSInteger scheduledTime;

/// 事件处理回调
@property (nonatomic, copy, readonly) IRWatchTimelineActionHandler handler;

@end

@interface IRWatchTimeline : NSObject

/// 时间线名 (日志前缀, 例如 "watch_video")
@property (nonatomic, copy, readonly) NSString *name;

/// 时间线总时长 (秒)
@property (nonatomic, assign, readonly) NSInteger duration;

/// 所有已注册的时间序事件 (按注册/排序顺序)
@property (nonatomic, strong, readonly) NSArray<IRTimelineAction *> *registeredActions;

/// 上一个真正执行的事件的计划执行时刻 (秒)。
/// - handler 返回 NO (判定跳过) 时不更新;
/// - 从未真执行过则为 0。
/// 用法: `action.scheduledTime - timeline.previousActionExecuteTime` 即离上次真动作隔了几秒。
@property (nonatomic, assign, readonly) NSInteger previousActionExecuteTime;

/// 是否已启动 (启动后禁止再动态注册事件)
@property (nonatomic, assign, readonly) BOOL started;

/// 是否已被提前终止
@property (nonatomic, assign, readonly) BOOL stopped;

/// 创建时间线对象
/// - Parameters:
///   - name: 日志前缀
///   - duration: 时间线总时长 (秒)
+ (instancetype)timelineWithName:(NSString *)name duration:(NSInteger)duration;

/// 注册时间序事件 (必须在 startWithDiagnostics: 前调用)
/// - Parameters:
///   - name: 事件名称
///   - desc: 事件描述 (可选)
///   - percent: 执行百分比 (0~100)
///   - handler: 事件处理者, 返回 YES 表示实际执行
- (void)registerAction:(NSString *)name
                  desc:(nullable NSString *)desc
               percent:(int)percent
               handler:(IRWatchTimelineActionHandler)handler;

/// 开始执行时间序 (同步阻塞, 总耗时 ≈ duration 秒)。
/// 每秒推进 (走任务检查点); 到达 checkpoint 触发并记录日志。
- (void)startWithDiagnostics:(nullable IRDiagnostics *)diag;

/// 提前终止时间线推进 (在事件 handler 中调用可立即结束后续等待与未触发事件)
- (void)stop;

@end

NS_ASSUME_NONNULL_END
