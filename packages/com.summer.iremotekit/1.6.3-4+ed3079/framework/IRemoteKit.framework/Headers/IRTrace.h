//
//  IRTrace.h — 观察 / 追踪(L2),零接触、无 hook
//
//  1. syscall 流(kdebug):订阅目标 pid 的 syscall + VFS_LOOKUP 路径,近实时推送。
//  2. 对象释放(deathwatch):关联 Mach 端口,目标对象 dealloc 时内核推 DEAD_NAME 回调。
//  都是被动可观测——只能看,不能改(要改走 IRHook)。
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRSession;
@class IRObject;

/// 一条 syscall / 内核事件。
@interface IRSyscallEvent : NSObject
/// 线程 id。
@property (nonatomic, readonly) uint64_t threadID;
/// 事件名(`BSC_open` / `MACH_SCHED` …,trace.codes 全量)。
@property (nonatomic, readonly) NSString *name;
/// 前 4 个原始参数(寄存器值)。
@property (nonatomic, readonly) uint64_t argument0;
@property (nonatomic, readonly) uint64_t argument1;
@property (nonatomic, readonly) uint64_t argument2;
@property (nonatomic, readonly) uint64_t argument3;
/// 关联到的 `VFS_LOOKUP` 路径(open/stat 等);无则 `nil`。
@property (nonatomic, readonly, nullable) NSString *path;
@end

/// 一次观察 / 追踪(syscall 流 / 对象释放),被动只读;`-stop` 停止。
@interface IRTrace : NSObject

#pragma mark - 状态

/// 是否正在订阅。
@property (nonatomic, readonly, getter=isRunning) BOOL running;

#pragma mark - 订阅

/// 订阅目标 syscall 流。`handler` 在后台读取线程回调每条事件。
/// - Parameters:
///   - session: 目标会话
///   - allEvents: `YES` 含调度/缺页/IPC 等全类事件;`NO` 只 BSD syscall
///   - handler: 每条事件的回调
///   - error: 失败时置错误
/// - Returns: 追踪句柄;失败 `nil`
/// - Warning: 需 root;kdebug 为系统级设施,全局单占用(与 Instruments 等冲突)
+ (nullable IRTrace *)syscallsInSession:(IRSession *)session
                              allEvents:(BOOL)allEvents
                                handler:(void (^)(IRSyscallEvent *event))handler
                                  error:(NSError **)error;

/// 监控某远程对象释放:目标对象 dealloc → `handler` 触发一次(之后失效)。
/// - Parameters:
///   - object: 被监控的远程对象
///   - handler: 释放时回调
///   - error: 失败时置错误
/// - Returns: 追踪句柄;失败 `nil`
+ (nullable IRTrace *)watchDeallocOfObject:(IRObject *)object
                                   handler:(void (^)(void))handler
                                     error:(NSError **)error;

#pragma mark - 收尾

/// 停止订阅 / 拆监控。幂等。
- (void)stop;

@end

NS_ASSUME_NONNULL_END
