//
//  IRPageNode+IRWatchTimeline.h
//  IRAutomationKit
//
//  IRPageNode 的看播时间轴分类 (node.timeline) 与链式语法糖 (on / waitMinInterval / start)
//

#import <IRAutomationKit/IRPageNode.h>
#import <IRAutomationKit/IRWatchTimeline.h>

NS_ASSUME_NONNULL_BEGIN

#pragma push_macro("IRPageNode")
#undef IRPageNode
@interface IRPageNode (IRWatchTimeline)

/// 创建绑定本节点 diag 的看播时间轴 (支持链式 .on(percent, name, desc, handler)...start())
@property (nonatomic, readonly) IRWatchTimeline *(^timeline)(NSString *name, NSInteger duration);

@end
#pragma pop_macro("IRPageNode")

@interface IRWatchTimeline (IRPageNode)

/// 创建绑定诊断的时间线对象 (供 node.timeline(...) 语法糖使用)
+ (instancetype)timelineWithName:(NSString *)name duration:(NSInteger)duration diag:(nullable IRDiagnostics *)diag;

/// 链式注册时间序事件 (必须在 start 前调用)
@property (nonatomic, readonly) IRWatchTimeline *(^on)(int percent, NSString *name, NSString *_Nullable desc, IRWatchTimelineActionHandler handler);

/// 若距上次真正执行的事件不足 minInterval 秒，则按任务检查点补足剩余间隔；否则不等待
@property (nonatomic, readonly) void (^waitMinInterval)(IRTimelineAction *action, NSTimeInterval minInterval);

/// 链式立即启动 (使用绑定的 diag)
@property (nonatomic, readonly) void (^start)(void);

@end

NS_ASSUME_NONNULL_END
