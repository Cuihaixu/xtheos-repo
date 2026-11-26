//
//  IRUITableView.h
//  IRAutomationKit
//
//  UITableView 的远程包装 (SCROLL.md 第 7 项)。下面的方法都是**远程调用** (在目标主线程上执行, 用应用的视图树 worker;
//  任何线程都能调, 主队列 app.queue 除外), 失败时返回 nil / -1 / CGRectNull, 原因写 diag。滚动状态见父类 (纯读内存)
//
//      IRUITableView *tv = [IRUITableView viewWithObject:list.object];
//      CGRect frame = [tv rectForRowAtIndexPath:[NSIndexPath indexPathForRow:22 inSection:0] diag:diag];   // 内容坐标
//

#import <IRAutomationKit/IRUIScrollView.h>
#import <IRAutomationKit/IRDiagnostics.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRUITableView : IRUIScrollView

/// 正在显示的 cell (顺序同 UIKit, 不保证按位置)
- (nullable NSArray<IRUIView *> *)visibleCellsWithDiag:(nullable IRDiagnostics *)diag;
/// 正在显示的行 (本地 NSIndexPath, 按 section / row 排序)
- (nullable NSArray<NSIndexPath *> *)indexPathsForVisibleRowsWithDiag:(nullable IRDiagnostics *)diag;
/// 失败返回 -1
- (NSInteger)numberOfSectionsWithDiag:(nullable IRDiagnostics *)diag;
- (NSInteger)numberOfRowsInSection:(NSInteger)section diag:(nullable IRDiagnostics *)diag;
/// 显示着的才有; 没显示返回 nil (不算错误)
- (nullable IRUIView *)cellForRowAtIndexPath:(NSIndexPath *)indexPath diag:(nullable IRDiagnostics *)diag;
/// cell 的位置; 不是这个列表正在显示的 cell 返回 nil (不算错误)
- (nullable NSIndexPath *)indexPathForCell:(IRUIView *)cell diag:(nullable IRDiagnostics *)diag;
/// 行在内容坐标里的位置 (rectForRowAtIndexPath:): 没显示也能拿到;
/// 越界返回 CGRectNull (不算错误)
- (CGRect)rectForRowAtIndexPath:(NSIndexPath *)indexPath diag:(nullable IRDiagnostics *)diag;

@end

NS_ASSUME_NONNULL_END
