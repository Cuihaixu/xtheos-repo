//
//  CHRTouchPlayback.h
//  CHRTouchKit
//
//  M5 公开回放门面。函数是 C ABI；Foundation 系统对象可用于调用边界，
//  但不暴露任何 CHR 自定义 Objective-C 类。
//

#import <Foundation/Foundation.h>
#import <CHRTouchKit/CHRTouchProtocol.h>

NS_ASSUME_NONNULL_BEGIN

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CHR_TOUCH_KIT_API
#define CHR_TOUCH_KIT_API __attribute__((visibility("default")))
#endif

typedef NSString * CHRTouchPlaybackOptionKey NS_TYPED_ENUM;
typedef NSString * CHRTouchPlaybackUpdateKey NS_TYPED_ENUM;

/// 坐标均为调用方的屏幕 points；AnchorX / AnchorY 必须同时提供。
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionAnchorX;
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionAnchorY;

CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionSpeed;
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionDurationMs;
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionRepeatCount;
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionScaleFactor;
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionDisplacementPt;

CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionNoiseEnabled;
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionCoordNoise;
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionCoordNoiseHz;
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionRoundDrift;
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionTimeJitterMs;
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionSpeedJitter;
/// 松手速度容差(比例,0~1,默认 0.1)。抬手前尾段按录制节奏整段统一缩放,松手速度最多偏离录制值 ±该比例;
/// 滑动手势的滚动距离由松手速度决定,所以它同时限定了滚动距离的波动。0 = 尾段严格按录制节奏。
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionReleaseVelocityTolerance;

/// 标签选取(仅对方案回放 LoadPlanId 生效;单条 LoadRecordId 已显式指定、忽略)。
/// 值为 NSArray<NSString *>,标签格式 `维度:值`;系统维度支持 `res/model/os/app` + `current` 本机值
/// (如 @"res:current");用户/预设标签为裸串或 `组名:值`。
/// MatchTags = 白名单:硬过滤,记录须含全部标签(AND),一个都不满足 → 回放报错。
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionMatchTags;
/// PriorityTags = 优先级:软排序,高→低逐级收窄,某级无匹配则跳过(永不报错);仅在未传 MatchTags
/// 时作为默认排序依据。两者都不传时,方案默认按 `res:current` 优先(优先同分辨率、没有则用其他)。
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionPriorityTags;
/// 指定回放方案范围内的某条记录(仅 LoadPlanId 生效;LoadRecordId 已显式指定、忽略)。值为 NSString recordId,
/// 只能放在 common 覆盖里。指定后跳过标签筛选与选择方式,直接回放这条;回放配置仍按方案
/// (配置文件 / 自定义参数 / 按段覆盖)。同时传 MatchTags / PriorityTags 时以 RecordId 为准。
/// recordId 不在方案范围内 → Start 返回 CHRTouchStatusNoMatch。
/// 典型用法:CHRTouchPlaybackPlanRecords 查出方案内记录(含滚动距离),挑一条后用它回放。
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionRecordId;
/// 同步回放(NSNumber BOOL,默认 NO,只能放在 common 覆盖里)。YES 时 CHRTouchPlaybackStart 阻塞到整条记录放完才返回
/// (含多段之间的间隔;设了重复次数则等全部重复放完;记录末尾带间隔会等间隔结束)。会阻塞调用线程,不要在主线程调用;
/// 保底超时 120s,超时返回 CHRTouchStatusTimeout(回放仍在进行)。
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionSync;
/// 期望惯性距离(NSNumber,pt,≥ 0;只能放在 common 覆盖里;方案与单条记录回放都支持)。期望,不保证。
/// 含义:手指抬起后列表靠惯性再滚多远(不含拖动部分),沿手势主方向(上下滑算垂直、左右滑算水平),方向跟随手势。
/// 做法:只改抬手前 140~200ms,速度平滑过渡到所需松手速度、再保持 80ms 到抬手,前面的轨迹不变;
/// 不叠加松手速度容差的随机 ±10%;以它为准,不受方案倍速影响。
///   - 0 = 不要惯性(抬手前停住)。
///   - 低于最小惯性(约 170pt)按最小值;超过这条手势能做到的最大值按最大值(结尾手指要有屏幕空间加速;
///     速度上限 8 pt/ms ≈ 4000pt)。这两种 status = clamped,实际值看 expectedPt。
///   - 组合手势(多段)、点击 / 原地不调整,照原样回放(status = skipped)。
/// 结果用 CHRTouchPlaybackLastResult 查。换算依据(iPhone 8 Plus,UIScrollView 默认减速率,两类记录 140 次实测):
///   惯性 ≈ 499.6 × 松手速度(pt/ms) − 5.2,R² = 0.99997,误差 ±5pt。
/// 调整会改变拖动距离(加速变长、减速变短),见结果的 dragOffsetDeltaPt;列表总偏移 ≈ 原拖动 + dragOffsetDeltaPt + 惯性。
CHR_TOUCH_KIT_API extern CHRTouchPlaybackOptionKey const CHRTouchPlaybackOptionInertiaDistance;

