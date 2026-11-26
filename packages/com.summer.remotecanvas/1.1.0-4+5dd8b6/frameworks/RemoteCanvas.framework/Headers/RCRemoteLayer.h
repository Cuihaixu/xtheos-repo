//
//  RCRemoteLayer.h
//  RemoteCanvas SDK
//
//  远程图层：在本进程里画，画面显示在画板上（所有 App 之上，触摸穿透）。
//  原理：本类在本进程建一块远程渲染上下文，把编号登记给画板；画板用图层托管把它嵌进自己的窗口，
//  之后每一帧由系统渲染服务直接合成，不再经过进程间通信。
//
//  生命周期跟对象走：创建即显示，对象释放（dealloc）即撤销、画面消失，没有 invalidate。
//  ⚠️ 必须强引用持有，否则会立刻被释放、画面一闪而过。只能在主线程使用。
//

#import <QuartzCore/QuartzCore.h>

NS_ASSUME_NONNULL_BEGIN

@interface RCRemoteLayer : NSObject

#pragma mark 创建

/// 创建远程图层并显示。失败返回 nil，error 说明原因（RCErrorDomain，如画板服务不可用）。
/// 每次调用都得到一个全新、独立的图层；创建几个由使用方决定，身份就是这个对象本身。
/// @param level 层级，越大越靠上；HUD 等公共组件始终在所有远程图层之上
+ (nullable instancetype)layerWithLevel:(NSInteger)level error:(NSError **)error;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

#pragma mark 实例

/// 往这里画：全屏，坐标为屏幕点坐标（竖屏，见 +canvasBounds）。可直接挂 CALayer，或 UIView 的 view.layer。
@property (nonatomic, readonly) CALayer *layer;

/// 唯一标识（UUID，创建时生成），打日志用
@property (nonatomic, readonly, copy) NSString *layerID;
/// 可选标签：只用于日志 / 调试（如画板日志、rcv 列表），不参与识别、允许重名
@property (nonatomic, copy, nullable) NSString *name;
/// 层级，改了立即生效
@property (nonatomic) NSInteger level;
/// 隐藏（本地生效，立即）。App 退到后台时 SDK 会自动隐藏，回前台按此值恢复。
@property (nonatomic, getter=isHidden) BOOL hidden;

#pragma mark 全局

/// 远程画板服务当前是否可用
@property (class, nonatomic, readonly) BOOL serviceAvailable;
/// 本进程当前存活的远程图层
@property (class, nonatomic, readonly) NSArray<RCRemoteLayer *> *layers;
/// 画布范围（屏幕点坐标，竖屏）
@property (class, nonatomic, readonly) CGRect canvasBounds;

@end

NS_ASSUME_NONNULL_END
