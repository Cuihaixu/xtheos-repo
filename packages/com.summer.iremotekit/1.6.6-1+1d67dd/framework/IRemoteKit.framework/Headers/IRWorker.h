//
//  IRWorker.h — 一条远程工作线程(执行者)(L0)
//
//  IRWorker 代表目标进程里【一条我方常驻工作线程】:用
//  `pthread_create_from_mach_thread` 引导、自旋在 `b .` gadget,后续被劫持跑一次
//  调用再复位。它是远程调用的【执行者 / 逻辑维护者】——无论函数最终落在:
//    - 子线程执行:worker 在自己这条子线程上劫持直接跑;
//    - 主线程执行:worker 把调用 post 到目标主线程,再等它跑完;
//  发起、等待、读回都由【这一条 worker】驱动。主线程本身不自驱,故不单列为一种线程。
//
//  串行 / 并发:同一条 IRWorker 上的调用【一律串行】(它就一条线程,不管子/主线程执行);
//  要并发就向会话再开多条 IRWorker。每条自带线程级异常端口 + 守卫页,互不串扰。
//
//  执行策略(`b .` 自旋 vs 线程级异常端口):worker 被劫持跑完后如何察觉返回,是这条
//  worker 的【执行策略】,由 `executionStrategy` 选——spinPoll(返回落 `b .` 忙轮询)或
//  exceptionWait(返回落只读守卫页→线程级异常端口即时唤醒)。线程级端口 / 守卫页每 worker 各一份。
//
//  生命周期决定"挂起 vs 销毁":一次调用完成后,只要 worker 对象还活着,就把远程线程
//  【挂起】驻留(park,不烧 CPU)等下次复用;worker `close` / 释放(生命周期终结)才
//  【销毁】远程线程(suspend + terminate)并还原其线程级异常端口。
//
//  IRWorker 由 `IRSession` 派发(`-withWorker:` 托管 / `-spawnWorker:` 显式),勿直接实例化;
//  会话 `close` 时兜底关闭其名下每一条 worker(RAII,保证不留悬空线程 / 端口)。
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRSession;
@class IRThread;
@class IRWorkerThread;
@class IRObject;
@class IRExecutionStrategy;
@class IRRemoteInvocation;
@class IRValue;
@class IRCallOptions;

/// 一条远程工作线程(执行者):同一 worker 内调用串行,多条 worker 并发;由 `IRSession` 派发。
@interface IRWorker : NSObject

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;

#pragma mark - 标识 / 状态

/// 所属会话。
@property (nonatomic, readonly, weak) IRSession *session;

/// 是否就绪:已引导、自旋在 gadget、会话仍 Open。
@property (nonatomic, readonly, getter=isReady) BOOL ready;

/// 本 worker 底层在目标进程中驻留的远程工作线程句柄 (`IRWorkerThread`)；尚未首次启动或已关闭时返回 `nil`。
/// 支持只读观测其 `.threadID` / `.cpu` / `-backtraceWithMaxFrames:error:` 进行自省诊断。
@property (nonatomic, readonly, nullable) IRWorkerThread *workerThread;

#pragma mark - 作用域

/// 以本 worker 为【当前 worker】跑一串,块内 `IRObject` 调用自动落在它上;返回块结果。不销毁(你管生命周期)。
///
/// 用线程局部"当前 worker"栈实现,可嵌套。同一 worker 内所有调用串行。
/// - Parameter block: 调用序列(返回值带出作用域)
/// - Returns: 块返回值(可空)
- (nullable id)run:(id _Nullable (^)(void))block;

/// 立即 drain 当前 `run:` 作用域已累积的目标侧自动释放池,随后重开一层空池。
///
/// worker 是裸线程,目标侧 `+0` autoreleased 返回值(`-UTF8String` 缓冲、`NSInvocation` 等)挂在本作用域池上,
/// 作用域结束才回收。**长驻单一 `run:`**(如循环内反复大调用的监控)需要在循环里主动调用它,把内存按节拍放掉,
/// 而不必退出作用域。必须在本 worker 的 `run:` 作用域内调用;作用域外是空操作。
- (void)drainAutoreleasePool;

/// 自动清池节拍:**在一个 `run:` 作用域内**,每满这么多次调用就自动 drain 一轮目标侧自动释放池,
/// 无需调用方手动 `drainAutoreleasePool`。专堵"在一个 worker 作用域里写 for 循环、不退出作用域、
/// 内存一路涨"——调用方通常不知道要清池,这个兜底让紧循环也不炸。
///
/// 默认 **64**(每 64 次调用自动清一轮);设 **0 关闭**。这是**兜底判据**,抓很多小返回的累积。
///
/// 语义影响:开启后,**裸指针 / C 字符串返回值**(如 `-UTF8String` 的 `char*`)的有效期收紧为
/// "到下一次自动清池前"——即**当次调用后立即读**;对象返回(Strong 句柄)和已拷回的本地值不受影响。
@property (nonatomic) NSUInteger autoreleaseDrainInterval;

