//
//  CHRTouchManual.h
//  CHRTouchKit
//
//  手动触摸原语。所有函数在调用方当前线程同步执行：调用方若不希望阻塞 UI，
//  应自行放到后台线程。函数之间不创建内部队列，调用顺序就是实际下发顺序。
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CHRTouchKit/CHRTouchProtocol.h>

NS_ASSUME_NONNULL_BEGIN

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CHR_TOUCH_KIT_API
#define CHR_TOUCH_KIT_API __attribute__((visibility("default")))
#endif

typedef uint32_t CHRTouchFingerId;

/// 单指原始事件。固定操作 fingerId = 0。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchDown(CGPoint point);
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchMove(CGPoint point);
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchUp(CGPoint point);

/// 多指原始事件。调用方负责为同时活跃的每根手指分配不同的 fingerId。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchFingerDown(CHRTouchFingerId fingerId, CGPoint point);
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchFingerMove(CHRTouchFingerId fingerId, CGPoint point);
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchFingerUp(CHRTouchFingerId fingerId, CGPoint point);

/// 触摸属性（与录制帧同单位：CHRTouchPlaybackRecordFramesForId 导出的 pressure / majorRadius / minorRadius 可原样传入）。
/// pressure < 0 表示不指定：由引擎按真机统计合成（整数力度、离散触面档位），与不带属性的接口相同。
typedef struct {
    float pressure;      ///< HID 力度，真机为整数（按下 0、抬起约 8）
    float majorRadius;   ///< 触面长轴，真机为离散档位（n × 0.0165）；抬起为 0
    float minorRadius;   ///< 触面短轴
} CHRTouchAttributes;

/// 带属性的原始事件：phase 为 CHRTouchPhaseDown / Move / Up。用于按模板帧还原真实力度与触面；
/// 同一根手指在一次触摸内建议全程传属性（或全程不传），混用时未传的帧回落到合成值。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchFingerEvent(CHRTouchFingerId fingerId, CHRTouchPhase phase, CGPoint point,
                                                     CHRTouchAttributes attributes);

/// 在调用方当前线程等待指定毫秒数。不发送触摸事件；用于编排 Down / Move / Up 的间隔。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchWait(uint32_t milliseconds);

/// 常用短点击：约 56–100 ms，包含随机数量的同点 Move 采样。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchTap(CGPoint point);

/// 常用长按：调用方指定按住时长；期间以 20–50 ms 间隔下发带微漂移的 Move。
/// durationMs 必须大于 0。函数同步阻塞至抬起或发生投递错误。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchLongPress(CGPoint point, uint32_t durationMs);

/// 系统按钮。不暴露底层 HID usage。
typedef uint32_t CHRTouchSystemButton;
enum {
    CHRTouchSystemButtonHome = 1,
    CHRTouchSystemButtonVolumeUp,
    CHRTouchSystemButtonVolumeDown,
    CHRTouchSystemButtonLock,
    CHRTouchSystemButtonMediaPlayPause,
    CHRTouchSystemButtonMediaNextTrack,
    CHRTouchSystemButtonMediaPreviousTrack,
    CHRTouchSystemButtonSnapshot,
};

/// 精确系统按钮原语。两次调用均同步返回“事件是否成功投递”；调用方必须为每个 Down 配对 Up。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchSystemButtonDown(CHRTouchSystemButton button);
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchSystemButtonUp(CHRTouchSystemButton button);
/// 同步完成一次系统按键。durationMs=0 时使用 50ms；函数在成功抬起或失败后才返回。
/// 长按请在调用方的后台队列执行，避免阻塞 UI 线程。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchSystemButtonPress(CHRTouchSystemButton button,
                                                           uint32_t durationMs);

/// 常用系统功能操作。实现细节（HID usage、按住时长）由 backboardd 管理。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchGoHome(void);
/// 打开系统 App 切换器。调用 SpringBoard 的系统级状态门面；已显示时不重复切换，
/// 返回前会确认切换器已显示。
typedef uint32_t CHRTouchAppSwitcherMethod;
enum {
    /// SpringBoard 系统 API。默认且已验证。
    CHRTouchAppSwitcherMethodSystemAPI = 1,
    /// 模拟双击 Home；返回前查询 SpringBoard 确认切换器已显示。
    CHRTouchAppSwitcherMethodHomeButton,
};
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchShowAppSwitcherWithMethod(CHRTouchAppSwitcherMethod method);
/// 默认使用 CHRTouchAppSwitcherMethodSystemAPI。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchShowAppSwitcher(void);
/// 关闭系统 App 切换器；未显示时直接成功，返回前会确认已关闭。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchDismissAppSwitcherWithMethod(CHRTouchAppSwitcherMethod method);
/// 默认使用 CHRTouchAppSwitcherMethodSystemAPI。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchDismissAppSwitcher(void);
/// 应用启动策略定义在 CHRTouchProtocol.h。Icon 与 LSApplication 严格二选一。
/// 以指定策略请求 SpringBoard 从主屏幕图标启动指定 Bundle ID 的应用。
/// 不做自动降级；同步返回所选策略是否被系统接受。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchLaunchApplicationWithMethod(NSString *bundleIdentifier,
                                                                     CHRTouchApplicationLaunchMethod method);
