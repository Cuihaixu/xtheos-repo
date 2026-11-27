//
//  IRSession.h — 会话:资源 + 安全 + 工厂(L0)
//
//  一个 IRSession = 挂在【某进程实例(pid)】上的一次连接,持有:
//    task 端口、常驻 worker(ropLoop/scratch/守卫页)、我方线程级异常端口 +
//    保存的目标原始异常端口。
//  一目标一会话一 worker、复用。close/dealloc 一定还原目标异常端口(焊死
//  "别留悬空端口"的安全性)。目标退出/重启 → 状态转 Dead。
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRObject;
@class IRWorker;
@class IRExecutionStrategy;
@class IRRemoteThread;
@class IRRemoteClassCheck;

/// 会话由 `IRSessionManager` 创建(`-sessionForProcessName:` / `-sessionForPID:`),勿直接实例化。
@interface IRSession : NSObject

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;

#pragma mark - 标识 / 状态

/// 目标进程 pid。
@property (nonatomic, readonly) pid_t pid;

/// 目标进程【这一次运行】的稳定标识 —— 跨"我方进程重启"一致,目标重启 / 设备重启即变。
///
/// 取值 = `kern.bootsessionuuid`(本次开机唯一)+ 目标 `p_uniqueid`(本次开机内进程实例唯一,
/// 内核保证不复用),格式 `"<bootUUID>:<uniqueid>"`;**不依赖 wall-clock**,无时钟跳变碰撞问题。
/// 纯结构原语:核心只给这个"哪次运行"的标识,业务拿它(可配 `obj.addr` / 自己解析的 itemID 等)
/// 拼自己的 key、做自己的判断(如按一次运行索引本地文件,目标重启后旧数据自然作废)。
/// 懒算、会话存活期内缓存;目标已 Dead / 读取失败返回 `nil`。
@property (nonatomic, readonly, nullable) NSString *runID;

/// 会话状态;目标退出/重启后转 `IRSessionStateDead`。
@property (nonatomic, readonly) IRSessionState state;
/// 按名字打开则记住此名(标识 / 日志用);按 pid 打开为 `nil`。
@property (nonatomic, readonly, nullable) NSString *name;

#pragma mark - 生命周期

/// 收尾:还原目标异常端口、终止 worker、释放资源。幂等;`dealloc` 与 `IRSessionManager` 收尾亦调用。
///
/// 会话不可跨目标复用:目标退出/重启后会话转 Dead,**丢弃它**、经 `IRSessionManager`
/// 按名重开一个新会话(task 端口 / worker / 地址缓存全失效,无可复用)。
/// - Warning: 必须还原目标异常端口,否则目标下次 fault 被内核杀(勿 `kill -9` 宿主)
- (void)close;

#pragma mark - 工厂

/// 远程取类(`objc_getClass`)。
/// - Parameter className: 类名
/// - Returns: 代表该类对象的 `IRObject`;找不到返回 `nil`
- (nullable IRObject *)classNamed:(NSString *)className;

/// 把一个已知远程地址包成 `IRObject`(不校验有效性)。
/// 同一活远程地址返回**同一个本地壳**(弱引用 dedup:同址同壳,挂在壳上的东西跟着走);业务不再引用即释放。
/// 命中时验活(`isValidObject`),远程对象死 / 地址被别类复用则换新壳。
/// - Parameter address: 目标进程内的远程地址
/// - Returns: 句柄(与同地址其它调用共享同一实例)
- (__kindof IRObject *)objectAtAddress:(IRRemoteAddress)address;

/// 便利接口：按地址取壳并校验本地子类类型（`obj.valid && [obj isKindOfClass:expectedClass]`）；
/// 地址无效或类型不匹配直接返回 `nil`。
- (nullable __kindof IRObject *)objectAtAddress:(IRRemoteAddress)address
                                        asClass:(Class)expectedClass;

/// 同 `objectAtAddress:`,但**核心额外强持**这个壳(缓存并保活):业务不用自己持有引用也不会释放。
/// 用于"可能稍后再访问、量不大"的对象(如页面控制器),省得业务自己到处存引用。
/// 与 `objectAtAddress:` 共享同一 dedup 表(同址同壳),只是额外钉在池里。
/// 释放靠业务调 `discardObjectAtAddress:`,或再访问时验活发现远程对象已死 → 自动换壳;会话销毁整清。
/// - Note: 只强持**本地壳**,不碰远程对象持有(仍是 Weak 借用,不影响目标内存)。
- (__kindof IRObject *)cacheObjectAtAddress:(IRRemoteAddress)address;

