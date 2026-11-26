//
//  IRDragGesture.h
//  IRAutomationKit
//
//  合成的拖动手势 (GESTURE.md): 我们自己按帧发 按下 → 移动 … → 抬起 (CHRTouchKit 手动触摸), 不带惯性地松手。
//  在持有执行权的 UI 任务线程上同步执行 (写操作)。只能 perform 一次。
//
//  这一版只做功能 (GESTURE.md 第 15 项): 手指直线、匀速 (speed), 不加任何猜出来的拟人参数;
//  唯一保留的是实测必须的: 停住时每帧发原地 Move (横向 ±0.2pt 交替, 完全同点偶尔被当成没动), 抬手才没有惯性 (第 9a、9c 项)。拟人: 设 templatePlanID 按真人录制的轨迹走 (第 18 项)。
//
//  三个入口, 底层同一套 (每帧 "下一帧往哪走" 由谁决定):
//    performWithHandler:  回调每帧返回方向 (零向量 = 按着不动), *stop = YES 松手 —— 目标在拖的过程中才知道 (拖到一个动态位置的视图、来回拖)
//    performMoveBy:       手指移动固定位移, 到了松手 (回调由库提供)
//    performScroll:by:    列表从当前偏移再滚 n pt (边拖边读偏移, 到了松手; 回调由库提供)
//
//      [element.dragGesture(diag) performScroll:list by:10];
//
//      [drag performWithHandler:^CGVector(IRDragState *s, BOOL *stop) {
//          if (s.segmentDistance < 50) return IRDragDirection(90);   // 往上 50
//          *stop = YES;
//          return CGVectorMake(0, 0);
//      }];
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <IRAutomationKit/IRDiagnostics.h>
#import <IRAutomationKit/IRUIView.h>
#import <IRAutomationKit/IRUIScrollView.h>

NS_ASSUME_NONNULL_BEGIN

@class IRApplication;

/// 角度 → 方向向量 (屏幕坐标): 0 = 右, 90 = 上, 180 = 左, 270 = 下 (y 往下为正, 所以 "上" 是 dy < 0)
FOUNDATION_EXPORT CGVector IRDragDirection(CGFloat degrees);

/// 走过的一段 (方向不变的连续几帧; 按着不动也算一段, 方向为零)
@interface IRDragSegment : NSObject
@property (nonatomic, readonly) CGVector direction;   ///< 单位向量; 按着不动为 (0, 0)
@property (nonatomic, readonly) CGFloat distance;     ///< 这段手指走了多远 (pt)
@property (nonatomic, readonly) NSTimeInterval duration;
@end

/// 每帧回调时的状态
@interface IRDragState : NSObject
/// 被拖的视图: element.dragGesture = 那个元素; 直接给坐标为 nil
@property (nonatomic, strong, readonly, nullable) IRUIView *view;
@property (nonatomic, readonly) CGPoint startPoint;
/// 手指当前位置 (屏幕 pt)
@property (nonatomic, readonly) CGPoint fingerPoint;
/// 从起点算起的总位移
@property (nonatomic, readonly) CGVector displacement;
/// 手指走过的路程总长 (来回都算)
@property (nonatomic, readonly) CGFloat pathLength;
/// 当前这一段: 方向 (单位向量; 按着不动为零) / 这段已走多远 / 这段多久。回调返回的方向一变就开始新的一段
@property (nonatomic, readonly) CGVector segmentDirection;
@property (nonatomic, readonly) CGFloat segmentDistance;
@property (nonatomic, readonly) NSTimeInterval segmentDuration;
/// 走完的段 (不含当前这段)
@property (nonatomic, copy, readonly) NSArray<IRDragSegment *> *segments;
/// 现在叫停, 被拖的东西 (列表 / 进度条) 还会再走多远: 投递延迟期间手指已发出的 (实测发出到跟上 ~5ms, 按 1.5 帧算; pt)。
/// 按被拖的东西读到的位置判断停不停时, 剩下的 ≤ leadDistance 就叫停
@property (nonatomic, readonly) CGFloat leadDistance;
/// 给回调自己存状态 (比如 "现在是第几阶段"), 整次手势内有效
@property (nonatomic, strong, readonly) NSMutableDictionary *userInfo;
@property (nonatomic, readonly) NSTimeInterval elapsed;
/// 第几次回调 (0 = 开始前, 决定第一帧往哪走)
@property (nonatomic, readonly) NSUInteger frame;
@end

