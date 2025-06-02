#!/bin/bash

# Git 仓库初始化脚本
# 使用方法: bash scripts/init-git-repo.sh

echo "==================================="
echo "Git 仓库初始化向导"
echo "==================================="

# 检查是否已经是 Git 仓库
if [ -d .git ]; then
    echo "⚠️  警告：当前目录已经是一个 Git 仓库"
    read -p "是否继续？(y/n): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# 初始化 Git
echo "📁 初始化 Git 仓库..."
git init

# 配置 Git（如果需要）
if [ -z "$(git config user.name)" ]; then
    read -p "请输入您的姓名: " name
    git config user.name "$name"
fi

if [ -z "$(git config user.email)" ]; then
    read -p "请输入您的邮箱: " email
    git config user.email "$email"
fi

# 添加 .gitignore
echo "📝 检查 .gitignore 文件..."
if [ ! -f .gitignore ]; then
    echo "创建 .gitignore 文件..."
    # 这里应该已经有了
fi

# 添加所有文件
echo "📦 添加文件到暂存区..."
git add .

# 显示状态
echo "📊 当前状态:"
git status --short

# 第一次提交
echo ""
read -p "📝 请输入提交信息 (默认: Initial commit): " commit_msg
commit_msg=${commit_msg:-"Initial commit"}

git commit -m "$commit_msg"

echo ""
echo "✅ 本地 Git 仓库初始化完成！"
echo ""
echo "下一步："
echo "1. 在 GitHub 上创建新仓库（不要初始化 README）"
echo "2. 运行以下命令连接到 GitHub："
echo ""
echo "   git remote add origin https://github.com/YOUR_USERNAME/DesktopTerminal-2025.git"
echo "   git branch -M main"
echo "   git push -u origin main"
echo ""
echo "或者使用 GitHub CLI："
echo "   gh repo create DesktopTerminal-2025 --private --source=. --remote=origin --push"
echo "" 