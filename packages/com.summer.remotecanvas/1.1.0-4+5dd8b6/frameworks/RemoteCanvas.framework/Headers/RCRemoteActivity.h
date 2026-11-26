//
//  RCRemoteActivity.h
//  RemoteCanvas SDK
//
//  远程活动：一件正在进行的事（进度、计时、步骤……），显示在屏幕 3/4 处的面板上，在所有 App 之上、不挡触摸。
//  这是基类，不能直接创建；用具体类型的子类（如 RCProgressActivity），对象本身就是句柄：
//
//      RCActivityGroup *downloads = [RCActivityGroup groupWithIdentifier:@"com.x.下载"];
//      RCProgressActivity *a = [[RCProgressActivity alloc] initWithGroup:downloads];
//      a.title = @"下载 A";
//      [a setCompleted:35 total:100];
//      [a show];
//      ...
//      [a finish];      // 或释放对象 = 撤销
//
//  显示规则（画板统一分配）：面板最多 N 行（设备配置，归用户）。两层抢占，规则相同——优先级高的先显示，同级先到先得，抢不过就等待：
//  ① 组和组抢面板的行，比较组的优先级；② 组里的活动抢这一组那一行，比较活动 show 时的优先级。
//  show 不会失败：抢不过就等待，等待中照常更新，有位置了自动补上。是否真的在屏幕上看 visible。
//  显示多久由调用方 / 各类型自己决定（show、hide、showForDuration:），核心不设超时。
//  所有方法在主线程调用（不在主线程时自动转到主线程）。
//

#import <Foundation/Foundation.h>
#import <RemoteCanvas/RCActivityGroup.h>

NS_ASSUME_NONNULL_BEGIN

/// 颜色：系统色名，跨进程传名字，画板用系统色绘制、深浅模式自动适配
typedef NSString *RCActivityColor NS_TYPED_EXTENSIBLE_ENUM;
FOUNDATION_EXPORT RCActivityColor const RCActivityColorBlue;
FOUNDATION_EXPORT RCActivityColor const RCActivityColorIndigo;
FOUNDATION_EXPORT RCActivityColor const RCActivityColorPurple;
FOUNDATION_EXPORT RCActivityColor const RCActivityColorPink;
FOUNDATION_EXPORT RCActivityColor const RCActivityColorRed;
FOUNDATION_EXPORT RCActivityColor const RCActivityColorOrange;
FOUNDATION_EXPORT RCActivityColor const RCActivityColorYellow;
FOUNDATION_EXPORT RCActivityColor const RCActivityColorGreen;
FOUNDATION_EXPORT RCActivityColor const RCActivityColorTeal;
FOUNDATION_EXPORT RCActivityColor const RCActivityColorGray;

/// 使用的生命周期
typedef NS_ENUM(NSInteger, RCActivityState) {
    RCActivityStateIdle   = 0,   ///< 刚创建，还没 show 过（画板不知道它）
    RCActivityStateActive = 1,   ///< 已登记到画板（show / hide 来回都算）
    RCActivityStateEnded  = 2,   ///< 终点：之后的调用一律忽略；要再显示就新建对象
};

/// 计时器状态（进行中 / 倒计时的计时回调用）
typedef NS_ENUM(NSInteger, RCActivityTimerState) {
    RCActivityTimerStateStarted = 0,   ///< 开始计时
    RCActivityTimerStateRunning = 1,   ///< 进行中：按间隔回调
    RCActivityTimerStateStopped = 2,   ///< 停止：倒计时到 0 / finish / fail
};

typedef NS_ENUM(NSInteger, RCActivityEndReason) {
    RCActivityEndReasonNone      = 0,   ///< 未结束
    RCActivityEndReasonFinished  = 1,   ///< 调用了 finish
    RCActivityEndReasonFailed    = 2,   ///< 调用了 fail
};

@interface RCRemoteActivity : NSObject

/// 基类不能直接创建：用子类的 init（自成一组）/ initWithGroup:
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)initWithGroup:(nullable RCActivityGroup *)group NS_UNAVAILABLE;

/// 所在的组；nil = 自成一组（此时 show 的优先级也当作组的优先级）
@property (nonatomic, readonly, nullable) RCActivityGroup *group;

/// 正文（超长时跑马灯滚动）
@property (nonatomic, copy, nullable) NSString *title;
/// 颜色；nil 用该类型的默认色
@property (nonatomic, copy, nullable) RCActivityColor tintColor;
/// 右侧文字：设了就替换该类型默认的右侧内容（如进度的 7/10、进行中的已用时）；nil 恢复默认
@property (nonatomic, copy, nullable) NSString *detail;

/// 警告：还在进行但出了问题（如「重试中」）。图标变成 !，附加区显示 warningText；设回 NO 恢复
@property (nonatomic, getter=isWarning) BOOL warning;
@property (nonatomic, copy, nullable) NSString *warningText;

#pragma mark 显示

/// 请求显示（普通优先级 500）。第一次 show 时登记到画板
- (void)show;
/// 带优先级请求显示（1–1000）
- (void)showWithPriority:(RCActivityPriority)priority;
/// 请求显示，duration 秒后自动隐藏（画板计时，使用方进后台也准）
- (void)showForDuration:(NSTimeInterval)duration;
- (void)showWithPriority:(RCActivityPriority)priority duration:(NSTimeInterval)duration;
/// 不显示了，但没结束，可以再 show
- (void)hide;

/// 现在是否真的在屏幕上（show 只是请求，满了会等待）
@property (nonatomic, readonly, getter=isVisible) BOOL visible;
/// visible 变化时回调（主线程）
@property (nonatomic, copy, nullable) void (^visibilityHandler)(__kindof RCRemoteActivity *activity, BOOL visible);

#pragma mark 结束

/// 完成：这一行退场。（一次性提示做好后会自动弹出结果）
- (void)finish;
/// 失败：这一行退场
- (void)fail;
- (void)failWithReason:(nullable NSString *)reason;

#pragma mark 状态

@property (nonatomic, readonly) RCActivityState state;
@property (nonatomic, readonly) RCActivityEndReason endReason;
/// 结束时回调（finish / fail；主线程）
@property (nonatomic, copy, nullable) void (^endHandler)(__kindof RCRemoteActivity *activity, RCActivityEndReason reason);

/// 活动标识（画板侧用）
@property (nonatomic, readonly, copy) NSString *activityID;

@end

NS_ASSUME_NONNULL_END
