#!/bin/bash
# 将 ICO 文件转换为 PNG 格式供 Linux 使用

# 检查是否安装了 ImageMagick
if ! command -v convert &> /dev/null; then
    echo "ImageMagick 未安装，尝试安装..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get update && sudo apt-get install -y imagemagick
    else
        echo "请手动安装 ImageMagick"
        exit 1
    fi
fi

# 转换图标
convert resources/simple_icon.ico -resize 256x256 resources/qt-shell.png
echo "图标已转换为 PNG 格式: resources/qt-shell.png" 