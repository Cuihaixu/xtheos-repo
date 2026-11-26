//
//  IRResponderChain.h
//  IRAutomationKit
//
//  一次 hitTest 的结果: 命中的视图沿 nextResponder 到 UIApplication 的整条链 (正式业务接口)。
//  链的顺序就是层级: 越靠前 (index 越小) 越在上层
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRResponderNode.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRResponderChain : NSObject

/// 全部节点: 命中的视图 → UIApplication
@property (nonatomic, copy, readonly) NSArray<IRResponderNode *> *nodes;

/// 链上所有 VC (叶子 → 根)
@property (nonatomic, copy, readonly) NSArray<IRResponderNode *> *viewControllers;

/// 叶子侧第一个类名为 className 的节点
- (nullable IRResponderNode *)firstNodeOfClass:(NSString *)className;

/// 所有类名为 className 的节点 (叶子 → 根)
- (NSArray<IRResponderNode *> *)nodesOfClass:(NSString *)className;

#pragma mark 语法糖 (判断条件常连着写, 用 block 风格)

/// 链上有类名为 className 的节点:  chain.contains(@"AWELoginWindow")
@property (nonatomic, readonly) BOOL (^contains)(NSString *className);

@end

NS_ASSUME_NONNULL_END
