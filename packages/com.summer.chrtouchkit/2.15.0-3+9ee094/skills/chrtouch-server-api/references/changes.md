# 滚动距离测量改为「机型 → 模式 → 最近 10 次」并由服务端合并（服务端变更请求 · 2026-11-04 · 待实现）

> 当前契约：[网络库 API 契约](network-library.md) §8.4 · 机器可读：[OpenAPI](openapi.json)
> `ScrollMeasurement` / `ScrollModeResult` / `ScrollSample` / `ScrollStats` / `ScrollMeasurementUpload` ·
> 请求样例：[`assets/examples/upload-scroll-measurement.json`](../assets/examples/upload-scroll-measurement.json)

## 一、背景

上一版（同日上线）每次上传是一份汇总（`mode` + `offsetPt` 中位数 + 分位数），服务端按「机型 + 分辨率 + 模式」整项覆盖。
现在改为：

- 每次测试同时记**偏移** `dx/dy` 和**用时** `durationMs`（发出回放 → 列表最后一次滚动），给调用方按「同样偏移谁更快」挑手势；
- 测试结果**跨多轮累积，只留最近 10 次**，统计改为 `min` / `max` / `avg`；
- 记录里按 **机型 + 分辨率 → 模式 → 每次测试** 三层组织；
- 网络库由**服务端合并**（追加、截断、重算），保证多台同机型设备同时上传不丢数据。

接口路径、请求 / 响应 `schema` 常量、事务、幂等、权限都**不变**。

## 二、需要服务端改的

1. **请求 `measurement`**：`{ deviceModel, screenPoints, screenScale?, osVersion?, mode, samples: [{dx, dy, durationMs, measuredAt}] }`，
   `samples` 1~10 个。去掉 `offsetPt` / `p25Pt` / `p75Pt` / `iterations`。
2. **记录结构**：`meta.scrollMeasurements` 每项 = 一个「机型 + 分辨率」：
   `{ deviceModel, screenPoints, screenScale, osVersion, modes: { vertical | horizontal | both: { samples, min, max, avg } } }`。
3. **合并**（契约 §8.4「服务端合并规则」）：定位项（没有则新建）→ 更新 `screenScale` / `osVersion` → 追加 `samples` 到
   `modes[mode].samples` 末尾、只留最后 10 个（按服务端收到的先后）→ 按截断后的样本**逐字段**重算 `min` / `max` / `avg`
   （`avg` 保留 1 位小数）。幂等重放**不重复追加**。
4. **校验** → `422 invalid_scroll_measurement`：`mode` 非法；`samples` 为空或超过 10 个；样本缺 `dx` / `dy` / `durationMs` /
   `measuredAt`、`durationMs` 为负；出现未声明字段（含旧的 `offsetPt` 等）。
5. **旧数据**：项目未上线，不兼容。已写入的旧格式测量（11-04 联调写入「视频列表 / 测试帧率」#3 的 3 个模式项）请直接删除。

## 三、验收

契约 §14 第 19、20 条。端到端：设备 A 对网络库记录测「垂直」2 次 → 再测 1 次 → GET 记录 `modes.vertical.samples` 为 3 个、
`avg` 按 3 个算；设备 B 同步后详情页能看到。

---

## 历史：滚动距离测量改为通用偏移（2026-11-04 上午，已上线并联调通过，字段已被上面的变更取代）


> 当前契约：[网络库 API 契约](network-library.md) §8.4 · 机器可读：[OpenAPI](openapi.json) `ScrollMeasurement` / `ScrollOffset` ·
> 请求样例：[`assets/examples/upload-scroll-measurement.json`](../assets/examples/upload-scroll-measurement.json)

## 一、背景

11-03 上线的滚动距离测量只支持「单段滑动手势」，结果是一个距离标量。现在改为**通用**：任何记录（单段 / 多段，
滑动 / 点击……）都测「回放整条记录后滚动视图的偏移 `(dx, dy)`」，点击类就是 0；并按测试画布的滚动方向分三种模式
（只垂直 / 只水平 / 垂直+水平）各存一份。接口路径、请求/响应 schema 常量、写入事务、幂等、权限都**不变**，只改
`measurement` 的字段和覆盖键。

## 二、需要服务端改的

1. **`measurement` 字段**（契约 §8.4 表格）：
   - 删除 `axis`、`distancePt`；
   - 新增必填 `mode`：`vertical` / `horizontal` / `both`；
   - 新增必填 `offsetPt`：`{dx, dy}`；
   - `p25Pt`、`p75Pt` 由数字改为 `{dx, dy}`（仍可选）；
   - `samplesPt` 由数字数组改为 `{dx, dy}` 数组；
   - `dx`、`dy` 带符号，**可以是负数**，按 JSON number 原样存取。
2. **覆盖键加上 `mode`**：`deviceModel` + `screenPoints`（四舍五入）+ `mode` 都相同才覆盖，否则追加。
   同一机型分辨率下三种模式各存一项。
3. **校验**：`mode` 非法、`offsetPt` / 样本缺 `dx` 或 `dy`、出现未声明字段（含旧的 `axis` / `distancePt`）→
   `422 invalid_scroll_measurement`。
4. **旧数据**：项目未上线，不做兼容。11-04 联调时写入「视频列表 / 测试帧率」3 条记录的旧格式测量项可以直接删掉
   （从这 3 条记录的 `meta.scrollMeasurements` 移除，同一事务递增 revision），客户端会把读到的旧格式项当作无效丢弃。

## 三、验收

契约 §14 第 19、20 条（已按新字段更新）。端到端：设备 A 对网络库任意记录（含点击、多段）选「全部」方向测量 →
提示「已上传 N 项测量结果」→ 设备 B 同步后，手势详情页「滚动距离」按方向显示设备 A 的结果。