/// 便利接口：先按地址取壳并校验本地子类类型（`obj.valid && [obj isKindOfClass:expectedClass]`），
/// 仅在匹配时才钉入缓存并返回；不匹配或地址无效返回 `nil` 且不会污染缓存。
- (nullable __kindof IRObject *)cacheObjectAtAddress:(IRRemoteAddress)address
                                             asClass:(Class)expectedClass;

/// 解除 `cacheObjectAtAddress:` 的强持:核心不再保活该地址的壳(业务仍持有则壳还在,弱表项随引用自然清)。
/// 只丢**本地**这份强持,不碰远程对象。
- (void)discardObjectAtAddress:(IRRemoteAddress)address;

#pragma mark - 内存原语(读)

/// 从目标读原始字节到调用方缓冲区。
/// - Parameters:
///   - buffer: 接收数据的本进程缓冲区,容量至少 `length` 字节
///   - length: 读取字节数
///   - address: 目标进程内的远程地址
///   - error: 失败时置错误(地址不可读 / 跨越未映射区)
/// - Returns: 全部读满 `YES`;失败或读不满 `NO`
- (BOOL)readBytes:(void *)buffer length:(size_t)length fromAddress:(IRRemoteAddress)address error:(NSError **)error;

/// 读目标一段内存,返回拷贝出的 `NSData`。
/// - Parameters:
///   - length: 读取字节数
///   - address: 远程地址
///   - error: 失败时置错误
/// - Returns: 成功返回含 `length` 字节的数据;失败返回 `nil`
- (nullable NSData *)readDataWithLength:(size_t)length fromAddress:(IRRemoteAddress)address error:(NSError **)error;

/// 读目标内一个 8 字节指针值。
/// - Parameters:
///   - outPointer: 成功时写入读到的指针(arm64e 不在此剥 PAC,交上层处理)
///   - address: 远程地址
///   - error: 失败时置错误
/// - Returns: 成功 `YES`
- (BOOL)readPointer:(IRRemoteAddress *)outPointer fromAddress:(IRRemoteAddress)address error:(NSError **)error;

/// 读目标内以 NUL 结尾的 C 字符串(按 UTF-8 解释)。
/// - Parameters:
///   - address: 字符串首地址
///   - maxLength: 最多读取字节数(含终止符;防未终止字符串读跑飞)
///   - error: 失败时置错误
/// - Returns: 成功返回字符串;失败返回 `nil`
- (nullable NSString *)readCStringFromAddress:(IRRemoteAddress)address maxLength:(size_t)maxLength error:(NSError **)error;

#pragma mark - 内存原语(写)

/// 把本进程缓冲区写入目标。
/// - Parameters:
///   - buffer: 源数据(本进程),至少 `length` 字节
///   - length: 写入字节数
///   - address: 目标远程地址
///   - error: 失败时置错误
/// - Returns: 成功 `YES`
/// - Warning: 目标地址所在页须可写(否则失败);写错地址可致目标崩溃
- (BOOL)writeBytes:(const void *)buffer length:(size_t)length toAddress:(IRRemoteAddress)address error:(NSError **)error;

/// 把 `NSData` 写入目标。
/// - Parameters:
///   - data: 要写入的数据
///   - address: 目标远程地址
///   - error: 失败时置错误
/// - Returns: 成功 `YES`
/// - Warning: 同 `writeBytes:length:toAddress:error:`,目标页须可写
- (BOOL)writeData:(NSData *)data toAddress:(IRRemoteAddress)address error:(NSError **)error;

#pragma mark - 内存原语(分配 / 保护)

/// 在目标进程内分配一块内存(`mach_vm_allocate`)。
/// - Parameters:
///   - size: 请求字节数(向上取整到页)
///   - error: 失败时置错误
/// - Returns: 远程基址;失败返回 `0`
/// - Note: 需用 `freeMemoryAtAddress:size:error:` 释放
- (IRRemoteAddress)allocateMemoryWithSize:(size_t)size error:(NSError **)error;

/// 释放先前分配的目标内存。
/// - Parameters:
///   - address: `allocateMemoryWithSize:error:` 返回的基址
///   - size: 分配时的大小
///   - error: 失败时置错误
/// - Returns: 成功 `YES`
- (BOOL)freeMemoryAtAddress:(IRRemoteAddress)address size:(size_t)size error:(NSError **)error;

/// 改目标内一段内存的保护属性。
/// - Parameters:
///   - address: 起始远程地址
///   - size: 字节数
///   - protection: 目标保护位(见 `IRMemoryProtection`)
///   - error: 失败时置错误
/// - Returns: 成功 `YES`
/// - Warning: 置可执行用于 no-JIT gadget / worker 代码页;注意 W^X 约束
- (BOOL)protectMemoryAtAddress:(IRRemoteAddress)address size:(size_t)size protection:(IRMemoryProtection)protection error:(NSError **)error;

