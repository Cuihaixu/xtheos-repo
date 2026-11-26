//
//  IRRemoteMethod.h — 远程可调用体的描述(L1):调什么 + 类型
//
//  统一两条路,抽象基类归一暴露返回/参数类型 + 名字注册:
//    - IRRemoteOCMethod   OC 方法:类型现成(目标 method_t.types),自动取。
//    - IRRemoteCFunction  C 函数:无元数据,调用方显式给类型编码;地址按 descriptor 懒解析。
//  非常规 ABI / 自封装的自定义 marshal,未来由「继承本类重写钩子」承接(见 DESIGN §2.4.1),暂不实现。
//  配合 `IRRemoteInvocation`(绑实参 → 按类型分派寄存器执行)+ `IRValue`(参/返载体)。
//
//  【类型编码】一律用官方 `\@encode`(OC/C 通用:`v`/`q`/`d`/`@`/`^v`/`{CGRect=dddd}` …),不自造。
//  OC 特有的帧封装(self `@` / _cmd `:` / 栈偏移)仅出现在 OC 的 method_t.types,解析时剥掉;
//  统一视图 = 返回类型 + 用户参数类型列表。
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRObject;

#pragma mark - 可调用体(抽象基类)

/// 远程可调用体(抽象基类,勿直接构造;用下面三个子类)。
@interface IRRemoteMethod : NSObject

/// 完整类型编码(OC=`method_t.types` 帧编码;C=返回+参数顺序拼接;block 版可为空)。
@property (nonatomic, readonly) NSString *typeEncoding;
/// 返回值类型编码(`\@encode` 风格:`v`/`q`/`d`/`@`/`{CGRect=dddd}` …)。
@property (nonatomic, readonly) NSString *returnType;
/// 用户参数个数(不含 OC 的隐藏 self/_cmd)。
@property (nonatomic, readonly) NSUInteger numberOfArguments;
/// 第 `index` 个用户参数的类型编码;越界返回 `nil`。
- (nullable NSString *)argumentTypeAtIndex:(NSUInteger)index;

#pragma mark 名字注册(转换器式复用)

/// 以名字登记一个可调用体,供 `methodNamed:` 复用(类似 `NSValueTransformer` 注册)。
+ (void)registerMethod:(IRRemoteMethod *)method forName:(NSString *)name;
/// 取已注册的可调用体;未注册返回 `nil`。
/// - Note: 存的是 descriptor(怎么找),地址延到具体会话里懒解析,故同名跨目标可复用。
+ (nullable IRRemoteMethod *)methodNamed:(NSString *)name;

@end

#pragma mark - OC 方法

/// 远程 OC 方法的**纯描述符**:只记 类名 + 选择子 + 实例/类 + 类型编码。
/// **不绑任何接收者实例、不碰运行时**——接收者属于一次调用(`IRRemoteInvocation.receiver`),不属于方法。
/// 故一个描述符可服务该类所有对象、可注册(`registerMethod:forName:`)、跨目标复用。
///
/// 从活对象解析签名并构造,是会话层的事(`-[IRSession ocMethodForObject:selector:]`,内部查缓存→
/// 共享缓存本地解 / App 类远程解 `method_t.types`→构造→缓存)。本类只接"四样都齐"的完整构造。
@interface IRRemoteOCMethod : IRRemoteMethod

/// 纯描述符构造:四样齐备(类名/选择子/是否实例方法/类型编码),不触目标运行时。
/// - Parameters:
///   - className: 类名(如 `@"UIView"`;类方法时为该类名,非元类名)
///   - selector: 选择子名(如 `@"recursiveDescription"`)
///   - isInstanceMethod: `YES`=实例方法、`NO`=类方法(决定缓存键区分与 x0 接收者语义)
///   - typeEncoding: 官方 `@encode` 帧编码(如实例对象上取自 `method_t.types`);构造时剥 self/_cmd
+ (instancetype)methodWithClassName:(NSString *)className
                           selector:(NSString *)selector
                     instanceMethod:(BOOL)isInstanceMethod
                       typeEncoding:(NSString *)typeEncoding;

/// 类名(纯名字;跨目标)。
@property (nonatomic, readonly) NSString *className;
/// 选择子名。
@property (nonatomic, readonly) NSString *selector;
/// 是否实例方法(`NO`=类方法)。
@property (nonatomic, readonly) BOOL isInstanceMethod;

@end

#pragma mark - C 函数(descriptor 懒解析)

/// 远程 C 函数:显式类型编码 + 地址 descriptor。地址按 descriptor 种类**懒解析、per-session**:
///   - 符号:共享缓存符号本进程 `dlsym` 即得(两进程同址),否则去目标 dyld 导出表解;
///   - 库+偏移:会话里查模块加载基址(ASLR slide)+ offset —— 无符号 C 函数用;
///   - 绝对地址:已从扫描等拿到运行时地址,直接用。
/// - Warning: 类型编码用官方 `\@encode`(返回 + 各参顺序拼接,无 self/_cmd);写错 ABI 会崩。
@interface IRRemoteCFunction : IRRemoteMethod

/// 按符号名(优先共享缓存 `dlsym`,否则目标导出表)。
+ (instancetype)functionWithSymbol:(NSString *)symbol typeEncoding:(NSString *)typeEncoding;
/// 按 库名 + 偏移(无符号 C 函数;会话里按模块基址 + offset 解)。
+ (instancetype)functionWithLibrary:(NSString *)library offset:(uint64_t)offset typeEncoding:(NSString *)typeEncoding;
/// 按目标进程内绝对地址(共享缓存函数可本地 `dlsym` 取)。
+ (instancetype)functionWithAddress:(IRRemoteAddress)address typeEncoding:(NSString *)typeEncoding;

@end

// —— 自定义 marshal(非常规 ABI / 自封装)——
// 原 `IRRemoteBlockFunction`(block + IRCPUContext)已砍;未来由「继承 IRRemoteMethod 重写 marshal/decode 钩子」
// 承接(见 DESIGN §2.4.1),`IRCPUContext` 只在那条高级路径出现。现在只留位、不实现。

NS_ASSUME_NONNULL_END
