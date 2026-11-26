//
//  CHRTouchProtocol.h
//  CHRTouchKit
//
//  App 侧 SDK 与 backboardd 侧 tweak 之间的线协议定义。
//  这个头文件被两端共同引用，任何改动都要同步升级 CHRTouchProtocolVersion。
//
//  设计要点：
//  - 坐标一律是归一化的 [0, 1]，参考系是屏幕的物理竖屏方向（原点左上）。
//    点 → 归一化 的换算、以及界面转屏的坐标映射，全部在 App 侧完成，
//    backboardd 端不需要知道任何屏幕参数。这样轨迹文件天然跨设备通用。
//  - 一条 Touch 命令可以携带多个手指的状态变更，多指手势因此是原子的：
//    不会像"一次只能改一根手指"那样把中间态也下发出去。
//
//  传输：全部走自研 mach IPC（chr_ipc + broker，见 chr_names.h / chripc.h）。分布式通知
//  在沙盒 App 里带不了数据（501 被系统拦），已彻底弃用。服务：chr:touch（backboardd 命令）、
//  chr:viz / chr:screenrec（SpringBoard）、chr:app（Studio 前台上报）、chr:lockscreen。
//  chr_ipc_call 有 1 字节状态回执；触摸帧用 chr_ipc_post（fire-and-forget，吞吐最高）。
//

#ifndef CHRTouchProtocol_h
#define CHRTouchProtocol_h

#include <stdint.h>

#define CHRTouchProtocolVersion   4u

/// 命令通知名。App 端 post，backboardd 端 observe。
#define CHRTouchNotificationName   "com.chr.touch.event"

/// 触摸可视化坐标推送通知名。backboardd 端 post（全局 HID 坐标），SpringBoard overlay observe。
/// userInfo 里键 CHRTouchKeyPayload 带 CHRTouchPointPacket[]（归一化坐标 + 相位）。
#define CHRTouchVizNotificationName "com.chr.touch.viz"

/// 前台 App 上报通知名。SpringBoard 端 post（前台 bundle id 变化时），backboardd observe 缓存。
/// 录制时手势归属到当时的前台 App。userInfo 里键 CHRTouchKeyBundle 带 bundle id 字符串。
#define CHRTouchFrontAppNotificationName "com.chr.touch.frontapp"
#define CHRTouchKeyBundle                "b"

/// 屏幕录制联动通知（backboardd post，SpringBoard overlay observe）。
/// 手势录制 start/stop 时，backboardd 通知 SpringBoard 同步开/停系统录屏（RPControlCenterClient
/// 必须跑在 SpringBoard 里）。start 的 userInfo 带键 CHRScreenRecKeySession = 会话名（UTF-8 字符串），
/// overlay 收到后把会话名写进重定向标志文件、再触发录屏；replayd 侧据此把视频命名为 <session>.mp4。
#define CHRScreenRecStartNotificationName "com.chr.screenrec.start"
#define CHRScreenRecStopNotificationName  "com.chr.screenrec.stop"
#define CHRScreenRecKeySession            "s"

/// 音量+ 长按热键通知（SpringBoard post，backboardd observe）。
/// 热键检测必须在 SpringBoard 侧做——只有那里能拦住 SBVolumeControl 的涨音量动作；
/// backboardd 收到后走和 RecordStart/RecordStop 一样的路（含屏录联动），保证热键录的
/// 会话同样有 <session>.mp4。无 userInfo，语义是 toggle（正在录 → 停，否则 → 开）。
#define CHRTouchHotkeyNotificationName    "com.chr.touch.hotkey"

/// 封面帧补生成请求（Studio App post，SpringBoard overlay observe）。
/// App 进程里解不了 H.264（AVFoundation -11800 / OSStatus -12437），录制封面只能由
/// SpringBoard 侧生成到 thumbs/<session>.jpg；App 发现缺图就发这个通知请它补一遍。
#define CHRThumbnailRequestNotificationName "com.chr.touch.thumbreq"

/// 回放全局配置变更（Studio 写完 replay_config.json 后 post，backboardd observe）。
/// Darwin 通知无 payload；引擎收到只标 dirty，下一次 play 才重读文件，不打断进行中回放。
#define CHRTouchReplayConfigNotificationName "com.chr.replay.config"

/// userInfo 里的键。
#define CHRTouchKeyVersion         "v"    ///< CFNumber，CHRTouchProtocolVersion
#define CHRTouchKeyCommand         "c"    ///< CFNumber，CHRTouchCommand
#define CHRTouchKeyPayload         "d"    ///< CFData，命令相关的二进制负载（可选）

