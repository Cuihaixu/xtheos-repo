# 代码组织：页面 Agent 怎么写（`DOC-01`）

> 文档编号：`DOC-01`（可在 `skills/` 下用 `rg "DOC-01"` 定位本文档）

用 IRAutomationKit 写业务时，页面相关的代码按下面组织。写代码前先读；写完按文末清单自查（review 也照这个清单）。


## 目录索引

- [`[DOC-01.1]` 规则](#DOC-01.1)
- [`[DOC-01.2]` 示例](#DOC-01.2)
- [`[DOC-01.3]` 自查清单（review 用）](#DOC-01.3)

---

<a id="DOC-01.1"></a>
## `[DOC-01.1]` 规则

1. **页面类以 `PageAgent` 结尾**，前缀用这个应用模块的前缀：`AWBFeedPageAgent`、`AWBSearchResultPageAgent`。
2. **每个应用一个类前缀**，在**同一个进程里**不能和别的前缀撞（别的应用模块、工程自己的类、链接进来的库）。
   不同进程之间不用避开（目标 App 自己的类在它的进程里，不在我们的进程里）。
3. **页面识别在页面类自己的 `+load` 里注册**（`registerAgent:`：`bindClass` / `pageIDs` / `IRPageResolving`）。
   每个应用有一个页面基类（如 `AWBPageAgent`），只在基类里写一次 `+bundleIDs`。
4. **`[R-ARCH-01]` 一个页面包含四件事（均在 `+load` 中按顺序注册）**：绑定页面（`registerAgent:`）→ 权重定义（`registerWeight:`）→ 原子事件（`registerAction:`）→ 控制流事件。
   - 页面不复杂：四件事都写在主类的 `.m` 里。
   - 页面代码量级大：按分类拆分——**绑定页面与权重统一留在原始主类中注册**；原子事件过多时按类型拆到分类；控制流复杂时直接拆到 `+Flow` 分类（详见 [DOC-02-page-agent.md](DOC-02-page-agent.md)）。
5. **一个页面的类都放在一个文件夹里**：主类和它的所有分类。
6. **用了分类，事件要归类**：每个事件注册在它所属关注点的分类里，不要把不相关的事件塞进同一个分类。
7. **权重按页面类注册**:空间 ID 是注册时的类名。
   - **权重统一在页面原始主类的 `+load` 中注册**，不分散到各个分类里。
   - **现在不沿继承链找**:基类(如应用的页面基类)注册的组,子类页面选不到;要用的页面类各自注册(bd `ir-r5i` 改成沿继承链找之前都这样)。
8. **退出当前页面的事件一律叫 `back`**：关评论面板、关弹窗、从搜索页返回都叫 `back`（说明里写清楚怎么退，如"关闭评论"）。
   `back` 只负责**退出当前页面、往回走一步**，不保证直接回到根页面：所有页面的 `back` 都往同一个方向走，
   多调几次就回到根页面（如抖音首页顶部频道 关注 → 商城 → 推荐，每次滑到上一个频道）。
   做完等页面变化（离开了当前页面就算成功）。流程里回根页面：一直 `back` 直到到了根页面。
9. **`[R-ARCH-03]` 分类里的方法要带分类名前缀**：同一个类的多个分类里 `+load` 各自都会调用、互不影响；
   但普通方法同名时，后加载的分类会悄悄覆盖前面的。所以分类里的辅助方法写成 `flow_xxx`、`aicontrol_xxx`。
   主类里的方法不加前缀。
10. **`[R-ARCH-05]` 几个页面逻辑一样、属于某个子控制器的事件，注册在子控制器 Agent 上**（只绑 `bindClass`、不设 `pageIDs`），
   用页面节点调用时，页面没有就沿响应链交给它（[DOC-08-actions.md](DOC-08-actions.md)）；某个页面特殊就在页面上注册同名事件覆盖。
   **多个页面共用的外层按钮（导航栏、tab 栏），注册在包着这些页面的容器 Agent 上**（页面节点会往上问到根）。
   只属于一个页面的事件仍写在页面上。
11. **目标 App 自己的类名（`AWE*` 等）只出现在 `bindClass` 等字符串中**，不作为我们自己类的类名前缀。
12. **`[R-ARCH-04]` 结构化数据与协议对象强制模型化（页面有模型时严禁在 PageAgent 内手写字典拼接）**：
   - 凡涉及跨端交互（Conduit/服务端决策/事件上报/复杂任务参数）、字段数量 ≥ 3 个、或含有嵌套结构的上下文，**严禁手写散装 NSDictionary 解析与字面量拼凑**。
   - 若宿主工程集成了基础模型库（如 `CHRFoundation` 的 `NSObject+CHRModel`），**必须声明强类型模型类**（以应用前缀命名，如 `AWBAwemeModel`、`AWBWatchDecision`），利用 `+modelCustomPropertyMapper` 统一映射服务端下划线命名与嵌套路径，用 `chr_modelWithJSON:` 与 `chr_modelToJSONObject` 进行类型安全的双向转换。
   - **页面已有模型时禁止手拼字典**：当 `PageAgent` 已有模型属性（如 `@property (nonatomic, readonly, nullable) AWBAwemeModel *currentAwemeModel;`）时，**严禁在 Action 或控制流中手写 `NSMutableDictionary` 往里塞字段（如 `info[@"item_type"] = @"live"; info[@"can_comment"] = ...;`）**。需要使用的任何信息（如直播信息），必须在模型中定义为强类型字段并在模型内部做好安全判断；若需导出字典（如上传直播信息），应定义为只读属性（如 `@property (nonatomic, copy, readonly, nullable) NSDictionary *liveInfo;`）从模型字段统一组装，在 Action 中直接通过 `agent.liveInfo` 获取。
13. **属性严格内聚与私有化（属性访问作用域三定律）**：
   PageAgent 的 `@property (readonly)` 严禁作为业务临时变量池，**唯一用途是包装目标控制器的 ivar 与状态**。
   包装属性必须声明在 `.m` 类扩展或内部私有头文件中，**公开 `.h` 保持绝对纯净**；
   属性访问严格收敛在当前 Agent Action 内部通过 `context.agent`（借助 `__kindof`）使用，**严禁跨 Agent 读属性**，跨 Agent 通信 100% 走事件总线（Action / Query）。
14. **`[R-FMT-01]` 禁止在 `.m` 中使用 `static` C 辅助函数，统一使用 Objective-C 方法**：
   具备业务语义、需跨层调度、需可用性探测或单测的，**必须注册为 Action**；
   仅在类内部使用的辅助逻辑（数据转换、枚举映射、校验、视图提取等），**统一写为当前类的 Objective-C 私有方法（类方法或实例方法）**，**严禁写成 `static` C 函数**。
   主类方法不加前缀，分类方法按第 9 条加分类名前缀（如 `flow_xxx`）。
15. **串流程事件优先使用 `IRFlowSession` 编码，裸用 `IRPageNode` 必须检查调用结果**：
   凡涉及多步骤时序流转、条件分支、漫游循环或页面跳转的 Action，**必须优先使用 `[IRFlowSession execute:...]` 编码**；只有纯单步原子事件（单按钮点击、单步手势、单属性查询）才允许裸写。
   在非 Session 场景下直接使用 `IRPageNode(pageID, diag)` 时，**必须显式检查每一个 `invoke` / `invokeWith` 的调用结果（`BOOL`）**，循环体中失败立即置 `*stop = YES; return;` 阻断，多阶段间必须检查 `if (node.hasError) return NO;`，严禁不看返回值盲目裸奔。

16. **禁止 `IRObject` 子类自定义构造方法创建实例**：
   所有 `IRObject` 子类**严禁**自行添加构造方法或工厂方法（如 `+awemeWithObject:`、`+modelWithObject:`、`initWithObject:` 等）来包装或创建实例，也应将 `-init` / `+new` 标记为 `NS_UNAVAILABLE`。
   `IRObject` 子类在 `+load` 中通过 `[IRObject registerSubclass:self forRemoteClass:@"..."]` 注册后，由 IRemoteKit 自动返回对应子类实例；调用方只需使用 `[object isKindOfClass:YourSubclass.class]` 做类型判断即可。

17. **`[R-FMT-06]` 禁止在属性自己的 `set`/`get` 方法外直接访问 `_ivar`**：
   除了在属性自身的 getter / setter 方法内部外，**严禁**在任何方法或 block 中直接读写 `_model`、`_json` 等实例变量，统一使用 `self.xxx` 访问属性（确保懒加载、缓存和副作用走统一入口）。

18. **`[R-FMT-03]` 禁止单行无大括号 `if (...) break;` / `if (...) return;`**：
   如 `if (diag.hasError) break;` 等单行无大括号写法一律禁止，所有条件分支必须按标准多行大括号格式书写：
   ```objc
   if (diag.hasError) {
       break;
   }
   ```

19. **`[R-FMT-04]` 禁止无意义的缩写变量名**：
   变量名必须语义完整清晰，严禁使用 `pb`（`playback`）、`sel`（`selection`）、`kw`（`keyword`）、`btn`（`button`）、`idx`（`index`）、`cnt`（`count`）等无意义缩写（通用约定简写如 `app`、`diag`、`url`、`id` 除外）。

20. **`[R-FMT-05]` 禁止用局部 Block / 辅助函数批量注册同构对称 Action**：
   严禁通过 `registerLook(@"scroll_down", ...)` / `registerLook(@"scroll_up", ...)` 批量注册多个仅方向/参数不同的 Action；同构行为只注册一个 Action，通过参数（如 `direction`）区分。

21. **`[R-ARCH-07]` 严禁“类方法 + 传 `context.agent` 实例”的伪 C 函数反模式（区分 `context.agent` 与 `context.page`）**：
   - `context.agent` **必定是当前注册并执行该 Action 的 Agent 类的实例**；而 `context.page` **是当前屏幕正在显示的页面代理（带 `pageID` 的页面 Agent）**，在子控制器/容器 Agent 中 `context.page` 不等于 `context.agent`（详见 `[DOC-03.4.3]`）。
   - 在 `+load` 的 Action 回调里，`self` 是类对象，`context.agent` 已经是当前类实例；凡访问当前 Agent 实例状态的辅助逻辑，**必须写成实例方法或实例属性**（通过 `context.agent` 直接调用），**严禁写成类方法 `+xxxForAgent:(AWBXxxPageAgent *)agent` 再在 `+load` 里调 `[self xxxForAgent:agent]` 把实例当参数传给类方法**。

22. **`[R-FLOW-05]` 子控制器与通用动作初始化当前页面 `IRPageNode` 必须使用 `session.page(context.page.pageID)`（详见 `[DOC-03.4.4]` / `[DOC-22.1.1]`）**：
   - 在子控制器 Agent（自身不设 `pageIDs`）或通用动作的 `setPerformBlock:` 内，通过 `IRFlowSession` 初始化当前页面节点时，**必须写 `IRPageNode *node = session.page(context.page.pageID);`**（严禁写死 `session.page(nil)`）。
   - 若宿主页面带页面标识则请求与节点校验自动带上，若未带则为 `nil`，从而保证子控制器无论嵌入在哪个页面中都能正确识别当前所处位置。

<!-- [/DOC-01.1] -->
<a id="DOC-01.2"></a>
## `[DOC-01.2]` 示例

```
Apps/Aweme/                      应用模块（前缀 AWB）
├── AWBPageAgent.h/.m            页面基类：+bundleIDs 只写这里
└── Pages/
    ├── Feed/                    一个页面一个文件夹
    │   ├── AWBFeedPageAgent.h/.m          +load: registerAgent（识别）+ 简单的事件 / 权重
    │   ├── AWBFeedPageAgent+Flow.h/.m     +load: 控制流相关的事件和逻辑
    │   └── AWBFeedPageAgent+AIControl.h/.m
    └── SearchResult/
        └── AWBSearchResultPageAgent.h/.m  不复杂：四件事都在这里
```

<!-- [/DOC-01.2] -->
<a id="DOC-01.3"></a>
## `[DOC-01.3]` 自查清单（review 用）

| # | 查 |
|---|---|
| 1 | 页面类名以 `PageAgent` 结尾，前缀是这个应用模块的前缀 |
| 2 | 前缀在本进程里没有和别的模块 / 工程类 / 库撞 |
| 3 | 页面识别（`registerAgent:`）在主类的 `+load` 里；`+bundleIDs` 只在应用的页面基类里 |
| 4 | 页面复杂时按关注点拆了分类，每个分类在自己的 `+load` 里注册自己的部分 |
| 5 | 主类和它的分类都在同一个页面文件夹里 |
| 6 | 每个事件注册在它所属关注点的分类里 |
| 7 | 分类里的辅助方法带分类名前缀（`flow_` …），主类方法不加 |
| 8 | 权重组注册在实际用它的页面类上(不放基类);同一个组只在一个分类里注册 |
| 9 | 目标 App 的类名（`AWE*` 等）只出现在 `bindClass` 等字符串里，不当作我们的类名前缀 |
| 10 | 退出当前页面的事件叫 `back` |
| 11 | 多个页面共用、属于子控制器的事件注册在子控制器 Agent 上，没有在每个页面各写一份；页面上的同名事件只用于特殊覆盖 |
| 12 | 跨端接口请求、响应、上报及复杂上下文是否定义了强类型模型对象（如 `CHRModel`）；当页面已有模型（如 `currentAwemeModel`）时，是否杜绝了在 Action 中手写 `NSMutableDictionary` 拼接字段（所有字段应在模型中定义并做安全判断，字典统一通过 `agent.xxxInfo` 只读属性获取，`[R-ARCH-04]`） |
| 13 | 属性是否严格包装目标控制器 ivar 并在 .m 类扩展中私有化；是否杜绝了跨 Agent 读属性、100% 走事件总线 |
| 14 | 是否清除了 `.m` 中的 `static` C 辅助函数，统一写为当前类的 OC 方法（主类不加前缀，分类加分类前缀） |
| 15 | 串流程 Action 是否优先使用了 IRFlowSession 编码；非 Session 裸用 IRPageNode 是否严格检查了 invoke 返回值与 hasError |
| 16 | 是否存在 `IRObject` 子类自行添加构造方法/工厂方法创建实例（应通过 `+load` 注册 `registerSubclass:forRemoteClass:`，并用 `isKindOfClass:` 判定） |
| 17 | 是否在属性自己的 getter/setter 之外直接使用了 `_ivar`（应统一使用 `self.xxx`） |
| 18 | 是否存在单行无大括号 `if (diag.hasError) break;` 等写法（应统一用标准多行大括号代码块） |
| 19 | 是否存在无意义的缩写变量名（如 `pb`、`sel`、`kw`、`btn`、`idx`、`cnt`） |
| 20 | 是否存在用局部 block / 辅助函数批量注册同构对称 Action（应合并为单个 Action 用参数区分） |
| 21 | 是否存在 `[self xxxForAgent:context.agent]` 把 `context.agent` 实例当参数传给类方法的反模式（`[R-ARCH-07]`，应改为实例方法/属性）；以及是否正确区分了 `context.agent`（当前 Agent 实例）与 `context.page`（当前显示的页面代理，`[DOC-03.4.3]`） |
| 22 | 子控制器与通用动作在 `IRFlowSession` 内初始化当前页面 `IRPageNode` 时，是否使用了 `session.page(context.page.pageID)` 而非硬编码 `session.page(nil)`（`[R-FLOW-05]` / `[DOC-03.4.4]` / `[DOC-22.1.1]`） |
<!-- [/DOC-01.3] -->
