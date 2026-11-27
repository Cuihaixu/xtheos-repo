//
//  IRWatchPointManager.h — 观察点注册表 + 唯一异常监听(中央 catch + demux)
//
//  随 IRSession,一目标一套(经 `-[IRSession watchpointManager]` 取,勿直接实例化)。
//  职责:注册/注销观察点;跑【唯一一条】异常监听(EXC_BREAKPOINT + EXC_BAD_ACCESS),
//        catch 后按地址/类型 demux 到对应 IRWatchPoint,再由它派发自己的 onEvent;
//        收尾统一清理(清寄存器 / 复页权限 / 恢复目标异常端口,信号兜底亦走)。
//  —— 多个观察点共享 task 级异常端口,故监听必须集中在 Manager,不能各起一个。
//
//  opt-in:经 IRToolkit.h 引入。
//

#import <Foundation/Foundation.h>
#import <IRemoteKit/IRSession.h>
#import <IRemoteKit/IRWatchPoint.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRWatchPointManager : NSObject

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;

#pragma mark - 注册表

/// 当前所有观察点(含 disable 的;deleteWatchPoint: 后移出)。
@property (nonatomic, readonly) NSArray<IRWatchPoint *> *watchPoints;

/// 武装并注册一个(标准构造 A 的)观察点。所选后端满足不了请求(如守卫页纯读的低效、
/// HW 范围超 8B / 无空闲寄存器)即失败并置 `error`,不偷偷降级。成功后其状态/能力可回读。
- (BOOL)addWatchPoint:(IRWatchPoint *)watchPoint error:(NSError **)error;

/// 停表 + 注销 + 释放一个观察点(永久;对标 lldb `watchpoint delete`)。幂等。
- (void)deleteWatchPoint:(IRWatchPoint *)watchPoint;

/// 停表 + 注销全部(收尾;信号兜底 Ctrl-C/退出亦走此)。
- (void)deleteAllWatchPoints;

#pragma mark - 目标线程访问器(填 IRWatchPoint.threadIds 用,免自己猜)

/// 目标主线程 id(mach thread)。盯主线程:`threadIds:@[mgr.mainThreadId]`。
@property (nonatomic, readonly) NSNumber *mainThreadId;

/// 目标当前所有线程 id(mach thread),挑特定线程用。
@property (nonatomic, readonly) NSArray<NSNumber *> *threadIds;

#pragma mark - 诊断 / 压测

/// 异常处理累计次数(一次逻辑命中 ≈ 2:观察点异常 + 单步异常)。
@property (nonatomic, readonly) uint64_t handleInvocations;
/// 异常处理累计耗时(纳秒);`totalHandleNs / handleInvocations` = 单次均耗,衡量开销量级。
@property (nonatomic, readonly) uint64_t totalHandleNs;

@end

#pragma mark - 便捷快捷接口(分类;核心类保持精简)

@interface IRWatchPointManager (Convenience)

/// B 便捷:一步构造 + 武装 + 注册,返回已武装的观察点(失败 nil + `error`)。
/// 等价于 `IRWatchPoint` 标准构造后 `addWatchPoint:`。
- (nullable IRWatchPoint *)watchAddress:(IRRemoteAddress)address
                                 length:(size_t)length
                                 access:(IRWatchAccess)access
                                backend:(IRWatchBackendKind)backend
                              threadIds:(nullable NSArray<NSNumber *> *)threadIds
                                onEvent:(IRWatchHandler)onEvent
                                  error:(NSError **)error;

@end

#pragma mark - 入口:挂到会话

@interface IRSession (IRWatch)

/// 本会话的观察点总管(懒建,一目标一套;随会话 close 收尾)。
@property (nonatomic, readonly) IRWatchPointManager *watchpointManager;

@end

NS_ASSUME_NONNULL_END