/// 按指定方式启动应用；若被启动前拦截器阻止，返回 CHRTouchStatusLaunchIntercepted 并在 error 中带出具体拒绝原因。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchLaunchApplicationWithError(NSString *bundleIdentifier,
                                                                    CHRTouchApplicationLaunchMethod method,
                                                                    NSError * _Nullable * _Nullable error);
/// 默认使用推荐的 CHRTouchApplicationLaunchMethodIcon。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchLaunchApplication(NSString *bundleIdentifier);
/// 按指定策略退出应用。Deactivate 仅影响当前前台目标；SwitcherCard 通过系统切换器移除卡片；
/// FBTerminate 在保留卡片的前提下请求 FrontBoard 终止并等待 completion；WorkspaceTerminate 仅复用上滑后的终止调用，
/// 不会进入切换 UI；ForceKill 为 SIGKILL 最后兜底。
/// 不做自动降级，调用方必须明确传入策略。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchExitApplication(NSString *bundleIdentifier,
                                                         CHRTouchApplicationExitMethod method);
/// 静默卸载指定 Bundle ID 的应用。直调桌面图标"X"删除在"确认框点是"之后的真实执行者
/// (SpringBoard SBApplicationController uninstallApplication:),跳过确认框;而非
/// LSApplicationWorkspace 的同步卸载——后者偶发卡死。不弹框、不阻塞 SpringBoard,
/// 磁盘删除异步转 installd;以卸载后目标已从系统在册列表移除为成功依据。
/// 目标未安装时返回 CHRTouchStatusDispatchFailed。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchUninstallApplication(NSString *bundleIdentifier);
/// 解锁策略。两种策略均不会绕过密码或生物认证。
typedef uint32_t CHRTouchUnlockMethod;
enum {
    /// SpringBoard 系统 API：唤醒背光并调用系统解锁请求。默认且已验证。
    CHRTouchUnlockMethodSystemAPI = 1,
    /// Home 键模拟：按 SpringBoard 锁屏状态决定是否补发第二次 Home。
    CHRTouchUnlockMethodHomeButton,
};
/// 使用指定策略唤醒并尝试进入已解锁状态；仅适用于未设置密码的测试设备。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchUnlockScreenWithMethod(CHRTouchUnlockMethod method);
/// 默认使用 CHRTouchUnlockMethodSystemAPI。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchUnlockScreen(void);
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchIncreaseVolume(void);
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchDecreaseVolume(void);
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchLockScreen(void);

#pragma mark - 屏幕/锁屏状态查询

/// 屏幕状态字典的键（值均为 NSNumber 包装的 BOOL）。
#define CHRTouchScreenStateScreenOnKey          @"screenOn"           ///< 是否亮屏
#define CHRTouchScreenStateLockedKey            @"locked"             ///< UI 是否已锁定
#define CHRTouchScreenStateLockScreenVisibleKey @"lockScreenVisible"  ///< 锁屏界面是否显示
#define CHRTouchScreenStateUnlockingKey         @"unlocking"          ///< 是否正在解锁

/// 查询屏幕/锁屏状态快照（只表达 UI 状态，不推断密码/生物认证是否可用）。
/// 成功返回含上面 4 个键的字典；服务不可用返回 nil 并填 error（域 @"CHRTouchLockScreen"）。
CHR_TOUCH_KIT_API NSDictionary<NSString *, NSNumber *> * _Nullable CHRTouchCopyScreenState(NSError * _Nullable * _Nullable error);

/// 便捷布尔：是否已锁定 / 是否亮屏（服务不可用时返回 NO）。
CHR_TOUCH_KIT_API BOOL CHRTouchIsScreenLocked(void);
CHR_TOUCH_KIT_API BOOL CHRTouchIsScreenOn(void);

CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchMediaPlayPause(void);
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchMediaNextTrack(void);
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchMediaPreviousTrack(void);
/// 触发系统截图（保存行为遵循系统设置）。不依赖机型的物理组合键。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchTakeScreenshot(void);
/// 直接抓取当前屏幕并写入指定 PNG 路径（仅允许 /var/mobile/ 下的绝对路径）。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchCaptureScreen(NSString *outputPath);

/// 系统整屏录制。outputPath 非空时由 replayd 在完成后移动到该路径；为 nil 时保存到系统相册。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchStartScreenRecording(NSString * _Nullable outputPath);
typedef void (^CHRTouchScreenRecordingCompletion)(CHRTouchStatus status,
                                                   NSString * _Nullable outputPath);
/// 同步返回停止请求是否送达；completion 在视频实际落盘（或相册导入完成）后于主队列调用一次。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchStopScreenRecording(CHRTouchScreenRecordingCompletion _Nullable completion);

