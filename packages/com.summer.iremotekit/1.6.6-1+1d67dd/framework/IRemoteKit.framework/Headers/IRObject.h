//
//  IRObject.h — 远程对象/类句柄 + 链式调用(L1)
//
//  IRObject 代表目标进程里的一个值:对象指针 / 类对象 / 调用结果(x0)。
//  链式:找对象 → 调方法 → 结果自动成下一步的 IRObject → 再调 → 链尾取值。
//  错误顺链传播(中途失败得到一个 error!=nil 的 IRObject,后续调用短路)。
//
//  例(调用须在 worker 作用域内;找实例走 IRObjectScan,免 worker):
//    IRObject *m = [s firstInstanceOfClass:@"AWEAwemeModel"];
//    NSString *itemID = [s withWorker:^{ return [m call:@"itemID"].stringValue; }];
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRSession;
@class IRIvar;
@class IRCallOptions;
@class IRWorker;
@class IRObject;

/// `IRObject` 本地壳对象失效的原因。
typedef NS_ENUM(NSInteger, IRObjectInvalidationReason) {
    /// 所属 `IRSession` 已关闭或目标进程已退出。
    IRObjectInvalidationReasonSessionClosed    = 0,
    /// dedup 对象池验活时检测到远程对象已释放或地址被复用，已换新壳。
    IRObjectInvalidationReasonRemoteObjectDead = 1,
    /// 调用方主动调用 `-[IRObject invalidate]` 标记失效。
    IRObjectInvalidationReasonDiscarded        = 2,
    /// 本地壳临 `dealloc` 释放前兜底触发（此前未因其它原因失效）。
    IRObjectInvalidationReasonDealloc          = 3,
};

/// 对象失效监听回调。`reason == IRObjectInvalidationReasonDealloc` 时为防止 ARC 复活已销毁对象，`object` 为 `nil`。
typedef void (^IRObjectInvalidationHandler)(IRObject * _Nullable object, IRObjectInvalidationReason reason);

/// 目标进程里一个值的句柄(对象指针 / 类对象 / 调用结果);链式调用 + 三档持有。
@interface IRObject : NSObject

#pragma mark - 标识 / 状态

/// 远程地址(0 = 空 / 无效)。
@property (nonatomic, readonly) IRRemoteAddress addr;
/// 所属会话。
@property (nonatomic, readonly, weak) IRSession *session;
/// 链中最近一次错误;`nil` 表示 OK。
@property (nonatomic, readonly, nullable) NSError *error;
/// 是否有效:`addr` 非 0、`error` 为 `nil`、未失效且会话仍 Open。**只查句柄结构,不碰目标**(免 I/O)。
@property (nonatomic, readonly, getter=isValid) BOOL valid;
/// 是否已触发过生命周期失效通知（`didInvalidateWithReason:` 触发后恒为 `YES`）。
@property (nonatomic, readonly, getter=isInvalidated) BOOL invalidated;
/// 对目标对象的持有关系(决定 `dealloc` 是否远程 `objc_release` 平账)。见 `IRObjectOwnership`。
@property (nonatomic, readonly) IRObjectOwnership ownership;

/// 校验目标对象**是否还是个活的合法 ObjC 对象**:读 isa 沿 对象→类→元类 两级验证(每级都得解析出合法类)。
/// 纯 mach 读、免 worker。与 `isValid`(只查句柄结构)互补:句柄不 retain 目标,对象可能已被释放/内存重用,
/// 用前想确认存活就调这个。
/// - Note: 只验"是不是活的合法对象",不验"还是不是原来那个"(内存重用成【别的】有效对象也会 `YES`;
///   要认身份再比 `className`)。目标已退出 / 地址不可读 → `NO`。
- (BOOL)isValidObject;

/// 是否 **tagged pointer(小对象)**:值直接编码在指针位里(小 NSNumber / 短 NSString / NSIndexPath / 部分 NSDate…),
/// 无 isa、不指向内存。免 worker、纯判位(arm64/arm64e:bit63)。tagged 对象 `isValidObject` 恒 `YES`;
/// `call:` / `className` / `classObject` 对它也已支持(内部远程 `object_getClass` 取类,需 worker 作用域)。
@property (nonatomic, readonly, getter=isTaggedPointer) BOOL taggedPointer;

