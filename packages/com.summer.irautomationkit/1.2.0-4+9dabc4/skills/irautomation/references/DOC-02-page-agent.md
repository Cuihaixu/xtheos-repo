---
title: PageAgent 页面与控制器使用规范
tags: [PageAgent, 控制器绑定, +load顺序, 权重注册, 原子事件, 分区规范]
keywords: [IRPageAgent, registerAgent, registerWeight, registerAction, pageIDs, bindClass, factorStrengthForName, PageAgent, 控制器绑定, 页面识别]
description: IRPageAgent 从创建基类、绑定远程控制器、严格 +load 四步注册顺序、权重因子绑定、原子动作/控制流注册到代码 pragma mark 分区标准。
---

# PageAgent 完整使用规范（`DOC-02`）

> 文档编号：`DOC-02`（可在 `skills/` 下用 `rg "DOC-02"` 定位本文档）

本文档梳理 `IRPageAgent` 从创建、绑定、事件注册、控制流编排到代码分区的标准写法，所有业务工程（如 `AwemeBrush`）的 `PageAgent` 均按此规范编写与审查。

---


## 目录索引

- [`[DOC-02.1]` 1. PageAgent 创建](#DOC-02.1)
  - [`[DOC-02.1.1]` 1.1 应用基类（每个 App 一个）](#DOC-02.1.1)
  - [`[DOC-02.1.2]` 1.2 命名与文件组织（一控制器一类一文件）](#DOC-02.1.2)
  - [`[DOC-02.1.3]` 1.3 `+load` 注册要求与复杂页面分类拆分](#DOC-02.1.3)
- [`[DOC-02.2]` 2. 控制器绑定 PageAgent 类](#DOC-02.2)
  - [`[DOC-02.2.1]` 2.1 绑定规则](#DOC-02.2.1)
  - [`[DOC-02.2.2]` 2.2 四种 `pageIDs` 绑定形态](#DOC-02.2.2)
  - [`[DOC-02.2.3]` 2.3 复用页面的识别（`<IRPageResolving>`）与运行时 `pageID` 用法](#DOC-02.2.3)
  - [`[DOC-02.2.4]` 2.4 远程数据模型直接复用映射类](#DOC-02.2.4)
  - [`[DOC-02.2.5]` 2.5 `[R-ARCH-02]` 属性访问作用域与 Action 神圣边界 (Property Access Scope)](#DOC-02.2.5)
    - [`[DOC-02.2.5.1]` 1. 属性访问作用域三定律 (Property Access Scope)](#DOC-02.2.5.1)
    - [`[DOC-02.2.5.2]` 2. 行为必须且只能是 Action，严禁自造私有业务方法](#DOC-02.2.5.2)
- [`[DOC-02.3]` 3. PageAgent 注册事件（渐进式导览）](#DOC-02.3)
- [`[DOC-02.4]` 4. PageAgent 控制流事件（渐进式导览）](#DOC-02.4)
- [`[DOC-02.5]` 5. PageAgent 代码分区与标准示例](#DOC-02.5)
  - [`[DOC-02.5.1]` 5.1 完整主类示例（独立页面 / 中小型 PageAgent）](#DOC-02.5.1)
  - [`[DOC-02.5.2]` 5.2 大型页面按分类拆分示例（原子事件分类 + `+Flow` 控制流分类）](#DOC-02.5.2)

---

<a id="DOC-02.1"></a>
## `[DOC-02.1]` 1. PageAgent 创建

<a id="DOC-02.1.1"></a>
### `[DOC-02.1.1]` 1.1 应用基类（每个 App 一个）
每个目标 App 创建一个继承自 `IRPageAgent` 的基类（如 `AWBPageAgent`）：
- **只在这里重写 `+bundleIDs`**，声明所属目标应用 Bundle ID；所有页面和子控制器 `PageAgent` 继承它，不再重复写 `bundleIDs`。
- 若整个 App 有全局通用动作（如返回 `back`），在基类 `+load` 中将 `pageIDs` 设为 `@[ IRPageIDCommon ]` 后注册。

<!-- [/DOC-02.1.1] -->
<a id="DOC-02.1.2"></a>
### `[DOC-02.1.2]` 1.2 命名与文件组织（一控制器一类一文件）
- **命名规范**：应用统一前缀 + 业务名 + `PageAgent` 后缀（如 `AWBFeedPageAgent`、`AWBPlayerPageAgent`、`AWBLivePreStreamPageAgent`）。
- **禁止**把目标 App 的原生类名（`AWE*` / `IES*` / `HTS*`）作为自己类的前缀；原生类名只出现在 `bindClass` 等配置字符串中。
- **独立归档**：一个页面或一个独立内容子控制器对应**独立的类与 `.h/.m` 文件**，放在对应功能目录下（`Pages/Feed/`、`Pages/Player/`、`Pages/Live/`、`Pages/ImageAlbum/` 等）。
- **严禁把多个不同业务类型的控制器塞在同一个 `.m` 里**，也**严禁**写一个带类型参数的公共注册函数去批量给多个控制器注册同构事件——每个控制器由自己的 `PageAgent` 类负责自己的权重、事件与数据。

<!-- [/DOC-02.1.2] -->
<a id="DOC-02.1.3"></a>
### `[DOC-02.1.3]` 1.3 `+load` 注册要求与复杂页面分类拆分
- **绑定页面、注册权重、注册事件、注册控制流，全部必须在 `+load` 方法中完成**。
- **`+load` 内部必须严格按以下顺序组织**：
  1. 绑定页面（`registerAgent:`）
  2. 注册权重（`registerWeight:`）
  3. 注册原子事件（`registerAction:`）
  4. 注册控制流事件（`registerAction:` / `registerListBrowse:`）
- 页面代码量级较大时，使用 **Objective-C 分类** 拆分注册：
  - **原始主类（`AWBExamplePageAgent`）**：页面绑定（`registerAgent:`）与**权重（`registerWeight:`）统一在原始主类中注册**。
  - **原子事件分类**：原子事件过多时，按类型/职责拆到对应分类（如 `AWBFeedPageAgent+Interaction`、`AWBFeedPageAgent+Navigation`），在各自 `+load` 中注册。
  - **控制流分类**：控制流较复杂时，直接拆到控制流分类（如 `AWBFeedPageAgent+Flow`），在其 `+load` 中注册。
  - 分类中的私有辅助方法必须带分类前缀（如 `flow_xxx`、`interaction_xxx`），防止后加载的分类覆盖主类或其它分类的同名方法。

---

<!-- [/DOC-02.1.3] -->
<!-- [/DOC-02.1] -->
<a id="DOC-02.2"></a>
## `[DOC-02.2]` 2. 控制器绑定 PageAgent 类

在 `PageAgent` 主类的 `+load` 中调用 `+registerAgent:` 完成绑定（内部写入 `IRAgentRegistry` 全局注册表）：

```objc
+ (void)load {
    [self registerAgent:^(id<IRPageAgentRegistration> registration) {
        registration.bindClass = @"AWEFeedTableViewController";
        registration.pageIDs   = @[ @"feed.recommend", @"feed.follow" ];
        registration.comment   = @"Feed 列表控制器";
    }];
}
```

<a id="DOC-02.2.1"></a>
### `[DOC-02.2.1]` 2.1 绑定规则
- **唯一性**：同一应用（`bundleID`）内，一个原生 `bindClass` **只能绑定一个 `PageAgent` 类**；重复注册第二次会失败。
- **动态子类剥离**：`bindClass` 直接写 `hitchain` 看到的原始控制器类名，框架会自动剥离 `NSKVONotifying_` 前缀和 `_hmd_subfix_` 后缀。

<!-- [/DOC-02.2.1] -->
<a id="DOC-02.2.2"></a>
### `[DOC-02.2.2]` 2.2 四种 `pageIDs` 绑定形态
| 形态 | `pageIDs` 写法 | `bindClass` | `<IRPageResolving>` | 能否注册动作 | 典型用途 |
|---|---|---|---|---|---|
| **固定页面** | `@[ @"search.middle" ]` | 必填 | 可选（单一页面且需按链判断本次是否算页面时遵守） | 可以 | 搜索中间页、登录页、设置页 |
| **复用页面** | `@[ @"feed.recommend", @"feed.follow" ]` | 必填 | **必须遵守**，由 `+pageIDInContext:` 返回当前实例所属 ID 或 `nil` | 可以 | 同一个 VC 类在多个 Tab/场景复用 |
| **通用页面基类** | `@[ IRPageIDCommon ]` | 不要求 | 不需要 | 可以 | 仅承载 `back` 等通用动作给子类继承，不参与识别 |
| **子控制器 / 容器控制器** | **不设（`nil`）** | 必填 | 不需要 | **可以** | 视频内容、直播预览、图集、外层导航/Tab 容器 |

> 注意：`IRPageIDCommon` 只能单独出现在数组中；`pageIDs` 传空数组 `@[]` 属于非法配置（非页面控制器请保持 `nil`）。

<!-- [/DOC-02.2.2] -->
<a id="DOC-02.2.3"></a>
### `[DOC-02.2.3]` 2.3 复用页面的识别（`<IRPageResolving>`）与运行时 `pageID` 用法

当**同一个控制器类名**在多个业务入口复用时，推荐只建**单个 `PageAgent` 类**，通过协议 `<IRPageResolving>` 在运行时动态返回不同 `pageID`，不要为同一 `bindClass` 拆出多个重复的 `PageAgent` 子类。

#### 1) 复写哪个方法 & 完整伪代码示例
```objc
// 1. .h 声明遵守 <IRPageResolving>
@interface AWBVideoDetailPageAgent : AWBPageAgent <IRPageResolving>
@end

// 2. .m +load 里把所有可能的 pageID 都写进 pageIDs
+ (void)load {
    [self registerAgent:^(id<IRPageAgentRegistration> registration) {
        registration.bindClass = @"AWEAwemeDetailTableViewController";
        registration.pageIDs = @[ @"video.detail.feed", @"video.detail.search", @"video.detail.user" ];
        registration.comment = @"视频详情列表（复用页）";
    }];
}

// 3. 复写 +pageIDInContext: —— 框架每次识别到该控制器实例时调用一次并缓存结果
+ (nullable NSString *)pageIDInContext:(IRChainContext *)context {
    // 方式 A：看所在父容器 / 响应链祖先（under / parentViewController）
    if (context.node.under(@"AWEHPSlidingViewController")) {
        return @"video.detail.feed";
    }
    // 方式 B：Push 进导航栈时，看控制器树（pvc）里前序页面的特征
    if ([context.pvc contains:@"AWEUserDetailViewControllerV2"]) {
        return @"video.detail.user";
    }
    // 方式 C：兜底或其它来源（也可直接返回 nil 表示本次不认作页面）
    return @"video.detail.search";
}
```

#### 2) 怎么写识别条件（3 条原则）
- **优先读内存链**：`context.node.under(@"...")`（底层容器）、`context.node.parentViewController.className`（直属父 VC）、`context.node.isFront`（是否最顶层）。
- **键盘弹起用 `pvc`**：`context.isKeyboardVisible` 为 `YES` 时 `context.node` 为空，需改用 `[context.pvc contains:@"..."]`；Push 场景也可通过 `context.pvc` 判断导航栈里的前序页面。
- **返回值限制与缓存**：必须返回 `pageIDs` 里声明过的值或 `nil`；实例首次出现时确定一次并缓存到 `agent.pageID`（若键盘模式返回 `nil`，下次完整链时会重判）。严禁在回调里发耗时远程方法调用。

#### 3) 运行时 `pageID` 怎么读与用
```objc
// 1. 在动作 / 控制流内区分当前来源（推荐用 context.page.pageID：即使动作转发给子控制器如 Player，它仍指向外层页面）
NSString *pid = context.page.pageID;
if ([pid isEqualToString:@"video.detail.feed"]) {
    // 走 Feed 双列进入时的策略
}

// 2. 在本页自有事件里：((IRPageAgent *)context.agent).pageID 与 context.page.pageID 一致
// 3. 在跨页流程里：app.currentAgent.pageID 读当前前台页；session.page(@"video.detail.feed") / app.waitPage(@"video.detail.feed", 5, diag) 按具体 ID 调度
// 4. 在子控制器动作里等外层页面变化：app.waitPageChange(context.page, 5, diag)
```

<!-- [/DOC-02.2.3] -->
<a id="DOC-02.2.4"></a>
### `[DOC-02.2.4]` 2.4 远程数据模型直接复用映射类
- 当目标 App 的某个远程类已通过 `[IRObject registerSubclass:forRemoteClass:]` 全局映射为强类型包装类时（如 `AWEAwemeModel` → `AWBAweme`），通过 `[agent.remoteObject ivarNamed:@"_model"].value.objectValue` 拿到的**直接就是该映射类实例**（如 `AWBAweme *`）。
- **严禁**再手动去拆原始 ivar 拼散装 `NSDictionary`；直接使用映射对象的强类型模型属性（如 `aweme.model`）及标准化方法（如 `aweme.dictionaryValue`）。

<!-- [/DOC-02.2.4] -->
<a id="DOC-02.2.5"></a>
### `[DOC-02.2.5]` 2.5 `[R-ARCH-02]` 属性访问作用域与 Action 神圣边界 (Property Access Scope)

> [!IMPORTANT]
> **PageAgent 的属性（`@property readonly`）与事件（`Action`）有绝对分明的权责与作用域边界。**
> 属性只服务于本 Agent 内部，跨 Agent 通信 100% 走事件总线。

<a id="DOC-02.2.5.1"></a>
#### `[DOC-02.2.5.1]` 1. 属性访问作用域三定律 (Property Access Scope)

##### ① 声明位置作用域 (Declaration Scope: 私有化)
- **严禁在公开 `.h` 中泄露控制器私有属性**！
- 包装目标 ViewController ivar 的只读属性（如 `duration`、`itemType`、`playbackTime`、`canComment` 等），**必须声明在 `.m` 顶部的类扩展（Class Extension）中**：
  ```objc
  // AWBPlayerPageAgent.m
  @interface AWBPlayerPageAgent ()
  /// 仅限本 Agent 内部 Action 访问的只读属性 (包装远程控制器 ivar)
  @property (nonatomic, readonly) double duration;
  @property (nonatomic, readonly) NSString *itemType;
  @property (nonatomic, readonly) BOOL canComment;
  @end
  ```
- 若因大型页面分类拆分（如 `+Flow`、`+Interaction`）需要跨文件共享，统一收敛在内部私有头文件（如 `AWBPlayerPageAgentInternal.h`）中，**公开 `.h` 保持绝对纯净，物理上隔绝外界直接访问**。

##### ② 调用主体作用域 (Caller Scope: 仅限自身 Action)
- **属性访问只能且必须发生在当前 Agent 自己的 Action 执行体内**；
- **唯一合法的调用入口**：借助框架在 `IRActionContext.h` 中为 `agent` 声明的 `__kindof IRAgent *` 特性，在当前 Action 内直接强类型安全赋值，**免写任何强制类型转换**：
  ```objc
  [self registerAction:IRIdentityMake(@"watch_dwell", @"按时长看播") builder:^(IRActionBuilder *action) {
      action.perform(^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
          // ✅ 合法: 借助 __kindof 直接强类型获取自己，只访问自己的属性
          AWBPlayerPageAgent *agent = context.agent;
          double totalDuration = agent.duration;
          NSString *type = agent.itemType;
          ...
          return diag.noError;
      });
  }];
  ```
- **严禁跨 Agent 读属性**：`FeedPageAgent` 严禁直接读取 `PlayerPageAgent` 的属性，`Player` 也严禁反向读取 `Feed` 的私有状态。

##### ④ `[R-ARCH-06]` 业务动作必须收口到 Action，禁止用私有方法替代事件复用
- **所有具备业务语义的操作（如关闭弹窗、切换页面状态）必须直接写在对应 Action（如 `back`）的执行体内**；
- 在 `handleInterrupt:` 或同类其他入口需要复用该行为时，**必须通过 `[self invokeAction:...]` 或 `IRPageNode` 触发事件**，严禁把业务动作抽成 `-closeXxxWithDiag:` / `-doXxxWithDiag:` 私有方法再由 Action 和回调两头调用；
- OC 私有方法/属性只负责包装远程状态与局部短小计算；多态复用优先走 OC 继承与子类覆写钩子（如 `AWBListPageAgent`），严禁外部静态注入。

##### ③ 数据跨界作用域 (Data Exchange Scope: 统一走 Query 事件)
- 任何外界 Agent（如推荐流 `Feed`、搜索流 `Search`）如果需要获取本 Agent 的数据（如当前作品类型、时长、作者名），**必须且只能通过该 Agent 暴露的 Query 事件提取**：
  ```objc
  // 1. 被访问方 (Player): 声明 result 开启查询模式支持，统一在 perform 执行体中处理 isQuery
  [self registerAction:IRIdentityMake(@"video_info", @"获取当前视频信息") builder:^(IRActionBuilder *action) {
      action.result(IRSchemaTypeObject).setComment(@"视频元数据");

      action.perform(^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
          if (context.isQuery) {
              AWBPlayerPageAgent *agent = context.agent;
              context.result = @{
                  @"duration": @(agent.duration),
                  @"item_type": agent.itemType ?: @"video"
              };
              return diag.noError;
          }
          return diag.noError;
      });
  }];

  // 2. 访问方 (Feed / Flow): 纯事件调用，完全零头文件耦合
  NSDictionary *info = feed.invokeQuery(@"video_info");
  ```

<!-- [/DOC-02.2.5.1] -->
<a id="DOC-02.2.5.2"></a>
#### `[DOC-02.2.5.2]` 2. 行为必须且只能是 Action，严禁自造私有业务方法
- **严禁随意在 PageAgent 上写私有业务方法**（例如 `[player performDwellWatch...]`）；
- 所有的逻辑、操作、流程封装，**必须全部注册为 Action（事件）**；
- 只有成为 Action，才能完整享受框架的 `isAvailable` 可用性探测、`amt ops` 命令行独立测试、`IRDiagnostics` 错误隔离与三级响应链寻址能力。

---

<!-- [/DOC-02.2.5.2] -->
<!-- [/DOC-02.2.5] -->
<!-- [/DOC-02.2] -->
<a id="DOC-02.3"></a>
## `[DOC-02.3]` 3. PageAgent 注册事件（渐进式导览）

事件注册格式、`do { ... break; } while (NO); return diag.noError;` 退出规范、`getParam` / `findParam`、`NSNull.null` vs `nil`、**事件应该放在当前页面、子控制器 Agent 还是父容器 Agent** 的归属决策，以及调用场景，统一见独立专题文档：

👉 **[DOC-03-page-actions.md — PageAgent 事件规范：注册、归属、调用与场景](DOC-03-page-actions.md)**

要点速记：
- `registerAction:` 的 `builder:` / `block:` 换行对齐，执行体结尾统一 `return diag.noError;`（禁硬写 `return YES;` / `return NO;`）。
- 可复用的内容组件动作放**子控制器 `PageAgent`**；外层共享外壳控件放**父容器 `PageAgent`**；页面独有动作与流控放**页面 `PageAgent`**。

---

<!-- [/DOC-02.3] -->
<a id="DOC-02.4"></a>
## `[DOC-02.4]` 4. PageAgent 控制流事件（渐进式导览）

`IRFlowSession` / `ListBrowse` 异常处理封装、单线程 UI 多层嵌套压栈交接（`A → B → C` 时序图）、`IRPageNode`（`loop` / `repeat` / `wait`）以及拟人权重与强类型传参，统一见独立专题文档：

👉 **[DOC-04-page-flow.md — PageAgent 控制流规范：压栈交接、异常处理与流控模式](DOC-04-page-flow.md)**

要点速记：
- 控制流事件**必须**使用 `IRFlowSession` 或 `IRPageAgent+IRListBrowse` 异常处理封装，禁止裸跑。
- 跨页面控制流采用**压栈 → 执行 → 退出弹栈**模式（支持 `A → B → C` 嵌套），下游页面跑完负责退出自己页面。
- 禁止裸写 `arc4random`；决策与时长统一走 `registerWeight:`。

---

<!-- [/DOC-02.4] -->
<a id="DOC-02.5"></a>
## `[DOC-02.5]` 5. PageAgent 代码分区与标准示例

为保证所有 `PageAgent` 实现风格一致、便于检索与维护，`.m` 文件（及分类 `.m`）统一按以下规范与 `#pragma mark -` 顺序分区（没有对应内容的分区可省略）：
- **禁止使用 `@implementation ClassName { ... }` 实例变量大括号语法**；所有私有状态统一在 `.m` 顶部的类扩展 `@interface ClassName ()` 中使用 `@property` 声明。
- **禁止使用系统/Objective-C 关键词作为变量名或参数名**（例如 `out`、`in`、`inout`、`id`、`class`、`new`、`default`、`register`、`auto` 等，应改为 `watchResult`、`itemID`、`defaultValue` 等语义明确的命名）。
- `+load` 内部严格按 **绑定页面 → 注册权重 → 注册原子事件 → 注册控制流事件** 的顺序调用。

<a id="DOC-02.5.1"></a>
### `[DOC-02.5.1]` 5.1 完整主类示例（独立页面 / 中小型 PageAgent）
```objc
// AWBPlayerPageAgent.m
#import "AWBPlayerPageAgent.h"

@implementation AWBPlayerPageAgent

#pragma mark - 识别与注册

+ (void)load {
    // 1. 绑定页面 / 子控制器
    [self registerAgent:^(id<IRPageAgentRegistration> registration) {
        registration.bindClass = @"AWEPlayInteractionViewController";
        registration.comment   = @"视频播放交互子控制器";
    }];
    // 2. 注册权重（直接写在主类 +load 中，不再多包一层方法）
    [self registerWeight:^(id<IRWeightSpaceDescriptorBuilder> space) {
        space.group(@"video_dwell", @"视频看多久")
            .item(@"short", 60).value(IRTimeRangeMake(5, 13))
            .item(@"medium", 30).value(IRTimeRangeMake(14, 60));
    }];
    // 3. 注册原子事件
    [self registerPageActions];
    // 4. 注册控制流事件（简单流控放主类；复杂则拆到 +Flow 分类）
    [self registerPageFlows];
}

// 若遵守 <IRPageResolving>，识别回调紧跟在绑定之后
+ (nullable NSString *)pageIDInContext:(IRChainContext *)context {
    return nil;
}

#pragma mark - 权重与因子

- (int)factorStrengthForName:(NSString *)name {
    return [super factorStrengthForName:name];
}

#pragma mark - 原子事件

+ (void)registerPageActions {
    [self registerAction:IRIdentityMake(@"watch", @"按视频时长权重观看")
                 builder:^(IRActionBuilder *action) {
        action.result(IRSchemaTypeObject).setComment(@"观看结果");

        action.perform(^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
            ...
            return diag.noError;
        });
    }];
}

#pragma mark - 控制流事件

+ (void)registerPageFlows {
    // 简单控制流写这里（同样必须使用 IRFlowSession / ListBrowse 异常处理封装）
}

#pragma mark - 中断处理

// 若遵守 <IRInterruptHandling>
- (BOOL)shouldInterruptInContext:(IRChainContext *)context { return context.node.front; }
- (BOOL)handleInterrupt:(IRDiagnostics *)diag { return diag.noError; }

#pragma mark - 内部辅助

- (nullable AWBAweme *)currentAweme {
    return [AWBAweme awemeWithObject:[self.remoteObject ivarNamed:@"_model"].value.objectValue];
}

@end
```

<!-- [/DOC-02.5.1] -->
<a id="DOC-02.5.2"></a>
### `[DOC-02.5.2]` 5.2 大型页面按分类拆分示例（原子事件分类 + `+Flow` 控制流分类）
```objc
// AWBFeedPageAgent.m（原始主类：只负责绑定页面 + 统一注册权重 + 基础原子事件）
@implementation AWBFeedPageAgent

#pragma mark - 识别与注册

+ (void)load {
    // 1. 绑定页面
    [self registerAgent:^(id<IRPageAgentRegistration> registration) {
        registration.bindClass = @"AWEFeedTableViewController";
        registration.pageIDs   = @[ @"feed.recommend", @"feed.follow" ];
        registration.comment   = @"Feed 列表控制器";
    }];
    // 2. 注册权重（直接写在原始主类 +load 中）
    [self registerWeight:^(id<IRWeightSpaceDescriptorBuilder> space) {
        space.group(@"feed_watch_count", @"刷几个作品").item(@"normal", 100).value(IRTimeRangeMake(2, 9));
    }];
    // 3. 注册基础原子事件（更多原子事件拆到 +Interaction / +Navigation 等分类）
    [self registerPageActions];
}

+ (nullable NSString *)pageIDInContext:(IRChainContext *)context {
    return context.node.under(@"AWEFollowTabView") ? @"feed.follow" : @"feed.recommend";
}

#pragma mark - 权重与因子
...
#pragma mark - 原子事件
...
@end

// AWBFeedPageAgent+Interaction.m（按类型拆分的原子事件分类）
@implementation AWBFeedPageAgent (Interaction)

#pragma mark - 原子事件

+ (void)load {
    [self interaction_registerActions];
}

+ (void)interaction_registerActions {
    [self registerAction:IRIdentityMake(@"like_video", @"点赞当前视频")
                   block:^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
        ...
        return diag.noError;
    }];
}

@end

// AWBFeedPageAgent+Flow.m（复杂的控制流直接拆分到 +Flow 分类）
@implementation AWBFeedPageAgent (Flow)

#pragma mark - 控制流事件

+ (void)load {
    [self flow_registerFlows];
}

+ (void)flow_registerFlows {
    [self registerAction:IRIdentityMake(@"flow", @"刷推荐流")
                 builder:^(IRActionBuilder *action) {
        action.perform(^BOOL(IRApplication *app, IRActionContext *context, IRDiagnostics *diag) {
            return [IRFlowSession execute:^(IRFlowSession *session) {
                ...
            } onException:^(IRFlowException *exception) {
                exception.abort();
            } onFinally:nil diag:diag];
        });
    }];
}

@end
```
<!-- [/DOC-02.5.2] -->
<!-- [/DOC-02.5] -->
