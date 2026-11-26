//
//  IRNSObject.h
//  IRAutomationKit
//
//  系统类远程包装的根类 (SCROLL.md 第 5-7 项): 扮演 NSObject 的角色, 包着 IRemoteKit 的 IRObject。
//  继承关系与系统类一致, 命名 = IR + 系统类名: IRNSObject → IRUIView → IRUIScrollView → IRUITableView / IRUICollectionView …
//  - 创建 = 转换: +objectWithObject: 检查对象是不是这个系统类 (或子类), 不是返回 nil; 子类同理 (IRUIScrollView 只接 UIScrollView)
//  - 实例比较: isEqual: / hash = 同一会话 + 同一地址 (可放进 NSSet / 当字典 key)
//  - 读 ivar 的属性: 纯读内存, 任何线程都能调; 远程调用的方法带 diag, 头文件里单独标出
//  不持有远程对象 (同 IRObject 默认的弱持有): 对象可能已经释放, 用 isValidObject 确认
//
//      IRUIScrollView *list = [IRUIScrollView viewWithObject:object];          // 不是 UIScrollView → nil
//      IRUICollectionView *cv = [IRUICollectionView viewWithObject:list.object];   // 转成 collectionView
//

#import <Foundation/Foundation.h>
#import <IRemoteKit/IRemoteKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRNSObject : NSObject

/// 对应的系统类名 (IRNSObject = "NSObject", IRUIView = "UIView" …); 子类覆盖, 创建时按它检查
@property (class, nonatomic, readonly) NSString *systemClassName;

/// 包装一个远程对象: 不是 systemClassName (或子类) 的实例返回 nil; nil / 无效的 IRObject 返回 nil。纯读内存
+ (nullable instancetype)objectWithObject:(nullable IRObject *)object;
/// 同上, 按地址
+ (nullable instancetype)objectWithAddress:(IRRemoteAddress)address session:(IRSession *)session;

/// 映射 (DESIGN.md 第 12 节): 按远程对象的类挑最贴切的包装类 —— 沿远程父类链找第一个登记过的 (所有 IRNSObject 子类按
/// systemClassName 自动登记, 业务的也一样, 如 AWBAweme 的 systemClassName = "AWEAwemeModel"), 都没有 = IRNSObject。
/// 同一个远程对象返回同一个实例 (缓存池里, 随远程对象生灭, 会话结束释放), 包装类可以把懒加载的东西缓存在自己身上。
/// 对象无效 / 已经死了返回 nil。**临时**: 等 IRemoteKit 支持注册 IRObject 子类后, 包装类改继承 IRObject
///
///     IRUICollectionView *cv = [IRNSObject mappedObjectWithObject:object];   // CachalotCollectionView → IRUICollectionView
+ (nullable __kindof IRNSObject *)mappedObjectWithObject:(nullable IRObject *)object;
+ (nullable __kindof IRNSObject *)mappedObjectWithAddress:(IRRemoteAddress)address session:(IRSession *)session;

/// 远程对象身上挂的本进程内容 (= IRemoteKit 的 object.userInfo, 只读视图; 随远程对象生灭)。写用 setUserObject:forKey:
@property (nonatomic, readonly, nullable) NSDictionary *userInfo;
/// 挂 / 改 / 删 (obj = nil) (= [object setUserObject:forKey:]); 取 (= [object userObjectForKey:])
- (void)setUserObject:(nullable id)obj forKey:(NSString *)key;
- (nullable id)userObjectForKey:(NSString *)key;

/// 包着的远程对象 (读别的 ivar / 调别的方法用它)
@property (nonatomic, strong, readonly) IRObject *object;
@property (nonatomic, readonly) IRRemoteAddress address;
@property (nonatomic, weak, readonly, nullable) IRSession *session;

/// 原类名 (去掉 KVO / 监控库动态建的子类); 读不到为 nil。纯读内存, 按类缓存在会话上
@property (nonatomic, copy, readonly, nullable) NSString *className;
/// 是不是 className 这个类或它的子类 (纯读内存沿 superclass 链; 按类缓存在会话上)。不叫 isKindOfClass: (那是本地 NSObject 的)
- (BOOL)isKindOfClassNamed:(NSString *)className;
/// 远程对象是否还活着 (读 isa 校验, 见 -[IRObject isValidObject])
- (BOOL)isValidObject;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