/// 单个事件里最多允许的手指数。
#define CHRTouchMaxFingers        10

#pragma mark - 触摸相位

typedef uint32_t CHRTouchPhase;
enum {
    CHRTouchPhaseDown = 0,   ///< 手指落下：建立一个新的活跃手指
    CHRTouchPhaseMove = 1,   ///< 手指移动：更新已有手指的位置
    CHRTouchPhaseUp   = 2,   ///< 手指抬起：本次事件发出后该手指被回收
};

#pragma mark - 命令字

typedef uint32_t CHRTouchCommand;
enum {
    CHRTouchCommandPing        = 0x00,  ///< 无 payload
    CHRTouchCommandTouch       = 0x01,  ///< payload: CHRTouchPointPacket[count]
    CHRTouchCommandReset       = 0x02,  ///< 无 payload，抬起并清空所有活跃手指
    /// payload: CHRTouchHIDKeyPacket；只发送一个原始 HID 键，不解释字符/文本。
    CHRTouchCommandHIDKey      = 0x03,
    /// 无 payload；同步返回 backboardd 插件编译时写入的 UTF-8 客户端版本。
    CHRTouchCommandClientVersion = 0x04,

    /* 以下为录制/回放预留，回放引擎完整跑在 backboardd 内部，
       App 侧只下发控制指令，逐帧时序不经过传输层。 */
    CHRTouchCommandRecordStart = 0x10,  ///< payload: 轨迹名 (UTF-8, 无结尾 NUL)
    CHRTouchCommandRecordStop  = 0x11,
    CHRTouchCommandRecordStatus = 0x12, ///< 无 payload；回执 CHRTouchStatusActive / OK
    CHRTouchCommandPlayStart   = 0x20,  ///< payload: CHRTouchPlayRequest + 轨迹名
    CHRTouchCommandPlayStop    = 0x21,
    CHRTouchCommandPlayStatus  = 0x22,  ///< 无 payload；回执 CHRTouchStatusActive（回放中）/ OK
    CHRTouchCommandPlayStartV2 = 0x23,  ///< payload: JSON {track, speed, repeatCount, scale, anchor, time, noise}（参数化回放）
    CHRTouchCommandPlayRecord  = 0x24,  ///< payload: JSON {libraryId, recordId, speed, repeatCount, ...}
    CHRTouchCommandPlayPlan    = 0x25,  ///< payload: chr.playback-request/v2 JSON（planId + 本次覆盖）
    CHRTouchCommandPlaybackPlanInfo = 0x26, ///< payload: planId UTF-8；同步回 chr.playback-plan-info/v1 JSON
    CHRTouchCommandPlaybackRecordInfo = 0x27, ///< payload: recordId UTF-8；同步回 chr.playback-record-info/v1 JSON（含 breadcrumb 路径）
    CHRTouchCommandPlaybackPlanRecords = 0x28, ///< payload: planId UTF-8；同步回 chr.playback-plan-records/v1 JSON（方案范围内记录列表）
    CHRTouchCommandPlaybackRecordFrames = 0x29, ///< payload: recordId UTF-8；同步回 chr.playback-record-frames/v1 JSON（完整帧数据）
    CHRTouchCommandPlaybackLastResult = 0x2A,   ///< 无 payload；同步回 chr.playback-result/v1 JSON（最近一次回放的调整说明）

    CHRTouchCommandVizStart    = 0x30,  ///< 开启触摸可视化（backboardd 推坐标给 overlay）
    CHRTouchCommandVizStop     = 0x31,
    CHRTouchCommandVizDotsSet  = 0x32,  ///< payload: uint32_t，独立开关触点小球
    CHRTouchCommandVizTrailsSet = 0x33, ///< payload: uint32_t，独立开关触摸轨迹
    CHRTouchCommandVizTrailAnnotSet = 0x34, ///< payload: uint32_t，轨迹标注位掩码（起终坐标/时长/点数/位移/速度）
    CHRTouchCommandVizConfigResend = 0x35,  ///< 无 payload；引擎把当前 viz 配置重新推给 overlay（SpringBoard 启动时要，补上它没收到的那次推送）

    CHRTouchCommandReloadConfig = 0x40, ///< 无 payload，令引擎**就地**重载 replay_config.json（C4）
    CHRTouchCommandHotkeyToggle = 0x41, ///< 无 payload，切换录制（SpringBoard 热键 → 引擎，C7）
    CHRTouchCommandFrontApp     = 0x42, ///< payload: 前台 App bundle(UTF-8)；SpringBoard reporter → 引擎 ForegroundStore（C5）
};

