//
//  RCActivityIcon.h
//  RemoteCanvas SDK
//
//  一个圆形图标：网络图片 / 系统图标 / 动画图标三选一，再加圆底颜色。远程活动里需要「放一个图标」的地方都用它
//  （头像堆叠、双值对比的两边、动画图标）。都是白色内容画在彩色圆底上；网络图片铺满圆形。
//

#import <Foundation/Foundation.h>
#import <RemoteCanvas/RCRemoteActivity.h>

NS_ASSUME_NONNULL_BEGIN

/// 动画图标（画板自绘，循环播放）
typedef NS_ENUM(NSInteger, RCActivityIconAnimation) {
    RCActivityIconAnimationNone      = 0,
    RCActivityIconAnimationSpinner   = 1,    ///< 转圈：进行中
    RCActivityIconAnimationWave      = 2,    ///< 波形：录音、音频
    RCActivityIconAnimationBlink     = 3,    ///< 闪烁点：录屏、直播中
    RCActivityIconAnimationHalo      = 4,    ///< 呼吸光圈：监听、在线
    RCActivityIconAnimationTyping    = 5,    ///< 三点跳动：等待回复、思考中
    RCActivityIconAnimationRadar     = 6,    ///< 雷达扫描：搜索、扫描设备
    RCActivityIconAnimationHeartbeat = 7,    ///< 心跳：保活、心跳包
    RCActivityIconAnimationUpload    = 8,    ///< 上传流动
    RCActivityIconAnimationDownload  = 9,    ///< 下载流动
    RCActivityIconAnimationHourglass = 10,   ///< 沙漏翻转：排队、等待时机
    RCActivityIconAnimationSignal    = 11,   ///< 信号递增：连接中
    RCActivityIconAnimationOrbit     = 12,   ///< 轨道小球：同步、处理中
    RCActivityIconAnimationPulseLine = 13,   ///< 心电图：监控、健康检查
};

@interface RCActivityIcon : NSObject <NSCopying>

/// 网络图片：本进程下载（同一 URL 有缓存），最多等 3 秒；等待中画一个空圆底，没拿到用兜底图标 photo.on.rectangle
+ (instancetype)iconWithURL:(NSURL *)url;
/// 系统图标（SF Symbols 名）；color nil = 灰
+ (instancetype)iconWithSymbol:(NSString *)symbolName color:(nullable RCActivityColor)color;
/// 动画图标；color nil = 蓝
+ (instancetype)iconWithAnimation:(RCActivityIconAnimation)animation color:(nullable RCActivityColor)color;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@property (nonatomic, readonly, nullable) NSURL *url;
@property (nonatomic, readonly, nullable) NSString *symbolName;
@property (nonatomic, readonly) RCActivityIconAnimation animation;
@property (nonatomic, readonly, nullable) RCActivityColor color;

@end

NS_ASSUME_NONNULL_END
