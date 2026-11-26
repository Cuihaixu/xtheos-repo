//
//  RCSparklineActivity.h
//  RemoteCanvas SDK
//
//  迷你走势图：右侧一条小折线（淡色面积 + 线 + 最新点），显示最近 N 个值的走势；右边文字用 detail（单位、格式归使用方）。
//  适合 CPU、内存、FPS、网速、延迟这类实时读数。每来一个值调一次 addValue:。
//

#import <RemoteCanvas/RCRemoteActivity.h>

NS_ASSUME_NONNULL_BEGIN

@interface RCSparklineActivity : RCRemoteActivity

- (instancetype)init;                                        ///< 自成一组
+ (instancetype)new;
- (instancetype)initWithGroup:(nullable RCActivityGroup *)group;

/// 保留最近多少个点（2–120，默认 30）
@property (nonatomic) NSUInteger capacity;
@property (nonatomic, readonly) NSArray<NSNumber *> *values;
- (void)addValue:(double)value;
- (void)removeAllValues;

/// 纵向范围：默认按当前这些点的最小 / 最大值自动缩放；固定范围（如 0–100）用这个，自动用 setAutomaticRange
- (void)setRangeMinimum:(double)minimum maximum:(double)maximum;
- (void)setAutomaticRange;

/// 左侧系统图标（SF Symbols 名），默认 chart.xyaxis.line
@property (nonatomic, copy, nullable) NSString *symbolName;

@end

NS_ASSUME_NONNULL_END
