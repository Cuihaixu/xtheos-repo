//
//  IRTimerTask.h
//  IRAutomationKit
//
//  定时任务: 继承 IRTask, 用 GCD dispatch_source 定时器按间隔重复执行 main。
//

#import <IRAutomationKit/IRTask.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRTimerTask : IRTask

/// 间隔 (秒)
@property (nonatomic, readonly) NSTimeInterval interval;

/// 允许的误差 (秒), 默认 0
@property (nonatomic, assign) NSTimeInterval leeway;

/// 是否重复, 默认 YES; NO 时触发一次后 Completed
@property (nonatomic, readonly) BOOL repeats;

/// - Parameters:
///   - name: 任务名称
///   - interval: 间隔 (秒)
///   - repeats: 是否重复
- (instancetype)initWithName:(NSString *)name interval:(NSTimeInterval)interval repeats:(BOOL)repeats NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithName:(NSString *)name NS_UNAVAILABLE;

/// 间隔回调: 每次定时器触发调用一次 (子类重写)
/// - main 由 IRTimerTask 内部实现 (创建并启动定时器), 子类不要重写 main
/// - 挂起时定时器暂停, 恢复后重新计时
/// - 上一次未执行完时, 期间的触发合并, 不会重叠执行
- (void)timerDidFire;

@end

NS_ASSUME_NONNULL_END
