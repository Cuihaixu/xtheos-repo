//
//  RCLoadingActivity.h
//  RemoteCanvas SDK
//
//  进行中：不知道要多久，只知道在进行（等待加载、同步）。图标转圈；右侧默认显示已用时（画板自己往上走）。
//  计时从第一次 show 开始；hide 不停止计时。
//

#import <RemoteCanvas/RCRemoteActivity.h>
#import <RemoteCanvas/RCActivityIcon.h>

NS_ASSUME_NONNULL_BEGIN

@interface RCLoadingActivity : RCRemoteActivity

- (instancetype)init;                                        ///< 自成一组
+ (instancetype)new;
- (instancetype)initWithGroup:(nullable RCActivityGroup *)group;

/// 右侧显示已用时（默认 YES）；设了 detail 时显示 detail
@property (nonatomic) BOOL showsElapsedTime;
/// 动画图标：默认转圈（None = 转圈）；可换成波形、闪烁点、呼吸光圈等
@property (nonatomic) RCActivityIconAnimation iconAnimation;
/// 计时起点（第一次 show 时）；未开始为 nil
@property (nonatomic, readonly, nullable) NSDate *startDate;
/// 已用时（秒）
@property (nonatomic, readonly) NSTimeInterval elapsedTime;
/// 计时清零重来：回调 Stopped → Started
- (void)restartTimer;

/// 计时回调：Started（开始）/ Running（每 milliseconds 毫秒；≤ 0 不回调）/ Stopped（finish / fail）；value = 已用时（秒）。
/// 回调在主线程；同一对象只保留最后设置的一个，传 nil 取消
- (void)observeTimerWithInterval:(NSInteger)milliseconds
                      usingBlock:(nullable void (^)(RCLoadingActivity *activity, RCActivityTimerState state, NSTimeInterval value))block;
/// 同上，回调在 queue（nil = 主线程）
- (void)observeTimerWithInterval:(NSInteger)milliseconds queue:(nullable dispatch_queue_t)queue
                      usingBlock:(nullable void (^)(RCLoadingActivity *activity, RCActivityTimerState state, NSTimeInterval value))block;

@end

NS_ASSUME_NONNULL_END
