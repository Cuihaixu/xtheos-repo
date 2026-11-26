//
//  IRCache.h
//  IRAutomationKit
//
//  解析结果的缓存, 分三级 (省掉重复的慢解析, 如扫共享缓存符号表):
//  - 进程级: 只在内存, 进程退出就没了 (按会话 / 进程算出来的东西)
//  - 开机级: 落文件, 重启后作废 (和开机有关的: 共享缓存 slide 等)
//  - 系统级: 落文件, 系统更新 (build 号) 或换机型后作废 (共享缓存里固定的地址 / 偏移等)
//  文件在 /var/mobile/Library/Caches/IRAutomationKit/ (逻辑路径, 用 xtheos_jbroot 换成本环境的真实路径; root 和 mobile 进程共用); 值要是 plist 类型
//  (NSString / NSNumber / NSData / NSDate / NSArray / NSDictionary)。线程安全。
//
//      NSArray *found = [IRCache objectForKey:@"ax.symbols" scope:IRCacheScopeSystem compute:^id { return 扫符号表(); }];
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, IRCacheScope) {
    IRCacheScopeProcess = 0,   ///< 进程级 (内存)
    IRCacheScopeBoot,          ///< 开机级 (文件, 重启作废)
    IRCacheScopeSystem,        ///< 系统级 (文件, 系统 build / 机型变了作废)
};

@interface IRCache : NSObject

+ (nullable id)objectForKey:(NSString *)key scope:(IRCacheScope)scope;
/// object 为 nil = 删掉; 文件级的马上写盘
+ (void)setObject:(nullable id)object forKey:(NSString *)key scope:(IRCacheScope)scope;
/// 有就返回, 没有就调 compute 算 (同一个 key 不会并发算两次), 结果非 nil 才存
+ (nullable id)objectForKey:(NSString *)key scope:(IRCacheScope)scope compute:(id _Nullable (^)(void))compute;

/// 当前的作废标记: 开机级 = 开机时间, 系统级 = build 号 + 机型 (日志 / 测试用)
+ (NSString *)stampForScope:(IRCacheScope)scope;
/// 文件级缓存的文件路径 (进程级为 nil)
+ (nullable NSString *)pathForScope:(IRCacheScope)scope;
/// 清掉一级 (内存和文件都清; 测试用)
+ (void)removeAllObjectsInScope:(IRCacheScope)scope;

@end

NS_ASSUME_NONNULL_END
