//
//  IRObjectScan.h — 目标对象扫描(IRSession 分类)(L1)
//
//  按类名在目标堆里枚举活实例(活块枚举 + isa 比对,跳僵尸块)。
//  纯【内存扫描】:不发远程调用、不需 worker,可在 worker 作用域外调(与内存原语同类)。
//  枚举出的 `IRObject` 为 `Weak` 持有(借用,可悬垂);要留存自行 `-strongReference`。
//

#import <IRemoteKit/IRSession.h>
#import <IRemoteKit/IRClassReference.h>   // id<IRClassReference>:类名 / 类对象统一输入

NS_ASSUME_NONNULL_BEGIN

@class IRObject;

/// `IRSession` 分类:按类名在目标堆枚举活实例(纯内存扫描,免 worker)。
@interface IRSession (IRObjectScan)

/// 枚举某类的活实例:**每找到一个回调一次**;把 `*stop` 置 `YES` 可提前结束。
/// - Parameters:
///   - className: 目标类名
///   - block: 每个实例回调一次——`object` 为实例句柄(`Weak`),`stop` 置 `YES` 提前停
- (void)enumerateInstancesOfClass:(NSString *)className
                       usingBlock:(void (^)(__kindof IRObject *object, BOOL *stop))block;

/// 某类的第一个活实例(枚举取首个即停)。
/// - Parameter className: 目标类名
/// - Returns: 实例句柄;无实例返回 `nil`
- (nullable __kindof IRObject *)firstInstanceOfClass:(NSString *)className;

/// 某类的所有活实例(枚举收集)。纯内存、免 worker,自动包含运行时动态子类(如 KVO `NSKVONotifying_*`)。
/// - Parameter className: 目标类名
/// - Returns: 实例句柄数组(可空)
- (NSArray<__kindof IRObject *> *)instancesOfClass:(NSString *)className;

#pragma mark - 多类一次遍历(走一遍堆认多个确切类)

/// 一次遍历堆枚举**多个类**的活实例:**每找到一个回调一次**,带回它属于输入里的**哪个类引用**。
/// 比逐类各走一遍堆省:只扫一遍、isa 比对走类地址集合、size 桶取并集仍过滤。
/// 编译期静态子类不自动计入;**运行时动态子类**(如 KVO `NSKVONotifying_*` / Aspects / RAC 等,通过 `class_rw_t`
/// `firstSubclass` 链表纯内存自动展开)默认自动包含并归并到原类下(若调用方同时显式传入动态类则显式项优先),免 worker。
/// - Parameters:
///   - classes: 类引用数组,元素可为 `NSString`(类名,走 `classNamed:` 解析)或 `IRObject`(类对象,直接取 `.addr`);
///              解析不到 / 重复的类自动跳过
///   - block: 每个实例回调一次——`cls` 为命中的那个**输入引用**(好对号),`object` 为实例句柄(`Weak`),`stop` 置 `YES` 提前停
- (void)enumerateInstancesOfClasses:(NSArray<id<IRClassReference>> *)classes
                         usingBlock:(void (^)(id<IRClassReference> cls, __kindof IRObject *object, BOOL *stop))block;

/// 一次遍历堆枚举多个类的活实例,按**类名**分好(运行时动态子类自动归并到原类名下)。
/// - Parameter classes: 类引用数组(同上;`NSString` 直接作名,`IRObject` 读一次 `className` 作名)
/// - Returns: `{类名: [实例句柄…]}`;某类无实例则不出现在字典里
- (NSDictionary<NSString *, NSArray<__kindof IRObject *> *> *)instancesOfClasses:(NSArray<id<IRClassReference>> *)classes;

@end

NS_ASSUME_NONNULL_END
