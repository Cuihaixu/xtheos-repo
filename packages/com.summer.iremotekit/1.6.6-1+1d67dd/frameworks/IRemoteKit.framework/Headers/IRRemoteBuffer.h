//
//  IRRemoteBuffer.h — 远程内存句柄(L1)
//
//  目标进程里的一块内存,自管【远程读/写/清空 + 释放】。与 `IRObject`(远程对象句柄)对仗:
//  IRObject 管远程对象生命周期,IRRemoteBuffer 管远程内存生命周期。**RAII**:句柄活着内存就在,
//  句柄 `dealloc`(或显式 `free`)才归还目标内存;可反复复用(`clear`/重写)。
//
//  **创建归 `IRSession` 实例**(见下方分类)——buffer 属于某个目标进程,由那个 session 开辟,
//  免得跨 session 混用(把 A 进程的 buffer 喂给 B 进程的调用)。
//
//  给 C 函数传【指针参】用:C 字符串 / 只读缓冲 / 出参缓冲,都先把字节放进这块内存,再把
//  `buffer.address` 当指针传给 `invoke:`——`invoke:` 只负责把地址塞进寄存器,不管这块内存的来源与生死。
//    IRRemoteBuffer *path = [session bufferWithCString:@"/path/lib.dylib"];
//    inv.arguments = @[ [IRValue valueWithUInt64:path.address], [IRValue valueWithInt64:2] ];  // dlopen
//  - Warning: 读写是跨进程 mach_vm(免 worker 作用域);`address` 是目标地址,不可本地解引用。
//

#import <IRemoteKit/IRTypes.h>
#import <IRemoteKit/IRSession.h>   // 下方在 IRSession 上定义创建分类,需完整接口

NS_ASSUME_NONNULL_BEGIN

@interface IRRemoteBuffer : NSObject

#pragma mark 身份

/// 目标进程内的基址(传参就用它:`[IRValue valueWithUInt64:buf.address]`)。释放后为 0。
@property (nonatomic, readonly) IRRemoteAddress address;
/// 容量(字节)。
@property (nonatomic, readonly) size_t size;

#pragma mark 远程读 / 写 / 清空(免 worker;越界即失败)

/// 读全部 `size` 字节;失败返回 `nil`。
- (nullable NSData *)readData;
/// 从 `offset` 读 `length` 字节;越界 / 失败返回 `nil`。
- (nullable NSData *)readDataWithLength:(size_t)length offset:(size_t)offset;
/// 把 `data` 写到 `offset` 处;越界 / 失败返回 `NO`。
- (BOOL)writeData:(NSData *)data offset:(size_t)offset;
/// 把 `bytes` 写到 `offset` 处;越界 / 失败返回 `NO`。
- (BOOL)writeBytes:(const void *)bytes length:(size_t)length offset:(size_t)offset;
/// 清零(memset 0)整块;复用前调用。
- (BOOL)clear;

#pragma mark 释放

/// 显式归还目标内存(幂等;借用视图 `bufferWrappingAddress:` 不归还)。`dealloc` 也会自动调。
- (void)free;

@end

#pragma mark - 创建(归 session 实例:buffer 属于某目标进程)

@interface IRSession (IRRemoteBuffer)

/// 在本目标里分配 `size` 字节 RW 内存(内容未清零);失败返回 `nil`。句柄释放时自动归还。
- (nullable IRRemoteBuffer *)bufferWithSize:(size_t)size;
/// 分配并写入一个 C 字符串(含结尾 `\0`);dlopen/dlsym 这类 `char*` 参直接用。
- (nullable IRRemoteBuffer *)bufferWithCString:(NSString *)cString;
/// 分配并写入 `data` 的裸字节(结构体指针 / 任意只读缓冲)。
- (nullable IRRemoteBuffer *)bufferWithData:(NSData *)data;
/// 包一个【已有的】目标地址(如从扫描/上次调用拿到),**借用视图,不接管释放**。
- (IRRemoteBuffer *)bufferWrappingAddress:(IRRemoteAddress)address size:(size_t)size;

@end

NS_ASSUME_NONNULL_END
