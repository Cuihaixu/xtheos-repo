//
//  RCPairActivity.h
//  RemoteCanvas SDK
//
//  双值对比：一行分左右两半，左「图标 · 值 · 单位」、右「值 · 单位 · 图标」，中间细分隔线；没有正文（title 只用于日志）。
//  适合 ↑上传 / ↓下载速度、成功 / 失败计数、A / B 两个账号。
//

#import <RemoteCanvas/RCRemoteActivity.h>
#import <RemoteCanvas/RCActivityIcon.h>

NS_ASSUME_NONNULL_BEGIN

/// 一边：图标 + 值 + 单位
@interface RCPairValue : NSObject <NSCopying>
+ (instancetype)valueWithIcon:(RCActivityIcon *)icon text:(NSString *)text unit:(nullable NSString *)unit;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
@property (nonatomic, readonly) RCActivityIcon *icon;
@property (nonatomic, readonly, copy) NSString *text;
@property (nonatomic, readonly, copy, nullable) NSString *unit;
@end

@interface RCPairActivity : RCRemoteActivity

- (instancetype)init;                                        ///< 自成一组
+ (instancetype)new;
- (instancetype)initWithGroup:(nullable RCActivityGroup *)group;

@property (nonatomic, copy, nullable) RCPairValue *leading;
@property (nonatomic, copy, nullable) RCPairValue *trailing;

@end

NS_ASSUME_NONNULL_END
