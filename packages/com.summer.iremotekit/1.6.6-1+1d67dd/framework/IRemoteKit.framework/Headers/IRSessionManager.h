//
//  IRSessionManager.h — 会话总管(L0 入口,单例)
//
//  按生命周期分层:Manager 随【宿主进程】常驻,持"宿主全局"数据 + 会话注册表;
//  每个 IRSession 随【目标进程实例】,持目标绑定资源(目标重启即失效)。
//
//  宿主全局(Manager 持):dyld 共享缓存符号地址(objc_msgSend/getClass/… 全目标同址,
//    dlsym 一次常驻)、trace.codes 名表、所有活会话的注册表(信号兜底遍历它 close)。
//  —— 目标 App 自己的类/符号带 ASLR,不进全局,归各 IRSession。
//
//  入口收敛到此:用 sessionForXxx: 建或复用会话(按 pid 去重,一目标一会话一 worker)。
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRSession;

/// 会话总管(单例):宿主全局缓存 + 会话注册表 + 建/复用会话的入口。
@interface IRSessionManager : NSObject

#pragma mark - 单例

/// 全局唯一总管。
@property (class, nonatomic, readonly) IRSessionManager *sharedManager;

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;

#pragma mark - 取会话(建或复用)

/// 按进程名取会话;已存在则复用,否则新建(内部 `sysctl` 解析 pid)。
/// - Parameters:
///   - processName: 目标进程名
///   - error: 失败时置错误(domain `IRErrorDomain`)
/// - Returns: 会话;失败返回 `nil`
- (nullable IRSession *)sessionForProcessName:(NSString *)processName error:(NSError **)error;

/// 按 pid 取会话;已存在则复用,否则新建。
/// - Parameters:
///   - pid: 目标进程 pid
///   - error: 失败时置错误
/// - Returns: 会话;失败返回 `nil`
- (nullable IRSession *)sessionForPID:(pid_t)pid error:(NSError **)error;

/// 只查已存在的会话,不新建。
/// - Parameter pid: 目标进程 pid
/// - Returns: 已注册的会话;无则 `nil`
- (nullable IRSession *)existingSessionForPID:(pid_t)pid;

#pragma mark - 收尾

/// 关闭并注销一个会话(还原其目标异常端口)。
/// - Parameter session: 要关闭的会话
- (void)closeSession:(IRSession *)session;

/// 关闭所有活会话(信号兜底 Ctrl-C/退出时亦走此,保证不留悬空异常端口)。
- (void)closeAll;

@end

NS_ASSUME_NONNULL_END
