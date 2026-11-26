# Conduit 0.2.4

## 新功能
- 接入 xtheos v1.0 打包与多源规范，统一导出三环境（rootful / rootless / roothide）成品包与通用 vendor 编译桩

## 改进
- 基于 origin/main (0.2.4) 统一迁移两层 vendor 搜索路径、5 级包信息继承与 6 个 DEBIAN/control（Name 继承、Author、X-XTheos-Env、X-XTheos-Build）
- 锁定部署版本为 iOS 14.0

## 修复
- 无

## 升级与兼容性
- 最低系统部署版本锁定为 iOS 14.0，兼容 rootful / rootless / roothide 全环境
