//
//  IRHook.h — 远程 Hook(L2)
//
//  三种,全程无注入(hook 的回调逻辑跑在宿主侧,不进目标):
//   1. 常量返回交换(swizbool):IMP 换成目标里现成 mov w0,#imm;ret gadget。改一次、零开销、可还原。
//   2. 守卫页 hook:IMP→只读守卫页 → App 调它取指 fault → 我方线程级异常端口接管 → handler 宿主侧跑 → 调原函数续跑。
//   3. 硬件断点:对裸函数/指令地址下 ARM_DEBUG_STATE64 断点,命中回调(可读/改寄存器)。
//  IRHook 对象即"已安装的 hook",- remove 还原(IMP / 断点 / 异常端口)。
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRSession;
@class IRObject;

/// 一个已安装的远程 Hook(常量返回 / 守卫页 / 硬件断点);`-remove` 还原。
@interface IRHook : NSObject

#pragma mark - 状态

/// hook 是否仍生效(未 remove、会话仍 Open)。
@property (nonatomic, readonly, getter=isActive) BOOL active;

#pragma mark - 安装

/// 常量返回:永久令 `[className sel]` 返回 `value`(适合返回 BOOL/整数的检测方法)。零开销。
///
/// IMP 换成目标里现成的 `mov w0,#imm; ret` gadget,不跑我方逻辑。
/// - Parameters:
///   - className: 类名
///   - sel: 方法选择子名
///   - value: 恒定返回值(取 x0 低位)
///   - session: 目标会话
///   - error: 失败时置错误
/// - Returns: 已安装的 hook;失败 `nil`
+ (nullable IRHook *)constantReturnForClass:(NSString *)className
                                   selector:(NSString *)sel
                                      value:(int64_t)value
                                  inSession:(IRSession *)session
                                      error:(NSError **)error;

/// 守卫页 hook:App 每次调 `[className sel]` → `handler(recv, args)` 在宿主侧执行 → 之后调原函数续跑。
/// - Parameters:
///   - className: 类名
///   - sel: 方法选择子名
///   - session: 目标会话
///   - handler: 命中回调(宿主侧),`recv` 为接收者、`args` 为参数句柄
///   - error: 失败时置错误
/// - Returns: 已安装的 hook;失败 `nil`
/// - Warning: 冷/中频方法适用;每命中一次跨进程往返 + 目标线程 halt,handler 要轻
+ (nullable IRHook *)hookClass:(NSString *)className
                      selector:(NSString *)sel
                     inSession:(IRSession *)session
                       handler:(void (^)(IRObject *recv, NSArray<IRObject *> *args))handler
                         error:(NSError **)error;

/// 硬件断点:命中 `address` 时回调。
/// - Parameters:
///   - address: 目标指令/函数地址
///   - session: 目标会话
///   - handler: 命中回调,`registers` 为命中瞬间的 x0..x30
///   - error: 失败时置错误
/// - Returns: 已安装的 hook;失败 `nil`
/// - Warning: 硬件断点每线程仅 6 个,且可被目标读 `ARM_DEBUG_STATE64` 自查
+ (nullable IRHook *)hardwareBreakpointAtAddress:(IRRemoteAddress)address
                                       inSession:(IRSession *)session
                                         handler:(void (^)(const uint64_t registers[31]))handler
                                           error:(NSError **)error;

#pragma mark - 收尾

/// 还原(swizbool/守卫页 → 复位 IMP;硬件断点 → 清断点),并清我方异常端口。幂等。
- (void)remove;

@end

NS_ASSUME_NONNULL_END