/// Update 数组中每项的结构键与 scope 值。options 继续使用上面的回放选项常量。
CHR_TOUCH_KIT_API extern CHRTouchPlaybackUpdateKey const CHRTouchPlaybackUpdateScopeKey;
CHR_TOUCH_KIT_API extern CHRTouchPlaybackUpdateKey const CHRTouchPlaybackUpdateOptionsKey;
CHR_TOUCH_KIT_API extern CHRTouchPlaybackUpdateKey const CHRTouchPlaybackUpdateSegmentIndexKey;
CHR_TOUCH_KIT_API extern NSString * const CHRTouchPlaybackUpdateScopeCommon;
CHR_TOUCH_KIT_API extern NSString * const CHRTouchPlaybackUpdateScopeStage;

/// 第一步（加载器之一）：以 Studio 已保存的本地回放方案 UUID 创建不透明回放实例。
/// 返回的 id 只可传给本头中的 Update / Start；其实现不含自定义 OC 类。
CHR_TOUCH_KIT_API id _Nullable CHRTouchPlaybackLoadPlanId(NSString *planId,
                                                           NSError * _Nullable * _Nullable error);

/// 第一步（加载器之二）：以收藏库中单条 record 的 UUID 创建不透明回放实例，
/// 直接精确回放该 record（不经方案的匹配/选择）。libraryId 由后端反查，调用方只需 recordId。
/// 返回的 id 与 LoadPlanId 同型，可传给同一套 Update / Start。
CHR_TOUCH_KIT_API id _Nullable CHRTouchPlaybackLoadRecordId(NSString *recordId,
                                                             NSError * _Nullable * _Nullable error);

/// 查询本地已保存回放方案的只读展示信息。返回 library / categories / recordGroup /
/// displayName / breadcrumb 等 Foundation 字段；调用方不直接读取库目录。
CHR_TOUCH_KIT_API NSDictionary<NSString *, id> * _Nullable CHRTouchPlaybackPlanInfoForId(
    NSString *planId, NSError * _Nullable * _Nullable error);

/// 查询收藏库中单条 record 的只读信息；调用方不直接读取库目录。不含点序列。
/// 常用属性(与 CHRTouchPlaybackPlanRecords 每项一致):
///   recordId / displayName / type(手势类型文案) / typeSignature / durationNs(整条回放时长,含间隔) /
///   tags(系统派生 res/model/os/app + 用户标签) / userTags / scrollMeasurements /
///   segmentCount(段数,含间隔段) / gestures / dragOffsetPt / releaseVelocity
/// 额外:source(local|network) / breadcrumb(库 -> 分类 -> ... -> 手势名) / meta(完整辅助信息)。
///
/// 动作摘要(从录制轨迹算出,列表视角,正负号与测量偏移一致:上滑 dy>0、左滑 dx>0;单位为录制设备设计分辨率 pt,
/// 即 meta.screenPoints;1 倍速无抖动的基准值,实际回放 ≈ 基准 × 方案倍速 × (1±10% 松手速度容差);多指取中心点):
///   gestures[]       每个手势段(按下→全部抬起)一项:{ fingers, dragOffsetPt{dx,dy}, releaseVelocity{dx,dy} }
///   dragOffsetPt     手指拖动阶段列表跟着走的偏移(= 手指位移),所有手势段合计
///   releaseVelocity  抬手那一刻列表获得的速度(pt/ms,取抬手前约 60ms),最后一个手势段;
///                    与 scrollViewWillEndDragging:withVelocity: 的 velocity 同义
///   经验:列表最终偏移 ≈ dragOffsetPt + 499 × releaseVelocity(系统默认减速率)
///
/// scrollMeasurements:每个「机型 + 分辨率」一项 { deviceModel, screenPoints{w,h}, screenScale, osVersion,
///   modes: { vertical | horizontal | both: { samples:[{dx,dy,durationMs,measuredAt}], min, max, avg:{dx,dy,durationMs} } } }
///   mode = 测试画布能往哪滚(只垂直 dx 恒 0 / 只水平 dy 恒 0 / 垂直+水平);dx/dy = 回放整条记录后列表偏移(pt,正负同上);
///   durationMs = 发出回放 → 列表最后一次滚动;samples 为最近 10 次,min/max/avg 按 samples 逐字段算;未测为空数组。
CHR_TOUCH_KIT_API NSDictionary<NSString *, id> * _Nullable CHRTouchPlaybackRecordInfoForId(
    NSString *recordId, NSError * _Nullable * _Nullable error);

