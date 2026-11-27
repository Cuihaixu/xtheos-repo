# 网络库设计参考

> 文档定位：**设计参考（为什么这么设计）**，与契约文档（`references/openapi.json`、
> `network-library.md`）互补。契约描述“接口是什么”，本文解释“架构与设计决策为什么这样”。
> 服务端实现以契约为准，遇到取舍时以本文的原则判断。

本文整合两部分：

1. **网络库同步机制**——列表 / 变更游标 / Snapshot / 按 hash 增量下载 / 只读镜像替换 / 删除处理。
2. **回放方案**——默认与自定义方案对象模型、创建 / 更新 / 删除写接口、创建网络库时导入本地方案与重写引用。

---

## 0. 核心定位与不变量

网络库让一个内网服务端集中管理手势记录与回放方案，多台设备同步同一份数据后各自离线回放。

四条贯穿全文的不变量：

1. **服务端是唯一真相源。** 服务端持有网络库目录、目录树、手势记录、回放方案、库
   `revision` 与变更游标 `changeToken`。客户端从不直接写网络镜像里的 `tree.json`、
   `playback_plans/` 或记录文件。所有变更都是对服务端的实时 HTTP 请求。
2. **手机只持有可重建的只读镜像。** 镜像随时可以丢弃并重新同步。回放引擎（backboardd）
   只读本地镜像，**回放链路绝不请求网络服务**——这样断网、服务端宕机都不影响已同步内容的回放。
3. **本地库与网络库完全隔离，创建即断开。** 本地库是纯客户端数据，客户端可任意改其目录与
   记录组类型；从本地库“创建网络库”是一次导入 + ID 重写生成的**一次性副本**，两者不共享
   `libraryId`、`nodeId`、`recordId`、`planId` 或任何文件，也不是引用共享。副本一旦创建即与
   原本地库**彻底断开**：本地库之后的改动不影响网络库，网络库的改动也不回写本地库。
4. **网络库的结构与类型归服务端所有。** 网络库的目录树与记录组类型（`groupKind` /
   `typeSignature`）由服务端持有。客户端对网络库只能读取 Snapshot、新增 / 删除手势记录、
   按所有权读写回放方案，**不能修改其目录结构或记录组类型**；改类型、改目录只能通过服务端
   （管理端或内部任务）。客户端往网络组加类型不符的手势时服务端直接 409 拒绝，**客户端不能转换
   网络组类型**，转类型只由服务端操作（见第 7 节）。（本地组是纯客户端数据，可自由转类型，不受此限。）

### ID 规约（贯穿所有接口）

- `recordId`、`nodeId`、`planId` 一律是**大写 UUID**，与本地
  `NSUUID.UUID.UUIDString` 完全同构，例如 `A0794FA8-D68E-4ECF-9D13-EF296F74D6CF`、
  `66BEC501-4A20-4BF3-9F70-6D175178B2F8`。
- **禁止任何前缀**（`record-`、`node-`、`group-`、`lib-` 等）。服务端生成 ID 时直接用裸
  UUID，不得沿用示例里出现过的可读占位形式。
- 客户端在创建请求里携带的临时 ID（例如导入包内部用来建立引用关系的
  `local-group-a`）**仅在该请求体内部有效**；服务端必须为它们生成新的网络 UUID，
  不得保留其字面值或前缀。
- 契约文档早期示例中出现过的 `lib-a`、`node-record`、`record-a` 等，均为可读占位，
  **不是格式约定**；真实数据一律为大写 UUID。

> 之所以强约束 ID 格式：`planId` 在 SDK 侧是回放入口的唯一键，本地与网络方案共用同一
> 解析路径。若网络侧 ID 带前缀或非 UUID，设备端就无法把本地库和网络镜像统一到一套
> `planId → 方案` 查找里，也无法保证跨源不冲突。

### 为什么没有鉴权

当前部署在受信任内网，UDID 归属是**产品权限规则**而非对抗性安全边界。因此：

