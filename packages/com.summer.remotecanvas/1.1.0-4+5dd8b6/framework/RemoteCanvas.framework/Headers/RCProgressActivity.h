//
//  RCProgressActivity.h
//  RemoteCanvas SDK
//
//  进度活动：知道总量、能算出完成多少（下载、上传、看视频 10/10）。
//  图标是进度环（会动的类型不能换图标，只能改颜色）；附加区显示 7/10 或 35%；整行底色按进度填充。
//  到总数不会自动结束（可能还要回退，如最后一片损坏重下）：只显示满格，结束由调用方 finish / fail。
//

#import <RemoteCanvas/RCRemoteActivity.h>

NS_ASSUME_NONNULL_BEGIN

@interface RCProgressActivity : RCRemoteActivity

/// 自成一组
- (instancetype)init;
+ (instancetype)new;
- (instancetype)initWithGroup:(nullable RCActivityGroup *)group;

@property (nonatomic, readonly) double completed;
@property (nonatomic, readonly) double total;

/// 更新进度。total 为 100 时附加区显示百分比，否则显示「已完成/总数」
- (void)setCompleted:(double)completed total:(double)total;

@end

NS_ASSUME_NONNULL_END
