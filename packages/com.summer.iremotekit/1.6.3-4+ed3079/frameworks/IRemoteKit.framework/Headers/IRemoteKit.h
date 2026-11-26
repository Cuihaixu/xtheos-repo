//
//  IRemoteKit.h
//  IRemoteKit
//
//  Created by cu on 2026/9/14.
//

#import <Foundation/Foundation.h>

//! Project version number for IRemoteKit.
FOUNDATION_EXPORT double IRemoteKitVersionNumber;

//! Project version string for IRemoteKit.
FOUNDATION_EXPORT const unsigned char IRemoteKitVersionString[];

// —— 公开接口(L0 会话 → L1 对象/调用 → L2 hook/观察)——
#import <IRemoteKit/IRTypes.h>          // 类型 / 枚举 / 错误码
#import <IRemoteKit/IRSessionManager.h> // 入口:单例 + 全局缓存 + 会话注册表/工厂
#import <IRemoteKit/IRSession.h>        // 会话:目标绑定资源 + 安全 + 派发 worker
#import <IRemoteKit/IRException.h>       // 远程调用异常 + runWorker:onException:onFinally:(失败即抛,通知失败点)
#import <IRemoteKit/IRRemoteModule.h>    // 目标已加载镜像句柄(base/slide/path)+ findRemoteModule:(懒加载缓存)
#import <IRemoteKit/IRWorker.h>         // 一条远程工作线程(执行者;串行,多条=并发)
#import <IRemoteKit/IRCPUContext.h>     // ARM64 CPU 寄存器上下文快照 + IRRegister 点语法视图
#import <IRemoteKit/IRThread.h>         // 三层远程线程体系(IRThread → IRRemoteThread → IRWorkerThread) + IRStackFrame
#import <IRemoteKit/IRExecutionStrategy.h> // 执行策略([IRExecutionStrategy exceptionWait]/spinPoll)
#import <IRemoteKit/IRWorkerQueue.h>    // 远程调用队列(串行 / 并发调度层)
#import <IRemoteKit/IRObject.h>    // 远程对象/类句柄 + 链式调用 + .remote 门面
#import <IRemoteKit/IRMutableArray.h> // __NSArrayM / __NSFrozenArrayM:纯内存 count
#import <IRemoteKit/IRCallOptions.h> // 单次调用可扩展配置(mainThread…);call:arguments:options: 用
#import <IRemoteKit/IRRemotable.h> // .remote 句柄线程糖 .main/.child(协议 + NSObject 分类声明)
#import <IRemoteKit/IRValue.h>     // 带类型的值装箱(标量/浮点/结构体/对象;调用参/返载体)
#import <IRemoteKit/IRIvar.h>      // 远程对象 ivar 访问器(元数据 + value 读写)
#import <IRemoteKit/IRRemoteBuffer.h> // 远程内存句柄(开辟/读/写/清空,RAII;给 C 函数传指针参)
#import <IRemoteKit/IRRemoteMethod.h>     // 远程可调用体(IRRemoteOCMethod / IRRemoteCFunction + 名字注册)
#import <IRemoteKit/IRRemoteInvocation.h> // 一次远程调用(method + IRValue 参 → 结果),OC/C 统一核心

// —— 工具/能力层(调试 dump / 实例扫描 / Hook / 观察)不在默认伞 ——
//    按需显式引:#import <IRemoteKit/IRToolkit.h>(模块 IRemoteKit.Toolkit)。
//    含:IRDebugOptions / IRObject+Debug / IRObjectScan / IRHook / IRTrace。