- 客户端不发 `Authorization`；服务端不要求 Token / API Key / OAuth。
- 同步类接口（列表、变更、Snapshot、读记录）不带任何身份。
- 回放方案的写接口带 `X-CHR-Device-UDID`，仅用于“谁能编辑/删除自己创建的方案”这条产品
  规则，不是身份认证。服务端仍必须做 Header 比对——客户端隐藏 UI 不等于权限边界。
- 手势记录本身不上传 UDID；只有回放方案的创建者字段用 UDID。

---

## 第一部分：同步机制

### 1. 数据流总览

```text
本地收藏库 ──创建网络库──▶ 网络库服务
Studio 录制结果 ──新增记录──▶ 网络库服务
网络库服务 ──列表 / 变更 / Snapshot / Record──▶ 手机只读镜像 ──▶ 回放引擎
```

同步是**拉取式**的：服务端不推送，客户端用变更游标轮询边界，命中变化再拉完整 Snapshot 与
增量记录。没有 WebSocket、没有 Tree Patch、没有双向合并、没有离线编辑队列——这些都被有意
排除，以换取“镜像永远等于某个服务端 revision 的完整快照”这个简单不变量。

### 2. 列表与变更游标

#### 2.1 列表

`GET /network-libraries` 返回全部网络库，附带一个 `changeToken`：它表示“生成这份列表时
服务端的变更边界”。没有网络库时 `libraries` 为 `[]`，但仍返回有效游标。

#### 2.2 变更游标（changeToken）

`changeToken` 是**不透明的同步游标**，不是身份凭证，也不携带语义：

- 客户端不得自行构造（如 `ct_0`）或解析其内容。
- `GET /network-libraries/changes?since={changeToken}` 返回自该边界以来 `changed`
  （库 + 新 revision）与 `deletedServerLibraryIds`。无变化时两者都是 `[]`。
- 游标按**规范化后的 API Base URL** 分开保存——不同服务端的游标互不通用。
- 游标过期返回 `410 change_token_expired`，客户端据此走全量恢复（见 §6）。

**关键时序规则：客户端必须在本轮全部更新和删除都落盘成功后，才保存响应里的新游标。**
否则一旦中途失败又保存了新游标，就会永久漏掉那批变更。游标推进 = 一批同步事务提交的确认点。

> 为什么用游标而不是让客户端逐库比 `revision`：游标让服务端能一次性告诉客户端“哪些库变了、
> 哪些被删了”，避免客户端遍历所有库轮询。`revision` 用于单库层面的缓存校验（ETag），游标用于
> 库集合层面的增量发现，两者职责不同。

### 3. Snapshot：单库完整快照

`GET /network-libraries/{serverLibraryId}/snapshot` 返回**某个库某个 revision 的完整状态**：
`library` 元信息 + 完整 `tree` + 完整 `records` 描述符清单（+ V2 的 `playbackPlans`，见第二部分）。

- 支持 `If-None-Match` / `ETag`（值为库 `revision`）；未变返回 `304`，无响应体。
- Snapshot 是**自洽且完整**的：客户端拿到它就能重建整棵树，不需要拼接历史增量。

目录树硬约束（服务端必须保证，客户端必须校验）：

- `tree.nodes` 是以 `nodeId` 为 Key 的扁平对象，Key 必须等于节点内 `nodeId`。
- Category 节点只用 `childIds`；Record Group 节点只用 `recordIds`。
- 每个非根节点恰好一个父；全树无环；所有节点从 `rootIds` 可达。
- 每条 Record 恰好属于一个 Record Group。
- Snapshot 里的 Record 描述符集合，必须与 Tree 实际引用的 Record 集合**完全相同**——
  不多不少。

> 为什么 Snapshot 一次给全树而不是增量补丁：完整快照让“原子替换镜像”成为可能（见 §5）。
> 增量补丁需要客户端维护可合并状态机，一旦某次补丁丢失就会静默偏离真相源。完整快照牺牲
> 少量带宽，换来“镜像永远是某个 revision 的忠实副本”这个可验证不变量。

### 4. 按 hash 增量下载 Record

Snapshot 的 `records[]` 只给**描述符**（`recordId` + `sha256`），不含记录正文。正文按需
单独下载：

`GET /network-libraries/{serverLibraryId}/records/{recordId}` 返回完整 Gesture Record，
`ETag` 是带双引号的 Record SHA-256。

