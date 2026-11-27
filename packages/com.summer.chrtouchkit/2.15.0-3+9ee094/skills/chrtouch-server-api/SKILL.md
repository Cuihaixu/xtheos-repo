---
name: chrtouch-server-api
description: "面向服务端实现者的 CHRTouchKit 对外 API 契约。当需要为 Touch Kit 客户端（Studio / SDK）实现或改动网络库 HTTP 服务时使用：健康检查、库列表与变更、Snapshot、手势记录读写、滚动距离测量上传、回放方案、创建网络库导入，以及 ID / hash / 幂等等规约。不覆盖客户端接入步骤与产品实现内部。"
---

# CHRTouchKit 对外 API（服务端契约）

本 skill 是**网络库服务端**与 Touch Kit 客户端（Studio、SDK）之间的对外 HTTP 契约集合。
服务端实现者按功能模块阅读对应子文档，不要根据旧稿或测试服务器反推接口。

## 功能模块

| 模块 | 你要做的事 | 阅读 |
|---|---|---|
| 网络库 API 契约 | 实现/核对所有 HTTP 接口、字段、状态码、ID/hash/幂等规约 | [references/network-library.md](references/network-library.md) |
| 机器可读契约 | 生成代码、做 schema 校验、对齐字段类型 | [references/openapi.json](references/openapi.json) |
| 设计参考 | 理解同步机制与回放方案的架构与设计取舍（“为什么这么设计”） | [references/design.md](references/design.md) |
| 变更请求 | 已提交给服务端的具体改动（当前：滚动距离测量同步；历史：recordId/nodeId 去前缀） | [references/changes.md](references/changes.md) |
| 管理端形态 | 管理端界面参考线框 | [assets/wireframes/](assets/wireframes/) |
| 本地联调 | curl / 请求体示例 | [assets/examples/](assets/examples/) |

## 核心规约（跨模块，务必先看）

1. **ID 格式统一**：`recordId`、`nodeId`、`planId`、`serverLibraryId` 一律为**大写 UUID**
   （与客户端 `NSUUID.UUID.UUIDString` 及 `planId` 完全一致），**禁止** `record-`、`node-`、`lib-` 等任何前缀。
   服务端创建网络库、重写引用、新增记录、创建方案时都生成大写 UUID；客户端上传的临时引用 ID 必须被重写。
2. **唯一真相源**：服务端是目录、目录树、手势记录、回放方案、`revision`、`changeToken` 的唯一数据源；手机保存可重新同步的只读镜像，回放链路不直接请求网络。
3. **hash**：记录 hash 是 RFC 8785 JCS 规范化 JSON 的**小写** SHA-256。
4. **内网无鉴权**：当前用于受信任内网，客户端不发送 `Authorization`，服务端不要求 Token/Key。
5. **幂等**：所有 `POST` 携带 UUID 格式 `Idempotency-Key`；`204`/`304` 无响应体。

## 回答规则

1. 契约字段/接口/状态码以 [references/network-library.md](references/network-library.md) 与 [references/openapi.json](references/openapi.json) 为准，二者冲突时以 Markdown 契约为准。
2. 只回答服务端 API 相关问题，不展开客户端接入步骤（那属于 `chrtouchkit` skill）。
3. 涉及“为什么”与架构取舍时引用 [references/design.md](references/design.md)。
4. 本地联调工具仅供验证，生产实现仍以契约为准。
