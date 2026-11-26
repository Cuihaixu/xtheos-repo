//
//  IRValue.h — 跨进程「带类型的值」装箱(L1)
//
//  统一标量 / 浮点 / 结构体 / OC 对象:目标内存里一段带 @encode 类型的字节。
//  当前只用于 ivar 读写(`IRIvar.value`);API 稳定后拟共用到调用参数 / 返回值。
//
//  对象类型('@'/'#')装的是【远程句柄 IRObject】—— IRValue 不管其生命周期:
//  写对象 ivar 前,调用方须自行 retain 好传入的对象(见 IRIvar 写入约束)。
//

#import <IRemoteKit/IRTypes.h>

NS_ASSUME_NONNULL_BEGIN

@class IRObject, IRRemoteBuffer;

/// 目标进程里一个带类型的值(标量 / 浮点 / 结构体 / OC 对象),按 `typeEncoding` 装箱与转换。
@interface IRValue : NSObject

/// ObjC `@encode` 风格类型编码:如 `"q"` / `"Q"` / `"d"` / `"@"` / `"{CGRect=dddd}"`。
@property (nonatomic, readonly) NSString *typeEncoding;

#pragma mark - 装箱(写)

+ (IRValue *)valueWithInt64:(int64_t)value;
+ (IRValue *)valueWithUInt64:(uint64_t)value;
+ (IRValue *)valueWithBool:(BOOL)value;
+ (IRValue *)valueWithDouble:(double)value;
/// 装一个远程对象('@')。生命周期由调用方负责(IRValue 不 retain 目标对象)。
+ (IRValue *)valueWithObject:(nullable IRObject *)object;
/// 装一个远程内存句柄作【指针参】(`^v`,传 `buffer.address`)。IRValue 强持有 buffer,保其内存活到调用完成——
/// 免调用方手写 `valueWithUInt64:buf.address`。给 C 函数传 C 字符串 / 缓冲 / 出参用。
+ (IRValue *)valueWithRemoteBuffer:(IRRemoteBuffer *)buffer;
/// 装任意字节(结构体 / 自定义宽度),`type` 为其 `@encode`。
+ (IRValue *)valueWithBytes:(const void *)bytes length:(NSUInteger)length typeEncoding:(NSString *)type;

#pragma mark - 拆箱(读,按需转换)

/// 原始字节取低 64 位作有符号整数(不足 8 字节按实际宽度)。
@property (nonatomic, readonly) int64_t int64Value;
/// 原始字节取低 64 位作无符号整数。
@property (nonatomic, readonly) uint64_t uint64Value;
/// 首字节非 0 即 `YES`。
@property (nonatomic, readonly) BOOL boolValue;
/// 作浮点:`typeEncoding` 为 `f` 按 float 解,否则按 double 解。
@property (nonatomic, readonly) double doubleValue;
/// 作远程对象('@'/'#'):把内含指针包成句柄(借用视图,Weak);非对象 / 无会话返回 `nil`。
@property (nonatomic, readonly, nullable) IRObject *objectValue;
/// 原始字节(结构体 / 任意宽度按此取)。
@property (nonatomic, readonly) NSData *data;

@end

NS_ASSUME_NONNULL_END
