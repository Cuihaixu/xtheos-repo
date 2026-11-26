//
//  IRChainContext.h
//  IRAutomationKit
//
//  识别时的现场。由框架创建并传入协议方法 (IRPageResolving / IRInterruptHandling), 不能自己创建。
//  每轮扫描有两种模式:
//    - hitTest (键盘没打开): 有响应链 chain 和当前节点 node, 判断用链和节点上的语法糖
//          context.node.under(@"AWELoginWindow") && !context.chain.contains(@"AWESearchView")
//    - pvc (键盘打开时自动切换: hitTest 会让部分页面收起键盘): 用 VC 层级树判断
//          context.pvc.contains(@"AWESearchSugViewController")
//      这时 chain 是空链、node 是空节点 (不是 nil): 链和节点上的语法糖一律不满足, 可以放心调用不会崩
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRResponderChain.h>
#import <IRAutomationKit/IRViewControllerHierarchy.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRChainContext : NSObject

/// 键盘打开 (本轮走 pvc)
@property (nonatomic, readonly, getter=isKeyboardVisible) BOOL keyboardVisible;

/// 本轮的响应链 (叶子 → 根); pvc 模式为空链 (没有节点)
@property (nonatomic, readonly) IRResponderChain *chain;

/// 当前节点 (页面判断: 这个控制器; 中断判断: 这个 Agent 绑定的对象);
/// pvc 模式为空节点 (类名为空、地址 0, under / above / front 都不满足, parentViewController 为 nil)
@property (nonatomic, readonly) IRResponderNode *node;

/// 本轮的 VC 层级树; hitTest 模式为 nil
@property (nonatomic, readonly, nullable) IRViewControllerHierarchy *pvc;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
