# 调试命令(mpa)与调试显示（`DOC-10`）

> 文档编号：`DOC-10`（可在 `skills/` 下用 `rg "DOC-10"` 定位本文档）

**mpa 随 IRAutomationKit 插件包装到设备**(装插件即有),在设备上运行;从 Mac 上:
`issh exec <设备> "mpa <进程名|bundleID> <命令> [参数]" -t <秒>`(默认超时 30 秒,长命令加 `-t`)。


## 目录索引

- [`[DOC-10.1]` 常用命令](#DOC-10.1)
- [`[DOC-10.2]` 调试显示(画在远程画板 RemoteCanvas 上:所有应用之上,不挡触摸)](#DOC-10.2)
- [`[DOC-10.3]` 跨工具分工速查（`mpa` vs `mpx` vs `cht`）](#DOC-10.3)

---

<a id="DOC-10.1"></a>
## `[DOC-10.1]` 常用命令
| 命令 | 输出 | 什么时候用 |
|---|---|---|
| `mpa screenshot [路径] [--base64]` | 抓当前屏幕存 PNG(默认 `/var/mobile/Media/mpa_screen.png`,只能 `/var/mobile/` 下;不走系统截图:不弹缩略图、不进相册);`--base64` 把 PNG 的 base64 写到 stdout | **认页面**第一步:`issh exec d35 "mpa screenshot --base64" -t 60 \| base64 -d > /abs/shot.png` |
| `mpa Aweme hitchain [x y]` | 屏幕中心(或指定点)hitTest → 响应链每层 `[view]` / `[VC]` 类名 + 地址;末尾 `VC 栈:叶子>…>根` | **认页面**:当前显示的是哪几个控制器、谁套着谁 |
| `mpa Aweme pvc` | VC 层级树原始输出(`_printHierarchy`) | 看整棵控制器层级(含不在屏幕中心的,如键盘打开时) |
| `mpa Aweme pviews` | SDK 视图树:按层级缩进,每行 `类名 地址 (x,y w×h) [不可见] ["文字"]`,末尾节点数 / 可见数 / 耗时 | **找控件**:当前页面有哪些控件、文字、坐标(和自动化里拍到的是同一棵) |
| `mpa Aweme pviews N 间隔ms [scan] [mt] [tap]` | 连拍 N 次,每次一行(成功 / 失败原因),最后汇总 | 排查拍树偶发失败:scan = 页面感知扫描器同时跑;mt = 每次换线程;tap = 自动化任务里每 10 次点一下 tab |
| `mpa Aweme axlabels` | 带 accessibilityLabel 的可见视图(来源:设置 / 重写)+ 文字;画板上框出 1 秒 | 按辅助信息找按钮(纯图标按钮多数有 label) |
| `mpa Aweme classchain <对象地址>` | 对象的 isa 往上的父类链:每个类的名字、`class_rw_t` flags(`CONSTRUCTED` = 运行时动态建的类) | 类名对不上时:看对象是不是被 KVO / 监控库换成了动态子类 |
| `mpa tests [套件]` | 跑测试套件(不带 = 全部);`scene` 是整体示例(页面 + 动作 + 流程会话) | 改完 SDK 回归 |

`pviews` 文字前的类名就是查询里 `views(@"…")` 写的类名;坐标是竖屏屏幕 points,和 `tapAt` 一致。
**SDK 显示 / 匹配的都是原类名**:运行时被 KVO / 监控库换成动态子类的对象(真实 isa 如 `NSKVONotifying_X_AWEPERF__hmd_subfix_`),
`hitchain`(SDK 扫描)、`pviews`、`registerAgent` 的 bindClass、`views(@"X")` 都按原类 `X`;注册和查询照源码里的类名写。

**拉回截图**:优先 `mpa screenshot --base64 | base64 -d`(一步到 Mac)。**不能直接 `> shot.png` 拿原始二进制**:
issh 把 stdout 当文本解码,PNG 会被改坏(多出 `efbfbd`);base64 原样。拉设备上的文件用
`issh file pull <设备> <设备路径> <本地绝对路径>`:**roothide 设备路径前加 `/rootfs`**,**本地写绝对路径**。
原图 2~4MB,`sips -Z 900` 缩小再看。

<!-- [/DOC-10.1] -->
<a id="DOC-10.2"></a>
## `[DOC-10.2]` 调试显示(画在远程画板 RemoteCanvas 上:所有应用之上,不挡触摸)
三层开关,`显示 = 总闸开 && 元素的值`:
| 开关 | 作用 | 默认 |
|---|---|---|
| `IRUIElement.debugEnabled` | 总闸,最高:关掉什么都不画 | 开 |
| `app.debugEnabled` | 应用默认:之后从 app 来的元素(`app.query…` / `node.element` / `agent.element`)创建时继承 | 关 |
| `element.debugEnabled` | 单个元素覆盖;往下查出来的继承 | 继承应用 |

开着时:`tap` / `longPress` 前画红框 + 点击位置红圈 + HUD "tap 推荐";查询每一步数量打 `[IRDebug] views(X) → 8 个`。
显式画:`element.debugOutline(毫秒, 文字)` / `element.debugHighlight(毫秒)`(只看总闸)。
画板图层只能在**本进程主线程**建:宿主进程的主线程要在跑 runloop(mpa 的测试已在后台线程跑、主线程空着)。
<!-- [/DOC-10.2] -->

<a id="DOC-10.3"></a>
## `[DOC-10.3]` 跨工具分工与动态查询入口
> 不在此处硬编码具体参数以防版本滞后：**有独立 Skill 的工具直接读其 Skill，无 Skill 的工具使用前先跑 `--help`。**

| 工具 | 位置 | 负责领域 | 获取最新用法 |
|---|---|---|---|
| **`xtheos` / `xtheos-deploy`** | Mac | 编译、打包与真机安装部署 | 读 `xtheos` / `xtheos-deploy` Skill |
| **`idm`** | Mac | 设备列表、状态、UDID、进程查看 | 读 `idm` Skill |
| **`issh`** | Mac | 远程在设备上执行命令 | 读 `issh` Skill |
| **`ilog`** | Mac | 设备历史与实时日志查询 | 读 `ilog` Skill |
| **`cht`** | 设备（经 `issh`） | 设备操作（点击/滑动/输入/解锁/App 启停/手势） | 先执行 `issh exec <dev> "cht --help"` |
| **`mpa`** | 设备（经 `issh`） | 页面识别、VC 树、视图树、无障碍标签 | 先执行 `issh exec <dev> "mpa --help"` |
| **`mpx`** | 设备（经 `issh`） | 跨进程查内存对象、活实例、ivar 与方法 | 先执行 `issh exec <dev> "mpx --help"` |
| **`amt`** | 设备（经 `issh`） | 自动化测试套件运行与守护状态管理 | 先执行 `issh exec <dev> "amt --help"` |
<!-- [/DOC-10.3] -->
