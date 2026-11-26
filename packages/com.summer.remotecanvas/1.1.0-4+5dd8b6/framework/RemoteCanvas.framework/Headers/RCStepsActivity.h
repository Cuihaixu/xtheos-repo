//
//  RCStepsActivity.h
//  RemoteCanvas SDK
//
//  步骤：一条串行流水线（自动化流程）。用 step: 领 token（带步骤名）——领了几个，总数就是几（可边走边领）；
//  start: 哪个 token，它就是下一步（按 start 的先后定第几步，不看领取顺序），前一步自动算完成。
//  这一行显示当前步骤名；右侧「当前/总数」；8 步以内分段环，超过 8 步连续进度环；全部走完停在 n/n，收起由使用方决定。
//  并行的流程用多个步骤活动（不同组同时显示 / 同组轮流显示）。
//

#import <RemoteCanvas/RCRemoteActivity.h>

NS_ASSUME_NONNULL_BEGIN

typedef NSInteger RCActivityStepToken;

@interface RCStepsActivity : RCRemoteActivity

- (instancetype)init;                                        ///< 自成一组
+ (instancetype)new;
- (instancetype)initWithGroup:(nullable RCActivityGroup *)group;

/// 领一个步骤 token（带步骤名）：总数 +1
- (RCActivityStepToken)step:(NSString *)title;
/// 开始这一步：它成为当前步骤（第 N 个 start 的就是第 N 步），前一步自动算完成。同一 token 再 start、或非本活动的 token：忽略
- (void)start:(RCActivityStepToken)token;
/// 当前步骤出错：图标变 !、右侧显示 message；下一次 start: 自动清除。传 nil 清除
- (void)error:(nullable NSString *)message;

@property (nonatomic, readonly) NSInteger stepCount;            ///< 总数（领了几个 token）
@property (nonatomic, readonly) NSInteger currentStepNumber;    ///< 当前第几步（0 = 还没开始）
@property (nonatomic, readonly, nullable) NSString *currentStepTitle;
@property (nonatomic, readonly, nullable) NSString *currentError;

@end

NS_ASSUME_NONNULL_END