/// 文本输入 SDK 分层：
///   1. 默认      CHRTouchInputText                    = 标准模式
///   2. 模式      CHRTouchInputTextWithMode            极速 / 标准 / 谨慎，每种模式是一整套拟人配置
///   3. 自定义    CHRTouchInputTextWithOptions         三旋钮 speed/errorRate/phraseRate，在标准配置上调
///                CHRTouchCopyInputConfig + CHRTouchInputTextWithConfig   取某模式整套配置 → 改 → 传入
///   监听         CHRTouchInputTextMonitored / WithConfig 的 monitor
/// 输入模式(预设)。默认 Standard。
typedef NS_ENUM(NSInteger, CHRTouchInputMode) {
    CHRTouchInputModeStandard = 0,  ///< 标准(默认)：拟人自然，偶有笔误、词组输入、带节奏停顿
    CHRTouchInputModeFast,          ///< 极速：最快最稳，无拟人(零笔误、整词、无额外停顿)
    CHRTouchInputModeCareful,       ///< 谨慎：慢而稳，少笔误
};

/// 自定义输入参数，各字段取值 0.0–1.0：
/// speed=打字快慢(1 最快)；errorRate=打错+纠正频率(0 不犯错)；phraseRate=整词输入占比(1 全整词)。
typedef struct {
    double speed;
    double errorRate;
    double phraseRate;
} CHRTouchInputOptions;

/// 向当前焦点输入框输入一段文字（需已聚焦输入框）。像真人一样敲键盘，不注入目标 App：
/// 按键盘能力分段（中文拼音 / 英文 / 注入），地球键切到对应键盘；汉字分词转拼音后选候选
///（全键盘敲字母，九键敲九宫格码），ASCII 走 HID 直发，全角标点 / emoji / 生僻字经 kbd hook 注入。
/// 每段设检查点，落屏校验不符只在段内修复。编排在 SpringBoard（v2 引擎）；同步阻塞至输入结束。
/// 默认走「标准」模式（原设计拟人权重：停顿 / 手误 / 分词 / 逐字确认）。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchInputText(NSString *text, NSError * _Nullable * _Nullable error);

/// 指定预设模式(极速/标准/谨慎)输入。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchInputTextWithMode(NSString *text, CHRTouchInputMode mode,
                                                           NSError * _Nullable * _Nullable error);

/// 自定义参数输入(speed/errorRate/phraseRate，各 0.0–1.0)。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchInputTextWithOptions(NSString *text, CHRTouchInputOptions options,
                                                              NSError * _Nullable * _Nullable error);

/// 输入过程监听回调。编排在 SpringBoard 内跑，每到变化点经 IPC 反向回调本进程；
/// block 在本进程的回调线程上执行(非主线程)。设 *stop=YES 可提前中止后续输入
/// (典型场景：候选/联想里已出现目标词，无需再打完)。
/// state.event（v2 引擎；v1 只有 candidates/charStep/wordCommitted）：
///   charStep          每个可见输入步进：拼音每个字母(含打错/退格)、逐字确认的中间一步、英文/数字/表情每个字符上屏
///   subtokenCompleted 分词输入时一个子词完成
///   wordCommitted     一个词确认上屏：中文词组确认、英文 token（含尾空格）完成
///   candidates        候选就绪（带 candidates，NSArray<NSString*>*）
///   beforeFinalSubmit 全部输完、即将最终提交前一次（整句完成后的最后拦截点）
/// 其它字段：originalText、committedText、pendingText、markedText(组合态文本，如 "zu qiu" / "足qiu"，无则不含)、
///   segment / segmentIndex / segmentKind(1 拼音 2 英文 3 注入)、token / tokenIndex。
typedef void (^CHRTouchInputMonitorBlock)(NSDictionary *state, BOOL *stop);

/// 带监听的输入。monitor 为 nil 时等价于 WithOptions。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchInputTextMonitored(NSString *text, CHRTouchInputOptions options,
                                                            CHRTouchInputMonitorBlock _Nullable monitor,
                                                            NSError * _Nullable * _Nullable error);

/// 取某个模式的整套拟人配置（v2 引擎的唯一数据源）。用户在它基础上修改后传给 CHRTouchInputTextWithConfig。
/// 返回的字典结构即下方 config 字段说明；键盘服务不可用时返回 nil。
CHR_TOUCH_KIT_API NSDictionary * _Nullable CHRTouchCopyInputConfig(CHRTouchInputMode mode, NSError * _Nullable * _Nullable error);

