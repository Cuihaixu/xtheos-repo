//
//  IRScrollStrategy.h
//  IRAutomationKit
//
//  滚动策略 (SCROLL.md 第 30、37、41、42 项): 每个决策点给出下一步, 返回 nil = 不再滚 (滚动操作只在策略返回 nil 时结束)。
//  开始时调一次, 之后每一步执行完 (停稳 / 松手) 各调一次。和滚动操作在同一条线程上串行、同步:
//  回调里可以做业务 (读可见 cell、点进去看、退回来), 卡多久都行; 返回下一步前要回到这个列表, 不在窗口里滚动操作报错结束。
//
//  策略按名字指定 (字符串, 可扩展): 内置的给常量; 外部的先注册 —— 注册 "造策略" 的 block, 每次滚动操作新建一个实例:
//
//      [IRScrollOperation registerStrategy:^id<IRScrollStrategy>{ return [MyBrowseStrategy new]; } forName:@"aweme.browse" diag:nil];
//      scroll.strategy = @"aweme.browse";
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRScrollStep.h>
#import <IRAutomationKit/IRScrollContext.h>

NS_ASSUME_NONNULL_BEGIN


/// 滑-点 + 纠正 (第一版的走法, 默认): 每步按剩余距离挑录制回放, 越过目标点停; 不在区间就再来, 最多 maxSteps 步
FOUNDATION_EXPORT IRScrollStrategyName const IRScrollStrategySwipeTap;
/// 内置策略的名字 (类和参数见 IRScrollBuiltinStrategies.h; 名字对应默认参数的实例)
FOUNDATION_EXPORT IRScrollStrategyName const IRScrollStrategyDrag;         ///< 拖动: ≤ 150 拖一次, 超过拆段, 段间停顿
FOUNDATION_EXPORT IRScrollStrategyName const IRScrollStrategySmallSteps;   ///< 小步快跑: 拆成几段滑-点, 段间停顿
FOUNDATION_EXPORT IRScrollStrategyName const IRScrollStrategyFastStep;     ///< 快速步进: 惯性自然停, 明显过头才点停
FOUNDATION_EXPORT IRScrollStrategyName const IRScrollStrategyFling;        ///< 连续快甩: 只等松手连甩, 最后一下滑-点
FOUNDATION_EXPORT IRScrollStrategyName const IRScrollStrategyOvershoot;    ///< 过头回看: 多走 10~30%, 停一下再拉回
FOUNDATION_EXPORT IRScrollStrategyName const IRScrollStrategyAuto;         ///< 自动: 按距离筛出适用的, 按权重随机挑一个
FOUNDATION_EXPORT IRScrollStrategyName const IRScrollStrategyCorrect;      ///< 纠正 (滚动操作的 correctionStrategy 默认): 差 ≤ 150 拖动, 否则滑-点

@protocol IRScrollStrategy <NSObject>
/// 下一步; nil = 不再滚 (结束时停在目标区间里算成功)
- (nullable IRScrollStep *)nextStepWithContext:(IRScrollContext *)context;
@end

/// 造一个策略实例 (每次滚动操作调一次)
typedef id<IRScrollStrategy> _Nonnull (^IRScrollStrategyFactory)(void);

NS_ASSUME_NONNULL_END
