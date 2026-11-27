//
//  IRRemotable.h — 远程句柄的线程语法糖声明(L1,纯声明头)
//
//  `.remote` 句柄上的 `.main` / `.child` 逐调用切线程(替代旧的第二访问器 `.remoteOnMain`)。
//  运行时由 `IRRemoteProxy` 自身实现(不 forward);本头只提供【编译期声明】,让 `.main`/`.child`
//  在句柄的静态类型上可见:
//    - App 私有类:业务协议继承 `IRRemotable`(genheader 生成时自动加 `<IRRemotable>`)→
//      `id<AWEAwemeModel> m = obj.remote; [m.main itemID];`
//    - 已知系统类:靠 `NSObject (IRRemote)` 分类 → `UIView *v = obj.remote; [v.main frame];`
//  `main`/`child` 返回 `instancetype`,链上保持句柄静态类型。
//  ⚠️ 仅对 `.remote` 代理有意义;在真本地对象上误调会崩(代理实现里对非法接收者抛异常)。
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// 远程句柄的线程限定词。业务协议继承它即可在 `id<协议>` 句柄上使用 `.main`/`.child`。
/// (返回 `id`:`id → 具体类型` 隐式转,不影响 `[m.main itemID]` 这类已声明选择子的调用。)
@protocol IRRemotable <NSObject>
/// 切换到主线程执行的同类句柄(逐调用生效)。
@property (nonatomic, readonly) id main;
/// 切换到子线程执行的同类句柄(默认)。
@property (nonatomic, readonly) id child;
@end

/// 让**已知系统类**(UIView / NSString …)的 `.remote` 句柄也能写 `.main`/`.child`。
/// 纯编译期声明;运行时由代理实现,真对象误调 → unrecognized selector。
@interface NSObject (IRRemote)
@property (nonatomic, readonly) id main;
@property (nonatomic, readonly) id child;
@end

NS_ASSUME_NONNULL_END
