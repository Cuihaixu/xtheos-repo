//
//  IRScrollMonitor.h
//  IRAutomationKit
//
//  滚动监视 (SCROLL.md 第 3、4、10 项): 在我们自己进程的采样线程上按刷新率读滚动视图的 ivar (纯读内存, 一帧 ~10µs),
//  认出 按着 → 松手 (拿到惯性终点) → 停稳, 以及"偏移越过某个位置"。调用方 (导航器 / 页面事件) 在自己的线程上等这些事件。
//  只读, 不碰界面: 点停由调用方在收到 IRScrollEventReached 后自己点。
//
//      IRScrollMonitor *monitor = [IRScrollMonitor monitorWithScrollView:list];
//      [monitor start];
//      [monitor armAtOffset:2400 axis:IRScrollAxisVertical];          // 偏移越过 2400 时报 Reached
//      app.playback(plan, diag);
//      IRScrollEvent event = [monitor waitForEvents:IRScrollEventReleased | IRScrollEventReached timeout:3];
//      if (event == IRScrollEventReleased && 惯性终点 monitor.releaseMotion.decelerationTarget 还没到) { 等停稳 }
//      [monitor stop];
//

#import <Foundation/Foundation.h>
#import <IRAutomationKit/IRUIScrollView.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, IRScrollAxis) {
    IRScrollAxisVertical = 0,
    IRScrollAxisHorizontal = 1,
};

/// 滚动阶段 (每帧按 tracking / decelerating 判断)
typedef NS_ENUM(NSInteger, IRScrollPhase) {
    IRScrollPhaseIdle = 0,           ///< 静止
    IRScrollPhaseTracking = 1,       ///< 手指按着 (拖动中)
    IRScrollPhaseDecelerating = 2,   ///< 松手后惯性 / 翻页动画
};

/// 事件 (可按位组合着等)
typedef NS_OPTIONS(NSUInteger, IRScrollEvent) {
    IRScrollEventNone = 0,
    IRScrollEventTouched = 1 << 0,   ///< 手指按下 (进入 tracking)
    IRScrollEventReleased = 1 << 1,  ///< 松手: 抬手后第一帧惯性时报 (那一帧 UIKit 才算好惯性终点, 在 releaseMotion.decelerationTarget); 没有惯性的在停稳时一起报
    IRScrollEventSettled = 1 << 2,   ///< 停稳: 连续几帧没按着、没惯性、偏移不变 (动过之后才报; 没动过的静止不算)
    IRScrollEventReached = 1 << 3,   ///< 偏移越过 armAtOffset: 设的位置 (按速度提前 lead 秒判断)
    IRScrollEventTimeout = 1 << 4,   ///< 等待超时 (只作 waitForEvents 的返回值)
    IRScrollEventLost = 1 << 5,      ///< 读不到了 (滚动视图释放 / 应用退出), 采样线程已停
};

@interface IRScrollMonitor : NSObject

+ (instancetype)monitorWithScrollView:(IRUIScrollView *)scrollView;
@property (nonatomic, strong, readonly) IRUIScrollView *scrollView;

/// 采样频率 (次/秒), 默认 120; start 之前设。按固定网格走拍, 落后就丢拍 (见 droppedFrames)
@property (nonatomic) double frequency;
/// 停稳要连续几帧不动, 默认 6 (120Hz 下 50ms; 太少会把松手和惯性之间的空档当成停稳)
@property (nonatomic) NSUInteger settleFrames;

/// 开始采样 (起一条采样线程); 第一帧读不到返回 NO。重复调用无效
- (BOOL)start;
/// 停止采样 (等采样线程退出)。dealloc 时自动停
- (void)stop;
@property (nonatomic, readonly, getter=isRunning) BOOL running;

/// 最新一帧 / 当前阶段 / 已采样帧数
@property (nonatomic, readonly) IRScrollMotion motion;
@property (nonatomic, readonly) IRScrollPhase phase;
@property (nonatomic, readonly) NSUInteger frames;
/// 丢掉的拍数: 节拍固定在 开始时间 + n × 周期 的网格上, 一帧处理超过一拍 (读慢了 / 线程被抢占) 时中间的拍直接丢掉,
/// 不排队、不补帧、不连读 (宁可帧率降下来, 不让采样堆积)
@property (nonatomic, readonly) NSUInteger droppedFrames;
/// 最近一次松手那一帧; 惯性中 decelerationTarget 会跟着 UIKit 更新 (惯性和终点谁先更新不固定, 报松手时最多等 15 帧),
/// 要用终点就读这里的最新值。不在惯性中 = 松手就停, 没有惯性; 还没松过手时全 0
@property (nonatomic, readonly) IRScrollMotion releaseMotion;
/// 读一帧要多久 (µs, 最近一次)
@property (nonatomic, readonly) double lastReadMicroseconds;

/// 设一个位置: 偏移越过它时报 Reached (再设覆盖; 报过一次就失效)。方向按设置时的偏移判断 (目标在前面 = 往偏移变大的方向越过)。
/// 手指按着时不判断 (也点不停)。lead: 提前量, 秒 (抵掉点击的延迟 / 人的反应时间), 默认 1/60:
/// 惯性中按指数衰减预测 lead 秒后的位置 (终点 D 已知: D − (D − x)·e^(−k·lead), k = 速度 / (D − x)), 越过就报
- (void)armAtOffset:(CGFloat)offset axis:(IRScrollAxis)axis;
- (void)armAtOffset:(CGFloat)offset axis:(IRScrollAxis)axis lead:(NSTimeInterval)lead;
- (void)disarm;

/// 清掉已发生还没被等到的事件 (每次放手势前调, 免得拿到上一次的)
- (void)resetEvents;
/// 等 events 里任一事件发生 (已经发生过且没被清的立即返回), 返回发生的那个 (同时发生按 Lost > Reached > Released > Settled > Touched);
/// 超时返回 Timeout。拿到的事件会被清掉
- (IRScrollEvent)waitForEvents:(IRScrollEvent)events timeout:(NSTimeInterval)timeout;

@end

NS_ASSUME_NONNULL_END
