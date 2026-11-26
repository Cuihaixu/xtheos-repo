//
//  IRTypes.h — IRemoteKit 公共类型 / 枚举 / 错误码
//
//  IRemoteKit:跨进程(task_for_pid)无注入 / 无 JIT 地读内存、找对象、
//  在目标进程真实对象上调方法、装 hook、观察行为。
//  由【特权宿主进程】(CLI/守护,持 task_for_pid 等 entitlement)链接使用;
//  永不注入目标——所有 hook 的回调逻辑也跑在宿主侧。
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// 目标进程内的远程地址(对象指针 / 函数地址 / ivar 地址)。跨进程,不可在本进程解引用。
typedef uint64_t IRRemoteAddress;

// 执行线程见 IRThread([IRThread mainThread] / [IRThread childThread]),执行策略见 IRExecutionStrategy
// ([IRExecutionStrategy exceptionWait] / [IRExecutionStrategy spinPoll])——均为类(便于携带配置),不在此处定义。

/// `IRObject` 对目标对象的持有关系(决定 `dealloc` 是否在目标里 `objc_release` 平账)。
typedef NS_ENUM(NSInteger, IRObjectOwnership) {
    IRObjectOwnershipWeak      = 0,   ///< 借用:目标里无我方 +1,可悬垂;dealloc 不释放(IRObjectScan 枚举 / objectAtAddress:)
    IRObjectOwnershipStrong    = 1,   ///< 拥有:目标里持我方 +1;dealloc 远程 objc_release 平账(new/copy/调用返回值)
    IRObjectOwnershipUnmanaged = 2,   ///< 常驻不归属:Class 对象等,永不释放(classNamed:)
};

/// 会话状态。
typedef NS_ENUM(NSInteger, IRSessionState) {
    IRSessionStateOpen = 0,   ///< 有效
    IRSessionStateDead = 1,   ///< 目标已退出/重启(task 端口死),须丢弃并重建会话
};

/// 统一错误码(NSError.domain = IRErrorDomain,code 取此枚举)。
typedef NS_ENUM(NSInteger, IRError) {
    IRErrorNone         = 0,   ///< 无错误
    IRErrorNoPermission = 1,   ///< task_for_pid 失败 / 缺 entitlement
    IRErrorTargetGone   = 2,   ///< 目标退出/重启,会话失效
    IRErrorNotForeground= 3,   ///< 目标不在前台活跃,worker 起不来 / 主线程不调度
    IRErrorNoWorker     = 4,   ///< 建 worker 失败(gadget 无效等)
    IRErrorTimeout      = 5,   ///< 调用超时(方法卡死/等锁);会话可能已毒化
    IRErrorFault        = 6,   ///< 远程调用中途崩(碰非法内存等)
    IRErrorNotFound     = 7,   ///< 类/实例/方法/ivar 找不到
    IRErrorPoisoned     = 8,   ///< worker 因既往超时被毒化,拒绝后续调用
    IRErrorBadArg       = 9,   ///< 参数非法
    IRErrorNoActiveWorker = 10,///< 在 worker 作用域外发起远程调用(硬约束:须包在 withWorker:/run: 内)
    IRErrorInternal     = 99,  ///< 内部错误(不该发生)
};

/// 内存保护属性(位掩码;映射到底层 `VM_PROT_*`)。
typedef NS_OPTIONS(NSUInteger, IRMemoryProtection) {
    IRMemoryProtectionNone    = 0,        ///< 不可访问
    IRMemoryProtectionRead    = 1 << 0,   ///< 可读
    IRMemoryProtectionWrite   = 1 << 1,   ///< 可写
    IRMemoryProtectionExecute = 1 << 2,   ///< 可执行(no-JIT gadget / worker 代码页用)
};

/// NSError.domain 用它。
FOUNDATION_EXPORT NSErrorDomain const IRErrorDomain;

NS_ASSUME_NONNULL_END
