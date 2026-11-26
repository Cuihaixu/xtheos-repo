//
//  IRAgent.h
//  IRAutomationKit
//
//  Agent 基类。子类和远程对象的类型一一对应:
//    IRPageAgent ↔ UIViewController
//    IRViewAgent ↔ UIView
//    (以后: IRWindowAgent ↔ UIWindow, IRApplicationAgent ↔ UIApplication)
//
//  - 每种 Agent 有自己的注册分类 (+registerAgent:); 元信息 (meta) 是框架内部实现, 见 IRAgentInternal.h
//  - 注册: 在 +load 里调用 +registerAgent: 注册自己, 进全局注册表, 按 bundleID 归到对应应用
//  - 实例由会话级 Agent 池创建: 同一远程对象 + 同一 Agent 类 = 同一个实例
//

#import <Foundation/Foundation.h>
#import <IRemoteKit/IRObject.h>

NS_ASSUME_NONNULL_BEGIN

@class IRApplication;

@interface IRAgent : NSObject

#pragma mark 实例

/// 所属应用
@property (nonatomic, weak, readonly, nullable) IRApplication *application;

/// 绑定的远程对象 (只在创建它的会话内有效)
@property (nonatomic, readonly, nullable) IRObject *remoteObject;

/// 绑定对象的类名: 注册过的 = 注册的 bindClass; 基础 IRPageAgent / IRViewAgent = 远程对象的类名 (日志用, 可能带 KVO 前缀)
@property (nonatomic, copy, readonly) NSString *bindClassName;

#pragma mark 类型

/// 所属应用 (子类重写): 一般写在每个 App 的 Agent 基类上, 子类继承; 注册时可覆盖。默认空
+ (NSArray<NSString *> *)bundleIDs;

@end

NS_ASSUME_NONNULL_END