/// 导出单条记录的完整帧数据(RecordInfoForId 不含点序列,需要轨迹时用它)。返回:
///   recordId / screenPoints{w,h}(录制设计分辨率 pt) / screenScale / orientation / durationNs /
///   segments[]:按顺序的段,每段 { type: "gesture" | "interval", startNs(相对记录开始), durationNs, … };
///     手势段另有 strokes[]:每根手指一项 { fingerID, points[] },
///     points[] = { tNs(相对该段开始), phase(0 Down / 1 Move / 2 Up), x, y(归一化 [0,1],物理竖屏;× screenPoints 得 pt),
///                  pressure, majorRadius, minorRadius(录制时的真实力度 / 触面,可原样传给 CHRTouchFingerEvent) }。
/// 不含设备 UDID。
CHR_TOUCH_KIT_API NSDictionary<NSString *, id> * _Nullable CHRTouchPlaybackRecordFramesForId(
    NSString *recordId, NSError * _Nullable * _Nullable error);

/// 查询回放方案范围内的全部记录:方案引用整组 = 组内当前全部;建方案时选了部分 = 只返回所选;
/// 已删除/失效的跳过。不做标签筛选与选择,同一 planId 在任何设备上结果一致。
/// 每项为常用属性(字段同 CHRTouchPlaybackRecordInfoForId 的常用属性),按记录组内顺序排列。
/// 要回放其中某条:LoadPlanId + Update(common: RecordId) + Start。
CHR_TOUCH_KIT_API NSArray<NSDictionary<NSString *, id> *> * _Nullable CHRTouchPlaybackPlanRecords(
    NSString *planId, NSError * _Nullable * _Nullable error);

/// 第二步：批量写入本次业务覆盖。每项为
/// @{ scope: common, options: {...} } 或
/// @{ scope: stage, segmentIndex: @0, options: {...} }。
/// 整批校验成功才生效；失败不改变 playback。
CHR_TOUCH_KIT_API BOOL CHRTouchPlaybackUpdate(id playback,
                                               NSArray<NSDictionary<CHRTouchPlaybackUpdateKey, id> *> *updates,
                                               NSError * _Nullable * _Nullable error);

/// 第三步：把方案/record 及覆盖下发给引擎（按加载器类型自动分派）。短手势自然结束；不公开 Stop。
/// 默认发出即返回（回放在后台进行）；覆盖里带 CHRTouchPlaybackOptionSync = YES 时等整条放完才返回。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchPlaybackStart(id playback);

/// 最近一次回放(Start 成功后即可查;同步回放返回后查最稳)的调整说明。目前:
///   inertia: { status: applied | clamped | skipped, reason?, requestedPt(期望), expectedPt(按实际使用的速度预计的惯性),
///              releaseVelocity{dx,dy}(实际使用的松手速度 pt/ms), recordedReleaseVelocity{dx,dy}(原轨迹松手速度,已乘倍速),
///              windowMs(调整窗口), dragOffsetDeltaPt{dx,dy}(拖动距离变化) }
///   没指定 CHRTouchPlaybackOptionInertiaDistance 时为空字典。
CHR_TOUCH_KIT_API NSDictionary<NSString *, id> * _Nullable CHRTouchPlaybackLastResult(NSError * _Nullable * _Nullable error);

/// 便利方法：加载方案 + 写入 options（common 覆盖，可带 RecordId 等）+ 同步回放，放完才返回。
/// 不要在主线程调用。加载或覆盖校验失败返回 CHRTouchStatusBadPayload 并填 error；超时返回 CHRTouchStatusTimeout。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchPlaybackPlanSync(NSString *planId,
                                                          NSDictionary<CHRTouchPlaybackOptionKey, id> * _Nullable options,
                                                          NSError * _Nullable * _Nullable error);
/// 便利方法：加载单条 record + 写入 options + 同步回放，放完才返回。规则同 CHRTouchPlaybackPlanSync。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchPlaybackRecordSync(NSString *recordId,
                                                            NSDictionary<CHRTouchPlaybackOptionKey, id> * _Nullable options,
                                                            NSError * _Nullable * _Nullable error);

#ifdef __cplusplus
}
#endif

NS_ASSUME_NONNULL_END
