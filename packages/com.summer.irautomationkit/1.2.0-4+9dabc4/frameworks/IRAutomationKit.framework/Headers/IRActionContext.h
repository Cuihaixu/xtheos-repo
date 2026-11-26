//
//  IRActionContext.h
//  IRAutomationKit
//
//  一次动作调用的现场: 由框架创建, 传给动作的执行 block (ACTION.md 第 14 项)
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRDiagnostics.h>

NS_ASSUME_NONNULL_BEGIN

@class IRAgent;
@class IRPageAgent;
@class IRApplication;

/// 调用模式 (ACTION.md 第 32 项): 同一个执行 block, 按模式做不同的事
typedef NS_ENUM(NSInteger, IRActionMode) {
    IRActionModePerform = 0,    ///< 执行 (默认): 做这件事, 也可以带回 result
    IRActionModeQuery,          ///< 查询: 只读数据写进 result 就返回, 不动手 (签名声明了返回值才会用这个模式调)
    IRActionModeAvailable,      ///< 可用性: 只回答能不能做 (返回 YES / NO), 不动手 (签名声明了 available 才会用这个模式调)
};

@interface IRActionContext : NSObject

/// 动作名
@property (nonatomic, copy, readonly) NSString *actionID;

/// 这次调用的模式; 查询 / 可用性模式在只读范围里跑: 写操作 (触摸 / 输入 / 回放) 和等待直接失败, 不会误点
@property (nonatomic, readonly) IRActionMode mode;
/// mode == IRActionModeQuery
@property (nonatomic, readonly) BOOL isQuery;
/// mode == IRActionModeAvailable
@property (nonatomic, readonly) BOOL isAvailable;

/// 动作说明 (注册时 IRIdentityMake 的第二个参数); 没写为 nil
@property (nonatomic, copy, readonly, nullable) NSString *comment;

/// 执行这个动作的 Agent = 接住这个事件的 Agent (被调用 invokeAction 的那个; 页面节点沿链交给子控制器时是子控制器), 保证:
/// - 一定是注册这个动作的那一种: 它的类就是注册这个动作的类 (或其子类, 动作沿继承链找到)
/// - 整个执行过程中不变: 即使执行到一半当前页面已经变了 (如点了评论之后), 这里仍然是它
@property (nonatomic, readonly) __kindof IRAgent *agent;

/// 转发的页面: 页面节点 (IRPageNode) 检查过的当前页面; 调页面自己的事件时 page == agent, 交给链上子控制器时是外面的页面
/// (ACTION.md 第 24 项)。直接 [agent invokeAction:] 时: Agent 是页面 (有 pageID) 则是它自己, 否则 nil; 事件里嵌套调用沿用外层的。
/// 子控制器的事件等页面变化用它: app.waitPageChange(context.page, 5, diag)
@property (nonatomic, readonly, nullable) IRPageAgent *page;

/// 所属应用: 点击、输入、等待、找控件等操作都挂在它上面 (第 17 项; 操作接口另定)
@property (nonatomic, readonly) IRApplication *app;

/// 调用时传入的参数
@property (nonatomic, copy, readonly) NSDictionary<NSString *, id> *params;

/// 取必需参数: 取不到就在 self.diag 上记错误 (相当于声明这个动作依赖它) 并返回 nil
@property (nonatomic, copy, readonly) id _Nullable (^getParam)(NSString *name);

/// 取可选参数: 取不到 (或为 NSNull) 返回默认值
@property (nonatomic, copy, readonly) id _Nullable (^findParam)(NSString *name, id _Nullable defaultValue);

/// 返回值: 动作要返回数据就写这里, 调用方从 invokeAction 的 result 出参拿到 (第 10 项)
@property (nonatomic, strong, nullable) id result;

/// 这次调用的诊断 = 调用方传入的那个 (传 nil 时框架临时建的); 动作里的 app 操作也传它 (第 16、20 项)
@property (nonatomic, readonly) IRDiagnostics *diag;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
