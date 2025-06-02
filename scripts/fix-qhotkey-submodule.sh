#!/bin/bash

echo "==================================="
echo "修复 QHotkey 嵌入式仓库问题"
echo "==================================="
echo ""
echo "请选择处理方式："
echo "1) 移除 Git 历史，作为普通代码包含（推荐）"
echo "2) 添加为 Git Submodule（需要网络访问 GitHub）"
echo "3) 暂时忽略（从暂存区移除）"
echo ""
read -p "请输入选择 (1-3): " choice

case $choice in
    1)
        echo "正在移除 QHotkey 的 Git 历史..."
        # 从暂存区移除
        git rm --cached qt-shell/QHotkey 2>/dev/null || true
        
        # 删除 .git 目录
        if [ -d "qt-shell/QHotkey/.git" ]; then
            rm -rf qt-shell/QHotkey/.git
            echo "✓ 已删除 QHotkey/.git 目录"
        fi
        
        # 重新添加为普通文件
        git add qt-shell/QHotkey
        echo "✓ QHotkey 已作为普通代码添加"
        echo ""
        echo "现在可以正常提交了："
        echo "  git commit -m \"Add QHotkey library source\""
        ;;
        
    2)
        echo "正在添加 QHotkey 为 Git Submodule..."
        # 从暂存区移除
        git rm --cached qt-shell/QHotkey 2>/dev/null || true
        
        # 添加为 submodule
        git submodule add https://github.com/Skycoder42/QHotkey.git qt-shell/QHotkey
        
        echo "✓ QHotkey 已添加为 submodule"
        echo ""
        echo "现在可以提交了："
        echo "  git add .gitmodules qt-shell/QHotkey"
        echo "  git commit -m \"Add QHotkey as submodule\""
        ;;
        
    3)
        echo "从暂存区移除 QHotkey..."
        git rm --cached qt-shell/QHotkey
        echo "✓ 已从暂存区移除"
        echo ""
        echo "QHotkey 现在被 Git 忽略，但文件仍然存在"
        echo "您可以稍后决定如何处理"
        ;;
        
    *)
        echo "无效的选择"
        exit 1
        ;;
esac

echo ""
echo "完成！" 