//
//  IRScrollContext.h
//  IRAutomationKit
//
//  滚动操作给策略的上下文 (SCROLL.md 第 40、43 项): 列表视图 + 执行过的所有步骤 (结果都在里面) + userInfo。
//  最新情况从视图上实时读 (offset / remaining / reached / 能滚的范围), steps 只是历史。
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <IRAutomationKit/IRUIScrollView.h>
#import <IRAutomationKit/IRDiagnostics.h>
#import <IRAutomationKit/IRScrollStep.h>

typedef NSString *IRScrollStrategyName NS_TYPED_EXTENSIBLE_ENUM;

NS_ASSUME_NONNULL_BEGIN

@interface IRScrollContext : NSObject

/// 列表视图
@property (nonatomic, strong, readonly) IRUIScrollView *scrollView;
@property (nonatomic, strong, readonly) IRDiagnostics *diag;

#pragma mark 目标 (开始时定好)

@property (nonatomic, readonly) CGFloat startOffset;
/// 目标偏移 = startOffset + 方向 × distance; 停在 targetOffset ± tolerance 里算成功
@property (nonatomic, readonly) CGFloat targetOffset;
@property (nonatomic, readonly) CGFloat tolerance;
/// 纠正策略的名字 (= 滚动操作的 correctionStrategy; 内置策略主体走完交给它)
@property (nonatomic, copy, readonly) IRScrollStrategyName correctionStrategy;
/// 内置策略最多走几步手势 (含第一次; = 滚动操作的 maxScrolls)。滚动操作本身不限步数, 自定义策略看 steps 自己判断
@property (nonatomic, readonly) NSUInteger maxSteps;

#pragma mark 最新情况 (每次读都实时读视图)

/// 现在的偏移
@property (nonatomic, readonly) CGFloat offset;
/// 离目标还有多远 (targetOffset − offset, 有正负: 正 = 还要往前)
@property (nonatomic, readonly) CGFloat remaining;
/// 现在停在目标区间里
@property (nonatomic, readonly) BOOL reached;
/// 能滚的范围 (扣掉滚动操作的 topMargin / bottomMargin; 内容变高了跟着变)
@property (nonatomic, readonly) CGFloat minOffset;
@property (nonatomic, readonly) CGFloat maxOffset;
/// 列表可视高度 (pt)
@property (nonatomic, readonly) CGFloat viewportHeight;

#pragma mark 历史与状态

/// 执行过的步骤 (就是策略返回过的那些对象, 结果已回填), 按执行顺序
@property (nonatomic, copy, readonly) NSArray<IRScrollStep *> *steps;
/// 其中发了手势的有几步 (拖动 / 回放)
@property (nonatomic, readonly) NSUInteger gestureCount;
/// 策略自己的状态; 滚动操作传入了 userInfo 就是那一个 (贯穿多次操作), 否则每次操作新建
@property (nonatomic, strong, readonly) NSMutableDictionary *userInfo;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
