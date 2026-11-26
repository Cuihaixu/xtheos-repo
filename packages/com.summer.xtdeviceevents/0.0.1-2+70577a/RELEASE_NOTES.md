# XTDeviceEvents 1.0.0

## 新功能
- 接入 xtheos v1.0 打包与多源规范，支持三环境（rootful / rootless / roothide）统一本地打包与开发源发布

## 改进
- 升级 3 个 DEBIAN/control（Name 继承、Author、X-XTheos-Env、X-XTheos-Build）与两层 vendor 搜索路径
- 锁定部署版本为 iOS 14.0 并补充 .gitignore 规则

## 修复
- 无

## 升级与兼容性
- 最低系统部署版本锁定为 iOS 14.0，兼容 rootful / rootless / roothide 全环境