/// 能否把这个远程对象**纯本地物化**成原生 Foundation 对象——**免 worker、免远程、无副作用**。
/// 不变式:`localizable==YES` ⟺ `localObject != nil`,且保证不碰目标代码。用法:
/// `id v = obj.localizable ? obj.localObject : [obj call:…]`(本地能给就本地,否则走远程)。
/// - Note: **当前恒 `NO`**:tagged pointer 的纯本地解码未落地(arm64e split tagged 需精确位置换,
///   未验准前不冒"给错值"的险)。接口已就位,将来对 tagged NSNumber/短 NSString 点亮,**不改接口**。
@property (nonatomic, readonly, getter=isLocalizable) BOOL localizable;

/// 纯本地物化出的原生对象(tagged NSNumber → `NSNumber *`、短 NSString → `NSString *`;将来可扩展容器)。
/// 不可物化返回 `nil`。见 `localizable`。**当前恒 `nil`**(纯本地解码待实现)。
@property (nonatomic, readonly, nullable) id localObject;

// 找某类的活实例 → 见 `IRSession (IRObjectScan)`(enumerateInstancesOfClass: 等,纯内存扫描)。

#pragma mark - 调用(走当前 worker;须在 worker 作用域内)

/// 唯一底层调用(兜底 / 封装用)。所有语法糖(`.remote` / `.main` / `.child`)最终都落到它。
/// - Parameters:
///   - sel: 方法选择子名(如 `@"itemID"`)
///   - arguments: 参数数组(`nil` = 无参),元素可为 `IRObject` / `NSNumber` / `NSString`(自动转)
///   - options: 单次调用配置(`nil` = 默认:子线程);见 `IRCallOptions`,承载线程 / 将来超时等
/// - Returns: 结果句柄;**作用域外调用** → 句柄带 `IRErrorNoActiveWorker`,链式短路
- (IRObject *)call:(NSString *)sel
         arguments:(nullable NSArray *)arguments
           options:(nullable IRCallOptions *)options;

/// 便利重载:在调用的**同一条 worker 作用域**里挂"调用前 / 调用后"回调(等价于往 `options` 填
/// `beforeCall` / `afterCall` 再走 `call:arguments:options:`)。两 block 均可 `nil`。
/// 典型:一小串依赖上一步返回值的调用——`after` 里拿到 `result`(`+0` 返回值仍活)接着读 / 调,落同一条 worker。
/// `options` 传 `nil` 则新建默认;非 `nil` 会被拷贝(不改调用方的 options),再塞这两个 block。
- (IRObject *)call:(NSString *)sel
         arguments:(nullable NSArray *)arguments
           options:(nullable IRCallOptions *)options
            before:(nullable void (^)(IRWorker *worker))before
             after:(nullable void (^)(IRWorker *worker, IRObject *result))after;

#pragma mark - 调用(NSProxy 原生语法门面;走当前 worker)

/// 原生语法门面:把 self 包成代理,`[(AWEAwemeModel *)obj.remote itemID]` 直接像本地调用。
///
/// 代理 `forwardInvocation:` 从目标 `method_t.types` 取签名 → marshal → 走【当前 worker】。默认子线程;
/// 句柄上用 `.main` / `.child` 逐调用切线程(私有类经协议 `<IRRemotable>`、系统类经 `NSObject (IRRemote)`)。
/// 标量返回直接回填本地,对象返回再包成 `.remote`(链式)。可 cast 成目标类 / `id<协议>` 得编译期检查。
/// - Returns: 代理(`id`);**作用域外调用** → 置 `session.lastError`、返回 nil/0
@property (nonatomic, readonly) id remote;

#pragma mark - 对象级自定义数据(userInfo)

