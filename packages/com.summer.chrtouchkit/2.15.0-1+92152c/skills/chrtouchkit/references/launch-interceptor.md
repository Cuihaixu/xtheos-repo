# App 启动拦截器（Launch Interceptor）

在特定业务 App 真正拉起之前触发拦截回调，业务方在自己进程的工作线程完成前置处理（如开启 VPN、校验运行环境）或交互确认后，直接返回决策控制是否放行。

---

## 一、 核心特性与行为契约

1. **物理零阻塞 SpringBoard**：拦截触发时 SpringBoard 主线程立刻释放，通过点对点可靠 RPC（`CHRBus`）反向询问业务进程；不会引发桌面假死或看门狗（Watchdog）重杀。
2. **多订阅者责任链（FIFO，一票否决）**：多个模块可同时注册拦截同一 App，按注册先后顺序依次询问；链上任一节点返回拒绝或未确认，立刻短路终止；全部返回 `allow` 才最终启动。
3. **GUI 与 API 双模式自适应**：
   - **GUI 模式**（`mode == @"gui"`，桌面点图标/多任务切换）：
     - 返回 `denyWithReason:` → SpringBoard 原生在桌面弹 Alert 提示具体拒绝原因。
     - 返回 `confirmWithReason:` → SpringBoard 弹「取消 / 仍然启动」确认框，给真人反悔机会。
     - 决策期间连续点击同一图标 → 自动忽略重复请求并在屏幕上方弹出轻量 Toast 提示（触摸穿透，1.8s 淡出）。
   - **API 模式**（`mode == @"api"`，通过 `CHRTouchLaunchApplication*` 启动）：
     - 绝不弹出阻塞式 Alert（避免无人值守脚本卡死）。
     - 返回 `denyWithReason:` 或 `confirmWithReason:` 时，启动接口直接返回 **`CHRTouchStatusLaunchIntercepted`（-14）**，并通过 `NSError **` 带出 `reason`。
4. **断联与崩溃瞬时兜底**：若订阅进程崩溃/退出，RPC 报错瞬间触发 `fallbackDecision` 并自动清除失效规则，不会锁死手机。

---

## 二、 API 参考（`CHRTouchManual.h`）

```objc
typedef NS_ENUM(NSInteger, CHRTouchLaunchDecision) {
    CHRTouchLaunchDecisionAllow = 0,  // 放行
    CHRTouchLaunchDecisionDeny  = 1,  // 拒绝
};

@interface CHRTouchLaunchResult : NSObject
+ (instancetype)allow;
+ (instancetype)denyWithReason:(nullable NSString *)reason;
+ (instancetype)confirmWithReason:(nullable NSString *)reason;
@end

// context 字典键
#define CHRTouchLaunchKeyRequestID        @"requestID"        // NSString: 请求 UUID
#define CHRTouchLaunchKeyBundleIdentifier @"bundleIdentifier" // NSString: 目标 App 包名
#define CHRTouchLaunchKeySource           @"source"           // NSNumber: 原始来源码 (1/3/13)
#define CHRTouchLaunchKeySourceLabel      @"sourceLabel"      // NSString: "icon" | "switcher" | "systemService" | "api-icon" | "api-lsapp" | "other"
#define CHRTouchLaunchKeyMode             @"mode"             // NSString: "gui" | "api"
#define CHRTouchLaunchKeyIsRunning        @"isRunning"        // NSNumber(BOOL): 进程是否已在后台运行
#define CHRTouchLaunchKeyEventLabel       @"eventLabel"       // NSString: 系统过渡事件名（可选）

typedef CHRTouchLaunchResult * _Nonnull (^CHRTouchLaunchInterceptorHandler)(NSDictionary *context);

// 注册 / 注销
id _Nullable CHRTouchRegisterLaunchInterceptor(
    NSArray<NSString *> *bundleIdentifiers,
    NSTimeInterval timeout,                     // <=0 默认 10s；最大 120s
    CHRTouchLaunchDecision fallbackDecision,    // 超时或断联兜底
    NSString * _Nullable pendingToast,          // 连点提示；nil 用默认，@"" 关闭
    CHRTouchLaunchInterceptorHandler handler
);
void CHRTouchUnregisterLaunchInterceptor(id _Nullable token);

// API 启动（带错误透传）
CHRTouchStatus CHRTouchLaunchApplicationWithError(
    NSString *bundleIdentifier,
    CHRTouchApplicationLaunchMethod method,
    NSError * _Nullable * _Nullable error
);
```

---

## 三、 使用示例