/// 自动清池的**字节主判据**:本 `run:` 作用域内,每从目标读回满这么多字节的返回数据(如 `-UTF8String`),
/// 就自动 drain 一轮。专抓 `recursiveDescription` 这种**单个巨返回**(几 MB 一发,远未到调用数阈值就该清;
/// 实测抖音一个 `run:` 循环 200 次 recursiveDescription,只按调用数清会 OOM 死目标,按字节清则 footprint 持平)。
///
/// 默认 **1 MB**;设 **0 关闭**。与 `autoreleaseDrainInterval` 取**先到者**触发。普通小调用攒不满、几乎不触发,
/// 故对紧的小调用循环无额外开销;巨返回则每发必清,把峰值内存压住。
@property (nonatomic) NSUInteger autoreleaseDrainBytes;

#pragma mark - 调用

/// 在指定线程上调用 `[object selector:arguments…]`,同步返回。
///
/// 同一条 worker 的调用串行执行(子线程 / 主线程执行都排在这一条线程的时间线上)。
/// - Parameters:
///   - object: 接收者(远程对象句柄)
///   - selector: 方法选择子名(如 `@"itemID"`)
///   - arguments: 参数数组(元素 `IRObject` / `NSNumber` / `NSString`,自动 marshal);无参传 `nil`
///   - thread: 执行线程——`[IRThread childThread]`(本 worker)或 `[IRThread mainThread]`(post 过去)
/// - Returns: 结果句柄;**返回对象默认 Strong**(调用后已在目标 `objc_retain` +1 归我)
/// - Warning: 主线程执行需目标前台且主线程空闲,否则超时
- (IRObject *)callObject:(IRObject *)object
                selector:(NSString *)selector
               arguments:(nullable NSArray *)arguments
                  thread:(IRThread *)thread;

/// 执行一次远程调用(worker 是唯一执行者:按 invocation 的 method 类型编码分派寄存器 →
/// 调用 → 按返回类型读回)。同一 worker 串行。
/// - Parameters:
///   - invocation: 被动的调用描述(纯"调什么":method + 实参)
///   - options: 本次派发配置(线程 / 冻结其它线程…;`nil` = 默认子线程 + 冻结)
/// - Returns: 结果 `IRValue`(对象→Strong 句柄 / 标量→int/double / 结构体→`.data` / void→`nil`);
///   失败返回 `nil`(错误经 `session.lastError`)
- (nullable IRValue *)invoke:(IRRemoteInvocation *)invocation options:(nullable IRCallOptions *)options;

#pragma mark - 配置

/// 执行策略(怎么察觉返回 + 超时),**worker 创建时固定,运行期不可换**
/// (exceptionWait/spinPoll 各绑守卫页 / 线程级端口等 per-worker 资源,飞行中切会破坏状态)。
/// 经 `-[IRSession spawnWorkerWithStrategy:error:]` 指定;`withWorker:` / `spawnWorker:` 用默认 `exceptionWait`。
@property (nonatomic, readonly) IRExecutionStrategy *executionStrategy;

// 「冻不冻结目标其它线程」是**逐调用**决定,由 `IRCallOptions.suspendOthers` 承载(走 `invoke:options:`)。
// 不再作为 worker 的公开属性;内部保留一个 per-call 传递用的字段(见 IRPrivate)。

#pragma mark - 一致读窗口(主线程停到安全点 → 块内读/调 → 放行)

/// 同步:把目标主线程停到 runloop 安全点并 hold → 在此窗口内跑 `block`(一致快照)→ 放行,返回块结果。
///
/// 为什么要它:跨进程读/操作"活树"(UI 层级/对象图)有 race——读一半目标主线程在改。本法把**主线程**(UI
/// 唯一改写者)停在 runloop 安全点,这一刻树一致;**只停主线程、不冻其它线程与 worker**,所以窗口内既能纯内存读、
/// **也能发远程调用**(如遍历 `nextResponder` 响应链),拿到同一时刻的一致快照。
/// - Parameters:
///   - thread: 要停到安全点的线程。`[IRThread mainThread]` 读 UI(目标 UIKit 在主线程改);
///             `nil` / 目标非前台 = 不会合,`block` 就地在**当前线程**跑(非前台时 UI 本就不变,直接读即一致)。
///   - block: 读/调序列,**可发远程调用**(`call:`/`.remote` 均可);**不可重入**(别在块内再调本法);
///            返回值原样带出(同 `run:` 的 id 透传)。
/// - Returns: `block` 的返回值;会合失败返回 `nil`。
/// - Note: 主线程被 hold 的时长 = 你的块执行时长,**短是调用方的责任**;另有 2s 硬上限兜底(driver 异常退出
///   主线程也最多 2s 自醒,不会被永久卡死)。放行由 RAII(`@try/@finally`)在块正常返回 / 抛异常时都保证。
- (nullable id)readWhileSuspended:(nullable IRThread *)thread block:(id (^)(void))block;

/// 便利:等价 `readWhileSuspended:[IRThread mainThread] block:`(读 UI 层级/对象图最常用)。
- (nullable id)readWhileMainSuspended:(id (^)(void))block;

#pragma mark - 收尾

/// 销毁远程线程(suspend + terminate)并还原其线程级异常端口,释放守卫页。幂等。
///
/// 生命周期终结点:调用间只是【挂起】驻留复用,唯有 `close` / `dealloc` 才【销毁】。
/// 由 `spawnWorker:` 开的并发 worker 用完应主动 `close`;`withWorker:` 托管的随块结束自动 close。
/// - Warning: worker 上挂着守卫页 hook 时勿强杀宿主,否则目标下次 fault 逃到悬空端口被杀
- (void)close;

@end

NS_ASSUME_NONNULL_END