/// SpringBoard 可视化层显示选项。通过 chr:viz-config 传输该位掩码。
typedef uint32_t CHRTouchVisualizationOptions;
enum {
    CHRTouchVisualizationOptionDots   = 1u << 0,
    CHRTouchVisualizationOptionTrails = 1u << 1,
    // 轨迹标注：画在 trail 起/终点或角落，需 Trails 开启才可见。由 VizTrailAnnotSet 设置，随同一位掩码下发。
    CHRTouchVisualizationOptionAnnotStartCoord = 1u << 2,  ///< 起点坐标（设计分辨率 pt）
    CHRTouchVisualizationOptionAnnotEndCoord   = 1u << 3,  ///< 终点坐标（设计分辨率 pt）
    CHRTouchVisualizationOptionAnnotDuration   = 1u << 4,  ///< 时长 ms
    CHRTouchVisualizationOptionAnnotPointCount = 1u << 5,  ///< 点数
    CHRTouchVisualizationOptionAnnotDistance   = 1u << 6,  ///< 总位移 pt
    CHRTouchVisualizationOptionAnnotAvgSpeed   = 1u << 7,  ///< 平均速度 pt/s
};
#define CHRTouchVisualizationAnnotMask (CHRTouchVisualizationOptionAnnotStartCoord | \
    CHRTouchVisualizationOptionAnnotEndCoord | CHRTouchVisualizationOptionAnnotDuration | \
    CHRTouchVisualizationOptionAnnotPointCount | CHRTouchVisualizationOptionAnnotDistance | \
    CHRTouchVisualizationOptionAnnotAvgSpeed)

// chr:screenrec 子命令(SpringBoard 录屏联动 + 封面;线格式 [uint32 cmd][payload])。C3/C6。
typedef uint32_t CHRScreenRecCommand;
enum {
    CHRScreenRecCommandStart = 1,  ///< payload: 会话名(UTF-8)
    CHRScreenRecCommandStop  = 2,  ///< 无 payload
    CHRScreenRecCommandThumb = 3,  ///< 无 payload，补缺失封面
    /// payload: chr.screen-recording-request/v1 JSON {token, outputPath|null}
    CHRScreenRecCommandStartV2 = 4,
    /// payload: token UTF-8；停止当前 v2 录制。
    CHRScreenRecCommandStopV2  = 5,
    /// payload: token UTF-8；同步返回对应的最终回执 JSON，不存在时返回空。
    CHRScreenRecCommandReceipt = 6,
    /// payload: 绝对 PNG 输出路径；SpringBoard 直接取屏并写入。
    CHRScreenRecCommandCapturePNG = 7,
};

// chr:lockscreen 子命令(远程锁/解屏,cht 触发;线格式 [uint32 cmd])。C8。
typedef uint32_t CHRLockScreenCommand;
enum {
    CHRLockScreenCommandLock   = 1,
    CHRLockScreenCommandUnlock = 2,
    /// 同步返回 CHRLockScreenState；由 SpringBoard 主线程读取。
    CHRLockScreenCommandQueryState = 3,
    /// 打开系统 App 切换器；同步返回 int32_t CHRTouchStatus，并在动画后确认可见。
    CHRLockScreenCommandShowAppSwitcher = 4,
    /// 同步返回 CHRAppSwitcherState；由 SpringBoard 主线程读取。
    CHRLockScreenCommandQueryAppSwitcherState = 5,
    /// 关闭系统 App 切换器；同步返回 int32_t CHRTouchStatus，并在动画后确认已关闭。
    CHRLockScreenCommandDismissAppSwitcher = 6,
    /// payload: UTF-8 bundle identifier；仅 SceneEntity 启动，返回 int32_t CHRTouchStatus。
    CHRLockScreenCommandLaunchApplication = 7,
    /// payload: CHRLaunchApplicationRequest + UTF-8 bundle identifier；严格按指定策略启动。
    CHRLockScreenCommandLaunchApplicationWithMethod = 8,
    /// payload: CHRExitApplicationRequest + UTF-8 bundle identifier；严格按指定退出策略执行。
    CHRLockScreenCommandExitApplication = 9,
    /// payload: UTF-8 bundle identifier；静默卸载。直调桌面图标"X"删除"点确认后"的真实执行者
    /// SBApplicationController uninstallApplication:，跳过确认框、不阻塞 SpringBoard、不卡。
    /// 返回 int32_t CHRTouchStatus，以卸载后目标已不在册为成功依据。
    CHRLockScreenCommandUninstallApplication = 10,
    /// payload: CHRInstalledAppsRequest(uint32 scope + uint64 fields)后接 UTF-8 JSON 包名数组
    /// (空/无 = 不限包名)。三层过滤:①scope 圈 user/system/placeholder ②包名列表收窄
    /// ③fields 裁字段。经 LSApplicationWorkspace/LSApplicationProxy 读注册表。回 JSON:内联
    /// {schema:"chr.lockscreen-installed/v1",apps:[…]},超 CHR_MAX_INLINE 落盘回
    /// {schema:"chr.lockscreen-spill/v1",path}。每项字段见 SDK CHRTouchInstalledApplications。
    CHRLockScreenCommandListInstalledApplications = 11,
    /// payload: 同 CHRLockScreenCommandLaunchApplicationWithMethod；返回 JSON {status: int32, reason?: string}
    CHRLockScreenCommandLaunchApplicationDetailed = 12,
};

