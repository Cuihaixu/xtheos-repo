//
//  IRScrollOperation.h
//  IRAutomationKit
//
//  滚动操作 (SCROLL.md 第 20~26、29~43 项): 竖直方向的一次滚动 —— 方向 + 距离 + 允许偏差, 怎么走由策略决定。
//  一个对象 = 一次滚动: 配参数 → start → 结果留在对象上 (成功失败都有)。start 只能调一次。
//  在持有执行权的 UI 任务线程上同步执行 (回放 / 点停是写操作)。
//
//  第二版: 跑策略循环 —— 问策略 (strategy, 按名字) 要下一步 (IRScrollStep) → 执行、结果回填、存进 context.steps → 再问, 策略返回 nil 才结束;
//  结束时停在目标区间里算成功。公共兜底: 目标整个在能滚的范围外不滑直接失败、到顶 / 到底 (往下先等加载更多)、每步开始前列表不在窗口里报错。
//  默认策略 IRScrollStrategySwipeTap = 第一版的走法: 按测量距离 (没测过按动作摘要估) 挑录制回放, 惯性会超出区间就在进区间时点停 (带人的反应时间),
//  停稳后不在区间里就纠正 (不够往前补, 多了往回拉), 最多 maxScrolls 步。
//
//      IRScrollOperation *scroll = element.scrollOperation(diag);
//      scroll.direction = IRScrollForward;   // 往下看
//      scroll.distance = 300;
//      scroll.tolerance = 40;
//      if (![scroll start]) { … scroll.failureReason … }
//      scroll.scrolledDistance / scroll.scrollCount / scroll.offset …
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRUIScrollView.h>
#import <IRAutomationKit/IRDiagnostics.h>
#import <IRAutomationKit/IRScrollPlaybackPlan.h>
#import <IRAutomationKit/IRScrollStep.h>
#import <IRAutomationKit/IRScrollContext.h>
#import <IRAutomationKit/IRScrollStrategy.h>
#import <IRAutomationKit/IRScrollBuiltinStrategies.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRScrollOperation : NSObject

#pragma mark 策略注册 (按名字; 内置的已注册)

/// 注册策略: 名字 → 造策略的 block (每次滚动操作新建一个实例)。名字已存在 (内置或先注册的) 返回 NO, 原因写 diag
+ (BOOL)registerStrategy:(IRScrollStrategyFactory)factory forName:(IRScrollStrategyName)name diag:(nullable IRDiagnostics *)diag;
/// 已注册的名字
@property (class, nonatomic, copy, readonly) NSArray<IRScrollStrategyName> *registeredStrategyNames;

/// 创建: 滚动视图 + 诊断 (过程日志、错误写进它; nil = 内部建一个)
+ (instancetype)operationWithScrollView:(IRUIScrollView *)scrollView diag:(nullable IRDiagnostics *)diag;
@property (nonatomic, strong, readonly) IRUIScrollView *scrollView;
@property (nonatomic, strong, readonly) IRDiagnostics *diag;

#pragma mark 参数

/// 方向, 默认往前 (偏移变大 = 往下看)
@property (nonatomic) IRScrollDirection direction;
/// 滚多远 (pt, > 0)
@property (nonatomic) CGFloat distance;
/// 允许偏差 (pt, ≥ 0): 停在 [distance − tolerance, distance + tolerance] 里算成功
@property (nonatomic) CGFloat tolerance;
/// 内置策略最多走几步手势, 包含第一次 (剩下的都用来纠正: 不够往前补, 多了往回拉); 默认 3 (context.maxSteps)。
/// 滚动操作本身不限步数: 自定义策略看 context.steps 自己判断
@property (nonatomic) NSUInteger maxScrolls;

/// 策略名字, 默认 IRScrollStrategySwipeTap
@property (nonatomic, copy) IRScrollStrategyName strategy;
/// 纠正策略名字 (内置策略主体走完还不在区间就交给它), 默认 IRScrollStrategyCorrect
@property (nonatomic, copy) IRScrollStrategyName correctionStrategy;
/// 给策略的 userInfo: 传入就用这一个 (贯穿多次滚动操作, 放页面级业务状态); 不传每次新建
@property (nonatomic, strong, nullable) NSMutableDictionary *userInfo;

/// 优先用的手势 (可多选, 软偏好): 先按距离筛候选 → 同时满足所有 → 满足任一 → 都没有就当没设 (diag 记一条)。默认不设
@property (nonatomic) IRScrollGestureKind preferredGestures;

/// 回放方案, 默认 IRScrollPlaybackPlan.defaultPlan
@property (nonatomic, strong) IRScrollPlaybackPlan *playbackPlan;
/// 拖动步骤按录制走的方案 (IRDragGesture.templatePlanID); 默认取 defaultDragTemplatePlanID (没设为 nil = 直线匀速)。步骤自己设了以步骤为准
@property (nonatomic, copy, nullable) NSString *dragTemplatePlanID;
/// 新建滚动操作的 dragTemplatePlanID 默认值 (进程级, 同 IRScrollPlaybackPlan.defaultPlan), 应用启动时设成自己的拖动录制方案;
/// 默认 nil (直线匀速)。element.scroll / scrollVertical 等内部建的滚动操作也用它
@property (class, nonatomic, copy, nullable) NSString *defaultDragTemplatePlanID;
/// 点停像人: 平均反应时间 (秒), 默认 0.22; 每次按正态分布随机 (±0.05, 限 0.12~0.4), 提前预判、真的等这么久再点;
/// 偏得不多时偶尔 (15%) 不点停。0 = 到点立即点 (最准)
@property (nonatomic) NSTimeInterval reactionTime;
/// 限定范围: 离顶部 / 底部不到这个距离 (pt) 就不再往那边滚; 默认 0 = 可以滚到头
@property (nonatomic) CGFloat topMargin;
@property (nonatomic) CGFloat bottomMargin;

#pragma mark 执行

/// 同步执行; 策略说停时停在区间里返回 YES。只能调一次
- (BOOL)start;

#pragma mark 结果 (start 之后; 成功失败都有)

/// 实际滚了几次 (发了手势的步数, 含纠正)
@property (nonatomic, readonly) NSUInteger scrollCount;
/// 点停了几次
@property (nonatomic, readonly) NSUInteger tapStopCount;
/// 开始时的偏移 / 现在停在哪
@property (nonatomic, readonly) CGFloat startOffset;
@property (nonatomic, readonly) CGFloat offset;
/// 实际滚了多远 (offset − startOffset, 有正负)
@property (nonatomic, readonly) CGFloat scrolledDistance;
/// 用了多久 (秒)
@property (nonatomic, readonly) NSTimeInterval duration;
/// 执行过的所有步骤 (结果已回填; 同 context.steps)
@property (nonatomic, copy, readonly) NSArray<IRScrollStep *> *steps;
/// 这次用的策略实例 (start 之后; 如自动策略看 chosen)
@property (nonatomic, strong, readonly, nullable) id<IRScrollStrategy> strategyInstance;
/// 上下文 (start 之后; 策略收到的那一个)
@property (nonatomic, strong, readonly, nullable) IRScrollContext *context;
/// 没成功的原因 (成功为 nil); 同时写进 diag.error
@property (nonatomic, copy, readonly, nullable) NSString *failureReason;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
