//
//  CHRTouchKit.h
//  CHRTouchKit
//
//  伞头文件。
//

#import <Foundation/Foundation.h>

FOUNDATION_EXPORT double CHRTouchKitVersionNumber;
FOUNDATION_EXPORT const unsigned char CHRTouchKitVersionString[];

/// 此 SDK 编译时写入的产品版本（与生成该 SDK zip 的 Studio 版本一致）。
FOUNDATION_EXPORT NSString *CHRTouchSDKVersion(void);

/// 通过 chr:touch 同步读取 backboardd 插件编译时写入的客户端版本；服务未运行或通信失败时返回 nil。
FOUNDATION_EXPORT NSString * _Nullable CHRTouchClientVersion(void);

/// 真实探测 CHRTouch 服务是否可用：向 backboardd 插件发送版本查询并等待回包。
/// timeout 是总等待上限（秒），必须大于 0；建议传 2.0。服务、broker 或 backboardd 插件未加载时返回 NO。
FOUNDATION_EXPORT BOOL CHRTouchServiceIsAvailable(NSTimeInterval timeout);

#import <CHRTouchKit/CHRTouchProtocol.h>
#import <CHRTouchKit/CHRTouchPlayback.h>
#import <CHRTouchKit/CHRTouchManual.h>
#import <CHRTouchKit/CHRTouchNetworkLibrary.h>
#import <CHRTouchKit/CHRTouchPlaybackConfiguration.h>