**增量策略：** 客户端比对 Snapshot 描述符里的 `sha256` 与本地镜像已有记录的 hash，
只下载 hash 变化或本地缺失的记录。未变的记录直接从旧镜像复用。这样一次 Snapshot 更新只传
真正改动的记录正文。

#### 统一 hash 算法

描述符 `sha256`、Record 响应 `ETag`、客户端校验值必须用同一算法：

```text
canonicalBytes = RFC 8785 JCS(recordObject)
sha256 = lowercase_hex(SHA-256(canonicalBytes))
etag   = DQUOTE + sha256 + DQUOTE
```

**必须对 JCS 规范化字节算 hash，不能对 pretty-print 的文件字节算。** 否则不同缩进/键序会
产生不同 hash，增量下载与去重全部失效。这是让“hash 相等 ⟺ 内容相等”成立的前提。

### 5. 只读镜像的原子替换

同步写镜像必须有事务语义，绝不做原地增量修改：

1. 在临时目录里构造新镜像。
2. 校验完整 Tree、Record 描述符、Record Model、JCS SHA-256 与 ETag（V2 还要校验方案与
   引用关系，见 §11）。
3. 全部通过后**原子替换**旧镜像目录。
4. 任一步失败，保留旧镜像与旧 `changeToken`，UI 继续用旧数据。

因此“同步失败”永远退化为“继续用上一份完整镜像”，不会出现半新半旧的镜像。UI 侧据此得到一条
强保证：**只有新数据已完整落盘，才会显示更新**（比如新建方案成功后，只有该方案已出现在本地
镜像里才提示“添加成功”）。

### 6. 删除处理与全量恢复

- **单条记录删除**由服务端驱动：`DELETE .../records/{recordId}` 幂等——记录存在则删除并
  从树移除引用、递增 revision 与游标返回 `204`；记录已不存在也返回 `204`（不再产生变更）；
  库不存在返回 `404`。重复 DELETE 不重复递增版本。
- **整库删除**通过变更接口的 `deletedServerLibraryIds` 传播：客户端在同步时删除对应本地镜像
  与订阅。
- **游标过期（410）→ 全量恢复：** 重新 `List`，对仍存在的已订阅库做完整同步（Snapshot +
  增量 Record），全部成功后保存新游标。这条路径保证客户端即使长期离线、游标失效，也能自愈到
  与服务端一致。

### 7. 新增记录（实时、非排队）

`POST /network-libraries/{serverLibraryId}/record-groups/{nodeId}/records` 向记录组新增记录：

- **实时请求，失败即失败**——不写镜像、不进离线队列。成功后客户端立即拉 Snapshot 让新记录
  进入镜像。
- 客户端在请求体里带的 `recordId` 只标识“本次客户端提交”；**服务端生成最终 `recordId`**
  （大写 UUID），写入记录与树并在响应返回。
- `typeSignature` 的**值由客户端计算**（`Shared/Model` 的几何启发式，如 `swipe_up:f1`），
  服务端**只存储和比对、绝不自己算**——算法只有一处、在客户端，避免两端实现漂移导致同一手势
  签名不一致。服务端把 `groupKind` / `typeSignature` 当不透明值锁定（单一组第一条记录锁定
  `typeSignature`）。后续记录签名与锁定值逐项比对，不一致 → `409 record_group_type_mismatch`。
  **网络组类型客户端不能转换**（不允许客户端把单一组转成混合组），转换类型只能由服务端
  管理端操作；本地组不受此限（纯客户端数据，可自由转类型）。
- 同一库内客户端提交的原始 `recordId` 必须唯一；换新 `Idempotency-Key` 重传相同原始
  `recordId` → `409 record_already_exists`，客户端应展示错误而非静默重试。

#### 记录身份与去重：一个对外身份 + 一个内部去重键

