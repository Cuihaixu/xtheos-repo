//
//  IRToolkit.h — IRemoteKit 工具/能力层(opt-in,**不进默认伞** IRemoteKit.h)
//
//  默认 `#import <IRemoteKit/IRemoteKit.h>` 只给**核心原语**:会话 / 对象 / 调用 / 读写内存 / invoke。
//  要用"封装能力层"(调试 dump / 实例扫描 / Hook / 观察),显式引本头:
//
//      #import <IRemoteKit/IRToolkit.h>        // 或模块:@import IRemoteKit.Toolkit;
//
//  这样默认 API 面保持精简,工具层按需开。mpx CLI 的 obj 命令(p/po/pivars/pmethods/
//  pprops/pclass/instances)即基于本层;IRAutomationKit 等消费方要这些能力也显式引本头。
//

#import <IRemoteKit/IRemoteKit.h>        // 先拿核心(对象 / 会话 / 调用)

#import <IRemoteKit/IRDebugOptions.h>    // 调试 dump 配置(source:静态 / runtime;可扩展)
#import <IRemoteKit/IRObject+Debug.h>    // 对象/类调试 dump(p/po/pivars/pmethods/pprops/pclass,对标 Chisel)
#import <IRemoteKit/IRClassReference.h>  // 类引用协议(类名 / 类对象统一输入;多类扫描用)
#import <IRemoteKit/IRObjectScan.h>      // 实例扫描(IRSession 分类:enumerateInstancesOfClass: / instancesOfClasses:)
#import <IRemoteKit/IRHook.h>            // 远程 Hook(常量返回 / 守卫页 / hwbp)
#import <IRemoteKit/IRTrace.h>           // 观察(syscall 流 / 对象释放)
#import <IRemoteKit/IRWatchPoint.h>    // 远程数据读写监控(观察点;Manager/实现后续)
#import <IRemoteKit/IRWatchPointManager.h>  // 观察点总管 + 便捷分类 + IRSession(IRWatch) 入口
