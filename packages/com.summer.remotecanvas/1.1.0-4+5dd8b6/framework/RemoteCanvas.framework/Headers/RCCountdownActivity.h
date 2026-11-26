//
//  RCCountdownActivity.h
//  RemoteCanvas SDK
//
//  倒计时：知道还剩多久（冷却、限时等待）。图标圆环随剩余时间缩短，右侧剩余时间，都由画板自己往下走；
//  到 0 停住（00:00、环空），这一行留着，什么时候收起由使用方决定（finish / hide）。
//

#import <RemoteCanvas/RCRemoteActivity.h>

NS_ASSUME_NONNULL_BEGIN

@interface RCCountdownActivity : RCRemoteActivity

- (instancetype)init;                                        ///< 自成一组
+ (instancetype)new;
- (instancetype)initWithGroup:(nullable RCActivityGroup *)group;

/// 设时长：从现在开始倒数（重设即重新开始：回调 Stopped → Started）
@property (nonatomic) NSTimeInterval duration;
/// 或者直接设截止时间：从现在倒数到它
@property (nonatomic, copy, nullable) NSDate *endDate;
/// 剩余时间（秒），到 0 后为 0
@property (nonatomic, readonly) NSTimeInterval remainingTime;

/// 计时回调：Started / Running（每 milliseconds 毫秒；≤ 0 不回调）/ Stopped（到 0、finish、fail）；value = 剩余时间（秒）。
/// 回调在主线程；同一对象只保留最后设置的一个，传 nil 取消
- (void)observeTimerWithInterval:(NSInteger)milliseconds
                      usingBlock:(nullable void (^)(RCCountdownActivity *activity, RCActivityTimerState state, NSTimeInterval value))block;
- (void)observeTimerWithInterval:(NSInteger)milliseconds queue:(nullable dispatch_queue_t)queue
                      usingBlock:(nullable void (^)(RCCountdownActivity *activity, RCActivityTimerState state, NSTimeInterval value))block;

@end

NS_ASSUME_NONNULL_END
