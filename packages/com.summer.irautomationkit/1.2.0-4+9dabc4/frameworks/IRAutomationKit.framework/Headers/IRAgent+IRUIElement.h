//
//  IRAgent+IRUIElement.h
//  IRAutomationKit
//
//  Agent → 元素 / 节点 (OPERATION.md 3.6): Agent 长期存在 (Agent 池按地址缓存), 节点跟着快照走, 所以 Agent 不持有节点。
//  视图 / 窗口 Agent 对应自己的视图, 页面 Agent 对应控制器的 view, 应用 Agent 对应 keyWindow。
//
//      - (BOOL)handleInterrupt:(IRInterruptContext *)context {        // 弹窗
//          self.query.texts(@"关闭").element.tap(diag);               // 这时才拍 (只拍弹窗的子树)
//          return YES;
//      }
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRAgent.h>
#import <IRAutomationKit/IRDiagnostics.h>

NS_ASSUME_NONNULL_BEGIN

@class IRUINode;
@class IRUIElement;
@class IRUIElementQuery;

@interface IRAgent (IRUIElement)

/// 这个 Agent 的元素 (表达式, UI 对象是 Agent 的远程对象): 不拍; 每次执行都现拍它的子树 (换算到视图)
@property (nonatomic, readonly) IRUIElement *element;

/// = element.query (入口简写)
@property (nonatomic, readonly) IRUIElementQuery *query;

/// 现拍以自己的视图为根的树, 返回树根; 远程对象已不在 / 拍不到为 nil, 原因写进 diag
@property (nonatomic, readonly) IRUINode *_Nullable (^node)(IRDiagnostics *_Nullable diag);

@end

NS_ASSUME_NONNULL_END