---

## 历史：滚动距离测量同步（2026-11-03，已于 11-04 上线并联调通过，字段已被上面的变更取代）


> 当前契约：[网络库 API 契约](network-library.md) §8.4 · 机器可读：[OpenAPI](openapi.json) `uploadScrollMeasurement` ·
> 请求样例：[`assets/examples/upload-scroll-measurement.json`](../assets/examples/upload-scroll-measurement.json)

## 一、背景

Studio 新增「测试滑动手势滚动距离」：对一条滑动手势回放 N 次，测出列表滚动距离（中位数 + P25/P75 + 每次原值），
按「机型 + 分辨率」存进这条记录的 `meta.scrollMeasurements`。网络库记录在客户端只读、同步会覆盖，
所以网络库的测量结果必须经服务端写回记录，再随正常同步下发到所有设备。客户端已实现并按契约调用。

## 二、需要服务端做的事

1. **新增接口** `PUT /network-libraries/{serverLibraryId}/records/{recordId}/scroll-measurements`
   （请求/响应/字段/错误见契约 §8.4）。
2. **按「`deviceModel` + `screenPoints` 四舍五入」覆盖或追加**到 `meta.scrollMeasurements`；
   先后以服务端收到的顺序为准，不比较 `measuredAt`。
3. **同一事务**内更新记录、重算该记录 JCS SHA-256、递增库 `revision`、推进 `changeToken`；`recordId` 不变。
   响应前完成落库（客户端收到 `200` 立刻同步）。
4. **任何客户端都可上传**；请求不带 `X-CHR-Device-UDID`，记录内**不得写入任何设备 UDID**。
5. **新错误码** `422 invalid_scroll_measurement`（客户端已有对应提示文案）；请求体上限 64 KiB。
6. **兼容现有接口**：`GestureRecordMetadata` 新增可选字段 `scrollMeasurements`。新增记录（§8.2）与创建网络库导入（§10）
   上传的记录 `meta` 可能已带该字段（本地测过的手势），须按 Schema 校验并原样保存，**不得返回 `422 invalid_record`**
   ——按旧 Schema（`additionalProperties: false`）校验的服务端会误拒，这一条请优先改。

## 三、验收

见契约 §14 第 19、20 条。端到端：设备 A 对网络库手势「测距离」→ 提示「已上传 N 条测量结果」→
设备 B 同步后，手势详情页「滚动距离」能看到设备 A 机型的结果。

---

## 历史：网络库 recordId / nodeId 前缀修复（已完成）

> 当前契约：[网络库 API 契约](network-library.md) ·
> 机器可读：[OpenAPI](openapi.json)

## 一、问题

服务端在**创建网络库**、**重写 Node/Record 引用**、**新增记录**时，生成的 `recordId` 和 `nodeId`
带了 `record-` / `node-` 前缀，应为**纯 UUID**。

对比同一个库里的 `planId` 是正确的纯 UUID —— 说明服务端对 `planId` 的处理对了，
`recordId` / `nodeId` 沿用了带前缀的字面值，没按纯 UUID 生成。

## 二、证据（现网 `lib-699394dc`）

```
GET http://aweme.rapp.lan/v1/network-libraries/lib-699394dc-2441-45e1-bc75-785e4396e35b/snapshot
```

返回：

| 字段 | 现状 | 是否正确 |
|---|---|---|
| `records[].recordId` | `record-254b8a1e-9eeb-4f76-...` | ❌ 带 `record-` 前缀 |
| `tree.nodes[].nodeId` | `node-cd1536a1-...` | ❌ 带 `node-` 前缀 |
| `tree.nodes[].recordIds[]` | `record-...` | ❌ 带前缀 |
| `playbackPlans[].planId` | `31515EFD-A3C2-4996-...`（纯 UUID） | ✅ 正确 |

客户端（Studio/SDK）本地生成 recordId 用的就是纯 UUID（`NSUUID.UUID.UUIDString`），
前缀不是客户端加的，是服务端在创建/重写时产生的。

## 三、应改为

创建网络库（`POST /network-libraries`）、重写引用、新增记录（`POST .../records`）时：

1. `recordId`、`nodeId` **一律生成大写 UUID（与本地 NSUUID.UUID.UUIDString 及 planId 一致）**，**禁止** `record-`、`node-` 等任何前缀，
   规则与 `planId` 完全一致（服务端生成、网络库全局唯一）。
2. 客户端上传的临时 ID（如 `local-record-a`、`local-group-a`）**仅用于本请求内建立引用关系**，
   服务端**不得沿用其字面值或前缀**，必须重写为服务端新生成的纯 UUID。
3. 重写后同步更新 `tree` 的 `recordIds` / `childIds` 与所有 `plan` 引用，保证引用一致。

## 四、依据文档（已更新，以此为准）

- [`network-library.md`](network-library.md) **「创建网络库导入」一节的 ID 规约**
  （明确 nodeId/recordId/planId 全部大写 UUID、禁前缀）
- [`openapi.json`](openapi.json)：`RecordDescriptor.recordId`、`GestureRecord.recordId`、
  `chr.network-record-create-result/v1.recordId` 均已标注 `"format": "uuid"`

## 五、现有数据处理

`lib-699394dc` 是带前缀的历史数据，服务端改完后不会自动变。二选一：

- **重建**该网络组（用当前客户端重新创建，最干净）；或
- 服务端跑一次**数据规范化**：把 `recordId`/`nodeId` 去前缀换纯 UUID，同步改 `tree` 的
  `recordIds`/`childIds` 和 `plan` 引用（同一事务，保持一致）。