/// 应用范围位(对应 SDK CHRTouchAppScope);按 kind 分类过滤。
#define CHR_APPSCOPE_USER         (1u << 0)
#define CHR_APPSCOPE_SYSTEM       (1u << 1)
#define CHR_APPSCOPE_PLACEHOLDER  (1u << 2)
#define CHR_APPSCOPE_ALL          (CHR_APPSCOPE_USER | CHR_APPSCOPE_SYSTEM | CHR_APPSCOPE_PLACEHOLDER)

/// 字段选择位(对应 SDK CHRTouchAppInfoFields;uint64,给扩展留足位)。bundleIdentifier 恒返回。
#define CHR_APPFIELD_NAME         (1ull << 0)  /* name */
#define CHR_APPFIELD_VERSION      (1ull << 1)  /* version + bundleVersion */
#define CHR_APPFIELD_KIND         (1ull << 2)  /* kind/isSystemApplication/isPlaceholder/applicationType/isUninstallSupported */
#define CHR_APPFIELD_INSTALLDATE  (1ull << 3)  /* installDate(首次安装) + updateDate(最近更新) */
#define CHR_APPFIELD_PATHS        (1ull << 4)  /* installPath + sandboxPath + appGroupPaths */
#define CHR_APPFIELD_TEAMID       (1ull << 5)  /* teamID */
#define CHR_APPFIELD_ICON         (1ull << 6)  /* icon(base64,重) */
#define CHR_APPFIELD_ALL          (~0ull)

/// 接在 CHRLockScreenCommandListInstalledApplications 后,随后是 UTF-8 JSON 包名数组(可空)。
typedef struct __attribute__((packed)) {
    uint32_t scope;    ///< CHR_APPSCOPE_*
    uint64_t fields;   ///< CHR_APPFIELD_*
} CHRInstalledAppsRequest;

/// 锁屏相关的稳定状态快照。只表达 UI 状态，不推断密码／生物认证是否可用。
typedef struct __attribute__((packed)) {
    uint8_t screenIsOn;
    uint8_t uiLocked;
    uint8_t lockScreenVisible;
    uint8_t uiUnlocking;
} CHRLockScreenState;

/// App 切换器的稳定状态快照。只表达当前是否已显示。
typedef struct __attribute__((packed)) {
    uint8_t visible;
} CHRAppSwitcherState;

typedef uint32_t CHRTouchApplicationLaunchMethod;
enum {
    /// 推荐：通过 SpringBoard 图标 + SceneEntity 激活，保留正常图标启动语义。
    CHRTouchApplicationLaunchMethodIcon = 1,
    /// 通过 LSApplicationWorkspace 的 LaunchServices 启动。
    CHRTouchApplicationLaunchMethodLSApplication = 2,
};

/// 接在 CHRLockScreenCommandLaunchApplicationWithMethod 后，随后是 UTF-8 Bundle ID。
typedef struct __attribute__((packed)) {
    uint32_t method;
} CHRLaunchApplicationRequest;

