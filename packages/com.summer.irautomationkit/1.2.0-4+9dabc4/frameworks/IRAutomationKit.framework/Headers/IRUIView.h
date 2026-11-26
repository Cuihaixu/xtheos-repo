//
//  IRUIView.h
//  IRAutomationKit
//
//  UIView 的远程包装 (SCROLL.md 第 5-7 项)。frame / hidden / alpha 在 CALayer 里, 不是 ivar, 位置和可见性用视图树 (IRUINode);
//  这里只放能纯读内存拿到的
//

#import <IRAutomationKit/IRNSObject.h>
#import <IRAutomationKit/IRDiagnostics.h>
#import <CoreGraphics/CoreGraphics.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRUIView : IRNSObject

/// 同 +objectWithObject: (不是 UIView 或子类 → nil)
+ (nullable instancetype)viewWithObject:(nullable IRObject *)object;

/// 所在窗口 (_window); 不在窗口上为 nil。纯读内存
@property (nonatomic, readonly, nullable) IRUIView *window;
/// 管理它的控制器 (_viewDelegate, 只有控制器的根视图有); 纯读内存
@property (nonatomic, readonly, nullable) IRNSObject *viewController;

// 远程调用 (目标主线程, 用应用的视图树 worker; 任何线程都能调, app.queue 除外); 失败返回 CGRectNull, 原因写 diag

/// bounds (UIScrollView 的 bounds.origin 就是 contentOffset)
- (CGRect)boundsWithDiag:(nullable IRDiagnostics *)diag;
/// 在窗口里的位置 ([self convertRect:self.bounds toView:nil]): 可视区域在屏幕上的位置 (窗口全屏时 = 屏幕坐标)
- (CGRect)frameInWindowWithDiag:(nullable IRDiagnostics *)diag;

@end

NS_ASSUME_NONNULL_END
