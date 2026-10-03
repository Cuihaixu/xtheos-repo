#!/bin/bash
# xtheos 引导安装脚本 —— 首次部署用。之后更新/卸载走 `xtheos update` / `xtheos uninstall`。
#   bash -c "$(curl -fsSL https://repo.iospkg.cn/install.sh)"
#
# 做的事:环境自检 → git clone 私有仓库的「用户子集」到 ~/xtheos(sparse)→ 切最新发布 tag → xtheos setup。
# 本脚本公开,但代码私有:git clone 用你机器已配的 git 凭证(gh auth / ssh / keychain),没权限会明确提示。
# 自定义目录:装前 `export XTHEOS=/your/path`。
set -euo pipefail

REPO="https://github.com/Cuihaixu/xtheos.git"
ROOT="${XTHEOS:-$HOME/xtheos}"
# clone 后只保留"用户该有的"目录(排除 harness 预签 debugserver / backend 源码 / 维护脚本 / backup)
SUBSET=(bin templates frameworks skills docs xcconfig vendor scripts)

say()  { printf '\033[1;36m==>\033[0m %s\n' "$*"; }
err()  { printf '\033[1;31mxtheos 安装失败:\033[0m %s\n' "$*" >&2; exit 1; }

# ── 1. 环境自检 ───────────────────────────────────────────────
[ "$(id -u)" != "0" ] || err "别用 root / sudo 跑。"
command -v git >/dev/null 2>&1 || err "缺 git(装 Xcode 命令行工具:xcode-select --install)。"
PY=""
for c in /usr/bin/python3 "$(command -v python3 2>/dev/null || true)"; do
    [ -n "$c" ] && [ -x "$c" ] || continue
    if "$c" -c 'import sys; sys.exit(0 if sys.version_info[:2] >= (3, 9) else 1)' 2>/dev/null; then PY="$c"; break; fi
done
[ -n "$PY" ] || err "需要 Python 3.9+(装 Xcode 14 及以上:xcode-select --install)。"
xcode-select -p >/dev/null 2>&1 || err "没装 Xcode 命令行工具:xcode-select --install"

# ── 2. clone(或已存在就走 update)────────────────────────────
if [ -d "$ROOT/.git" ]; then
    say "已安装于 $ROOT,改走更新…"
    exec "$ROOT/bin/xtheos" update
fi
[ ! -e "$ROOT" ] || err "$ROOT 已存在但不是 git 仓库,先挪走或删掉再装。"

say "clone 到 $ROOT(私有库,用你已配的 git 凭证)…"
if ! git clone --filter=blob:none --no-checkout "$REPO" "$ROOT" 2>/tmp/xtheos-clone.err; then
    cat /tmp/xtheos-clone.err >&2 || true
    err "clone 失败。多半是没有仓库权限:先 \`gh auth login\`(或配好 SSH key),并联系作者给你开通访问。"
fi
cd "$ROOT"
git sparse-checkout init --cone
git sparse-checkout set "${SUBSET[@]}"
LATEST=$(git tag --list 'v*' --sort=-v:refname | head -1 || true)
git checkout "${LATEST:-main}" >/dev/null 2>&1 || git checkout main
say "版本:${LATEST:-main}"

# ── 3. setup(建软链 + PATH/lldb 注入)────────────────────────
say "跑 xtheos setup…"
"$ROOT/bin/xtheos" setup || true

# ── 4. 旧 brew 安装 / 残留提示(不自动动用户的包管理器与配置)──
if command -v brew >/dev/null 2>&1 && brew list --versions xtheos >/dev/null 2>&1; then
    printf '\033[1;33m提示:\033[0m 检测到旧的 brew 版 xtheos,建议卸掉避免冲突:brew uninstall xtheos\n'
fi
if grep -qs 'HOMEBREW_GITHUB_API_TOKEN' "$HOME/.zshrc" 2>/dev/null; then
    printf '\033[1;33m提示:\033[0m ~/.zshrc 里旧的 HOMEBREW_GITHUB_API_TOKEN 已不需要,可手动删掉那行。\n'
fi

say "完成。新开终端即可用 \`xtheos\`;AI skills 另跑 \`xtheos skills install\`。"
