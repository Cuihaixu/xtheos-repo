//
//  IRIvar.h — 远程对象的 ivar 访问器(L1)
//
//  绑定到某个 IRObject 实例的一个 ivar:带元数据(名/类型/偏移/大小),
//  并经 `value`(IRValue)真正读写目标内存。纯跨进程读写,免 worker。
//
//  由 `-[IRObject ivars]` / `-[IRObject ivarNamed:]` 产出,不单独构造。
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRValue;

/// 绑定到具体远程对象的一个 ivar 访问器:元数据 + 经 `value` 读写。
@interface IRIvar : NSObject

/// ivar 名(如 `"_tag"`)。
@property (nonatomic, readonly) NSString *name;
/// ObjC `@encode` 类型编码(如 `"q"` / `"@"` / `"{CGRect=dddd}"`)。
@property (nonatomic, readonly) NSString *typeEncoding;
/// 继承链上定义此 ivar 的类名(如 `"NSObject"`);解析不到为 `nil`。用于按定义类分段展示。
@property (nonatomic, readonly, nullable) NSString *definingClass;
/// 在宿主对象内的字节偏移。
@property (nonatomic, readonly) NSInteger offset;
/// 字节大小。
@property (nonatomic, readonly) NSUInteger size;
/// 此 ivar 在目标进程的绝对地址(宿主对象 `addr` + `offset`)。
@property (nonatomic, readonly) IRRemoteAddress address;

/// 读写此 ivar 的值。
/// - 读:按 `typeEncoding` 从目标读 `size` 字节装成 `IRValue`;失败返回 `nil`。
/// - 写:把 `IRValue` 的字节写回 `offset`(纯字节写)。**写对象 ivar 时,其生命周期由调用方负责**
///   (IRValue 不 retain 目标对象);写宽度取 `min(size, value.data.length)`。
@property (nonatomic, strong, nullable) IRValue *value;

@end

NS_ASSUME_NONNULL_END