/// 完整拟人配置输入（v2 引擎）。config 为 nil = 标准模式整套配置（= 原设计默认权重空间）。
/// config 字段（与 CHRUIAutomationKit 原设计 "keyboard_input" 权重空间同名，逐项覆盖默认）：
///   preset        @"standard" | @"fast" | @"careful"（先套预设，再应用下面各项）
///   weights       { 组ID: { 项ID: 权重 } }，组与默认权重：
///                   shouldSplitChineseWord  {phrase:70, character:30}   中文词分词输入概率
///                   shouldConfirmCharByChar {phrase:70, char:30}        中文逐字确认概率
///                   shouldTypoPinyinChar    {normal:80, typo:20}        拼音打字错误概率
///                   shouldTypoEnglishChar   {normal:75, typo:25}        英文打字错误概率
///                   replaceInputEmojiCount  {"0":50, "1":40, "2":10}    表情输错次数
///                   <停顿>_long_duration    {long, short}               偶尔长停顿，停顿见 durations
///   durations     { 停顿名: { long:[lo,hi], short:[lo,hi] } }（微秒）：
///                   input_char 字母间 / switch_keyboard 切键盘后 / input_word 词间 / input_segment 段间 /
///                   select_word 选词 / submit 提交前 / notice_typo 发现打错 / delete_char 逐个删除 /
///                   delete_emoji 删表情 / select_emoji_recent、select_emoji_new 选表情(最近用过/没用过)
///   durationScale 停顿整体缩放（0 = 无拟人停顿）
///   emojiPool     输错表情时取的表情池
///   submit        输完后是否提交（回车），默认 NO
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchInputTextWithConfig(NSString *text, NSDictionary * _Nullable config,
                                                             CHRTouchInputMonitorBlock _Nullable monitor,
                                                             NSError * _Nullable * _Nullable error);

#pragma mark - 键盘状态读取

/// CHRTouchCopyKeyboardState 返回字典的键。宏定义(编译期字面量,不引入任何导出符号,不会冲突)。
#define CHRTouchKBStateVisible        @"visible"          ///< NSNumber(BOOL) 键盘是否显示
#define CHRTouchKBStateLanguage       @"language"         ///< NSString 当前语言/输入法标识(如 zh_Hans-Pinyin)
#define CHRTouchKBStateInputMode      @"inputMode"        ///< NSString 输入模式串(lang@sw=..;hw=..)
#define CHRTouchKBStateContextBefore  @"contextBefore"    ///< NSString 光标前文本(正常框即全文)
#define CHRTouchKBStateMarkedText     @"markedText"       ///< NSString 组合态文本(拼音等),无则不含该键
#define CHRTouchKBStateContextAfter   @"contextAfter"     ///< NSString 光标后文本
#define CHRTouchKBStateSelectedText   @"selectedText"     ///< NSString 选中文本
#define CHRTouchKBStateSecureEntry    @"secureTextEntry"  ///< NSNumber(BOOL) 是否密码框
#define CHRTouchKBStateCursorIndex    @"cursorIndex"      ///< NSNumber 光标位置(=光标前文本字符数)
#define CHRTouchKBStateSelectionLoc   @"selectionLocation"///< NSNumber 选区起点(=cursorIndex)
#define CHRTouchKBStateSelectionLen   @"selectionLength"  ///< NSNumber 选区长度(=selectedText 字符数)
#define CHRTouchKBStateReturnKeyType  @"returnKeyType"    ///< NSString return 键类型:default/go/join/next/route/search/send/done/continue/...
#define CHRTouchKBStateReturnKeyEnabled @"returnKeyEnabled" ///< NSNumber(BOOL) return 键当前是否可点(空框+enablesReturnKeyAutomatically 时为 NO)
#define CHRTouchKBStateKeyboardType   @"keyboardType"     ///< NSNumber UIKeyboardType 原始值(0=default/4=number/7=URL/...)
/// 说明:整框文字 = contextBefore + markedText + contextAfter;光标位置 = contextBefore 末尾(before|after 分界)。
/// contextBefore/After 是光标周围窗口,正常输入框窗口足够大、一次即读全。

/// 读当前键盘状态快照(单次快读)。服务不可用返回 nil;无焦点键盘时字典 visible=NO。
/// 键见上方 CHRTouchKBState* 宏。
CHR_TOUCH_KIT_API NSDictionary * _Nullable CHRTouchCopyKeyboardState(NSError * _Nullable * _Nullable error);

/// 便捷:键盘当前是否显示(有焦点输入框)。
CHR_TOUCH_KIT_API BOOL CHRTouchIsKeyboardVisible(void);

#pragma mark - 键盘状态通知(NSNotification)

/// 键盘状态变化通知(宏,无导出符号)。kbd 单向推送状态事件到本进程,SDK 转成本地 NSNotification
/// (在主线程 post)。用标准 [NSNotificationCenter defaultCenter] 观察即可,支持多观察者。
/// userInfo 键复用上方 CHRTouchKBState*(如 DocumentDidChange 带 contextBefore/markedText/... )。
#define CHRTouchKeyboardDidShowNotification            @"CHRTouchKeyboardDidShow"
#define CHRTouchKeyboardDidHideNotification            @"CHRTouchKeyboardDidHide"
#define CHRTouchKeyboardDocumentDidChangeNotification  @"CHRTouchKeyboardDocumentDidChange"

