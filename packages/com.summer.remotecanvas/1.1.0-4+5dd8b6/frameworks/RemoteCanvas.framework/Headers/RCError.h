//
//  RCError.h
//  RemoteCanvas SDK
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// RemoteCanvas SDK 的错误域
FOUNDATION_EXPORT NSErrorDomain const RCErrorDomain;

typedef NS_ERROR_ENUM(RCErrorDomain, RCErrorCode) {
    RCErrorServiceUnavailable = 1,   ///< 远程画板服务不可用（remotecanvasd 未运行 / 未就绪）
    RCErrorLayerCreationFailed = 2,  ///< 本进程无法创建远程图层（渲染服务拒绝 / 私有接口不可用）
    RCErrorSendFailed = 3,           ///< 消息发送失败
    RCErrorNotMainThread = 4,        ///< 需要在主线程调用
};

NS_ASSUME_NONNULL_END
