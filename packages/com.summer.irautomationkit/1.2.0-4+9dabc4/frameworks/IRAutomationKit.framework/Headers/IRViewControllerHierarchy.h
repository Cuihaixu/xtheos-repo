//
//  IRViewControllerHierarchy.h
//  IRAutomationKit
//
//  pvc: VC 层级树。由 -[UIViewController _printHierarchy] 的原始输出解析而来 (根 VC 一般是 keyWindow.rootViewController)。
//  原始输出每行一个 VC, 前缀里每个 "   | " 是一层:
//
//      <AWENormalModeTabBarController 0x14636ae00>, state: appeared, view: <UILayoutContainerView: 0x15e6d6220>
//         | <AWEBaseRootNavigationController 0x145131800>, state: appeared, view: <UILayoutContainerView: 0x15e6dc6e0>
//         |    | <AWEFeedRootViewController 0x15e6daa80>, state: appeared, view: <UIView: 0x144d98c70>
//

#import <Foundation/Foundation.h>
#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

/// 层级树上的一个 VC (原始输出的一行)
@interface IRViewControllerNode : NSObject

/// 类名
@property (nonatomic, copy, readonly) NSString *className;

/// 远程地址
@property (nonatomic, readonly) IRRemoteAddress address;

/// 层级深度: 0 = 根
@property (nonatomic, readonly) NSUInteger depth;

/// 打印顺序 (从 0 开始; 父节点总在子节点前面)
@property (nonatomic, readonly) NSUInteger index;

/// 生命周期状态, 原样保留 (如 appeared / disappeared)
@property (nonatomic, copy, readonly) NSString *state;

/// view 的类名 / 地址; 原始输出里没有 (如 view 未加载) 为 nil / 0
@property (nonatomic, copy, readonly, nullable) NSString *viewClassName;
@property (nonatomic, readonly) IRRemoteAddress viewAddress;

/// 父节点 (上方最近一个深度小一层的); 根为 nil
@property (nonatomic, weak, readonly, nullable) IRViewControllerNode *parent;

/// 子节点 (按打印顺序)
@property (nonatomic, copy, readonly) NSArray<IRViewControllerNode *> *children;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

@interface IRViewControllerHierarchy : NSObject

/// 解析 _printHierarchy 的原始输出; 一行都解析不出来返回 nil
+ (nullable instancetype)hierarchyWithString:(NSString *)string;

/// 原始输出
@property (nonatomic, copy, readonly) NSString *string;

/// 根节点
@property (nonatomic, readonly) IRViewControllerNode *root;

/// 全部节点 (打印顺序)
@property (nonatomic, copy, readonly) NSArray<IRViewControllerNode *> *nodes;

/// 第一个类名为 className 的节点 (打印顺序)
- (nullable IRViewControllerNode *)firstNodeOfClass:(NSString *)className;

/// 所有类名为 className 的节点 (打印顺序)
- (NSArray<IRViewControllerNode *> *)nodesOfClass:(NSString *)className;

#pragma mark 语法糖

/// 有 VC 正在出现 / 消失 (state 为 appearing / disappearing): 容器正在切换子控制器
/// (系统的 push / present, 以及按规范调用 beginAppearanceTransition 的自定义容器, 如抖音首页左右滑动)
@property (nonatomic, readonly, getter=isTransitioning) BOOL transitioning;

/// 树里有类名为 className 的 VC:  pvc.contains(@"AWESearchSugViewController")
@property (nonatomic, readonly) BOOL (^contains)(NSString *className);

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