/// 开启键盘状态通知(幂等)。注册本进程回调服务并向 kbd 订阅;之后即可用 NSNotificationCenter 观察上面的通知。
/// 不调用则不会收到通知(避免无谓订阅)。
CHR_TOUCH_KIT_API void CHRTouchStartKeyboardNotifications(void);

#pragma mark - App 生命周期事件通知(Conduit / CDTNotificationCenter)

/// App 生命周期事件通知。SpringBoard 在外部观测任意 App 的进程 + 场景,**发生一个原子事件就广播一条**,
/// 通过 Conduit 通知中心(CDTNotificationCenter)发出。所有事件共用同一个通知名,用 userInfo[event] 区分类型。
///
/// 事件语义对齐 UIApplicationDelegate(BecomeActive/ResignActive/EnterForeground/EnterBackground/Terminate),
/// 并补上系统代理给不了的 Launch(进程诞生)/Suspend(挂起)/Resume(恢复),以及带原因的 Terminate(含崩溃/被杀)。
/// 注意是**外部事后观测**:事件都是"已经发生"后才收到,故除 ResignActive 外一律 Did* 语义。
///
/// 使用方自行用 Conduit 订阅(需链接 Conduit 并 #import <Conduit/objc.h>)：
///
///   CDTNotificationToken *tok =
///     [[CDTNotificationCenter defaultCenter]
///        addObserverForName:CHRTouchApplicationLifecycleNotification
///                    policy:CDTNotificationPolicyHold   // 事件是离散序列，务必用 Hold；默认 Coalesce 会丢中间事件
///                     queue:dispatch_get_main_queue()
///                usingBlock:^(CDTNotification *n) {
///          NSDictionary *info = n.userInfo;                   // 明文字典，无需解码
///          NSString *event    = info[CHRTouchAppKeyEvent];    // 见 CHRTouchAppEvent*
///          NSString *bundleId = info[CHRTouchAppKeyBundleIdentifier];
///          NSNumber *pid      = info[CHRTouchAppKeyPID];       // 哪个进程实例(重启会换 pid)
///          if ([event isEqual:CHRTouchAppEventTerminate]) {
///              NSString *why = info[CHRTouchAppKeyReason];     // 仅终止可能带:退出原因(可辨崩溃/被杀)
///          }
///        }];
///   // 取消订阅：[[CDTNotificationCenter defaultCenter] removeObserver:tok];
///
/// 要点：
///  - 发布方恒为 SpringBoard，n.senderPID / n.senderBundleID 由内核填写、不可伪造，可据此校验来源。
///  - **边沿触发**：只在事件发生时发一条，订阅前已处于某态的 App 不补发历史。需要当前态基线用
///    CHRTouchApplicationState() / CHRTouchRunningApplications() 拉一次(与本通知是互补的两个接口:
///    通知="刚发生了什么事件",查询="现在处于什么状态")。
///  - 典型序列:冷启动前台 `launch`→`becomeActive`；回桌面 `resignActive`→`enterBackground`→(数秒后)`suspend`；
///    从后台切回 `enterForeground`→`becomeActive`；前台内下拉通知再收起 `resignActive`→`becomeActive`(无 enterForeground);
///    被杀/崩溃 `terminate`(带 reason)。重启表现为老 pid `terminate` + 新 pid `launch`。
///  - `becomeActive`/`resignActive` 来自解析场景私有状态,随 iOS 版本相对脆;进程级事件(launch/terminate/
///    enterForeground/enterBackground/suspend/resume)是读系统字段,较稳。

/// Conduit 通知名。所有生命周期事件共用此名,用 userInfo[CHRTouchAppKeyEvent] 区分。
#define CHRTouchApplicationLifecycleNotification  @"chr:app-lifecycle"

/// userInfo 键
#define CHRTouchAppKeyEvent   @"event"   ///< NSString，事件类型，取值见 CHRTouchAppEvent*
#define CHRTouchAppKeyReason  @"reason"  ///< NSString，可选：仅 terminate 可能带，退出原因文本(辨正常退出/崩溃/被杀)
// 另含 bundleIdentifier(NSString) 与 pid(NSNumber，哪个进程实例)，键复用下方"App 状态查询"区的同名宏。

