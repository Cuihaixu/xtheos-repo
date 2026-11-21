# xtheos 0.9.0

## 新功能
- 全面适配 xtheos 1.0 标准工程规范与多产物打包体系。
- 迁移 IPC 通信架构至 Conduit (com.summer.conduit)，由系统级 ConduitService 统一接管跨进程与沙盒通信。

## 改进
- 移除遗留 hand-rolled mach IPC 与 MobileGestaltHelper 自建 broker 逻辑，提升系统级通信稳定性。
- 统一由 xtheos 驱动三环境 (rootful / rootless / roothide) 打包与依赖链管理。

## 修复
- 无

## 升级与兼容性
- 强依赖 com.summer.conduit (>= 0.2.3)，安装时将由包管理器自动拉取并装配。
