//
//  IRApplication+IRTouch.h
//  IRAutomationKit
//
//  触摸操作: 点击 / 长按 / 滑动 / 回放, 全部走 CHRTouchKit (OPERATION.md 第 4 节, ACTION.md 第 17、19、20 项)。
//  - 坐标: 屏幕 points, 竖屏固定坐标空间, 原点左上 (同 CHRTouchKit 公开接口, 归一化由 CHRTouchKit 内部完成); 超出屏幕直接失败
//  - 都是写操作: 只能在持有执行权的 UI 任务 (自动化 / 中断) 线程上调用, 否则返回 NO; 应用不是 Active 也返回 NO (触摸是全局注入的)
//  - 同步执行: 手势做完才返回; 整个手势在不可挂起区里 (中断只会落在两个手势之间, 不会留下按住的手指)
//  - 调用前过检查点: 已取消 / 应用退出时抛中止
//  - 返回 YES 只表示手势已投递, 不代表点中了想点的东西: 效果用之后的识别确认 (如等页面变化)
//  - 诊断显式传入: 失败原因写进 diag; 传 nil = 不关心结果, 错误照样打日志
//  - 滑动是兜底: 自己用 Down / Move / Up 拼的轨迹不如真实录制, 能录的手势优先用回放
//
//      app.tap(100, 100, diag);
//      app.longPress(100, 100, 1.5, diag);
//      app.swipe(200, 600, 200, 200, 0.3, diag);
//      app.playback(@"6F1C…", diag);
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <IRAutomationKit/IRApplication.h>
#import <IRAutomationKit/IRDiagnostics.h>

NS_ASSUME_NONNULL_BEGIN

@interface IRApplication (IRTouch)

#pragma mark 底层

/// 点击 (CHRTouchTap: 约 56–100ms, 带同点微移采样)
/// - Parameters:
///   - point: 屏幕 points
///   - diag: 诊断, 可为 nil
/// - Returns: 是否已投递
- (BOOL)tapAt:(CGPoint)point diag:(nullable IRDiagnostics *)diag;

/// 长按 (CHRTouchLongPress: 按住期间带微漂移)
/// - Parameters:
///   - point: 屏幕 points
///   - duration: 按住时长 (秒), 必须 > 0
///   - diag: 诊断, 可为 nil
/// - Returns: 是否已投递
- (BOOL)longPressAt:(CGPoint)point duration:(NSTimeInterval)duration diag:(nullable IRDiagnostics *)diag;

/// 滑动 (兜底): from 按下, 沿轨迹移到 to, 抬起
/// - Parameters:
///   - from: 起点, 屏幕 points
///   - to: 终点, 屏幕 points
///   - duration: 从按下到抬起的时长 (秒), 必须 > 0
///   - diag: 诊断, 可为 nil
/// - Returns: 是否已投递
- (BOOL)swipeFrom:(CGPoint)from to:(CGPoint)to duration:(NSTimeInterval)duration diag:(nullable IRDiagnostics *)diag;

/// 回放 (推荐): 手机上 CHRTouchKit App 录制的回放方案或收藏库里的单条手势, 自带拟人抖动 (噪声用激活的命名回放配置)。
/// 先按 UUID 认出是方案还是单条手势 (CHRTouchPlaybackPlanInfoForId / RecordInfoForId), 再用对应的加载器 (同 cht pany), 调用方不用区分
/// - Parameters:
///   - uuid: 方案 UUID 或单条手势 UUID
///   - updates: 本次覆盖, 原样交给 CHRTouchPlaybackUpdate, 格式与 key 以 CHRTouchKit 为准 (CHRTouchPlayback.h):
///       @[ @{ CHRTouchPlaybackUpdateScopeKey: CHRTouchPlaybackUpdateScopeCommon,
///             CHRTouchPlaybackUpdateOptionsKey: @{ CHRTouchPlaybackOptionSpeed: @1.1 } } ]
///     nil = 全用录制时的设置。整批校验不通过: 不回放, 返回 NO, CHRTouchKit 给的原因写进 diag
///   - diag: 诊断, 可为 nil
/// - Returns: 是否已投递 (加载不到、覆盖无效、标签过滤后没有可用记录都返回 NO)
- (BOOL)playback:(NSString *)uuid
         updates:(nullable NSArray<NSDictionary<NSString *, id> *> *)updates
            diag:(nullable IRDiagnostics *)diag;

#pragma mark block 型 (转到上面的方法)

/// app.tap(x, y, diag)
@property (nonatomic, readonly) BOOL (^tap)(CGFloat x, CGFloat y, IRDiagnostics *_Nullable diag);

/// app.longPress(x, y, duration, diag)
@property (nonatomic, readonly) BOOL (^longPress)(CGFloat x, CGFloat y, NSTimeInterval duration, IRDiagnostics *_Nullable diag);

/// app.swipe(fromX, fromY, toX, toY, duration, diag)
@property (nonatomic, readonly) BOOL (^swipe)(CGFloat fromX, CGFloat fromY, CGFloat toX, CGFloat toY,
                                              NSTimeInterval duration, IRDiagnostics *_Nullable diag);

/// app.playback(uuid, diag): 不带覆盖; 要覆盖用 -playback:updates:diag:
@property (nonatomic, readonly) BOOL (^playback)(NSString *uuid, IRDiagnostics *_Nullable diag);

@end

NS_ASSUME_NONNULL_END
