# 静默失败目录(不报错,但读错 / 点错)（`DOC-07`）

> 文档编号：`DOC-07`（可在 `skills/` 下用 `rg "DOC-07"` 定位本文档）

写完对一遍。每条:症状 → 原因 → 正确做法。

| # | 症状 | 原因 | 正确做法 |
|---|---|---|---|
| 1 | 页面一直识别不出(`pageID` 为 nil),没报错 | 没注册 / 注册被跳过(条件注册、同一个类注册了两次第二次失败) | 看 `registerAgent` 的返回值;到页面上打 `app.currentAgent` 看实际类和 `bindClassName` |
| 2 | 脚本一开始就点不到东西 | 假设了起始页面,实际停在上次留下的别的页面 | 开头先 `app.currentAgent.pageID` 确认,不对先回首页 |
| 3 | 读到的文字和页面对不上,怎么滑都不变 | 从整屏查,读到了**被盖在下面的上一个页面**(它的视图还在树里、几何上也"可见") | 从当前页面查:`app.currentAgent.query…` |
| 4 | `waitPage` 立即返回但页面没变 | 已经是这个 pageID(旧页面) | `waitPageChange(before)` + 看新页面的 `pageID` |
| 5 | 页面对了,内容是旧的 / 占位的 | 页面对象先到,内容后加载 | 再等一个代表新内容的元素 `query.wait` |
| 6 | `app.query.text(@"登录")` 永远 0 个 | `text` 是过滤,链头只过滤起点自己 | 找文字用 `texts(@"登录")` |
| 7 | `views(@"X").first()` 点到屏幕外 / 被裁的那个 | 默认不按可见性筛,预加载 cell 里同样有 X | 先 `visible()`(必要时 `hittable()`)再 `first()` |
| 8 | label 判成"点不到" | UILabel 默认不接收触摸,hitTest 命中的是外层控件 | 文字要判断点得到:找外层控件 `texts(…).ancestor(@"UIControl").hittable()`;或直接点文字位置 |
| 9 | 存下来的元素一直是旧数据 / 一直是新数据 | `node.element` 用节点那份数据;`app.element` 每次执行现拍 | 要新数据就从 `app` / 页面重新查,或链上 `refresh()` |
| 10 | 点 / 输入失败"不在 UI 任务的线程上" | 在测试线程 / 主线程直接调写操作 | 放进 `IRAutomationTask`,`[app setAutomation:task]` |
| 11 | 应用下次启动弹"崩溃"提示 | 用 force-kill(前台 SIGKILL)关应用 | `cht exit <bundleID> switcher-card`(多任务划掉),不强杀 |
| 12 | `issh exec` 跑到一半断了 | 默认超时 30 秒 | 长命令加 `-t 300` |
| 13 | 调试红框没出来 | 实例开关没开(`app.element` 每次都是新的) | `app.debugEnabled = YES`(之后的元素都继承);宿主主线程要在跑 runloop |
| 14 | 手写坐标滑动在别的机型上滑偏 | 坐标写死(如 650 → 220) | 用回放标识 `app.playback`,或按控件 frame 算 |
| 15 | `issh file pull` 报 No such file / 拉完找不到文件 | roothide 下 shell 路径要加 `/rootfs` 前缀;本地写了相对路径 | 设备路径 `/rootfs/var/mobile/…`,本地写绝对路径 |
| 16 | `issh exec "cat x.png" > x.png` 拉回的图打不开 | issh 把 stdout 当文本解码,二进制被改坏 | 设备端 base64(`mpa screenshot --base64`),Mac 上 `base64 -d` |
| 17 | 流程会话里点了 `onException` 还是终止了 | 在 `onException` 里又用会话的节点、又失败了(回调里再失败直接终止);或超过 10 次重试 | 回调 / `onFinally` 里用原始写法 `IRPageNode(pageID, diag)`,失败只返回 NO |
| 18 | 流程终止后内存涨 / 锁住了 | 终止靠异常:业务工程没加 `-fobjc-arc-exceptions`;block 里拿着锁调了节点 | 加编译选项;锁 / 资源用 `@try/@finally`;节点别放进 `dispatch_sync` |
| 19 | `@interface IRPageNode (X)` 编译报宏参数错误 | `IRPageNode(…)` 是函数式宏,吃掉了分类的括号 | 先 `#pragma push_macro("IRPageNode")` + `#undef`,写完 `pop_macro` |
| 20 | 列表滑动陷入死循环(空转几十轮, 日志飞涨) | 滑动虽返回 YES，但实际上由于到底或边界，条目并未前移；View 阶段跳过、Slide 阶段继续滑动，互相踢皮球无限循环 | 必须设 `maxItems` 最大步数硬上限；记录 `last_item_id`，连续 2 次滑动当前条目未变化时强制触发安全退出 |
| 21 | 设备突然无响应 / 卡死 / 看门狗 Panic 重启 | 高频 IPC 占满目标主线程，此时又执行了带 `task_suspend` 挂起进程特性的 `mpa hitchain` 命令；当目标持有渲染锁被挂起时，直接引发看门狗超时保护导致真机重启 | 严禁在高频循环或目标主线程重负荷时盲目调用 `hitchain`；依靠代码逻辑的防死循环熔断退出，不要用挂起调试命令探测活死锁状态 |
| 22 | `[R-FLOW-04]` 手动沿 `_parentViewController` 爬链耗时慢，或用 `hasPrefix:` 匹配 `_hmd_subfix_` 类名遗漏/冗余 | 自己写静态函数逐层 `ivarNamed:@"_parentViewController"` 跨进程查父控制器，重复造 `IRResponderChain` 的轮子 | 直接读内存快照里的 `self.application.chain`（`contains(...)` / `firstNodeOfClass:` / `IRResponderNode.parentViewController`），零 IPC 且框架已自动剥离 `NSKVONotifying_` / `_hmd_subfix_` |
