//
//  IRMutableArray.h — 远程可变数组句柄(纯内存读 count)
//
//  目标进程里的 `__NSArrayM` / `__NSFrozenArrayM` 创建句柄时用本类(+load 登记)。
//  count 读 storage.state.used(对象 +0x24),不调 -count、不缓存。
//  只这两类有这份 storage;其它 NSMutableArray 子类仍是 IRObject。
//

#import <IRemoteKit/IRObject.h>

NS_ASSUME_NONNULL_BEGIN

/// 远程 `__NSArrayM` / `__NSFrozenArrayM`。
@interface IRMutableArray : IRObject

/// 条目数。空数组为 0。每次现读内存,不缓存。
/// 读失败、`used > size`、或 `used ≥ 1<<24` 返回 `NSNotFound`(与 0 区分)。
@property (nonatomic, readonly) NSUInteger count;

@end

NS_ASSUME_NONNULL_END
