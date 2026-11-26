//
//  IRAutomationKit.h
//  IRAutomationKit — 远程(无注入)iOS 自动化 SDK;建在 IRemoteKit(感知)+ CHRTouchKit(操作)之上。
//

#import <Foundation/Foundation.h>

FOUNDATION_EXPORT double IRAutomationKitVersionNumber;
FOUNDATION_EXPORT const unsigned char IRAutomationKitVersionString[];

// —— 公开 API ——
#import <IRAutomationKit/IRApplication.h>   // 应用门面:按 bundleID 持久实例 + 生命周期 + UI 自动化 (currentScreen / setAutomation:)
#import <IRAutomationKit/IRAgent.h>         // 代理基类:所属应用 (+bundleIDs)
#import <IRAutomationKit/IRAgentRegistration.h>   // 注册配置协议 (基础):+registerAgent: 的 block 里填写, 各种 Agent 的子协议在各自头文件里
#import <IRAutomationKit/IRResponderChain.h>   // 响应链 + 节点 (正式接口, 带判断用的语法糖)
#import <IRAutomationKit/IRViewControllerHierarchy.h>   // pvc:VC 层级树 (_printHierarchy 解析成的节点树)
#import <IRAutomationKit/IRChainContext.h>   // 识别时的现场:响应链 + 当前节点 (传入协议方法)
#import <IRAutomationKit/IRScreenState.h>   // 感知结果:当前页面 + 链上第一个需要自动处理的 Agent
#import <IRAutomationKit/IRPageAgent.h>     // 控制器 Agent:+registerAgent: 注册, pageID 标识页面
#import <IRAutomationKit/IRPageResolving.h>   // 页面判断协议:控制器复用时按响应链判断实例是哪个页面
#import <IRAutomationKit/IRViewAgent.h>     // 视图 Agent:+registerAgent: 注册 (控件 / 视图型弹窗)
#import <IRAutomationKit/IRWindowAgent.h>   // 窗口 Agent ↔ UIWindow
#import <IRAutomationKit/IRApplicationAgent.h>   // 应用 Agent ↔ UIApplication
#import <IRAutomationKit/IRResponderAgent.h>   // 通用 Agent ↔ UIResponder (UIWindowScene 等)
#import <IRAutomationKit/IRTask.h>          // 任务基类:状态由应用写, 按应用生命周期启动/挂起/恢复
#import <IRAutomationKit/IRTimerTask.h>     // 定时任务:GCD 定时器按间隔回调 timerDidFire
#import <IRAutomationKit/IRUITask.h>         // UI 任务基类:任务线程 / 挂起 / 检查点 / 不可挂起区
#import <IRAutomationKit/IRAutomationTask.h>   // 自动化任务:操作 UI 的业务流程, 每个应用一个槽位 (setAutomation:)
#import <IRAutomationKit/IRInterruptHandling.h>   // 自动处理协议 (shouldInterruptInContext: / handleInterrupt:) + 上下文
#import <IRAutomationKit/IRInterruptTask.h>   // 中断任务:UI 执行权按感知结果创建, 同一时间最多一个
#import <IRAutomationKit/IRDiagnostics.h>   // 诊断:错误 / 警告 / 过程日志, 调用方显式传入 (拷贝自 CHRFoundation)
#import <IRAutomationKit/IRCache.h>         // 解析结果缓存: 进程级 (内存) / 开机级 / 系统级 (落文件)
#import <IRAutomationKit/IRTimeRecorder.h>  // 分段计时 (拷贝自 CHRFoundation)
#import <IRAutomationKit/IRIdentity.h>      // 标识:名字 + 说明 (IRIdentityMake)
#import <IRAutomationKit/IRActionBuilder.h>   // 动作构建器: 签名 (入参 / 返回值)、支持的模式、执行体 (registerAction:builder:)
#import <IRAutomationKit/IRActionSignature.h>   // 事件签名: 入参 / 返回值 / 支持的模式 (ACTION.md 第 30 项)
#import <IRAutomationKit/IRActionContext.h> // 动作的执行现场
#import <IRAutomationKit/IRApplication+IRTouch.h>   // 触摸操作:点击 / 长按 / 滑动 / 回放 (屏幕 points, 写操作, 诊断显式传入)
#import <IRAutomationKit/IRApplication+IRWait.h>   // 等待:waitPage / waitPageChange / waitUntil (读, 任何线程, 主队列除外)
#import <IRAutomationKit/IRApplication+IRInput.h>   // 文字输入 / 清空 / 回车 (写) + 键盘状态 keyboardVisible / keyboardInfo (读)
#import <IRAutomationKit/IRKeyboardInfo.h>   // 键盘状态快照:整框文字 / 光标 / return 键 / 输入法
#import <IRAutomationKit/IRUINode.h>   // 视图节点 (底层): 只有视图; 一个节点就是一棵子树; 树根可 refresh, 树内节点不可变, detach 分离成新树
#import <IRAutomationKit/IRUIElement.h>   // 元素 (主要接口): 抽象节点 = 表达式 (同 CHRUIElement), UI 对象 / 查询 / 父元素三种来源, 取结果时才执行
#import <IRAutomationKit/IRUIElementQuery.h>   // 查询链: 过滤 / 跳转 / 切片 / 刷新点 (mode / refresh) / 收尾 (element / elements / wait)
#import <IRAutomationKit/IRApplication+IRUIElement.h>   // app.element / app.node / app.chain
#import <IRAutomationKit/IRAgent+IRUIElement.h>   // agent.element (表达式) / agent.node(diag) (现拍, 树根)
#import <IRAutomationKit/IRNSObject.h>   // 系统类远程包装的根类: 包着 IRObject, 实例比较 / 类名 / isKindOfClassNamed (纯读内存)
#import <IRAutomationKit/IRInstanceScanner.h>      // 实例扫描: 按类名定期扫实例, 报 新发现 / 不在了 / 每轮扫到 (页面变化 + 定时 + 手动)
#import <IRAutomationKit/IRUIView.h>   // UIView 的远程包装 (window / viewController)
#import <IRAutomationKit/IRUIScrollView.h>   // UIScrollView 的远程包装: 偏移 / 速度 / 惯性终点 / 手指按着 / 惯性中 (纯读内存, 每帧可读)
#import <IRAutomationKit/IRUITableView.h>   // UITableView 的远程包装: visibleCells / 可见行 / 行数 / cell ↔ indexPath / 行的位置 (远程调用, 带 diag)
#import <IRAutomationKit/IRUICollectionView.h>   // UICollectionView 的远程包装: visibleCells / 可见 item / 个数 / cell ↔ indexPath / item 的位置 (远程调用)
#import <IRAutomationKit/IRScrollMonitor.h>   // 滚动监视: 采样线程按刷新率读滚动视图 ivar, 等 按着 / 松手 (惯性终点) / 停稳 / 越过位置
#import <IRAutomationKit/IRScrollPlaybackPlan.h>   // 滚动用的回放方案: 往前 / 往后两个 cht 方案 (按测量距离挑录制); defaultPlan
#import <IRAutomationKit/IRScrollStep.h>   // 滚动的一步: 来源 (拖动 / 按距离挑录制 / 指定录制) × 结束方式, 另有点停 / 停顿; 执行后回填结果
#import <IRAutomationKit/IRScrollContext.h>   // 滚动上下文: 列表视图 + 执行过的步骤 + userInfo, 最新情况实时读视图
#import <IRAutomationKit/IRScrollStrategy.h>   // 滚动策略: 每步给出下一步, nil = 结束; 按名字注册 / 指定
#import <IRAutomationKit/IRScrollBuiltinStrategies.h>   // 内置滚动策略: 滑-点 / 拖动 / 小步快跑 / 快速步进 / 连续快甩 / 过头回看 / 自动 / 纠正 (参数在类上)
#import <IRAutomationKit/IRScrollOperation.h>
#import <IRAutomationKit/IRListScroller.h>   // 照 UIKit 滚动 API 的列表滚动 (scrollToIndexPath:atPosition: / scrollRectToVisible: / scrollBy: …), 模拟触摸实现   // 滚动操作: 竖直滚动 (方向 + 距离 + 偏差), 跑策略循环, 结果在对象上
#import <IRAutomationKit/IRDragGesture.h>   // 合成拖动手势: 按帧 按下 → 移动 → 抬起, 每帧回调, 不带惯性 (精准拖动)
#import <IRAutomationKit/IRAgent+IRAction.h>   // 动作:Agent 能做哪些动作 (registerAction:block: / invokeAction:)
#import <IRAutomationKit/IRPageNode.h>   // 页面节点 (流程层原始写法): 期望页面 + 绑定诊断, invoke 时检查当前页面再调动作
#import <IRAutomationKit/IRFlowException.h>   // 流程里的一次失败: 交给 onException 选重试 / 跳过 / 终止
#import <IRAutomationKit/IRFlowSession.h>   // 流程会话 (语法糖, 复刻 CHRFlowSession): session.page 建节点, 失败统一交给 onException, 终止退出 execute
#import <IRAutomationKit/IRWatchTimeline.h>   // 声明式看播时间轴: 按总时长百分比调度伴生动作, 先注册后启动
#import <IRAutomationKit/IRPageNode+IRWatchTimeline.h>   // IRPageNode (IRWatchTimeline): node.timeline 与链式语法糖 (on / waitMinInterval / start)
#import <IRAutomationKit/IRDecisionPoint.h>   // 页面级决策点 (registerDecisionPoint:builder: + node.decisionPoint(...).execute())
#import <IRAutomationKit/IRPageAgent+IRListBrowse.h>   // 列表浏览 (BROWSE.md): 一个场景一个 flow 事件, 询问 → 操作循环 (查看 / 滑动 / 退出)
#import <IRAutomationKit/IRListBrowseContext.h>         // 列表浏览的 步 (当前这一步, 在上面做决定) + 上下文 (这个列表, 可继承)
#import <IRAutomationKit/IRListBrowseRemote.h>          // 列表浏览的远程决策 (发请求的接口 + 全局单例)
#import <IRAutomationKit/IRListBrowseDelegate.h>        // 列表浏览的代理 (统计通知 + 每步决策前给业务数据)

// 权重模块 (独立, 可选; SDK 核心不依赖它): 一比一复刻 CHR Weight + IRPageAgent (IRWeight) 对接
#import <IRAutomationKit/IRWeight.h>
