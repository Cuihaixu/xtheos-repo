//
//  IRUICollectionView.h
//  IRAutomationKit
//
//  UICollectionView 的远程包装 (SCROLL.md 第 7 项)。下面的方法都是**远程调用** (在目标主线程上执行, 用应用的视图树 worker;
//  任何线程都能调, 主队列 app.queue 除外), 失败时返回 nil / -1 / CGRectNull, 原因写 diag。滚动状态见父类 (纯读内存)
//
//      IRUICollectionView *cv = [IRUICollectionView viewWithObject:list.object];
//      CGRect frame = [cv frameForItemAtIndexPath:[NSIndexPath indexPathForItem:22 inSection:0] diag:diag];   // 内容坐标
//

#import <IRAutomationKit/IRUIScrollView.h>
#import <IRAutomationKit/IRDiagnostics.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRUICollectionView : IRUIScrollView

/// 正在显示的 cell (顺序同 UIKit, 不保证按位置)
- (nullable NSArray<IRUIView *> *)visibleCellsWithDiag:(nullable IRDiagnostics *)diag;
/// 正在显示的 item 的位置 (本地 NSIndexPath, 按 section / item 排序)
- (nullable NSArray<NSIndexPath *> *)indexPathsForVisibleItemsWithDiag:(nullable IRDiagnostics *)diag;
/// 失败返回 -1
- (NSInteger)numberOfSectionsWithDiag:(nullable IRDiagnostics *)diag;
- (NSInteger)numberOfItemsInSection:(NSInteger)section diag:(nullable IRDiagnostics *)diag;
/// 显示着的才有; 没显示返回 nil (不算错误)
- (nullable IRUIView *)cellForItemAtIndexPath:(NSIndexPath *)indexPath diag:(nullable IRDiagnostics *)diag;
/// cell 的位置; 不是这个列表正在显示的 cell 返回 nil (不算错误)
- (nullable NSIndexPath *)indexPathForCell:(IRUIView *)cell diag:(nullable IRDiagnostics *)diag;
/// item 在内容坐标里的位置 (layoutAttributesForItemAtIndexPath: 的 frame): 布局已算出的, 没显示也能拿到;
/// 布局没有 (越界 / 瀑布流还没排到) 返回 CGRectNull (不算错误)
- (CGRect)frameForItemAtIndexPath:(NSIndexPath *)indexPath diag:(nullable IRDiagnostics *)diag;
/// 可见 cell 里中心最接近所在窗口中心的 indexPath; 找不到返回 nil
- (nullable NSIndexPath *)centerIndexPathWithDiag:(nullable IRDiagnostics *)diag;
/// 按 section/item 顺序往前或往后推 offset 步 (跨 section 自动衔接); 越界返回 nil
- (nullable NSIndexPath *)adjacentIndexPath:(NSIndexPath *)indexPath offset:(NSInteger)offset diag:(nullable IRDiagnostics *)diag;

@end

NS_ASSUME_NONNULL_END
