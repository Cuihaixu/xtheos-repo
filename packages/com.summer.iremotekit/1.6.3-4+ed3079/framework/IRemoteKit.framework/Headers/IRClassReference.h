//
//  IRClassReference.h — "指代一个类"的统一输入(L1)
//
//  多实例扫描 `instancesOfClasses:` 的元素类型:既可以传**类名字符串**(`NSString`,走
//  `classNamed:` 解析,命中进程级缓存),也可以传**已解析的类对象**(`IRObject`,直接取 `.addr`)。
//  两者都遵守本协议,扫描内部多态解析成"类地址"(= 实例 isa 比对的目标),到底层归一成一种。
//
//  动态类(如 KVO 的 `NSKVONotifying_X`)不在静态 classlist、按名解析不到:由**上层**自己解析成
//  类对象传下来(走 `IRObject` 入口),核心直接用 `.addr`、不关心它是不是动态子类。
//

#import <IRemoteKit/IRObject.h>   // 为 IRObject (IRClassReference) 分类(需完整类定义;顺带带入 IRTypes/Foundation)

NS_ASSUME_NONNULL_BEGIN

@class IRSession;

/// 能指代目标进程里"一个类"的东西:类名(`NSString`)或已解析的类对象(`IRObject`)。
@protocol IRClassReference <NSObject>
/// 在会话里解析出该类的远程地址(类对象地址,即实例 isa 比对的目标);解析不到返回 `0`。
- (IRRemoteAddress)ir_resolveRemoteClassInSession:(IRSession *)session;
@end

/// 类名字符串作类引用:走 `IRSession -addressForClassNamed:`(进程级缓存;静态类)。
@interface NSString (IRClassReference) <IRClassReference>
@end

/// `IRObject` 作类引用:直接取 `.addr`(约定调用方传的是**类对象**)。
@interface IRObject (IRClassReference) <IRClassReference>
@end

NS_ASSUME_NONNULL_END
