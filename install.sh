#!/bin/bash
# xtheos 引导安装脚本 —— 首次部署用。之后更新/卸载走 `xtheos update` / `xtheos uninstall`。
#   在线方式: bash -c "$(curl -fsSL https://repo.iospkg.cn/install.sh)"
#   离线/自满拷贝方式: cd <xtheos目录> && ./install.sh
#
# 做的事:
#   1. 环境自检 (Python 3.9+, git, xcode-select)
#   2. 自满模式判定:
#      - 若当前目录已包含完整 xtheos 源码/产物，直接原地就地安装，零联网零 clone
#      - 若远程管道运行，克隆到 ~/xtheos
#      - 若已存在完整安装，自动转接执行更新
#   3. 初始化软链、PATH、AI skills 与本地开发分支
#   4. 事务保障: 任一步骤失败，自动捕获并彻底回滚清理半成品脏环境
set -euo pipefail

REPO="https://github.com/Cuihaixu/xtheos.git"
ROOT="${XTHEOS:-$HOME/xtheos}"
SUBSET=(bin templates frameworks skills docs xcconfig vendor scripts)

say()  { printf '\033[1;36m==>\033[0m %s\n' "$*"; }
err()  { printf '\033[1;31mxtheos 安装失败:\033[0m %s\n' "$*" >&2; exit 1; }

# ── 0. 模式探测 (自满离线就地部署 vs 在线一键拉取) ───────────────
SCRIPT_DIR=""
if [ -n "${BASH_SOURCE[0]:-}" ] && [ -f "${BASH_SOURCE[0]}" ]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd -P)"
fi

LOCAL_SELF_CONTAINED=0
if [ -n "$SCRIPT_DIR" ] && [ -f "$SCRIPT_DIR/bin/xtheos.py" ] && [ -d "$SCRIPT_DIR/templates" ]; then
    LOCAL_SELF_CONTAINED=1
    # 自满模式：优先直接以当前离线目录为根（或使用显式指定的 XTHEOS）
    ROOT="${XTHEOS:-$SCRIPT_DIR}"
fi

JUST_CLONED=0
INSTALL_SUCCESS=0

cleanup_on_failure() {
    local code=$?
    if [ "$INSTALL_SUCCESS" -eq 1 ] || [ "$code" -eq 0 ]; then
        return
    fi
    printf '\n\033[1;31m==> 安装未完成 (退出码: %s)，正在自动清理环境现场...\033[0m\n' "$code" >&2
    if [ "$LOCAL_SELF_CONTAINED" -eq 0 ] && [ "$JUST_CLONED" -eq 1 ] && [ -d "$ROOT" ]; then
        rm -rf "$ROOT"
        printf '  已清理未完成的克隆目录: %s\n' "$ROOT" >&2
    fi
    # 拆除半成品软链（仅当软链确指由本次安装产生时）
    if [ -L "$HOME/.local/bin/xtheos" ] && [ "$(readlink "$HOME/.local/bin/xtheos" 2>/dev/null || true)" = "$ROOT/bin/xtheos" ]; then
        rm -f "$HOME/.local/bin/xtheos"
        printf '  已清理半成品命令行软链: ~/.local/bin/xtheos\n' >&2
    fi
    if [ -L "$HOME/Library/Developer/Xcode/Templates/xtheos" ]; then
        local t_link
        t_link="$(readlink "$HOME/Library/Developer/Xcode/Templates/xtheos" 2>/dev/null || true)"
        if [ "$t_link" = "$ROOT/templates" ]; then
            rm -f "$HOME/Library/Developer/Xcode/Templates/xtheos"
            printf '  已清理半成品 Xcode 模板软链\n' >&2
        fi
    fi
    printf '\033[1;31mxtheos 安装已安全回滚，未在系统中留下残余脏数据。\033[0m\n' >&2
}
trap cleanup_on_failure EXIT

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

# ── 2. 准备 SDK 目录 ─────────────────────────────────────────
if [ "$LOCAL_SELF_CONTAINED" -eq 1 ]; then
    say "检测到自满源码/拷贝目录，直接原地部署: $ROOT"
else
    if [ -d "$ROOT/.git" ]; then
        say "已安装于 $ROOT，改走更新…"
        exec "$ROOT/bin/xtheos" update
    fi
    [ ! -e "$ROOT" ] || err "$ROOT 已存在但不是 git 仓库，先挪走或删掉再装。"

    say "clone 到 $ROOT (使用本机 git 凭证)…"
    if ! git clone --filter=blob:none --no-checkout "$REPO" "$ROOT" 2>/tmp/xtheos-clone.err; then
        cat /tmp/xtheos-clone.err >&2 || true
        err "clone 失败。若无公开网络或仓库权限，可直接拷贝完整 xtheos 目录后进入目录执行 ./install.sh 离线安装。"
    fi
    JUST_CLONED=1
    cd "$ROOT"
    git sparse-checkout init --cone
    git sparse-checkout set "${SUBSET[@]}"
    LATEST=$(git tag --list 'v*' --sort=-v:refname | head -1 || true)
    git checkout "${LATEST:-main}" >/dev/null 2>&1 || git checkout main
    say "版本:${LATEST:-main}"
fi

# ── 3. 初始化部署与自检 ───────────────────────────────────────
say "执行初始化部署 (建立命令/模板软链、开发源与环境自检)…"
if [ "$ROOT" != "$HOME/xtheos" ]; then
    "$ROOT/bin/xtheos" setup --dev
else
    "$ROOT/bin/xtheos" setup
fi

say "装 AI skills…"
"$ROOT/bin/xtheos" skills install || true

# ── 4. 旧 brew 安装 / 残留提示 ────────────────────────────────
if [ -d "/opt/homebrew/Cellar/xtheos" ] || [ -d "/usr/local/Cellar/xtheos" ]; then
    printf '\033[1;33m提示:\033[0m 检测到旧的 brew 版 xtheos,建议卸掉避免冲突:brew uninstall xtheos\n'
fi
if grep -qs 'HOMEBREW_GITHUB_API_TOKEN' "$HOME/.zshrc" 2>/dev/null; then
    printf '\033[1;33m提示:\033[0m ~/.zshrc 里旧的 HOMEBREW_GITHUB_API_TOKEN 已不需要,可手动删掉那行。\n'
fi

INSTALL_SUCCESS=1

case ":$PATH:" in
    *":$HOME/.local/bin:"*) say "完成。\`xtheos\` 命令当前终端即可用。" ;;
    *) say "完成。当前终端先 \`exec zsh\`(或新开终端)让 \`xtheos\` 命令生效。" ;;
esac
