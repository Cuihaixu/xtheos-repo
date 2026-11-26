// 命名回放配置管理。全部为同步文件/IPC 操作；不暴露 CHR 自定义对象。
#import <Foundation/Foundation.h>
NS_ASSUME_NONNULL_BEGIN
#ifndef CHR_TOUCH_KIT_API
#define CHR_TOUCH_KIT_API __attribute__((visibility("default")))
#endif

CHR_TOUCH_KIT_API NSArray<NSString *> *CHRTouchPlaybackConfigurationFetchList(void);
CHR_TOUCH_KIT_API NSString *CHRTouchPlaybackConfigurationGetActive(void);
CHR_TOUCH_KIT_API BOOL CHRTouchPlaybackConfigurationSetActive(NSString *configurationId, NSError * _Nullable * _Nullable error);
CHR_TOUCH_KIT_API BOOL CHRTouchPlaybackConfigurationCreate(NSString *configurationId, NSString * _Nullable copyFromConfigurationId, NSError * _Nullable * _Nullable error);
CHR_TOUCH_KIT_API NSDictionary * _Nullable CHRTouchPlaybackConfigurationLoad(NSString *configurationId, NSError * _Nullable * _Nullable error);
/// 整批覆盖 noise 配置；字段仅允许 enabled / coordNoise / coordNoiseHz / roundDrift / timeJitterMs / speedJitter / releaseVelocityTolerance。
CHR_TOUCH_KIT_API BOOL CHRTouchPlaybackConfigurationUpdate(NSString *configurationId, NSDictionary *options, NSError * _Nullable * _Nullable error);
NS_ASSUME_NONNULL_END
