---
name: irautomation-process
description: 快速开始——用 IRAutomationKit 做一个自动化进程(守护进程):建 target、接库、每个应用一个应用对象管生命周期和开关、页面自己注册、提交自动化、部署跑通;以及进程要维护哪些功能。触发词:自动化进程、自动化守护进程、新建自动化 target、接入 IRAutomationKit、快速开始、加一个应用的自动化。
---

# irautomation-process:自动化进程快速开始

一个进程可以承载多个应用的自动化。进程里**每个应用 = 一个应用对象 + 一组页面 Agent**:
应用对象管生命周期和开关,页面 Agent 自己注册识别 / 事件 / 权重 / 控制流。

## 快速开始

1. **创建守护进程**:xtheos 守护进程模板(带 `-rootful/-rootless/-roothide` 打包 target)。
   - 链接:`-framework IRAutomationKit -framework IRemoteKit -framework CHRTouchKit`(本机 `~/xtheos/vendor/frameworks`)
   - 编译:`-fobjc-arc-exceptions`
2. **配置守护进程权利**:把 [`resources/automation.entitlements`](resources/automation.entitlements) 的内容整份替换进 target 的权利文件
   (模板生成的 `xtheos.entitlements`,即 `CODE_SIGN_ENTITLEMENTS` 指向的那个)。改权利必须重编重装。
3. **定应用前缀**:每个应用一个类前缀,只需在本进程内不冲突。
4. **写页面基类**:只在这里写所属应用,页面全部继承它。
   ```objc
   @interface AWBPageAgent : IRPageAgent @end
   @implementation AWBPageAgent
   + (NSArray<NSString *> *)bundleIDs { return @[ @"com.ss.iphone.ugc.Aweme" ]; }
   @end
   ```
5. **写页面**:每个页面在自己的 `+load` 里注册识别、原子事件、权重、控制流。按 `irautomation` skill 的
   `references/DOC-01-structure.md` 组织(`PageAgent` 结尾、复杂页面拆分类、一个页面一个文件夹)。
6. **写应用对象**:实现 `IRApplicationDelegate`,管生命周期和开关,激活时提交自动化。
   ```objc
   @interface AWBAwemeApplication : NSObject <IRApplicationDelegate>
   @property (nonatomic, strong, readonly) IRApplication *application;
   @property (atomic, assign) BOOL autoRun;      // 开关
   + (instancetype)shared;
   - (void)start;
   @end

   @implementation AWBAwemeApplication
   - (void)start {
       _application = [IRApplication applicationWithBundleID:@"com.ss.iphone.ugc.Aweme"];   // 没运行也能拿到
       _application.delegate = self;
       IRApplication *app = _application;
       dispatch_async(app.queue, ^{   // 回调是边沿触发: 设代理前已经 Active 的不会补发, 自己看一眼
           if (app.state == IRApplicationStateActive) [self startAutomationIfNeeded];
       });
   }
   - (void)applicationDidBecomeActive:(IRApplication *)application {   // 会反复触发 (收起通知栏也会)
       [self startAutomationIfNeeded];
   }
   - (void)startAutomationIfNeeded {
       if (!self.autoRun || self.application.currentAutomation != nil) return;   // 关着 / 已在跑 (后台回来 SDK 自动恢复)
       IRAutomationTask *task = [IRAutomationTask automationWithName:@"main" block:^(IRAutomationTask *t) {
           [IRFlowSession execute:^(IRFlowSession *session) { /* 控制流: session.page(@"…").invoke(@"…") */ }
                      onException:^(IRFlowException *exception) { /* 重试 / 跳过 / 不处理 = 终止 */ }
                        onFinally:^(IRFlowSession *session, IRDiagnostics *diag) { /* 回根页面 */ }
                             diag:nil];
       }];
       task.completionBlock = ^{ /* 在 app.queue 上: 正常结束 / 取消 / 应用退出中止, 看 task.error */ };
       [self.application setAutomation:task];
   }
   @end
   ```
7. **写 main**:每个应用调一次 `start`,然后常驻。
   ```objc
   int main(int argc, const char *argv[]) {
       @autoreleasepool {
           NSLog(@"IRAutomationKit %.0f", IRAutomationKitVersionNumber);   // 记下用的是哪一版库
           [AWBAwemeApplication shared].autoRun = YES;
           [[AWBAwemeApplication shared] start];
           [[NSRunLoop currentRunLoop] run];
       }
       return 0;
   }
   ```
8. **部署跑通**:`xcodebuild -project <工程> -scheme <target>-roothide -destination 'id=<UDID>' build`;
   设备上 `launchctl list | grep <label>` 在跑,手动打开应用,看日志里自动化开始、走完。

## 进程要维护的功能

| 功能 | 谁做 | 要点 |
|---|---|---|
| 应用实例 | SDK | `applicationWithBundleID:` 同一 bundleID 永远同一实例,应用没运行也能拿到 |
| 页面识别 / 事件 / 权重归属 | SDK | 页面类 `+load` 注册,按页面基类的 `+bundleIDs` 归到应用,不用手动关联 |
| 启动时补判断当前状态 | **进程** | 回调边沿触发,`start` 里自己看一眼 `state` |
| 激活时提交自动化 | **进程** | `applicationDidBecomeActive:` 会反复触发,提交前查开关和 `currentAutomation`,不重复提交 |
| 开关 | **进程** | 每个应用一个(应用对象的 `autoRun`);来源(写死 / 文件 / 远程)由进程决定 |
| 失活 / 后台时挂起,回前台恢复 | SDK | 业务什么都不用做 |
| 应用退出时中止自动化 | SDK | 下一个检查点中止,回调 `completionBlock`;下次激活由进程重新提交 |
| 弹窗等自动处理 | SDK | 遵守 `IRInterruptHandling` 的 Agent,槽位里有自动化时自动处理 |
| 结束记录 | **进程** | `completionBlock` 里记 `task.error`(成功 / 失败 / 中止) |
| 日志 | **进程** | 启动打印库版本;流程用会话的 diag,失败打 `diag.errorMessage` |
| 卡住时保留现场 | **进程**(以后) | 截图 + 响应链 + 视图树 + VC 层级存本地,再决定是否重启应用 |

## 注意

- 所有回调都在 `application.queue` 上:回调里别做等待 / 长时间阻塞(主队列上的等待会被拒),操作界面放进自动化任务里。
- 测试和调试不要强杀应用(抖音会弹崩溃提示),用多任务划掉。
- 写自动化流程本身(查控件、动作、等待、权重、流程会话)见 `irautomation-write` 和 `irautomation` 的 references。