/// event 取值(11 个原子事件)
#define CHRTouchAppEventWillLaunch       @"willLaunch"          ///< 准备启动：用户点击图标或系统请求拉起前（紧接发出 allow/deny）
#define CHRTouchAppEventAllowLaunch      @"didAllowLaunch"      ///< 允许启动：未设拦截器默认允许，或拦截责任链全部放行
#define CHRTouchAppEventDenyLaunch       @"didDenyLaunch"       ///< 拒绝启动：被拦截器阻止或用户取消确认（reason 带原因）
#define CHRTouchAppEventLaunch           @"didLaunch"           ///< 启动：新进程诞生(非 App 内部"启动完成")
#define CHRTouchAppEventBecomeActive     @"didBecomeActive"     ///< 激活：变前台可交互
#define CHRTouchAppEventResignActive     @"willResignActive"    ///< 失活：前台失去交互(来电/下拉通知/控制中心)
#define CHRTouchAppEventEnterForeground  @"didEnterForeground"  ///< 进前台：从后台回到前台(前台内失焦恢复不发此事件)
#define CHRTouchAppEventEnterBackground  @"didEnterBackground"  ///< 进后台
#define CHRTouchAppEventSuspend          @"didSuspend"          ///< 挂起：后台被冻结(UIApplicationDelegate 无此回调)
#define CHRTouchAppEventResume           @"didResume"           ///< 恢复：从挂起恢复运行(UIApplicationDelegate 无此回调)
#define CHRTouchAppEventTerminate        @"didTerminate"        ///< 终止：进程结束(含强杀/崩溃，reason 带原因)

#pragma mark - App 状态查询(拉取快照,与订阅互补)

// 事件是边沿触发：订阅前就已运行的 App 不会补发。用下列查询接口在启动时拉一次当前态做基线，
// 再叠加事件即可（事件按 bundleIdentifier+state 幂等）。
//
// 快照 dict 的键与取值（键与生命周期通知 userInfo 对齐）：
#define CHRTouchAppKeyBundleIdentifier  @"bundleIdentifier"  ///< NSString
#define CHRTouchAppKeyPID               @"pid"               ///< NSNumber，未运行=0
#define CHRTouchAppKeyIsRunning         @"isRunning"         ///< NSNumber(BOOL)
#define CHRTouchAppKeyIsForeground      @"isForeground"      ///< NSNumber(BOOL)，前台(active/inactive)为 YES
#define CHRTouchAppKeyState             @"state"             ///< NSString，取值见下

#define CHRTouchAppStateNotRunning  @"notRunning"   ///< 未运行(仅单查会出现)
#define CHRTouchAppStateSuspended   @"suspended"    ///< 已挂起(进程在内存但被冻结)
#define CHRTouchAppStateBackground  @"background"   ///< 后台运行
#define CHRTouchAppStateInactive    @"inactive"     ///< 前台但失活(来电/控制中心等覆盖)
#define CHRTouchAppStateActive      @"active"       ///< 前台活跃可交互

// 已安装应用列表项(CHRTouchInstalledApplications)的键；bundleIdentifier 复用上方同名键。
#define CHRTouchAppKeyName                 @"name"                 ///< NSString，应用显示名(可能空串)
#define CHRTouchAppKeyVersion              @"version"              ///< NSString，短版本号(如 8.1.0)
#define CHRTouchAppKeyBundleVersion        @"bundleVersion"        ///< NSString，构建号
#define CHRTouchAppKeyTeamID               @"teamID"               ///< NSString，签名 TeamID(可能空串)
#define CHRTouchAppKeyApplicationType      @"applicationType"      ///< NSString，LS 类型(System/User/Internal)
#define CHRTouchAppKeyKind                 @"kind"                 ///< NSString，见 CHRTouchAppKind*
#define CHRTouchAppKeyIsSystemApplication  @"isSystemApplication"  ///< NSNumber(BOOL)，系统 App=YES
#define CHRTouchAppKeyIsPlaceholder        @"isPlaceholder"        ///< NSNumber(BOOL)，占位(下载/离线)App=YES
#define CHRTouchAppKeyIsUninstallSupported @"isUninstallSupported" ///< NSNumber(BOOL)，可被卸载=YES
#define CHRTouchAppKeyInstallDate          @"installDate"          ///< NSNumber，首次安装时间(Unix 秒，0=未知；取数据容器创建时间，更新不变、卸载重装重置)
#define CHRTouchAppKeyUpdateDate           @"updateDate"           ///< NSNumber，最近一次安装/更新时间(Unix 秒，0=未知；LaunchServices registeredDate)
#define CHRTouchAppKeyInstallPath          @"installPath"          ///< NSString，App bundle 安装路径
#define CHRTouchAppKeySandboxPath          @"sandboxPath"          ///< NSString，数据沙盒容器路径(可能空串)
#define CHRTouchAppKeyAppGroupPaths        @"appGroupPaths"        ///< NSDictionary，groupID→路径(可能空字典)
#define CHRTouchAppKeyIcon                 @"icon"                 ///< NSString，PNG 图标 base64(仅 IncludeIcon 时有)
#define CHRTouchAppKeyExtra                @"extra"                ///< NSDictionary，预留扩展

#define CHRTouchAppKindUser         @"user"        ///< 用户安装 App
#define CHRTouchAppKindSystem       @"system"      ///< 系统 App(System/Internal)
#define CHRTouchAppKindPlaceholder  @"placeholder" ///< 占位 App(下载中/离线待恢复)