#pragma mark - 工作线程(所有远程调用的唯一入口)

/// 托管作用域:建一条 worker →【块内以它为当前 worker】跑一串 → 块结束销毁,返回块结果。
///
/// **硬约束**:所有远程调用必须在 worker 作用域内(此法 或 `-[IRWorker run:]`)发起;作用域外调用
/// `IRObject` 的 `call:` 系会失败(手动档挂 `IRErrorNoActiveWorker`,门面置 `lastError`)。
/// poison 只损这一串;下次 `withWorker:` 换新 worker。
/// - Parameter block: 调用序列(块内 `IRObject` 调用自动落在这条 worker;返回值带出)
/// - Returns: 块返回值(可空);建 worker 失败返回 `nil`
- (nullable id)withWorker:(id _Nullable (^)(void))block;

/// 显式新建一条可跨多次 `run:` 复用的 worker(默认执行策略 `exceptionWait`);用完须 `-[IRWorker close]`。
///
/// 独立子线程 + 独立线程级异常端口 + 守卫页,与其它 worker 互不串扰。不关也随会话 `close` 兜底。
/// - Parameter error: 失败时置错误(目标非前台 / 建 worker 失败)
/// - Returns: 新 worker;失败返回 `nil`
- (nullable IRWorker *)spawnWorker:(NSError **)error;

/// 同上,但指定执行策略(超时 / spinPoll 等);策略随 worker 固定,运行期不可换。
/// - Parameters:
///   - strategy: 执行策略(见 `IRExecutionStrategy`)
///   - error: 失败时置错误
/// - Returns: 新 worker;失败返回 `nil`
- (nullable IRWorker *)spawnWorkerWithStrategy:(IRExecutionStrategy *)strategy error:(NSError **)error;

#pragma mark - 用户存储(会话作用域)

/// 取挂在本会话上的用户数据。
/// - Parameter key: 消费方自定义键(建议带命名空间前缀,如 `@"IRAuto.xxx"`,避免跨消费方撞键)
/// - Returns: 之前挂的对象;无则 `nil`
///
/// 纯结构原语:核心**不解释** value 语义(消费方自用,如挂进程对象缓存)。生命周期=本会话
/// (即一个 pid 的存活期):`close`/`dealloc` 时整体清空;目标退出/重启后会话转 Dead,连同存储一并丢弃。
/// - Warning: **不保证线程安全**——并发读写交由消费方自行协调(它最清楚该落在哪个线程)。
- (nullable id)userObjectForKey:(NSString *)key;

/// 挂 / 改 / 删(`obj` 为 `nil` 即删)用户数据。约束与线程语义同 `-userObjectForKey:`。
/// - Parameters:
///   - obj: 要挂的对象;传 `nil` 删除该键
///   - key: 消费方自定义键
- (void)setUserObject:(nullable id)obj forKey:(NSString *)key;

#pragma mark - 错误

/// 最近一次经 NSProxy 门面(`IRObject.remote` 系)调用的错误;`nil` 表示上次 OK。
/// 门面无法返回 `NSError`(标量返 0 / 对象返 nil),故错误落此供事后查询。线程局部。
@property (nonatomic, readonly, nullable) NSError *lastError;

/// 本会话所有远程调用的**默认超时(毫秒)**。init 取环境变量 `IREMOTE_TIMEOUT`(有则),否则 `3000`;程序化可随时改。
/// 单次调用可用 `IRCallOptions.timeoutMs(>0)` 覆盖它。超时**不强断**卡住的线程(会丢锁带崩目标),只毒化该 worker。
@property (nonatomic) uint32_t defaultTimeoutMs;

#pragma mark - 隐式 worker 池

/// 隐式 worker 空闲多久(毫秒)后回收其目标线程。性能↔隐蔽旋钮:大=常驻复用最快(但目标留劫持线程);
/// 小=空闲快回收、少留痕(高安全 App);0=不回收(常驻)。默认 5000。worker 调用间本就 park(0 CPU)。
@property (nonatomic) NSUInteger poolCooldownMs;

#pragma mark - worker 线程健康(消费方据此择机重启目标 App)