- **一个对外身份 = 服务端 recordId。** 客户端提交的原始 `recordId`（A）被服务端替换成新生成的大写
  UUID（B）；B 是这条记录在网络库里唯一对外的身份，Snapshot / 下载 / 回放 / 管理后台都只用 B。
  原始 A **不作为身份保留、不下发客户端、不在管理后台展示**——服务端只把 A 存为**内部去重键**。
  > 为什么不把 A 当身份：设备上本地库镜像与网络库镜像并存，若网络侧沿用本地 A，同一个 ID 会在两处
  > 都存在、设备解析歧义（见 §0 不变量 3 / §1.1）。服务端重铸 B 保证两套命名空间不重叠。
- **去重范围是"同一网络库内"，不是全库。** 同一个原始 A 可被上传 / 导入到**多个**网络库，各自得到
  **不同**的 B（一个原始 → 多个替换）；只有往同一个库塞第二次 A 才 `409`。库存的 `(server_library_id,
  client_record_id)` 唯一即实现此语义。
- **平铺一次查找，不递归。** 客户端提交的永远是它稳定的本地 UUID，不会回传服务端生成的 B，所以不存在
  `A→B→C` 的链需要向上追溯；服务端按"当前库 + 提交的原始 ID"查一次即可。

### 7.1 滚动距离测量（写回记录 meta）

`PUT /network-libraries/{serverLibraryId}/records/{recordId}/scroll-measurements` 上传一轮新测试，服务端把它合并进记录
`meta.scrollMeasurements`：**机型 + 分辨率 → 画布模式 → 最近 10 次测试**，每次测试记回放整条记录后的列表偏移 `(dx, dy)`
与用时 `durationMs`，并按这 10 次给 `min` / `max` / `avg`。

- **为什么记用时**：调用方按「同样偏移，用时更短 = 更快」挑手势；快慢看结果，不看手势本身按了多久。
- **为什么只留最近 10 次、由服务端合并**：10 次足以说明稳定程度；合并放在服务端（唯一真相源），两台同机型设备同时上传也不会互相覆盖。

- **为什么是通用偏移而不是「滑动距离」**：任何手势回放后都对滚动视图产生一个偏移，点击就是 0；多段手势（上滑 →
  点击让惯性停下、上滑 → 下滑抵消）也只是偏移不同。统一成 `(dx, dy)` 后所有记录结构一致，不需要按手势类型分支。
- **为什么分画布模式**：列表能往哪滚会影响同一手势最终滚多远（单向列表只吃手势在该方向的分量），所以只垂直 /
  只水平 / 垂直+水平各存一份。数据结构三种一致，单向模式另一轴为 0。
- **为什么放在记录里、不单独建表下发**：测量结果是这条手势的属性，SDK 查询与回放选择都按记录读取；
  放进记录就复用现有「revision → Snapshot → 按 sha256 下载记录」的同步链路，客户端同步逻辑零改动。
  代价是每次上传都会让这条记录的 hash 变化、被各设备重新下载一次（记录通常几 KB，可接受）。
- **为什么截断按服务端收到的先后**：设备时钟不可信，`measuredAt` 只做展示。
- **为什么不记录测量设备**：沿用隐私约束（§8），网络 API 不把设备 UDID 写进手势数据。
- **为什么任何客户端都能传**：不同机型需要各自测量，只允许记录上传者写入就测不全。

### 8. 图标与隐私

- Node Icon 四种 `kind`：SF Symbol / Emoji / 网络图片 / 内联 PNG。网络 API 的内联图用
  `{kind:3, mediaType:"image/png", dataBase64:...}`，服务端原样保存与返回，不得改成设备本地路径。
- `dataRef`（设备本地文件引用）禁止出现在网络请求/响应，收到即 `422 invalid_library`。
- Gesture Segment 不得含 `deviceUDID`；客户端上传前移除，服务端收到即 `422 invalid_record`。
  网络 API 不传输录屏、缩略图、临时录制文件或设备 UDID（回放方案创建者字段除外）。

---

## 第二部分：回放方案

### 9. 模型与调用不变量

回放方案由服务器统一管理；手机只保存可离线回放的只读镜像。SDK 调用模型保持不变：

```text
App → planId → CHRTouchPlaybackLoadPlanId / Start
```

