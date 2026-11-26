//
//  RCRemoteToast.h
//  RemoteCanvas SDK
//
//  一次性提示：从屏幕顶端滑下，停几秒自己消失，在所有 App 之上、不挡触摸（不能交互）。
//  发出去就不用管：没有句柄、没有状态；所有进程的提示排成一个队列，一条接一条显示。
//  样子自动决定：没设详情 = 紧凑（图标 · 标题 · 副标题），设了详情 = 展开（大图标 · 标题 · 副标题 · 详情）。
//
//      [RCRemoteToast showSuccess:@"已复制回复" subtitle:nil detail:nil];
//
//      [RCRemoteToast show:^(RCToastContent *c) {
//          c.style    = RCToastStyleFailure;
//          c.title    = @"上传截图失败";
//          c.subtitle = @"已重试 3 次";
//          c.detail   = @"网络超时";
//      } completion:^(BOOL displayed) { ... }];
//
//  可以在任意线程调用。
//

#import <Foundation/Foundation.h>
#import <RemoteCanvas/RCRemoteActivity.h>

NS_ASSUME_NONNULL_BEGIN

/// 预设图标（带出现动画）
typedef NS_ENUM(NSInteger, RCToastStyle) {
    RCToastStyleInfo    = 0,   ///< 蓝 i（默认）
    RCToastStyleSuccess = 1,   ///< 绿 ✓
    RCToastStyleFailure = 2,   ///< 红 ✕
    RCToastStyleWarning = 3,   ///< 橙 !
};

/// 一条提示的内容。show 时会复制一份，之后改它不影响已经发出去的那条，可以当模板反复用
@interface RCToastContent : NSObject <NSCopying>
@property (nonatomic) RCToastStyle style;
/// 网络图标：设了就先下载（SDK 在本进程下载，同一 URL 有缓存），最多等 3 秒——拿到就用这张图（裁成圆形铺满）；
/// 超时 / 失败（不管什么原因）用兜底图标（图片图标 photo.on.rectangle，圆底默认灰，设了 tintColor 用它的颜色）。
/// 等待期间不占全局队列；弹出后图标不再变。优先级：iconURL > symbolName > style
@property (nonatomic, copy, nullable) NSURL *iconURL;
/// 系统图标（SF Symbols 名）：设了就用它（静止），不用 style 的动画图标；名字无效画白点
@property (nonatomic, copy, nullable) NSString *symbolName;
@property (nonatomic, copy) NSString *title;
/// 紧凑时在右侧，展开时在标题同一行右上
@property (nonatomic, copy, nullable) NSString *subtitle;
/// 设了就展开显示（第二行，最多两行）
@property (nonatomic, copy, nullable) NSString *detail;
/// 缩略图（展开时右侧 64pt 圆角小图，如截图预览）。设了就展开显示（即使没有 detail）。
/// thumbnailData 优先（本地图片数据，如截图）；thumbnailURL 按网络图标同样的规则下载（最多等 3 秒，没拿到就不显示这块）
@property (nonatomic, copy, nullable) NSURL *thumbnailURL;
@property (nonatomic, copy, nullable) NSData *thumbnailData;
/// 颜色；nil = style 的颜色（系统图标默认灰）
@property (nonatomic, copy, nullable) RCActivityColor tintColor;
/// 停留秒数；0 = 默认（紧凑 2.5、展开 3.5）。后面有排队的提示时，最少停 1 秒就提前收起
@property (nonatomic) NSTimeInterval duration;
@end

@interface RCRemoteToast : NSObject

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

/// 填内容并显示。configure 在调用线程同步执行；completion 在提示离开屏幕后回调（主线程），
/// displayed = NO 表示没显示出来（画板不可用，或排队时画板重启丢了）
+ (void)show:(void (NS_NOESCAPE ^)(RCToastContent *content))configure
  completion:(nullable void (^)(BOOL displayed))completion;

+ (void)showContent:(RCToastContent *)content;
+ (void)showContent:(RCToastContent *)content completion:(nullable void (^)(BOOL displayed))completion;

+ (void)showSuccess:(NSString *)title subtitle:(nullable NSString *)subtitle detail:(nullable NSString *)detail;
+ (void)showFailure:(NSString *)title subtitle:(nullable NSString *)subtitle detail:(nullable NSString *)detail;
+ (void)showWarning:(NSString *)title subtitle:(nullable NSString *)subtitle detail:(nullable NSString *)detail;
+ (void)showInfo:(NSString *)title subtitle:(nullable NSString *)subtitle detail:(nullable NSString *)detail;

@end

NS_ASSUME_NONNULL_END
