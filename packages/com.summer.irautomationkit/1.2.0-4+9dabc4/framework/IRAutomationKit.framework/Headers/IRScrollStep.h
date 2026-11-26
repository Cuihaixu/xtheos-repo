//
//  IRScrollStep.h
//  IRAutomationKit
//
//  滚动操作里的一步 (SCROLL.md 第 38、39 项): 策略返回它, 滚动操作执行, 结果回填到同一个对象, 存进 context.steps。
//  一步 = 手势来源 × 结束方式; 另有两种不发手势的步骤 (点停 / 停顿)。尽量满足预期, 不完全保证 (差的由下一步补)。
//
//      [IRScrollStep dragBy:80];                                         // 拖动 80pt, 不带惯性
//      [IRScrollStep swipeBy:context.remaining ending:IRScrollStepEndingTapStop];   // 滑-点
//      [IRScrollStep pauseFor:0.15];                                     // 段间停顿
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <IRAutomationKit/IRScrollPlaybackPlan.h>

NS_ASSUME_NONNULL_BEGIN

/// 手势偏好 (可多选; 软偏好, 见 preferredGestures)
typedef NS_OPTIONS(NSUInteger, IRScrollGestureKind) {
    IRScrollGestureNone = 0,
    IRScrollGestureDrag = 1 << 0,         ///< 拖动: 松手基本没速度, 不靠惯性 (惯性部分 < 拖动的 25% 或松手 < 0.1pt/ms)
    IRScrollGestureMultiSwipe = 1 << 1,   ///< 多滑组合: 一条记录里有 ≥ 2 个手势段
};

/// 这一步做什么
typedef NS_ENUM(NSInteger, IRScrollStepAction) {
    IRScrollStepActionDrag,       ///< 合成拖动 (IRDragGesture performScroll, 按列表偏移闭环), 只能 不带惯性 结束
    IRScrollStepActionSwipe,      ///< 按距离从回放方案里挑一条录制回放
    IRScrollStepActionPlayback,   ///< 回放指定的方案 / 录制 (距离只用来点停和核对)
    IRScrollStepActionTapStop,    ///< 不发手势: 列表还在滚就点一下停住
    IRScrollStepActionPause,      ///< 不发手势: 等一会儿 (段间间隔)
};

/// 手势怎么结束
typedef NS_ENUM(NSInteger, IRScrollStepEnding) {
    IRScrollStepEndingNoInertia,   ///< 停住再抬手, 不带惯性 (拖动)
    IRScrollStepEndingTapStop,     ///< 惯性滚到目标 (起点 + distance) 时点停; 滚不到就自然停
    IRScrollStepEndingNatural,     ///< 不点, 等惯性自己停
    IRScrollStepEndingRelease,     ///< 只等松手, 惯性没停就开始下一步 (连续快甩)
};

@interface IRScrollStep : NSObject

#pragma mark 创建 (distance 带正负: 正 = 偏移变大 = 往前 / 往下看)

/// 拖动 distance (列表滚多远), 不带惯性
+ (instancetype)dragBy:(CGFloat)distance;
/// 按距离挑录制回放 (预计滚 |distance| 左右的), 按 ending 结束
+ (instancetype)swipeBy:(CGFloat)distance ending:(IRScrollStepEnding)ending;
/// 回放指定方案 (recordID 不为空就回放方案里的这条录制); distance = 预计滚多远 (点停用; 不点停时只用来核对方向)
+ (instancetype)playbackPlan:(NSString *)planID record:(nullable NSString *)recordID distance:(CGFloat)distance ending:(IRScrollStepEnding)ending;
/// 列表还在滚就点一下停住 (已经停了不点, 免得点开 cell)
+ (instancetype)tapStop;
/// 等 seconds 秒
+ (instancetype)pauseFor:(NSTimeInterval)seconds;

@property (nonatomic, readonly) IRScrollStepAction action;
@property (nonatomic, readonly) IRScrollStepEnding ending;
@property (nonatomic, readonly) CGFloat distance;
@property (nonatomic, copy, readonly, nullable) NSString *planID;
@property (nonatomic, copy, readonly, nullable) NSString *recordID;
@property (nonatomic, readonly) NSTimeInterval pause;

#pragma mark 可选参数

/// 挑录制时的手势偏好 (swipe), 默认取滚动操作的 preferredGestures
@property (nonatomic) IRScrollGestureKind preferredGestures;
/// 拖动按录制走的方案 (drag, 见 IRDragGesture.templatePlanID), nil = 取滚动操作的 dragTemplatePlanID
@property (nonatomic, copy, nullable) NSString *dragTemplatePlanID;

#pragma mark 结果 (执行后回填)

@property (nonatomic, readonly) BOOL executed;
/// 开始 / 结束时的偏移 (结束 = 停稳后; 只等松手的是松手时), 实际滚了多少 (有正负)
@property (nonatomic, readonly) CGFloat fromOffset;
@property (nonatomic, readonly) CGFloat toOffset;
@property (nonatomic, readonly) CGFloat scrolledDistance;
/// 挑录制时预计滚多远 (swipe; 0 = 没有预计)
@property (nonatomic, readonly) CGFloat estimatedDistance;
/// 用的哪条录制 (回放 / 按录制拖动); 手势类型: drag / swipe / multi / synthetic (直线拖动) / - (不发手势)
@property (nonatomic, copy, readonly, nullable) NSString *playedRecordID;
@property (nonatomic, copy, readonly, nullable) NSString *playedRecordName;
@property (nonatomic, copy, readonly) NSString *gestureKind;
/// 点停了没有
@property (nonatomic, readonly) BOOL tapStopped;
@property (nonatomic, readonly) NSTimeInterval duration;
/// 这一步没做到 / 出错的原因 (成功为 nil)。没做到预期 (如拖动差了几 pt) 滚动操作接着问策略; 出错 (投递失败、列表读不到) 滚动操作结束
@property (nonatomic, copy, readonly, nullable) NSString *failureReason;

/// 发不发手势 (拖动 / 回放 = YES)
@property (nonatomic, readonly) BOOL isGesture;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