SDK 与业务 App **不需要知道方案来自本地组还是网络组**。设备端按已同步镜像解析 `planId`，
回放时不请求网络服务。backboardd 的方案查找从“仅本地库”扩展为“本地库 + 完整网络镜像”，
两种来源用相同的归属校验；`CHRTouchPlaybackPlanInfoForId` 对网络方案返回 `source: network`
及网络库分类链路。禁止回放：网络缺失、镜像不完整、UUID 冲突、归属不匹配的方案。

> **本设计没有版本概念，这就是当前唯一设计。** URL 里的 `/v1` 是服务端的挂载前缀（在用户
> 配置的 Base URL 内），与本设计无关，不存在 `/v2/...` 路由，也无需兼容任何旧格式。JSON
> `schema` 字段里的 `chr.xxx/vN`（如 `chr.network-library-snapshot/v2`）是线格式标识**常量**，
> 末尾的 `/vN` 只是各常量的固定字面值，不代表契约有多个版本；服务端按契约字面值原样下发，
> 客户端按 `schema` 字面值判断数据结构。详见契约文档 `network-library.md` 第 2 节的版本号说明。

### 10. 方案对象模型

默认方案与自定义方案是**同一种持久化对象**，仅 `kind` 不同：

```json
{
  "planId": "66BEC501-4A20-4BF3-9F70-6D175178B2F8",
  "kind": "custom",
  "name": "自定义方案 1",
  "alias": "默认滑动",
  "createdAt": 1787000000000,
  "updatedAt": 1787000000000,
  "revision": 1,
  "createdByDeviceUDID": "00008110-001234567890801E",
  "targetGroupId": "A0794FA8-D68E-4ECF-9D13-EF296F74D6CF",
  "plan": {
    "schema": "chr.playback-plan/v1",
    "target": {
      "libraryId": "3F2C9B14-77AE-4D0B-8C31-2E6F5A8B3D40",
      "recordGroupId": "A0794FA8-D68E-4ECF-9D13-EF296F74D6CF"
    },
    "match": { "required": [] },
    "selection": { "kind": "random" },
    "replay": { "mode": "configId", "configId": "default" }
  }
}
```

设计约束及其理由：

1. **`planId` 由服务端生成，跨所有网络库全局唯一 UUID，永不改变**；客户端创建请求不得传入。
   `planId` 是 SDK 回放入口的稳定键，必须由真相源分配以保证全局唯一与不可变。
2. **目标归属由 URL 与服务端重写决定**：`targetGroupId` 和 `plan.target` 的库/组不由客户端
   跨组指定。客户端不能把方案“挪”到别的组。
3. **`createdByDeviceUDID` 由请求 Header 写入**，客户端不能在 JSON Body 指定或更新它——
   否则归属校验形同虚设。
4. **方案只能指向本库内的记录组**；若出现 `plan.scope.recordIds`，其中每个 ID 必须引用该组
   内记录。缺 `scope` 表示整个记录组。
5. **别名唯一性**：非空 `alias` 在同一记录组内大小写不敏感唯一（`name` 可重复）。服务端生成
   `alias_normalized`（去首尾空白、Unicode NFC、大小写折叠），建唯一索引
   `(server_library_id, target_group_id, alias_normalized)`。空 alias 存为 `NULL`，不参与唯一约束。
6. **`revision` 从 1 起，每次更新递增**，创建者不可变。用于乐观并发（见 §12）。

`plan` 策略字段以客户端实际序列化为准：筛选条件在 `match.required`；选择方式在 `selection`；
非空候选范围在 `scope.recordIds`；阶段覆盖（仅非空时出现）是 `stageReplays` 数组，每项
`{ "segmentIndex": 0, "replay": { ... } }`。`replay.mode` 只能是 `configId`（用 `configId`）
或 `inline`（用 `options`）。服务端**不得**把旧草案的 `match.kind`、`mode:"profile"` 或
对象型 `stageReplays` 当作本协议格式。

#### 默认方案：每组恰好一条、不可删

- **每个记录组必须恰好有一个 `kind:"default"` 方案。** 它与自定义方案共用相同的 `planId`、
  持久化、Snapshot 与回放链路，但**不可删除、不可迁移**。其余方案为 `kind:"custom"`。
