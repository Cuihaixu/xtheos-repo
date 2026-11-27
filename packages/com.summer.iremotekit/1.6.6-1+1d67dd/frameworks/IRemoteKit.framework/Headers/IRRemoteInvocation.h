//
//  IRRemoteInvocation.h — 一次远程调用(L1):方法 + 实参 → 执行 → 结果
//
//  只做一件事:描述"这一次调用"——调哪个函数(`IRRemoteMethod`)、输入什么(`IRValue` 参数)、
//  怎么调(`IRCallOptions`:线程/超时…)。**纯被动值对象,自己不执行**——交给
//  `-[IRWorker invoke:]`(worker 是唯一执行者,按 method 的类型编码把参数按 AAPCS64 分派到
//  x0-x7 / v0-v7 / 栈 / x8,调用,再按返回类型读回 `IRValue`)。
//
//  OC/C 统一:proxy(`.remote`)/ `call:` / C 调用都建一个 invocation 交 worker 执行,是调用的收敛核心。
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRRemoteMethod, IRValue, IRObject;

/// 一次远程调用的描述:**只有"调什么"——可调用体 + 实参**。纯被动值对象,可跨线程/配置复用。
/// "在哪条线程、要不要冻线程"等 per-call 派发配置**不在这里**,由 `-[IRWorker invoke:options:]` 的
/// `IRCallOptions` 决定(what 与 where/how 分离)。
@interface IRRemoteInvocation : NSObject

/// 可调用体(`invocationWithMethod:` 绑定)。
@property (nonatomic, readonly) IRRemoteMethod *method;
/// 接收者:OC 方法这一次调在谁身上(x0);C 函数为 `nil`。属于"这一次调用",不属于 `method` 描述符。
@property (nonatomic, strong, nullable) IRObject *receiver;
/// 实参(顺序对应 `method` 的用户参数;元素为带类型的 `IRValue`)。
@property (nonatomic, copy, nullable) NSArray<IRValue *> *arguments;

/// 用一个可调用体构造(接收者 / 参数随后设)。
+ (instancetype)invocationWithMethod:(IRRemoteMethod *)method;
/// 用可调用体 + 接收者构造(OC 调用首选;参数随后设)。
+ (instancetype)invocationWithMethod:(IRRemoteMethod *)method receiver:(nullable IRObject *)receiver;

@end

NS_ASSUME_NONNULL_END