/// 轴1 应用范围(按 kind 过滤,可组合)。
typedef NS_OPTIONS(uint32_t, CHRTouchAppScope) {
    CHRTouchAppScopeUser        = CHR_APPSCOPE_USER,        ///< 用户 App
    CHRTouchAppScopeSystem      = CHR_APPSCOPE_SYSTEM,      ///< 系统 App(System/Internal)
    CHRTouchAppScopePlaceholder = CHR_APPSCOPE_PLACEHOLDER, ///< 占位 App(下载中/离线待恢复)
    CHRTouchAppScopeAll         = CHR_APPSCOPE_ALL,
};

/// 轴3 字段选择(位掩码,uint64;bundleIdentifier 恒返回,不受此控制)。
typedef NS_OPTIONS(uint64_t, CHRTouchAppInfoFields) {
    CHRTouchAppInfoFieldName        = CHR_APPFIELD_NAME,        ///< name
    CHRTouchAppInfoFieldVersion     = CHR_APPFIELD_VERSION,     ///< version + bundleVersion
    CHRTouchAppInfoFieldKind        = CHR_APPFIELD_KIND,        ///< kind/isSystemApplication/isPlaceholder/applicationType/isUninstallSupported
    CHRTouchAppInfoFieldInstallDate = CHR_APPFIELD_INSTALLDATE, ///< installDate(首次安装) + updateDate(最近更新)
    CHRTouchAppInfoFieldPaths       = CHR_APPFIELD_PATHS,       ///< installPath + sandboxPath + appGroupPaths
    CHRTouchAppInfoFieldTeamID      = CHR_APPFIELD_TEAMID,      ///< teamID
    CHRTouchAppInfoFieldIcon        = CHR_APPFIELD_ICON,        ///< icon(base64,重,按需开)
    CHRTouchAppInfoFieldAll         = CHR_APPFIELD_ALL,
};

/// 运行中 App 快照。filterBundleIDs=nil → 全部在跑；传列表 → 只回其中在跑的(不跑的不返回)。
/// 返回 nil = 服务不可用/查询失败；@[] = 查询成功但无匹配。每项 dict 见上方键。
CHR_TOUCH_KIT_API NSArray<NSDictionary *> * _Nullable
CHRTouchRunningApplications(NSArray<NSString *> * _Nullable filterBundleIDs);

/// 当前前台 App 的 bundleId(多值，支持 iPad 分屏/台前调度)。filterBundleIDs 同上。
/// 返回 nil = 服务不可用；@[] = 无前台 App(如主屏/锁屏)。
CHR_TOUCH_KIT_API NSArray<NSString *> * _Nullable
CHRTouchForegroundApplications(NSArray<NSString *> * _Nullable filterBundleIDs);

/// 查单个 App 当前状态。返回 nil = 服务不可用；否则必回 dict(不跑则 isRunning=NO / state=notRunning)。
CHR_TOUCH_KIT_API NSDictionary * _Nullable
CHRTouchApplicationState(NSString *bundleIdentifier);

/// 已安装应用查询(经 LSApplicationWorkspace/LSApplicationProxy 读注册表,不管在不在跑)。三轴过滤:
///   1) scope           —— 圈定 user/system/placeholder(可组合;传 0 视为 All)。
///   2) filterBundleIDs —— nil=不限;传列表=只回列表中且已安装的(单查即传 @[一个包名])。
///   3) fields          —— 每项只装选中字段;bundleIdentifier 恒返回。
/// 返回 nil=服务不可用/失败;@[]=无匹配。每项 dict 键见上方 CHRTouchAppKey*(icon 仅 IncludeIcon 时有)。
/// 判"装没装":传 @[bid] + 看结果是否非空;配合 CHRTouchUninstallApplication 挑 isUninstallSupported=YES 卸。
CHR_TOUCH_KIT_API NSArray<NSDictionary *> * _Nullable
CHRTouchInstalledApplications(CHRTouchAppScope scope,
                              NSArray<NSString *> * _Nullable filterBundleIDs,
                              CHRTouchAppInfoFields fields);

/// 清空当前焦点输入框(读键盘状态得知文本长度,发等量退格,循环到空)。需已聚焦输入框。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchClearText(NSError * _Nullable * _Nullable error);

/// 发送确认/回车键，触发当前焦点框的 return 动作(搜索/发送/前往/换行，由字段决定)。
CHR_TOUCH_KIT_API CHRTouchStatus CHRTouchKeyboardReturn(NSError * _Nullable * _Nullable error);

#ifdef __cplusplus
}
#endif

#pragma mark - App 启动前拦截生命周期(Launch Interceptor)

/// 启动前拦截兜底决策枚举（用于超时或订阅进程掉线兜底）
typedef NS_ENUM(NSInteger, CHRTouchLaunchDecision) {
    CHRTouchLaunchDecisionAllow = 0,  ///< 放行：携带内部绕过标记由系统重新拉起该 App
    CHRTouchLaunchDecisionDeny  = 1,  ///< 拒绝：保持取消状态，阻止该 App 启动
};