- 默认方案的服务端固定内容：`name` = `默认方案`、`alias` 为空、不出现 `scope` 与
  `stageReplays`、`match.required` 为空数组、`selection` = `{"kind":"random"}`、`replay` =
  `{"mode":"configId","configId":"default"}`。创建库时服务端校验并规范化为此内容。
- 任何修改/删除默认方案的请求返回 `422 default_playback_plan_protected`。

> 为什么默认方案也是一条持久化方案而非隐式行为：让“整组随机 + default 配置”走和自定义方案
> 完全相同的对象、Snapshot 字段与回放解析路径，设备端不需要为“没有方案的组”写特例分支。
> 每组恒有至少一条方案，回放入口永远有确定的 `planId` 可解析。

### 11. Snapshot 中的方案（V2）

`GET .../snapshot` 返回 `chr.network-library-snapshot/v2`，在 V1 的 `library`/`tree`/`records`
基础上新增：

- 记录组节点上的 `playbackPlanIds[]`（该组的方案 ID 列表）。
- 顶层 `playbackPlans[]`（完整方案对象清单）。

服务端必须保证的引用完整性：

- 每个 `playbackPlans[].planId` 恰好被一个记录组的 `playbackPlanIds[]` 引用，两侧集合完全一致。
- 方案 `targetGroupId`、`plan.target.recordGroupId` 与引用它的记录组一致；`plan.target.libraryId`
  等于 `serverLibraryId`。
- Snapshot、手势清单、方案清单来自**同一个库 revision**。
- 方案 / 树 / 引用关系任一损坏，**不得返回部分成功 Snapshot**，返回 `500 invalid_library_state`
  并记录服务端错误。

> 单库 revision 同时覆盖树、记录、方案，是“原子替换镜像”在 V2 的延伸：三者必须来自同一
> revision，客户端才能把它们作为一个一致镜像整体替换。

### 12. 写接口：创建 / 更新 / 删除

所有方案写请求带 `Idempotency-Key` 与 `X-CHR-Device-UDID`。

#### 12.1 创建

`POST /network-libraries/{serverLibraryId}/record-groups/{recordGroupId}/playback-plans`

请求体带 `kind:"custom"` 与 `name`/`alias`/`plan`，**不得**带 `planId`、`createdByDeviceUDID`、
`revision` 或 `target`——任一存在即 `422 invalid_playback_plan`，服务端不能静默忽略。这些字段
分别由服务端 UUID、请求 Header、当前版本、URL 目标生成。成功返回
`201 chr.network-playback-plan-result/v2`，含最终 `planId`、库 `revision`、`planRevision` 与
`changeToken`。

#### 12.2 更新

`PUT /network-libraries/{serverLibraryId}/playback-plans/{planId}`

额外带 `If-Match-Plan-Revision`（当前方案 revision）做乐观并发。Body 与创建同构，`kind` 仍须
`custom`；默认方案的 PUT 一律 `422 default_playback_plan_protected`。不允许改目标记录组；带
服务端控制字段同样 `422 invalid_playback_plan`。**仅 `createdByDeviceUDID` 与 Header 相等时
允许更新**，否则 `403 playback_plan_owner_required`。`If-Match-Plan-Revision` 落后返回
`409 plan_revision_conflict`。

#### 12.3 删除

`DELETE /network-libraries/{serverLibraryId}/playback-plans/{planId}`

额外带 `If-Match-Plan-Revision`。仅创建者可删 `custom` 方案；删默认方案
`422 default_playback_plan_protected`。成功事务必须**同时**删除方案、从记录组 `playbackPlanIds[]`
移除 ID、递增库 revision 并推进 changeToken，返回 `204`。

#### 错误一览（方案）

