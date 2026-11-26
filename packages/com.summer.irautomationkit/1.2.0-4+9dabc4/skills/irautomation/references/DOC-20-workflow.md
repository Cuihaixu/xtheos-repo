# AI 写自动化的工作流（`DOC-20`）

> 文档编号：`DOC-20`（可在 `skills/` 下用 `rg "DOC-20"` 定位本文档）

> 给 AI 执行用。示例统一:设备 `d35`(`idm list --ai` 取)、目标抖音 `com.ss.iphone.ugc.Aweme`(进程名 `Aweme`)。
> 设备上的命令都经 `issh exec <设备> "<命令>" -t <秒>` 跑;**默认超时只有 30 秒**,跑测试 / 长流程加 `-t 300`。

**最大的未知是"现在在哪个页面"。** 每一步都先确认页面,再找控件;别凭记忆或需求描述假设起始状态
(上一次跑可能停在别的页面)。


## 目录索引

- [`[DOC-20.1]` 1. 认页面](#DOC-20.1)
- [`[DOC-20.2]` 2. 找控件](#DOC-20.2)
- [`[DOC-20.3]` 3. 写](#DOC-20.3)
- [`[DOC-20.4]` 4. 跑和看](#DOC-20.4)
- [`[DOC-20.5]` 5. 收尾自查](#DOC-20.5)

---

<a id="DOC-20.1"></a>
## `[DOC-20.1]` 1. 认页面
1. **截图**判断这是什么页面(如"搜索结果页"):
   ```bash
   issh exec d35 "mpa screenshot --base64" -t 60 | base64 -d > /abs/path/shot.png   # 一步拉到 Mac
   sips -Z 900 /abs/path/shot.png --out /abs/path/shot_small.png                      # 缩小 (原图 2~4MB) 再看
   ```
2. **看响应链**:`issh exec d35 "mpa Aweme hitchain" -t 60` → 屏幕中心命中的视图沿 nextResponder 到 UIApplication,
   每层 `[view]` / `[VC]` + 类名;末尾一行 `VC 栈:A>B>C…`(叶子 → 根)。见 [DOC-10-debug-tools.md](DOC-10-debug-tools.md)。
3. **定识别规则**:选一个代表这个页面的控制器(通常是 VC 栈里业务含义最清楚的那个);它若在别的页面里复用
   (如抖音的 `AWEFeedTableViewController` 在推荐 / 关注里都有),用外层容器区分(`IRPageResolving`)。
   写成页面 Agent 注册,见 [DOC-14-pages.md](DOC-14-pages.md)。
4. **验证**:到这个页面跑一遍,`app.currentAgent.pageID` 应是你注册的值;换到复用它的别的页面,应不是。

<!-- [/DOC-20.1] -->
<a id="DOC-20.2"></a>
## `[DOC-20.2]` 2. 找控件
1. **在当前页面**看控件:`issh exec d35 "mpa Aweme pviews" -t 60` → 整棵视图树(类名 / 地址 / frame / 不可见 / 树里的文字)。
   内容多时 `| grep '"'` 只看带文字的,`| grep -v 不可见` 去掉看不见的。
2. 要按辅助信息找(按钮多数有 accessibilityLabel,比类名稳定):`mpa Aweme axlabels`。
3. 选查询依据,优先级:**可见文字 → accessibilityLabel → 类名 / 层级 → 位置**。类名会随版本变,文字 / label 稳定。
4. **从当前页面开始查**(`app.currentAgent.query…`),别从整屏查:被盖住的上一个页面也在树里、几何上也"可见"。
   见 [DOC-07-pitfalls.md](DOC-07-pitfalls.md)。

<!-- [/DOC-20.2] -->
<a id="DOC-20.3"></a>
## `[DOC-20.3]` 3. 写
- 流程放进 `IRAutomationTask`(持有执行权的 UI 线程才能点 / 输入),`[app setAutomation:task]` 起跑。
- 找控件 + 操作:`page.query.views(@"…").axLabel(@"*评论*").element.tap(diag)`,语法见 [DOC-15-query.md](DOC-15-query.md)、[DOC-08-actions.md](DOC-08-actions.md)。
- 每一步传 `diag`,失败就看 `diag.errorMessage` 报出来、停在哪一步。**不要用 `sleep` 等页面**,用等待接口,见 [DOC-06-waiting.md](DOC-06-waiting.md)。
- 页面能做的业务操作(打开评论、返回)写成页面 Agent 的动作,流程只调动作;流程用流程会话 `IRFlowSession`
  (或页面节点 / 底层 `currentAgent`),见 [DOC-11-flow.md](DOC-11-flow.md)。

<!-- [/DOC-20.3] -->
<a id="DOC-20.4"></a>
## `[DOC-20.4]` 4. 跑和看
- 打开调试显示 `app.debugEnabled = YES`:点了哪里(红圈)、框了什么(红框)、HUD 文字在手机上看得到;
  查询每一步数量打 `[IRDebug]` 日志。
- 跑:写成 mpa 命令或测试套件,`issh exec d35 "mpa …" -t 300`。
- 看日志里的 diag:"元素数量不为 1 (0 个)" = 没找到;"(16 个: …)" = 不唯一,加条件或 `first()`;
  "节点不可见" = 找到了但看不见(被裁 / 在屏幕外)。

<!-- [/DOC-20.4] -->
<a id="DOC-20.5"></a>
## `[DOC-20.5]` 5. 收尾自查
对一遍 [DOC-07-pitfalls.md](DOC-07-pitfalls.md);退出应用用多任务划掉,**不要强杀**(前台 SIGKILL 在应用看来就是闪退)。
<!-- [/DOC-20.5] -->