@interface IRDragGesture : NSObject

+ (instancetype)gestureWithApplication:(IRApplication *)application diag:(nullable IRDiagnostics *)diag;
@property (nonatomic, weak, readonly, nullable) IRApplication *application;
@property (nonatomic, strong, readonly) IRDiagnostics *diag;

#pragma mark 参数

/// 起点 (屏幕 pt)
@property (nonatomic) CGPoint startPoint;
/// 被拖的视图 (回调里的 state.view); element.dragGesture 会设成那个元素
@property (nonatomic, strong, nullable) IRUIView *view;
/// 手指速度 (pt/s), 默认 300。每帧重新读: 回调里改它就能中途变速
@property (nonatomic) CGFloat speed;
/// 帧间隔 (秒), 默认 0.008
@property (nonatomic) NSTimeInterval frameInterval;
/// 松手前原地停多久 (秒, 期间每帧发原地 Move; 实测不发会带惯性), 默认 0.12
@property (nonatomic) NSTimeInterval holdBeforeRelease;
/// 拟人模板 (cht 回放方案 ID, 方案里要有拖动型录制; 如 抖音 / 视频列表 / 拖动滑动): 设了它, performMoveBy / performScroll 改为 **按录制走** ——
/// 挑一条拖动距离接近的真人录制, 把它的轨迹用 旋转 + 缩放 映射到这次的起点 → 终点 (横向漂移、节奏原样保留), 每帧的时间、力度、触面原样发
/// (CHRTouchFingerEvent); performScroll 边放边读偏移, 到了就截住, 原地按录制结尾 "停住" 那段的节奏和力度发完再抬起。nil = 直线匀速 (功能版)
@property (nonatomic, copy, nullable) NSString *templatePlanID;
/// 用了哪条录制 (按录制走时; 调试用)
@property (nonatomic, copy, readonly, nullable) NSString *templateRecordID;
/// 兜底: 最长路程 (pt, 默认 3000) / 最长时间 (秒, 默认 15); 到了按失败处理。手指到屏幕边也失败
@property (nonatomic) CGFloat maxDistance;
@property (nonatomic) NSTimeInterval timeout;

#pragma mark 执行 (只能调一次)

/// 回调每帧返回下一帧往哪走 (只看方向, 长度忽略; 零向量 = 按着不动, 每帧发原地 Move), *stop = YES 松手。开始前先回调一次 (frame = 0)
- (BOOL)performWithHandler:(CGVector (^)(IRDragState *state, BOOL *stop))handler;
/// 手指从起点移动固定位移 (屏幕 pt), 到了松手
- (BOOL)performMoveBy:(CGVector)displacement;
/// 列表从当前偏移再滚 distance pt (正 = 偏移变大 = 往下看, 手指往上; 负 = 往回), 到了松手; 只做竖向。
/// 快到时放慢 (剩 30pt 内 120pt/s), 按 leadDistance 叫停。返回 YES = 实际滚动和要求差 ≤ 2pt; 实际滚了多少见 scrolledDistance
- (BOOL)performScroll:(IRUIScrollView *)list by:(CGFloat)distance;

#pragma mark 结果 (成功失败都有)

@property (nonatomic, readonly) CGPoint endPoint;
/// 手指走过的路程
@property (nonatomic, readonly) CGFloat movedDistance;
/// performScroll 时列表实际滚了多少 (抬手后停住时读)
@property (nonatomic, readonly) CGFloat scrolledDistance;
@property (nonatomic, readonly) NSUInteger frameCount;
@property (nonatomic, readonly) NSTimeInterval duration;
@property (nonatomic, copy, readonly, nullable) NSString *failureReason;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