| 状态 | `error.code` | 场景 |
|---:|---|---|
| 400 | `invalid_device_udid` | UDID Header 缺失或非法 |
| 403 | `playback_plan_owner_required` | 非创建设备更新/删除 |
| 404 | `library_not_found` / `record_group_not_found` / `playback_plan_not_found` | 目标不存在 |
| 409 | `playback_plan_alias_conflict` | 同组别名冲突 |
| 409 | `plan_revision_conflict` | `If-Match-Plan-Revision` 落后 |
| 409 | `idempotency_conflict` | 同幂等 Key 搭配不同 Body |
| 422 | `invalid_playback_plan` | 方案/候选范围/组类型配置无效，或含服务端控制字段 |
| 422 | `playback_plan_target_invalid` | 试图跨库/跨组或改目标 |
| 422 | `default_playback_plan_protected` | 试图改/删默认方案，或默认方案内容不符合固定约束 |

### 13. 创建网络库时导入本地方案与重写引用

`POST /network-libraries`（`chr.network-library-create/v2`）的创建请求**必须包含
`playbackPlans`**，且请求 Tree 中每个记录组恰好对应一条默认方案。方案的 `targetGroupId` 引用
请求 Tree 的**临时 node ID**；每个 `plan` 是可编辑策略体，**不含** `target`、`planId`、
`createdByDeviceUDID`、`revision`。

服务端处理流程：

1. 生成新的 `serverLibraryId`。
2. 为每个记录组、记录、方案生成新的**网络全局大写 UUID**，禁止 `record-`/`node-` 等前缀；
   客户端上传的临时 ID（如 `local-group-a`、`local-record-a`）只用于本请求内建立引用关系，
   服务端不得沿用其字面值或前缀。
3. 重写 `rootIds`、`parentId`、`childIds`、`recordIds` 引用，以及每个方案的 `targetGroupId`
   与 `plan.target`（重写为最终 `serverLibraryId` 与最终记录组 ID）。
4. 把请求 Header 的 UDID 写入每个方案的 `createdByDeviceUDID`。
5. 返回含最终方案的完整 V2 Snapshot。

**本地原方案不删除；其 `planId` 不迁移、不复用**——本地与网络是两份独立数据。

服务端必须拒绝（`422 invalid_playback_plan`，且不创建部分网络库）：缺 `playbackPlans`、某组
没有或有多条默认方案、`targetGroupId` 不属于请求 Tree 的记录组、默认方案内容不符合 §10 固定
约束、`plan` 含服务端控制字段。

服务端此后若通过管理端或内部任务新增记录组，也必须在同一事务内为该组创建唯一默认方案；本协议
不新增 HTTP 路径。

### 14. 原子性、幂等与权限边界

- **单行加锁的单事务写。** 任何修改网络库树或方案的操作——新增记录、节点变更、方案的
  创建/更新/删除——都必须先锁定同一 `NetworkLibrary` 行，再在一个事务内修改实体、树索引、
  库 revision、更新时间与 changeToken。不得先写 JSON 树后写方案，也不得让两个并发写各自基于
  旧树互相覆盖索引。
- **幂等范围含设备。** 幂等唯一键为 `method + canonicalPath + deviceUDID + idempotencyKey`
  并保存请求 Body 哈希：同范围同 Body 重放首次结果；同范围不同 Body →
  `409 idempotency_conflict`；不同设备即便偶然撞 Key 也互不冲突、互不重放。服务端至少保留
  幂等结果 24 小时。
- **UDID 是产品权限不是安全边界**：服务端仍必须做 Header 比对，客户端不能仅靠隐藏 UI 越权。
- **管理端可绕过创建者限制**，但每次越权操作必须写审计日志。

---

## 15. 管理端形态（设计约束，非 UI 规格）

服务端后台点击**手势组（Record Group）**后进入详情页，顶部两个 Tab：**手势列表 ｜ 回放记录**；
不在左侧树增加“回放计划”一级入口。手势列表为默认 Tab；回放记录 Tab 提供创建/查看/编辑/删除，
默认方案与自定义方案一并显示。设备创建者仍受 UDID 归属限制，管理员可覆盖但每次操作写审计。
本设计不含账号、角色、签名或强认证，但服务端必须提供管理端可调用的超管操作能力。

## 16. 明确不在范围

- 登录鉴权、角色、签名、强认证（当前受信任内网，UDID 归属是产品权限）。
- WebSocket、Tree Patch、离线编辑队列、双向合并。
- 跨服务器迁移。
- Web 管理端的具体像素级 UI（仅约束信息架构与超管能力）。
