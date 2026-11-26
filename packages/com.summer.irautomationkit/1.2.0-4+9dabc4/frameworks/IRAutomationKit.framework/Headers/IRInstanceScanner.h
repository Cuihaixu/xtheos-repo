//
//  IRInstanceScanner.h
//  IRAutomationKit
//
//  实例扫描 (DESIGN.md 第 12 节): 按类名定期扫目标进程里的实例, 报生命周期 —— 新发现的 / 不在了的 / 每轮扫到的。
//  只报告, 不解析不存储: 解析成什么、存哪里由业务在回调里自己处理 (挂 object.userInfo (IRemoteKit setUserObject:forKey:) / 包装类 / 本地数据库 …)。
//
//  合并 (bd ir-1xj.10): 同一个 App 上的扫描器自动合在一起 —— 不管谁触发, 每轮都带上这个 App 全部扫描器的类, 只走一遍堆
//  (IRemoteKit instancesOfClasses:), 再按类分发。所以多建几个扫描器几乎不多花时间, 要分开处理的类就各建一个。
//
//  时机:
//  - start 就扫一轮; 回到前台先扫一轮; 应用重启 (会话变了) 上一轮的全部当作不在了
//  - 定时: 距离上一次扫到我 (谁触发的都算) 满 interval 秒扫一次 —— 任何一轮带上了我的类, 我的计时都清零
//  - 请求: scanNow / scanAfter: / scanTimeRangeMin:max:, 在哪都能调; 同一个 App 的请求一起排, 用最少的轮数满足
//    **能接受一个范围就给范围**: 范围里任何一轮 (别的扫描器的定时、别人的请求) 都算满足, 才能和别人合并;
//    scanNow / scanAfter: 是一个时刻, 基本都要单独扫一轮
//  - 页面变化: onPageChange: 设了就通知 (from → to), 要不要扫、什么时候扫在回调里调请求; 没设就不扫 (不搞后台默认自动扫)
//  - 定时和页面变化只在前台 (Active / Inactive); 请求只要有会话就扫
//
//  线程: 扫描在这个 App 共用的队列上; 回调 (observe: / onPageChange:) 投到各扫描器自己的串行队列, 一个回调慢不影响别的扫描器。
//  远程调用不用自己管 worker: 回调里直接调, 走 IRemoteKit 的隐式 worker 池。不占自动化的执行权、不调目标主线程。
//  扫的是类本身 + KVO 动态子类 NSKVONotifying_<类名> (IRemoteKit 只比确切的类; 被 KVO 换了类的实例算作这个类;
//  它的 className 是真实 isa (NSKVONotifying_X), 按类区分用 originalClassName);
//  扫到的先验活 + 原类名 (去掉 KVO 前缀) 再确认; 某个回调抛异常只影响它自己。
//  对象: 新发现的用 IRemoteKit cacheObjectAtAddress 取 (同址同壳, 核心钉住; 注册过子类 registerSubclass:forRemoteClass: 的就是子类实例),
//  挂在壳上的东西 (解析结果) 一直在, 直到报不在了 —— 回调之后 discardObjectAtAddress 解除钉住 (+ 清缓存池里的包装实例), 远程对象不动。
//  已知局限: 两轮之间地址被释放又分配给同类对象认不出 (还是同一个地址, 不报)。
//
//      IRInstanceScanner *scanner = [IRInstanceScanner scannerWithApplication:app classNames:@[@"AWEAwemeModel", @"AWEUserModel"]];
//      [scanner observe:IRInstanceEventAdded | IRInstanceEventRemoved block:^(IRInstanceEvent event, NSArray<IRObject *> *objects) {
//          for (IRObject *o in objects) {   // 几个类混在一起给, 看 o.originalClassName 区分 (className 可能是 NSKVONotifying_…)
//              if (event == IRInstanceEventAdded && [o.originalClassName isEqualToString:@"AWEAwemeModel"]) [o setUserObject:… forKey:@"json"];
//          }
//      }];
//      [scanner onPageChange:^(IRPageAgent *from, IRPageAgent *to) {
//          if ([to.pageID isEqualToString:@"search.result"]) [scanner scanNow];   // 要实时
//          else [scanner scanTimeRangeMin:0.5 max:3];                            // 不急: 0.5 ~ 3 秒里扫到就行
//      }];
//      [scanner start];
//

#import <Foundation/Foundation.h>
#import <IRemoteKit/IRemoteKit.h>

