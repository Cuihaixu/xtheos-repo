//
//  IRListScroller.h
//  IRAutomationKit
//
//  照 UIKit 滚动 API 的列表滚动 (SCROLL.md 第 49 项): 接口像 scrollToRowAtIndexPath:atScrollPosition: / scrollRectToVisible: /
//  setContentOffset:, 实现是模拟触摸 (滚动操作 IRScrollOperation: 策略 / 按录制拖动 / 点停)。
//
//  流程: ① 开始前 (列表静止) 算一次目标偏移 (表格 rectForRowAtIndexPath: / 集合视图 frameForItemAtIndexPath:, 布局数据, 和 cell 重用无关);
//  ② 滚动中只比坐标 (只读偏移, 纯内存, 不调列表的远程方法); ③ 停稳后核对一次, 差得比 tolerance 多才补 (小段拖动), 最多 maxRounds 轮;
//  ④ 目标还没布局出来或远于 1.5 屏: 先往那边滚 1.5 屏再算 (不计入 maxRounds, 最多 8 次; 读不到位置时按 indexPath 比可见的定方向)。
//  布局给不出位置 (瀑布流等自定义布局可能只给附近的) 但 cell 在屏幕上时, 用可见 cell 的位置。
//  只支持 UITableView / UICollectionView。同步执行, 在持有执行权的自动化线程上调 (写操作)。一个对象可以连续调多次, 结果属性是最近一次的。
//  diag 只在创建时给一个: 每个方法返回 BOOL, 失败原因写进 diag (同时在 failureReason); 失败后要不要 diag.clearError() 由调用方决定
//  (diag 里已经有错误时, 后面的失败记成警告)
//
//      IRListScroller *scroller = listElement.scroller(diag);
//      [scroller scrollToIndexPath:ip rect:mediaRectInCell atScreenY:368];   // 把 cell 里的媒体区中心滚到屏幕中心
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <IRAutomationKit/IRDiagnostics.h>
#import <IRAutomationKit/IRUIScrollView.h>
#import <IRAutomationKit/IRScrollStrategy.h>
#import <IRAutomationKit/IRScrollStep.h>

NS_ASSUME_NONNULL_BEGIN

/// 对齐到列表可见区域的哪里 (同 UITableViewScrollPosition)
typedef NS_ENUM(NSInteger, IRListPosition) {
    IRListPositionNone = 0,   ///< 刚好露全: 已经整块可见就不动, 在上面就对齐顶部, 在下面就对齐底部
    IRListPositionTop,        ///< 块的顶边对齐可见区域顶边
    IRListPositionMiddle,     ///< 块的中心对齐可见区域中心
    IRListPositionBottom,     ///< 块的底边对齐可见区域底边
};

@interface IRListScroller : NSObject

/// scrollView 要是 UITableView / UICollectionView (或子类), 不是返回 nil, 原因写 diag
+ (nullable instancetype)scrollerWithScrollView:(IRUIScrollView *)scrollView diag:(nullable IRDiagnostics *)diag;
@property (nonatomic, strong, readonly) IRUIScrollView *scrollView;
@property (nonatomic, strong, readonly) IRDiagnostics *diag;

#pragma mark 参数

/// 允许偏差 (pt): 停稳后离目标差得比它多才补; 默认 20
@property (nonatomic) CGFloat tolerance;
/// 主体怎么滚 (第一轮), 默认 IRScrollStrategyAuto; 补用拖动策略 (小段拖动)
@property (nonatomic, copy) IRScrollStrategyName strategy;
/// 最多几轮 (每轮: 算目标 → 滚 → 停稳核对), 默认 3
@property (nonatomic) NSUInteger maxRounds;

#pragma mark 按 indexPath (表格 / 集合视图)

- (BOOL)scrollToIndexPath:(NSIndexPath *)indexPath atPosition:(IRListPosition)position;
/// rect: cell 里的一块 (相对 cell, 如媒体区), CGRectNull = 整个 cell; offset: 目标线再往下挪多少 (pt, 可负)
- (BOOL)scrollToIndexPath:(NSIndexPath *)indexPath rect:(CGRect)rect atPosition:(IRListPosition)position offset:(CGFloat)offset;
/// 把块的中心滚到屏幕上的 screenY (屏幕坐标)
- (BOOL)scrollToIndexPath:(NSIndexPath *)indexPath rect:(CGRect)rect atScreenY:(CGFloat)screenY;

#pragma mark 按内容坐标 / 偏移

/// 内容坐标的一块: 已经整块可见就不动, 否则滚到刚好露全
- (BOOL)scrollRectToVisible:(CGRect)rect;
- (BOOL)scrollToContentOffset:(CGFloat)offset;
/// 从当前位置滚 distance (正 = 往下看)
- (BOOL)scrollBy:(CGFloat)distance;
- (BOOL)scrollToTop;
/// 滚到底 (到底时滚动操作会等加载更多)
- (BOOL)scrollToBottom;

#pragma mark 结果 (最近一次调用)

/// 现在的偏移 / 离目标还差多少 (目标 − 现在, 有正负) / 跑了几轮 (不含远距离先靠近的) / 执行过的所有步骤
@property (nonatomic, readonly) CGFloat offset;
@property (nonatomic, readonly) CGFloat error;
@property (nonatomic, readonly) NSUInteger rounds;
@property (nonatomic, copy, readonly) NSArray<IRScrollStep *> *steps;
/// 目标超出能滚的范围、停在了边上 (如最后几项滚不到正中)
@property (nonatomic, readonly) BOOL clamped;
@property (nonatomic, copy, readonly, nullable) NSString *failureReason;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
