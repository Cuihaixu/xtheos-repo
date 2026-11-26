# XTCommand 1.3.0

## 新功能
- 完整接入 xtheos v1.0 打包与多源规范，统一导出三环境（rootful / rootless / roothide）成品包与通用 vendor 编译桩

## 改进
- 清理废弃的 XTCommand-mac Target，统一使用两层 vendor 搜索路径与 5 级包信息继承
- 完善 DEBIAN/control 元数据（Name 继承、Author、X-XTheos-Env、X-XTheos-Build）

## 修复
- 补齐 xtcmd CLI Target 的 arm64/arm64e 双架构配置，修复 rootless/roothide 环境缺 arm64e 切片问题

## 升级与兼容性
- 最低系统部署版本锁定为 iOS 14.0，兼容 rootful / rootless / roothide 全环境