NS_ASSUME_NONNULL_BEGIN

@class IRApplication, IRPageAgent;

/// 生命周期 (可以组合订阅; 回调时 event 是单个值)
typedef NS_OPTIONS(NSUInteger, IRInstanceEvent) {
    IRInstanceEventAdded   = 1 << 0,   ///< 新发现的 (上一轮没有)
    IRInstanceEventRemoved = 1 << 1,   ///< 不在了 (已释放 / 应用重启; 只剩地址, 别再远程调它)
    IRInstanceEventScanned = 1 << 2,   ///< 这一轮扫到的全部 (含新发现的)
};

/// 回调: 每一轮里每种 event 最多一次, objects 不为空才回调 (Scanned 除外: 每轮都回调, 可能是空数组)。
/// 多个类的对象混在一起, 看 object.originalClassName 区分 (KVO 换过类的 className 是 NSKVONotifying_…)。在扫描器自己的队列上, 要远程调用直接调 (隐式 worker)
typedef void (^IRInstanceObserver)(IRInstanceEvent event, NSArray<IRObject *> *objects);

/// 页面变化 (页面识别发布的页面变了): from 上一个页面 (第一次可能为 nil), to 现在的页面。在扫描器自己的队列上
typedef void (^IRInstancePageChangeHandler)(IRPageAgent *_Nullable from, IRPageAgent *to);

@interface IRInstanceScanner : NSObject

+ (instancetype)scannerWithApplication:(IRApplication *)application className:(NSString *)className;
/// 多个类: 回调里混在一起给
+ (instancetype)scannerWithApplication:(IRApplication *)application classNames:(NSArray<NSString *> *)classNames;
@property (nonatomic, weak, readonly, nullable) IRApplication *application;
@property (nonatomic, copy, readonly) NSArray<NSString *> *classNames;

#pragma mark 时机

/// 定时间隔 (秒), 默认 10; 0 = 不定时。从上一次扫到我算起 (别的扫描器触发的那轮带上了我的类, 也从那时重新计时)
@property (nonatomic) NSTimeInterval interval;

- (void)start;
- (void)stop;
@property (atomic, readonly, getter=isRunning) BOOL running;

/// 页面变化时通知 (只在 running、前台时); 默认不自动扫, 要扫在回调里调请求 (如 scanTimeRangeMin:max:)。再设一次替换, nil = 取消监听
- (void)onPageChange:(nullable IRInstancePageChangeHandler)handler;

#pragma mark 请求 (异步, 马上返回; 时间从调用这一刻算; 没 start 也扫, 只要有会话)

/// 立即扫: 调用之后开始的一轮才算 (正在扫的那轮来不及加入, 扫完马上再扫一轮)
- (void)scanNow;
/// delay 秒后扫 (精准: 那之前的轮不算, 到点一定扫) = scanTimeRangeMin:delay max:delay
- (void)scanAfter:(NSTimeInterval)delay;
/// 不早于 min 秒、不晚于 max 秒扫一轮: 这段时间里开始的任何一轮都算 (谁触发的都行), 没有就在 max 时扫。
/// 尽量用它, 范围越宽越容易和别人合并成一轮。max < min 按 min
- (void)scanTimeRangeMin:(NSTimeInterval)min max:(NSTimeInterval)max;

#pragma mark 回调

/// 订阅: events 可以组合; 返回 token 给 removeObserver:
- (id)observe:(IRInstanceEvent)events block:(IRInstanceObserver)block;
- (void)removeObserver:(id)token;

#pragma mark 查询 (扫描器自己的记录, 不触发扫描、不远程调用; 要最新的先 scanNow)

/// 上一轮扫到的全部 (活着的)
@property (nonatomic, copy, readonly) NSArray<IRObject *> *objects;
/// 这个地址在不在上一轮的结果里: 在返回那个 IRObject, 不在 nil
- (nullable IRObject *)objectForAddress:(IRRemoteAddress)address;
/// 上一轮 (带上了我的类的) 什么时候扫完的 (没扫过 nil) / 一共扫了几轮 / 上一轮用了多久 (秒; 整轮的, 含别的扫描器的类)
@property (nonatomic, readonly, nullable) NSDate *lastScanDate;
@property (nonatomic, readonly) NSUInteger scanCount;
@property (nonatomic, readonly) NSTimeInterval lastScanDuration;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
