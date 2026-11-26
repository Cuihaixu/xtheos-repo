//
//  IRScrollBuiltinStrategies.h
//  IRAutomationKit
//
//  内置滚动策略 (SCROLL.md 第 33、44、45 项)。名字 (IRScrollStrategy.h) 对应默认参数的实例;
//  要别的参数就配一个实例按自己的名字注册:
//
//      [IRScrollOperation registerStrategy:^id<IRScrollStrategy>{
//          IRScrollDragStrategy *s = [IRScrollDragStrategy new];
//          s.maxDragDistance = 120;
//          return s;
//      } forName:@"app.drag120" diag:nil];
//
//  内置策略都分两段: 主体 (子类的 mainStepWithContext:) 走完 → 还不在区间就交给纠正策略 (滚动操作的 correctionStrategy, 默认 correct)。
//  一个实例只用于一次滚动操作 (状态在实例上)。
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRScrollStrategy.h>

NS_ASSUME_NONNULL_BEGIN

/// 内置策略的基类: 到了就停; 主体走完交给纠正。子类重写 mainStepWithContext:
@interface IRScrollBuiltinStrategy : NSObject <IRScrollStrategy>
/// 主体的下一步; nil = 主体走完 (之后不再调)
- (nullable IRScrollStep *)mainStepWithContext:(IRScrollContext *)context;
/// 主体结束后转交纠正 (默认 YES)
@property (nonatomic) BOOL correctsAfterwards;
@end

/// 滑-点 (swipe_tap, 第一版的走法): 主体 = 按剩余距离滑一次, 越过目标点停
@interface IRScrollSwipeTapStrategy : IRScrollBuiltinStrategy
@end

/// 拖动 (drag, 第 44 项): 段数 n = ⌈|距离| / maxDragDistance⌉; 每段 = 还差的 ÷ 剩下段数 × 随机 (±segmentJitter), 不超过上限且保证后面够,
/// 最后一段 = 还差的全部; 段间停顿 (pauseMin ~ pauseMax)
@interface IRScrollDragStrategy : IRScrollBuiltinStrategy
@property (nonatomic) CGFloat maxDragDistance;   ///< 默认 150
@property (nonatomic) CGFloat segmentJitter;     ///< 默认 0.15
@property (nonatomic) NSTimeInterval pauseMin;   ///< 默认 0.08
@property (nonatomic) NSTimeInterval pauseMax;   ///< 默认 0.25
@end

/// 小步快跑 (small_steps): 拆成 k 段, 每段一次滑-点, 段间停顿。每段长度落在 "舒服范围" (方案里这个方向录制预计距离的 30%~70% 分位);
/// k 从能落进的段数里随机挑, 偏向少的 (权重 1/k); 每段 ±segmentJitter, 最后一段 = 还差的全部
@interface IRScrollSmallStepsStrategy : IRScrollBuiltinStrategy
@property (nonatomic) CGFloat segmentJitter;     ///< 默认 0.15
@property (nonatomic) NSTimeInterval pauseMin;   ///< 默认 0.15
@property (nonatomic) NSTimeInterval pauseMax;   ///< 默认 0.4
@end

/// 快速步进 (fast_step): 每次滑一下, 惯性自然停; 只有越过 目标 + 允许偏差 才点停 (明显过头)。
/// 差得不多 (< 舒服范围下限) 或主体手势用完 (⌈距离 / 舒服范围上限⌉ + 1) 交给纠正
@interface IRScrollFastStepStrategy : IRScrollBuiltinStrategy
@end

/// 连续快甩 (fling): 距离 > minScreens 屏时, 前几下只等松手连着甩 (每下按录制预计距离的 90% 分位算), 最后一下滑-点停在目标; 否则就是一次滑-点
@interface IRScrollFlingStrategy : IRScrollBuiltinStrategy
@property (nonatomic) CGFloat minScreens;        ///< 默认 2
@end

/// 过头回看 (overshoot): 先滑-点到目标再往前 overshootMin ~ overshootMax, 停 lookMin ~ lookMax 秒 (像在看), 再交给纠正拉回
@interface IRScrollOvershootStrategy : IRScrollBuiltinStrategy
@property (nonatomic) CGFloat overshootMin;      ///< 默认 0.1
@property (nonatomic) CGFloat overshootMax;      ///< 默认 0.3
@property (nonatomic) NSTimeInterval lookMin;    ///< 默认 0.3
@property (nonatomic) NSTimeInterval lookMax;    ///< 默认 0.8
@end

/// 纠正 (correct): 差 ≤ maxDragDistance 拖动 (准、不带惯性), 否则滑-点; 最多 maxSteps − 1 次
@interface IRScrollCorrectStrategy : NSObject <IRScrollStrategy>
@property (nonatomic) CGFloat maxDragDistance;   ///< 默认 150
@end

/// 自动 (auto): 第一次按距离筛出适用的 (拖动 ≤ 300pt, 小步快跑 > 200pt, 连续快甩 > 2 屏; 其它都适用),
/// 按 weights 加权随机挑一个, 之后整次操作都交给它。weights 里可以放注册的用户策略 (没注册的跳过)
@interface IRScrollAutoStrategy : NSObject <IRScrollStrategy>
/// 名字 → 权重; 默认 swipe_tap 3, drag 2, small_steps 2, fast_step 2, fling 1, overshoot 1
@property (nonatomic, copy) NSDictionary<IRScrollStrategyName, NSNumber *> *weights;
/// 挑中的 (第一次调用后)
@property (nonatomic, copy, readonly, nullable) IRScrollStrategyName chosen;
@end

NS_ASSUME_NONNULL_END
