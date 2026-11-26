//
//  RCRemoteHUD.h
//  RemoteCanvas SDK
//
//  在画板上显示 HUD（显示在所有 App 之上，不挡触摸）。
//  本类只是接口：把调用打包成消息经通道发给画板（remotecanvasd），由画板用 MBProgressHUD 显示。
//  调用是异步的，立即返回；画板未运行时消息会丢弃。
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface RCRemoteHUD : NSObject

/// 显示文字 HUD，2 秒后自动消失
+ (void)showText:(NSString *)text;
/// 显示标题 + 详情，2 秒后自动消失
+ (void)showText:(NSString *)text details:(nullable NSString *)details;
/// 显示标题 + 详情，duration 秒后自动消失
+ (void)showText:(NSString *)text details:(nullable NSString *)details duration:(NSTimeInterval)duration;
/// 立即隐藏
+ (void)hide;

@end

NS_ASSUME_NONNULL_END
