//
//  IRResponderNode.h
//  IRAutomationKit
//
//  响应链上的一个节点 (正式业务接口: 处理弹窗时可直接从链上取遮罩、按钮等节点)
//

#import <Foundation/Foundation.h>
#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRAgent;

/// 节点类型 (按远程对象的类继承关系判断, 纯内存读, 不远程调用); 决定没注册时用哪种 Agent 兜底
typedef NS_ENUM(NSInteger, IRResponderNodeKind) {
    IRResponderNodeKindOther = 0,         ///< 其它 responder (如 UIWindowScene、AppDelegate) → IRResponderAgent
    IRResponderNodeKindView,              ///< UIView → IRViewAgent
    IRResponderNodeKindViewController,    ///< UIViewController → IRPageAgent
    IRResponderNodeKindWindow,            ///< UIWindow → IRWindowAgent
    IRResponderNodeKindApplication,       ///< UIApplication → IRApplicationAgent
};

@interface IRResponderNode : NSObject

/// 类名 (已去掉 KVO 等前缀)
@property (nonatomic, copy, readonly) NSString *className;

/// 远程地址
@property (nonatomic, readonly) IRRemoteAddress address;

/// 是否为视图控制器
@property (nonatomic, readonly, getter=isViewController) BOOL viewController;

/// 节点类型
@property (nonatomic, readonly) IRResponderNodeKind kind;

/// 在链上的位置: 0 = 叶子 (hitTest 命中的视图), 越大越靠根
@property (nonatomic, readonly) NSUInteger index;

/// 这个节点对应的 Agent (走会话的 Agent 池), 映射是确定的:
///   这个类名在本应用注册过 → 注册的 Agent 类; 没有 → 按节点类型: VC → IRPageAgent, 视图 → IRViewAgent
/// 一个节点只对应一个 Agent: 和感知结果的 pageAgent / interruptAgent、中断的 ctx.agent 指向同一节点时是同一个实例。
/// 每次取都会校验对象仍存活且类型未变 (防地址复用); 校验失败 / 会话已失效为 nil
@property (nonatomic, readonly, nullable) __kindof IRAgent *agent;

#pragma mark 语法糖 (都相对这个节点; 判断条件常连着写, 用 block 风格)

/// 是叶子侧第一个 VC (最上层的控制器)
@property (nonatomic, readonly, getter=isFront) BOOL front;

/// 根侧 (更底层, index 更大) 有类名为 className 的节点:  node.under(@"AWELoginWindow")
@property (nonatomic, readonly) BOOL (^under)(NSString *className);

/// 叶子侧 (更上层, index 更小) 有类名为 className 的节点
@property (nonatomic, readonly) BOOL (^above)(NSString *className);

/// 根侧第一个 VC (外层容器); 没有为 nil
@property (nonatomic, readonly, nullable) IRResponderNode *parentViewController;

@end

NS_ASSUME_NONNULL_END