/// 挂在这个远程对象上的本进程自定义数据(如解析过的字段),给不想定义类型的调用方临时用。
/// **同一个活远程对象(会话+地址)不管拿到几个句柄,userInfo 是同一份**(靠 dedup 池);随对象生灭
/// (对象死 / 地址被别类复用 → 换新壳,旧 userInfo 连壳一起丢);会话结束整池释放。远程对象本身不动。
/// 懒创建;对象无效(addr 0 / 会话关 / 已死)返回 nil。
/// - Note: 只读**不可变视图**(直接返回内部字典、不拷贝,勿 cast 回 mutable 去改);写一律走 `setUserObject:forKey:`。
///   线程:字典本身不加锁,同一份被多线程改由调用方自己保护(同 `session.userObject`)。
@property (nonatomic, readonly, nullable) NSDictionary *userInfo;

/// 挂 / 改 / 删(`obj`=nil 删)这个对象的自定义数据。对象无效则空操作。
- (void)setUserObject:(nullable id)obj forKey:(NSString *)key;

/// 取这个对象某个键的自定义数据;无 / 对象无效返回 `nil`。
- (nullable id)userObjectForKey:(NSString *)key;

#pragma mark - 类型映射(远程类 → IRObject 子类)

/// 注册:目标进程里 `remoteClassName` 类(及其子类)的对象,创建句柄时用 `subclass`(必须是 `IRObject` 子类)而非裸 `IRObject`。
/// 创建对象时沿远程对象的**父类链**找第一个注册过的类,用它的子类;没注册的就是 `IRObject`。"远程类 → 子类"按类缓存在会话上。
/// 同一远程类重复注册:打日志、后注册的生效。子类可加自己的属性 / 懒加载缓存,跟着 dedup 池里的实例走。
/// - Note: 让子类"加载时自动登记":在子类的 `+load` 里调本方法即可(不需核心全类扫描)。
+ (void)registerSubclass:(Class)subclass forRemoteClass:(NSString *)remoteClassName;

#pragma mark - 生命周期 (供子类重写与外部失效监听)

/// 有效对象壳（`addr != 0 && error == nil`）完成初始化后立即调用（保证最多触发一次）。
/// - Note: 在 `IRSession` 对象池路径中，本方法会在**释放池锁之后**触发，因此子类可在其中安全初始化本地缓存、
///   读取 ivar 或发起远程调用（如预拉取远程 JSON）；但若该类会被 `instancesOfClass:` 批量扫描，重型远程调用
///   建议改为属性懒加载，避免批量扫描时触发风暴。错误壳（`addr == 0` 或 `error != nil`）不触发本方法。
- (void)didInitialize;

/// 对象壳变为失效状态的第一时间调用（原子幂等，全生命周期最多触发一次）。
/// 典型用途：撤掉关联的 Watchpoint、停止定时器、取消异步任务、通知上层 UI 或状态机丢弃过期句柄。
/// 子类重写时建议调用 `[super didInvalidateWithReason:reason]`。
- (void)didInvalidateWithReason:(IRObjectInvalidationReason)reason;

/// 本地壳对象 `dealloc` 释放前调用（位于 `Strong` 句柄的远程 `enqueueRelease:` 之前）。
/// 仅用于清理本地非 ARC 资源（如 C 缓冲、观察者等）。
/// - Warning: 此时 `self.session`（weak）可能已为 `nil`，**严禁**在本方法中发起任何同步远程调用或新建指向 `self` 的弱引用。
- (void)willDealloc;

/// 主动将当前对象壳标记为失效（触发 `didInvalidateWithReason:IRObjectInvalidationReasonDiscarded` 与所有监听块；幂等）。
- (void)invalidate;

/// 为不写子类的调用方提供轻量失效监听。若对象当前已失效，会立即同步回调一次；否则在首次失效时回调并自动清空。
- (void)addInvalidationHandler:(IRObjectInvalidationHandler)handler;

#pragma mark - 类型安全与远程类型判断

/// 本地子类类型校验便利方法：若 `self.valid && [self isKindOfClass:expectedClass]` 返回 `self`，否则返回 `nil`。
/// 配合 `+registerSubclass:forRemoteClass:` 与 `__kindof` 使用，免手写强转。
- (nullable __kindof IRObject *)asClass:(Class)expectedClass;

