//
//  IRScrollPlaybackPlan.h
//  IRAutomationKit
//
//  滚动用的回放方案 (SCROLL.md 第 26、27 项): 两个 cht 回放方案 —— 往前 (偏移变大, 看下面的) / 往后 (偏移变小, 看上面的), 都必须有
//  (多了要往回拉)。cht 里记录组按 "手指方向·看到的内容" 命名: 竖向 往前 = 手指上滑·看下面, 往后 = 手指下滑·看上面;
//  横向 (预留) 往前 = 手指左滑·看右边, 往后 = 手指右滑·看左边。
//  默认对象 defaultPlan 内置 cht 网络库 "通用手势" / 列表滚动 的方案, 滚动操作不设就用它, 大多数情况不用管;
//  布局特殊的列表 (如半屏评论) 自己建一个换上。
//
//  挑哪条录制 (滚动操作里做): 录制带 cht 测量的滚动距离 (scrollMeasurements) 时按距离挑最接近的一条, 用 RecordId 回放;
//  方案里没有测量时交给 cht 按方案自己挑。录制只记客观数字 (测量距离 / 时长), 不打快慢标签
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

NS_ASSUME_NONNULL_BEGIN

/// 方向: 按偏移定义 (不说"上滑 / 下滑", 那说的是手指, 容易反)
typedef NS_ENUM(NSInteger, IRScrollDirection) {
    IRScrollForward = 1,     ///< 偏移变大 (竖向列表: 内容往上走, 看下面的; 手指上滑)
    IRScrollBackward = -1,   ///< 偏移变小 (看上面的; 手指下滑)
};

/// 方案里的一条录制 (CHRTouchPlaybackPlanRecords 的一项, CHRTouchKit 2.13.0 起)。距离都是**偏移空间、带正负**:
/// 正 = 偏移变大 (往前, 手指上滑; 同 CHRTouchKit "上滑 dy > 0"), 已按本机屏幕高度和录制屏幕的比例换算
@interface IRScrollPlaybackRecord : NSObject
@property (nonatomic, copy, readonly) NSString *recordID;
@property (nonatomic, copy, readonly, nullable) NSString *name;
@property (nonatomic, copy, readonly) NSArray<NSString *> *tags;
/// 动作摘要: 手指拖动阶段列表跟着走的偏移 (所有手势段合计, dragOffsetPt.dy)
@property (nonatomic, readonly) CGFloat dragDistance;
/// 动作摘要: 松手速度 (pt/ms, 最后一个手势段, releaseVelocity.dy)
@property (nonatomic, readonly) CGFloat releaseVelocity;
/// 手势段数 (gestures.count; ≥ 2 = 多滑组合)
@property (nonatomic, readonly) NSUInteger gestureCount;
/// cht 测量的滚动距离 (本机型 / 同屏幕的竖直测量 avg.dy); 没测过为 0
@property (nonatomic, readonly) CGFloat measuredDistance;
/// cht 测量的回放时长 (秒, 发出回放 → 列表最后一次滚动, avg.durationMs); 没测过为 0
@property (nonatomic, readonly) NSTimeInterval measuredDuration;
/// 整条回放时长 (秒, durationNs, 含间隔)
@property (nonatomic, readonly) NSTimeInterval duration;
/// 预计滚多远: 有测量用测量; 没有按摘要估 = 拖动 + inertiaConstant × 松手速度 (inertiaConstant = 列表减速率算的常数, 默认减速 ≈ 499)
- (CGFloat)estimatedDistanceWithInertiaConstant:(CGFloat)inertiaConstant;
@end

@interface IRScrollPlaybackPlan : NSObject

/// 默认方案: 应用注册过就用注册的, 否则用内置的 cht 网络库 "通用手势" / 列表滚动 (和 App 无关):
/// 往前 = "手指上滑·看下面", 往后 = "手指下滑·看上面"。每次取都是新对象 (调用方改了不影响别人)。
/// 应用在启动时注册自己的 (如按 App 录的手感不同), 设 nil 恢复内置
@property (class, nonatomic, strong, null_resettable) IRScrollPlaybackPlan *defaultPlan;

/// 两个方向的 cht 回放方案 ID, 都必须有 (空字符串: 滚动操作开始时失败)
+ (instancetype)planWithForward:(NSString *)forwardPlanID backward:(NSString *)backwardPlanID;
@property (nonatomic, copy, readonly) NSString *forwardPlanID;
@property (nonatomic, copy, readonly) NSString *backwardPlanID;
- (NSString *)planIDForDirection:(IRScrollDirection)direction;

/// 读方案里的录制和测量 (CHRTouchPlaybackPlanRecords; CHRTouchKit 还没有这个接口时返回空数组, 滚动操作交给 cht 按方案挑)
- (NSArray<IRScrollPlaybackRecord *> *)recordsForDirection:(IRScrollDirection)direction;

@end

NS_ASSUME_NONNULL_END