/// 累计发生【超时毒化】的 worker 数(调用没按时完成:阻塞 / 等锁 / 目标挂起 / CPU 饿死)。
///
/// 超时时我们**不终止线程、不碰目标锁、也不向目标写入未签名代码**,只把该 worker 的异常端口交给后台延迟回收线程:
/// 被调方法继续在后台跑,**跑完返回落到只读守卫页触发异常那一刻,回收线程将 PC 导向系统合法 `pthread_exit(NULL)`
/// 让线程自己退出并释放其守卫/scratch 页**(锁它自己解、活它自己干完,零终止、零 JIT);只有方法**真死锁/
/// 永不返回**时,那条线程才留驻目标(0 CPU、随目标退出清)。此值**单调累计**,随会话(一个 pid),目标重启后归零。
/// **升高说明目标在频繁超时**(可能挂起/卡死),消费方可据此在合适时机重启目标 App。降低它靠减少毒化成因
/// (合理超时、别调阻塞方法、按生命周期在失活时停止调用——生命周期由消费方协调,核心不管)。线程安全。
@property (nonatomic, readonly) NSUInteger leakedWorkerThreadCount;

/// 当前存活的【正常】worker 线程数(健康、在用或 park,可正常回收)。= 已起线程 − 已回收 − 已放弃。
/// 与 `leakedWorkerThreadCount` 互补:二者之和 = 我方在目标里当前存活的 worker 线程总数。线程安全。
@property (nonatomic, readonly) NSUInteger activeWorkerThreadCount;

#pragma mark - 远程类契约检查(启动期,静态/免 worker)

/// 校验远程类是否"定义了"协议里声明的那套(类存在 + 属性有背后 ivar + 方法可响应)。
/// 开自动化前跑一次:App 随时更新,过了才放心用协议访问器(属性读免 worker)。全静态、无 worker、无实例。
/// 类名 = 协议名(`NSStringFromProtocol`);类名 ≠ 协议名时用下方带 className 的变体。
- (IRRemoteClassCheck *)verifyRemoteClassDefines:(Protocol *)proto;
/// 同上,显式指定远程类名(协议名与类名不一致时)。
- (IRRemoteClassCheck *)verifyRemoteClassNamed:(NSString *)className defines:(Protocol *)proto;

#pragma mark - 进程挂起与恢复(幂等布尔开关)

/// 目标进程当前是否处于挂起状态 (`task_info(TASK_BASIC_INFO).suspend_count > 0`)。
@property (nonatomic, readonly, getter=isSuspended) BOOL suspended;

/// 挂起整个目标进程 (幂等布尔开关: 若已处于挂起态则直接返回 `YES`，绝不叠加内核挂起计数)。
- (BOOL)suspend:(NSError **)error;

/// 恢复整个目标进程 (幂等布尔开关: 彻底清零 `task` 级挂起计数与除 `IRWorkerThread` 以外所有业务线程的挂起计数，确保应用立即恢复运行)。
- (BOOL)resume:(NSError **)error;

#pragma mark - 远程线程自省与控制(免 worker)

/// 目标进程当前所有真实业务线程列表 (`#1` 为主线程；默认过滤掉框架内部驻留的 `IRWorkerThread`)。
@property (nonatomic, readonly) NSArray<IRRemoteThread *> *threads;

/// 目标进程主线程 (`#1`)；失败返回 `nil`。
@property (nonatomic, readonly, nullable) IRRemoteThread *mainThread;

/// 枚举目标进程所有线程；`includeWorkers` 为 `YES` 时包含框架驻留工作线程 (实例化为子类 `IRWorkerThread`)。
- (nullable NSArray<IRRemoteThread *> *)threadsIncludingWorkers:(BOOL)includeWorkers error:(NSError **)error;

/// 按全局唯一线程 ID (`tid`) 查找指定线程 (含业务线程与 worker 线程)。
- (nullable IRRemoteThread *)threadWithID:(uint64_t)threadID error:(NSError **)error;

/// 按 1-based 序号 (`#1..N`) 查找指定线程 (`1` 为主线程)。
- (nullable IRRemoteThread *)threadAtIndex:(NSUInteger)index error:(NSError **)error;

@end

/// `verifyRemoteClassDefines:` 结果。`ok` 给门禁(`if (c.ok) …`),`missing*` 给精细回退。
@interface IRRemoteClassCheck : NSObject
/// 类存在 + 所有属性有背后 ivar + 所有必需方法可响应。
@property (nonatomic, readonly) BOOL ok;
/// 远程类是否存在。
@property (nonatomic, readonly) BOOL classExists;
/// 没有背后 ivar(计算属性/weak/类里没有)的属性名——这些读不走 ivar 快路径。
@property (nonatomic, readonly) NSArray<NSString *> *missingIvars;
/// 类不响应的必需方法选择子。
@property (nonatomic, readonly) NSArray<NSString *> *missingMethods;
@end

NS_ASSUME_NONNULL_END