### 示例 1：启动前自动开 VPN，失败时让用户选择反悔或强行打开（GUI / API 通用）

```objc
#import <CHRTouchKit/CHRTouchKit.h>

@interface MyGuardDaemon ()
@property (nonatomic, strong, nullable) id launchToken; // 务必强引用持有
@end

@implementation MyGuardDaemon

- (void)startGuard {
    self.launchToken = CHRTouchRegisterLaunchInterceptor(
        @[ @"com.ss.android.ugc.aweme", @"com.zhihu.ios" ], // 勿将 VPN App 自身放入
        30.0,                                               // 留足 30s 给开 VPN 与真人确认
        CHRTouchLaunchDecisionDeny,                         // 进程掉线时宁可拦住
        @"正在检查 VPN，请稍候...",                           // 连续点击时的 Toast 提示
        ^CHRTouchLaunchResult *(NSDictionary *ctx) {
            NSString *bundle = ctx[CHRTouchLaunchKeyBundleIdentifier];
            NSString *mode   = ctx[CHRTouchLaunchKeyMode]; // "gui" 或 "api"

            // 1. VPN 已连接 → 直接放行
            if ([MyVPNManager isConnected]) {
                return [CHRTouchLaunchResult allow];
            }

            // 2. 尝试同步自动开启 VPN（跑在工作线程，不会卡死桌面）
            if ([MyVPNManager connectSyncWithTimeout:6.0]) {
                return [CHRTouchLaunchResult allow];
            }

            // 3. VPN 没连上：GUI 弹窗给真人反悔机会；API 自动返回拒绝原因
            if ([mode isEqualToString:@"gui"]) {
                return [CHRTouchLaunchResult confirmWithReason:
                    [NSString stringWithFormat:@"VPN 启动失败，是否仍要启动 %@？", bundle]];
            }
            return [CHRTouchLaunchResult denyWithReason:@"VPN 未连接，自动化启动已阻止"];
        }
    );
}

- (void)stopGuard {
    CHRTouchUnregisterLaunchInterceptor(self.launchToken);
    self.launchToken = nil;
}

@end
```

### 示例 2：只拦桌面手点图标冷启动（后台热切回不拦）

```objc
self.launchToken = CHRTouchRegisterLaunchInterceptor(
    @[ @"com.target.app" ],
    10.0,
    CHRTouchLaunchDecisionAllow,
    nil,
    ^CHRTouchLaunchResult *(NSDictionary *ctx) {
        BOOL isRunning   = [ctx[CHRTouchLaunchKeyIsRunning] boolValue];
        NSString *source = ctx[CHRTouchLaunchKeySourceLabel];

        // 后台切回前台 或 非桌面点击，直接放行
        if (isRunning || ![source isEqualToString:@"icon"]) {
            return [CHRTouchLaunchResult allow];
        }
        return [MyEnvChecker isReady]
            ? [CHRTouchLaunchResult allow]
            : [CHRTouchLaunchResult denyWithReason:@"环境未就绪，禁止冷启动"];
    }
);
```

### 示例 3：API 模式调用启动并处理被拦截原因

```objc
NSError *err = nil;
CHRTouchStatus status = CHRTouchLaunchApplicationWithError(@"com.ss.android.ugc.aweme",
                                                           CHRTouchApplicationLaunchMethodIcon,
                                                           &err);
if (status == CHRTouchStatusLaunchIntercepted) {
    NSLog(@"启动被拦截阻止，原因: %@", err.localizedDescription);
    // 例如：先调用开 VPN 流程，成功后再重新启动
} else if (status == CHRTouchStatusOK) {
    NSLog(@"启动成功");
}
```

---

## 四、 配套生命周期通知事件（`CHRTouchApplicationLifecycleNotification`）

无论目标 App 是否注册了拦截器，启动阶段均会向 `CHRTouchApplicationLifecycleNotification` (`chr:app-lifecycle`) 广播完整状态：
- `CHRTouchAppEventWillLaunch` (`@"willLaunch"`)：准备启动（触发拦截或系统拉起前）
- `CHRTouchAppEventAllowLaunch` (`@"didAllowLaunch"`)：允许启动（无拦截器默认允许，或拦截责任链全通过时）
- `CHRTouchAppEventDenyLaunch` (`@"didDenyLaunch"`)：拒绝启动（被拦截器或用户取消阻止，`reason` 键带原因）

典型序列：
- **未设拦截器 / 放行**：`willLaunch` → `didAllowLaunch` → `didLaunch` → `didBecomeActive`
- **拦截拒绝**：`willLaunch` → `didDenyLaunch`（无后续 `didLaunch`）
