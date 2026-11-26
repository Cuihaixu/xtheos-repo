//
//  IRUIScrollView.h
//  IRAutomationKit
//
//  UIScrollView 的远程包装 (SCROLL.md 第 3、7 项): 滚动状态全部纯读内存 (ivar), 任何线程、每帧都能读。
//  iOS 16.7 上验证过 (mpa scrolltrace): 偏移 / 惯性终点 / 惯性中 / 手指按着; 其它版本上 ivar 不存在时对应属性读不到 (readMotion 返回 NO)
//
//      IRUIScrollView *list = [IRUIScrollView viewWithObject:object];
//      IRScrollMotion motion;
//      if ([list readMotion:&motion] && motion.decelerating) { … motion.decelerationTarget … }
//

#import <CoreGraphics/CoreGraphics.h>
#import <UIKit/UIKit.h>
#import <IRAutomationKit/IRUIView.h>

NS_ASSUME_NONNULL_BEGIN

/// 一帧的滚动状态 (一次读内存拿全)
typedef struct {
    CGPoint contentOffset;        ///< 当前偏移 (_lastSetContentOffsetUnrounded)
    CGPoint velocity;             ///< 速度 pt/ms (x = _horizontalVelocity, y = _verticalVelocity); 正 = 偏移变大
    CGPoint decelerationTarget;   ///< 惯性会停在哪 (_adjustedDecelerationTarget, 松手时 UIKit 算好); 只在 decelerating 时有意义
    BOOL tracking;                ///< 手指按着 (_scrollViewFlags 最低位)
    BOOL decelerating;            ///< 惯性滚动 / 滚动动画中 (_scrollHeartbeat 不为 nil)
} IRScrollMotion;

@interface IRUIScrollView : IRUIView

/// 一次读完一帧的滚动状态; 读不到 (对象没了 / 这个系统版本没有对应 ivar) 返回 NO
- (BOOL)readMotion:(IRScrollMotion *)motion;

// 单项读取 (各读一次内存; 读不到为 0 / NO)
@property (nonatomic, readonly) CGPoint contentOffset;
@property (nonatomic, readonly) CGSize contentSize;
@property (nonatomic, readonly) UIEdgeInsets contentInset;
@property (nonatomic, readonly) CGPoint velocity;
@property (nonatomic, readonly) CGPoint decelerationTarget;
@property (nonatomic, readonly, getter=isTracking) BOOL tracking;
@property (nonatomic, readonly, getter=isDecelerating) BOOL decelerating;
/// 减速率: 惯性每毫秒保留的速度比例 (_decelerationFactor; 普通 0.998, fast 0.99); 读不到为 0
@property (nonatomic, readonly) CGSize decelerationFactor;
/// 在滚: 手指按着或惯性中
@property (nonatomic, readonly, getter=isScrolling) BOOL scrolling;

/// adjustedContentInset (contentInset + 安全区等系统加的; 能滚的范围 = [-top, contentSize - bounds + bottom])。远程调用, 失败返回 NO
- (BOOL)readAdjustedContentInset:(UIEdgeInsets *)inset diag:(nullable IRDiagnostics *)diag;
/// 等列表停稳 (没按着、不在惯性 / 滚动动画中, 且偏移连续 3 次间隔 50ms 变化 < 0.3pt); 超时返回 NO
- (BOOL)waitStillWithTimeout:(NSTimeInterval)timeout;

@end

NS_ASSUME_NONNULL_END