typedef uint32_t CHRTouchApplicationExitMethod;
enum {
    /// 仅目标当前在前台时回到桌面；后台目标不作操作并成功返回。
    CHRTouchApplicationExitMethodDeactivate = 1,
    /// 直接向目标进程发送 SIGKILL；仅用于其他退出策略失效时的兜底。
    CHRTouchApplicationExitMethodForceKill = 2,
    /// 推荐：通过系统应用切换器移除该 Bundle 的所有卡片/布局，走系统场景销毁链路。
    CHRTouchApplicationExitMethodSwitcherCard = 3,
    /// 通过 FrontBoard 系统服务终止目标进程，但保留其卡片；
    /// 仅在终止 completion 回调后报告成功。
    CHRTouchApplicationExitMethodFBTerminate = 4,
    /// 通过 SBWorkspaceTerminateApplication 终止目标应用；复用系统“上滑卡片后的终止”调用，
    /// 但本策略本身不会展示、进入或关闭切换 UI。
    CHRTouchApplicationExitMethodWorkspaceTerminate = 5,
};

/// 接在 CHRLockScreenCommandExitApplication 后，随后是 UTF-8 Bundle ID。
typedef struct __attribute__((packed)) {
    uint32_t method;
} CHRExitApplicationRequest;

#pragma mark - 报文结构

/// 单根手指的一次状态变更。
typedef struct __attribute__((packed)) {
    uint32_t fingerID;       ///< 调用方自己维护的手指编号，单指场景固定用 0
    uint32_t phase;          ///< CHRTouchPhase
    float    x;              ///< 归一化 [0, 1]，物理竖屏方向
    float    y;              ///< 归一化 [0, 1]
    float    pressure;       ///< 录制时的真实力度(HID 单位)；< 0 = 未录制，回放时由 emitter 合成
    float    majorRadius;    ///< 录制时的真实触面长轴；pressure < 0 时忽略
    float    minorRadius;    ///< 触面短轴；同上
} CHRTouchPointPacket;

/// 单个原始 HID 键事件。KeyboardAgent 负责把字符映射为 page/usage，也负责时序与输入法逻辑；
/// backboardd 只按本包构造并派发一次键事件。
typedef uint32_t CHRTouchHIDKeyPhase;
enum {
    CHRTouchHIDKeyPhaseDown  = 0,
    CHRTouchHIDKeyPhaseUp    = 1,
    CHRTouchHIDKeyPhasePress = 2,
};

typedef struct __attribute__((packed)) {
    uint32_t usagePage;  ///< HID usage page，例如 Keyboard=0x07 / Consumer=0x0C
    uint32_t usage;      ///< HID usage
    uint32_t phase;      ///< CHRTouchHIDKeyPhase
} CHRTouchHIDKeyPacket;

/// 回放参数。
typedef struct __attribute__((packed)) {
    float    speed;          ///< 回放倍速，1.0 为原速
    uint32_t repeatCount;    ///< 0 表示无限循环
    uint32_t nameLength;     ///< 紧跟其后的轨迹名字节数
} CHRTouchPlayRequest;

#pragma mark - 状态码

//  分布式通知是单向的，命令的执行结果（如 UnknownFinger）传不回 App。
//  这里的状态码只覆盖 App 本地能判定的情况：投递成功 / 本地校验失败 / 投递失败。
//  backboardd 端的详细错误只写进它自己的日志。
typedef int32_t CHRTouchStatus;
enum {
    CHRTouchStatusOK               = 0,   ///< 已投递（不代表已成功执行）
    CHRTouchStatusActive           = 1,   ///< 状态查询：目标当前活跃

    /* App 本地能判定的（会作为 sendCommand 返回值） */
    CHRTouchStatusBadPayload       = -4,  ///< 参数非法，未投递
    CHRTouchStatusTooManyFingers   = -5,  ///< 手指数超限，未投递
    CHRTouchStatusNotConnected     = -8,  ///< 通知中心不可用
    CHRTouchStatusTransportError   = -9,  ///< 投递失败
    CHRTouchStatusBusy             = -10, ///< 已有回放进行中，本次未投递（快速连播时正常出现，不是错误）
    CHRTouchStatusInputFailed      = -11, ///< 文本输入某关键点校验未通过（详情见响应 error 字段：哪段/期望/实得）
    CHRTouchStatusNoMatch          = -12, ///< 方案回放:标签/分辨率过滤(白名单)后无候选记录，未回放
    CHRTouchStatusTimeout          = -13, ///< 同步回放(Sync)等待超时：回放仍在进行
    CHRTouchStatusLaunchIntercepted = -14, ///< 应用启动被启动前拦截器阻止

    /* backboardd 内部（只写进它自己的日志，传不回 App） */
    CHRTouchStatusUnknownFinger    = -6,  ///< 对未 Down 的 fingerID 发 Move/Up
    CHRTouchStatusDispatchFailed   = -7,  ///< HID 事件合成/下发失败
};

#endif /* CHRTouchProtocol_h */