/// 启动前拦截器回调返回结果（统一适配 GUI 与 API 双模式）
CHR_TOUCH_KIT_API
@interface CHRTouchLaunchResult : NSObject
@property (nonatomic, assign, readonly) CHRTouchLaunchDecision decision;
@property (nonatomic, copy, readonly, nullable) NSString *reason;
@property (nonatomic, assign, readonly) BOOL needsConfirm;

/// 放行本次启动（责任链中需全部处理者返回 allow 才最终启动）
+ (instancetype)allow;
/// 拒绝本次启动并附带原因（GUI 模式弹 Alert 提示原因；API 模式直接在启动接口返回失败和 reason）
+ (instancetype)denyWithReason:(nullable NSString *)reason;
/// 请求二次确认（GUI 模式在桌面弹出「取消 / 仍然启动」给用户反悔机会；API 模式不弹窗，直接视为未满足条件阻止启动并返回 reason）
+ (instancetype)confirmWithReason:(nullable NSString *)reason;
@end

/// 拦截上下文 dict 键（传入回调 context 中）
#define CHRTouchLaunchKeyRequestID        @"requestID"        ///< NSString，本次拦截请求唯一 ID
#define CHRTouchLaunchKeyBundleIdentifier @"bundleIdentifier" ///< NSString，即将启动的 App 包名
#define CHRTouchLaunchKeySource           @"source"           ///< NSNumber(long long)，系统原始过渡来源码
#define CHRTouchLaunchKeySourceLabel      @"sourceLabel"      ///< NSString，可读来源："icon"|"switcher"|"systemService"|"api-icon"|"api-lsapp"|"other"
#define CHRTouchLaunchKeyMode             @"mode"             ///< NSString，启动模式："gui"(桌面/多任务手动操作) | "api"(接口自动化启动)
#define CHRTouchLaunchKeyIsRunning        @"isRunning"        ///< NSNumber(BOOL)，拦截瞬间目标进程是否已在后台运行
#define CHRTouchLaunchKeyEventLabel       @"eventLabel"       ///< NSString，可选，系统原始过渡事件标签

/// 启动前拦截处理器（点对点可靠 RPC 回调）：
///   - 运行在业务进程自己的后台工作队列中（不会阻塞 SpringBoard 主线程，可安全等待开 VPN 或校验环境）。
///   - 直接返回 CHRTouchLaunchResult（allow / denyWithReason: / confirmWithReason:）。
typedef CHRTouchLaunchResult * _Nonnull (^CHRTouchLaunchInterceptorHandler)(NSDictionary *context);

#ifdef __cplusplus
extern "C" {
#endif

/// 注册「App 启动前拦截器」（独立于只读通知流，与 CHRTouchApplicationLifecycleNotification 互补）。
///
/// 架构特性：
///   - 多订阅者责任链（FIFO，一票否决）：支持多个模块同时注册，按注册顺序依次询问；任一节点拒绝或未确认即短路阻止，全通过才放行。
///   - 点对点可靠 RPC：业务进程在本地开启 CHRBus 回调服务，若崩溃/掉线 SpringBoard 毫秒级感知并立即走兜底策略。
///   - 双模式自适应：GUI 模式下 deny/confirm 自动在桌面弹原生 Alert；API 模式下绝不弹窗，直接由启动接口返回错误原因。
///   - 连点去重：决策期间重复点击同一业务 App 图标直接忽略并在屏幕弹出轻量 Toast 提示。
///
/// 参数说明：
///   - bundleIdentifiers : 要拦截的业务 App 包名列表（不建议填无关系统 App；传 @[@"*"] 匹配全部）。
///   - timeout           : 等待业务方返回决策的超时秒数（传 <=0 默认 10.0 秒；最大 120.0 秒，足够支撑真人弹窗交互）。
///   - fallbackDecision  : 当业务方超时未返回或本进程意外退出时，SpringBoard 的兜底动作（Allow/Deny）。
///   - pendingToast      : 重复点击正在决策中的业务 App 时显示的轻量提示文案（传 nil 用默认 @"正在处理中，请稍候..."；传 @"" 关闭提示）。
///   - handler           : 拦截决策回调。
///
/// 返回值：拦截器 token（传给 CHRTouchUnregisterLaunchInterceptor 注销）；注册失败返回 nil。
/// 注意：若在 handler 内需要拉起辅助 App（如 VPN App），请勿将该辅助 App 放入 bundleIdentifiers 中，以免互相拦截。
CHR_TOUCH_KIT_API id _Nullable
CHRTouchRegisterLaunchInterceptor(NSArray<NSString *> *bundleIdentifiers,
                                  NSTimeInterval timeout,
                                  CHRTouchLaunchDecision fallbackDecision,
                                  NSString * _Nullable pendingToast,
                                  CHRTouchLaunchInterceptorHandler handler);

/// 注销启动前拦截器（传入注册时返回的 token）。
CHR_TOUCH_KIT_API void
CHRTouchUnregisterLaunchInterceptor(id _Nullable token);

#ifdef __cplusplus
}
#endif

NS_ASSUME_NONNULL_END