/// 判断当前远程对象是否为指定远程类或其子类（含自身；沿远程父类链比对类名，纯内存免 worker）。
/// 适用于未注册本地 `IRObject` 子类的远程类型判断（如 `[obj isKindOfRemoteClassNamed:@"UIView"]`）。
/// 无效句柄或空类名返回 `NO`。
- (BOOL)isKindOfRemoteClassNamed:(NSString *)remoteClassName;

#pragma mark - 持有

/// 取【强持有】句柄:立即在目标 `objc_retain`(+1),`dealloc` 时远程 `objc_release` 平账。
/// 把"借来的"实例转成"我方拥有",防其被目标 pool drain / 并发释放。保留当前本地子类类型。
/// - Returns: Strong 句柄(独立持一份 +1)
- (instancetype)strongReference;

/// 取【弱持有】句柄:不 +1、`dealloc` 不 release(借用视图,可悬垂)。保留当前本地子类类型。
/// - Returns: Weak 句柄
- (instancetype)weakReference;

#pragma mark - 取值

/// 结果 x0 原值(无符号)。
@property (nonatomic, readonly) uint64_t unsignedValue;
/// 结果 x0 作有符号整数。
@property (nonatomic, readonly) int64_t integerValue;
/// 结果 x0 低字节作 BOOL。
@property (nonatomic, readonly) BOOL boolValue;
/// 结果作 `double`(读调用的 NEON 返回 v0;仅对返回 double/float 的方法有意义)。
@property (nonatomic, readonly) double doubleValue;
/// 把结果(远程 `NSString`)读回本地;非字符串或失败返回 `nil`。
@property (nonatomic, readonly, nullable) NSString *stringValue;
/// 此对象的类名。
@property (nonatomic, readonly, nullable) NSString *className;
/// 此对象的类(读 isa → 类对象句柄,`Unmanaged`);失败返回 `nil`。与 `IRSession -classNamed:`(按名取类)
/// 互补的"按对象取类"。裸类地址包装请用 `IRSession -objectAtAddress:`,此处不收裸地址。
@property (nonatomic, readonly, nullable) IRObject *classObject;

/// 原类名:从真实类(`className`)沿父类链跳过【运行时动态建的类】(KVO `NSKVONotifying_X`、
/// 监控库动态子类等),取第一个编译期类的名字。绝大多数对象 == `className`;被 KVO/监控换过类的
/// 对象返回其原类名(如 `NSKVONotifying_AWEAwemeModel` → `AWEAwemeModel`)。按类结构判(读 class_rw_t
/// flags),不猜名字前缀;一个类只解析一遍(会话缓存)。纯读内存,免 worker。失败返回 `nil`。
@property (nonatomic, readonly, nullable) NSString *originalClassName;
/// 原类(类对象句柄,`Unmanaged`);同 `originalClassName`,返回类句柄。失败返回 `nil`。
@property (nonatomic, readonly, nullable) IRObject *originalClassObject;


#pragma mark - ivar

/// 此对象的全部 ivar(含继承链),各自绑定 self;经 `IRIvar.value` 读写。纯内存,免 worker。
/// - Returns: `IRIvar` 数组(可空数组);无会话 / 解析失败返回 `@[]`
- (NSArray<IRIvar *> *)ivars;

/// 按名取一个 ivar 访问器(含继承链),绑定 self。
/// - Parameter name: ivar 名(如 `@"_tag"`)
/// - Returns: `IRIvar`;无此 ivar 返回 `nil`
- (nullable IRIvar *)ivarNamed:(NSString *)name;

// 序列化 / 层级描述属 L2 业务(modeljson / uidump),不在此层:
//   JSON  → App 特定(如 Mantle MTLJSONAdapter),非通用原语;
//   tree  → 一行 `[vc.remote.main recursiveDescription].stringValue`,现成门面即可拼。

@end

NS_ASSUME_NONNULL_END
