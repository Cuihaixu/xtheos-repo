//
//  IRApplication+IRUIElement.h
//  IRAutomationKit
//
//  入口 (OPERATION.md 3.6): 应用上取节点 / 元素。拍树是读操作, 任何线程都能调 (主队列 app.queue 上除外)
//
//      IRUIElement *login = app.query.texts(@"登录").element;          // 表达式, 不执行 (app.query = app.element.query)
//      login.tap(diag);                                                // 这时才执行 (第一次使用拍整屏)
//      IRUINode *root = app.node;                                      // 底层: 现拍整屏, 得到树根
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRApplication.h>

NS_ASSUME_NONNULL_BEGIN

@class IRUINode;
@class IRUIElement;
@class IRUIElementQuery;
@class IRResponderChain;

@interface IRApplication (IRUIElement)

/// 应用的元素 (表达式, UI 对象是应用, 换算到 keyWindow): 不拍; 每次执行都现拍整屏 (元素不缓存执行结果)
@property (nonatomic, readonly) IRUIElement *element;

/// 这个应用的调试默认值 (默认 NO): 从它产生的元素 (app.element / app.query…、这个应用的 node.element、agent.element) 创建时继承;
/// 元素上的 debugEnabled 可以再覆盖。总闸 IRUIElement.debugEnabled 关着时都不显示。只影响之后新建的元素
@property (nonatomic) BOOL debugEnabled;

/// = element.query (入口简写)
@property (nonatomic, readonly) IRUIElementQuery *query;

/// 现拍整屏 (以 keyWindow 为根), 返回树根; 拍不到为 nil (原因打日志)
@property (nonatomic, readonly, nullable) IRUINode *node;

/// 最新的响应链 (= currentScreen.chain); 节点树不和链绑定, 用到时从这里取最新的
@property (nonatomic, readonly, nullable) IRResponderChain *chain;

@end

NS_ASSUME_NONNULL_END
