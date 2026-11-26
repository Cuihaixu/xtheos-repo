//
//  RCActivityGroup.h
//  RemoteCanvas SDK
//
//  远程活动的组：一个组在面板上只占一行，组里的活动轮流用这一行（可理解为「只有一行的小面板」）。
//  组标识全局：标识相同即同一组，不同进程用同一标识共用一行。标识建议带前缀防撞名，如 @"com.x.下载"。
//  建一次存起来复用，避免每次手敲字符串。
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// 优先级：1–1000，数值大的先显示
typedef NSInteger RCActivityPriority;
static const RCActivityPriority RCActivityPriorityLow       = 1;      ///< 兜底：其他都不显示时才轮到
static const RCActivityPriority RCActivityPriorityNormal    = 500;    ///< 默认
static const RCActivityPriority RCActivityPriorityImportant = 1000;

@interface RCActivityGroup : NSObject

+ (instancetype)groupWithIdentifier:(NSString *)identifier;
- (instancetype)initWithIdentifier:(NSString *)identifier NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@property (nonatomic, readonly, copy) NSString *identifier;

/// 组的优先级：组和组之间抢面板的行时比较它（默认 500）。
/// 同一标识在多个进程设了不同值，取最大。组内显示谁由活动自己的优先级决定，与此无关。
@property (nonatomic) RCActivityPriority priority;

@end

NS_ASSUME_NONNULL_END
