//
//  RCStatusActivity.h
//  RemoteCanvas SDK
//
//  状态：只挂一句话（当前账号、等待用户操作）。图标可以是系统图标、动画图标或一叠图标（头像堆叠），右侧一小段文字（用 detail，颜色同活动颜色）。
//  图标优先级：stackedIcons > iconAnimation > symbolName > 白点。
//

#import <RemoteCanvas/RCRemoteActivity.h>
#import <RemoteCanvas/RCActivityIcon.h>

NS_ASSUME_NONNULL_BEGIN

@interface RCStatusActivity : RCRemoteActivity

- (instancetype)init;                                        ///< 自成一组
+ (instancetype)new;
- (instancetype)initWithGroup:(nullable RCActivityGroup *)group;

/// SF Symbols 名（如 @"person.fill"）；nil 或名字无效时显示一个白点
@property (nonatomic, copy, nullable) NSString *symbolName;
/// 动画图标（白色画在活动颜色的圆底上）；设了就不用 symbolName
@property (nonatomic) RCActivityIconAnimation iconAnimation;
/// 头像堆叠：图标位置放前 3 个叠在一起的小圆，多的显示「+N」。每个可以是网络图片 / 系统图标 / 动画图标（各自的颜色）
@property (nonatomic, copy, nullable) NSArray<RCActivityIcon *> *stackedIcons;

@end

NS_ASSUME_NONNULL_END
