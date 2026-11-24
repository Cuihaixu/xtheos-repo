# ixtheos 0.9.1

## 新功能
- 守护进程 xtheosd 纯内存跨进程会话管理与集中调度
- 支持多 Target 并发构建与无冲突路由

## 改进
- Hook 插件全面轻量化，移除 run.plist 磁盘依赖，全走 Mach IPC 委托
- 守护进程自适应 roothide / rootless / rootful 路径解析

## 修复
- 修复 SpringBoard 等常驻系统进程在 Attach to Process 时卡死的问题，实现秒级原生透传依附

## 升级与兼容性
- 兼容 iOS 14.0+，支持 rootful、rootless、roothide 三种越狱环境
