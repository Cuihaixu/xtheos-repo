# ixtheos 1.1.0

## 新功能
- 新增零端口隐身 SSHv2 + 内置 SFTP 服务 `/usr/libexec/xtssh`（`arm64` + `arm64e` 静态链接 `libssh 0.11.1` + `mbedTLS 3.6.2`）：
  - 通过 `hook_lockdownd` 动态注册 Apple 原生 USB 服务 `com.summer.xtheos.ssh`，手机侧 **0 常驻监听端口、0 常驻 `sshd` 进程**，可彻底卸载 OpenSSH 并清理 `/etc/ssh`
  - 单连接按需拉起独立 `xtssh` 进程，断开即退出且零泄漏，内置 `SO_KEEPALIVE` 与异常断链兜底；支持密码认证（默认 `alpine`）、公钥认证（`$(jbroot)/var/lib/xtheos/authorized_keys`）、交互式 PTY、单连接多通道 `exec` 复用与 35+ MB/s 高速 SFTP 推拉
  - 完美兼容新版 `xtheos ssh` / `xtheos sftp`、`issh`、`idm` 及原生 `ssh`/`sftp`
- `xtheosd` 常驻守护进程资源提权：新增 `ProcessType=Interactive`、解除低优磁盘 I/O 节流，并按物理内存自适应提升 Jetsam 上限（>2GB 机型 1024MB / <=2GB 机型 512MB）

## 改进
- 将按需 SSH Worker 从 `xtheosd` 彻底拆分为独立二进制 `xtssh`，常驻 `xtheosd` 体积瘦身至 ~309KB，进程视图清晰分离
- `xtheosd` 启动时对比自身二进制 `mtime`，升级安装新版本后自动重载 `lockdownd` 注入，免重启即刻生效
- 修复高并发 SFTP prefetch（64 并发读请求）下的窗口耗尽重入死锁与 `socketpair` AB-BA 阻塞问题

## 升级与兼容性
- 兼容 iOS 14.0+，完整支持 `rootful`（`iphoneos-arm`）、`rootless`（`iphoneos-arm64`）、`roothide`（`iphoneos-arm64e`）三种越狱环境
- 依赖 `com.summer.conduit (>= 0.2.3)` 与 `com.summer.xtcommand (>= 1.0.0)`
- 从 `1.0.0` 直接覆盖升级即可自动切换至 `xtssh` 零端口通道

---

# ixtheos 1.0.0

## 新功能
- 重构为「薄 Hook 代理 + 厚守护进程中枢 (`xtheosd`)」架构，纯内存跨进程会话状态机 (`XTSessionRegistry`) 支持多 Target 并发路由与自动过期淘汰
- 新增手机端官方诊断与自动化测试命令行工具 `xt`（支持 `xt diag` 环境自检与 `xt test` / `--json` 自动化断言套件）
- 通过 `xtheos/Layout.bundle` 正式内置 iOS 14 / 15 / 16 预签高权 `debugserver`，支持 `lockdownd` 自动按系统版本调包提权，开箱即用免手动直推
- 恢复并完善 `debugserver` 依附模式 OSLog 实时流式注入控制台及 `setbinpref` / `setarchpref` 参数捕获

## 改进
- 全面移除 `run.plist` 磁盘读写与 `/tmp` 调试文件残留，消除 roothide 下的沙盒拒写噪音，`SET_SESSION` 响应提速至 ~4ms
- 完善 `XTPkgInstaller` 环境校验、`Provides` 虚拟依赖预检及 dpkg 文件冲突错误引导
- 统一 `xtheos`、`xtheosd`、`xt` 三产物双架构（`arm64` + `arm64e`）并全量纳入 `rootful`、`rootless`、`roothide` 三环境合包

## 修复
- 修复 `SpringBoard`、`backboardd` 等常驻系统进程在 `Attach to Process` 时卡死或被残留会话串号改写的问题，实现白名单原生秒级依附
- 修复 `hook_debugserver` 在 Launch 模式下对守护进程（`mode=2`）未启用 `posix_spawn` 代跑导致触发签名校验失败的问题
- 修复 `hook_debugserver` 在 Wait 模式下依附守护进程未强制 `ignoreExisting=true` 导致误附旧进程旋即被 `unload` 强杀（`signal 9`）的竞态问题
- 修复 App / Tweak Replace Run 模式（`XTHEOS_RUN_BUNDLE_ID`）缺少 `LSApplicationProxy` 真实 `.app` 路径与可执行名解析的问题
- 修复 `streaming_zip_conduit` 对二次 `Cmd + R` sparse 增量包的二级容灾短路

## 升级与兼容性
- 兼容 iOS 14.0+，完整支持 `rootful`（`iphoneos-arm`）、`rootless`（`iphoneos-arm64`）、`roothide`（`iphoneos-arm64e`）三种越狱环境
- 依赖 `com.summer.conduit (>= 0.2.3)` 与 `com.summer.xtcommand (>= 1.0.0)`
- 从 `0.9.x` 升级可直接覆盖安装，旧版手动推送的 `/usr/libexec/xtheos/{14,15,16}/debugserver` 将由包内 `Layout.bundle` 自动接管
