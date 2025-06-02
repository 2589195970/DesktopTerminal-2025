# QT 套壳浏览器

本目录为 QT 实现的套壳浏览器，具体功能和配置请参考顶级 README。

## 环境依赖与安装说明

### 1. 必须安装的环境
- Qt5（推荐 5.12 及以上）或 Qt6
- QtWebEngine 模块（用于嵌入网页）
- CMake（用于跨平台构建）
- C++17 编译器（如 g++、clang、MSVC）

### 2. 各平台安装方法

#### macOS
```bash
# 安装 Homebrew（如未安装）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装 Qt5 和 CMake
brew install qt@5 cmake

# 设置 Qt 环境变量（可加入 ~/.zshrc 或 ~/.bash_profile）
export PATH="/usr/local/opt/qt@5/bin:$PATH"
export CMAKE_PREFIX_PATH="/usr/local/opt/qt@5/lib/cmake:$CMAKE_PREFIX_PATH"
```

#### Ubuntu/Linux/麒麟
```bash
sudo apt update
sudo apt install qtbase5-dev qtwebengine5-dev cmake build-essential
```

#### Windows
- 推荐使用 [Qt 官方安装器](https://download.qt.io/official_releases/online_installers/)
- 选择 Qt5/Qt6 + QtWebEngine + MinGW 或 MSVC 工具链
- 安装 CMake（[下载地址](https://cmake.org/download/)）
- 安装 NSIS（[下载地址](https://nsis.sourceforge.io/Download)）用于创建安装程序

### 3. 构建与运行
```bash
cd qt-shell
mkdir build && cd build
cmake ..
cmake --build .
```

### 4. 退出说明
- 按 F10 弹出密码输入框，输入 123456 可安全退出 

## Windows 打包说明

### 打包步骤

1. 确保已安装以下软件：
   - Qt 5.12 或更高版本
   - CMake 3.14 或更高版本
   - Visual Studio 2019 或 MinGW
   - [NSIS](https://nsis.sourceforge.io/Download) (Nullsoft Scriptable Install System)

2. 设置环境变量：
   - 设置 QTDIR 环境变量指向您的 Qt 安装目录
   - 示例: `set QTDIR=C:\Qt\5.15.2\msvc2019_64`

3. 打开命令提示符或 PowerShell，进入项目目录，运行打包脚本：
   ```
   cd qt-shell
   .\build-windows.bat
   ```

4. 脚本会自动构建 32 位和 64 位版本（如果您的 Qt 安装包含两种架构），并生成对应的安装包：
   - `机考霸屏桌面端-1.0.0-x64-setup.exe` (64位)
   - `机考霸屏桌面端-1.0.0-x86-setup.exe` (32位)

## 安装包说明

生成的 Windows 安装包具有以下特性：

1. 支持 32 位和 64 位 Windows 系统
2. 自动安装所有必要的 Qt 依赖库
3. 创建开始菜单和桌面快捷方式
4. 提供标准的卸载功能
5. 安装路径：
   - 64位：`C:\Program Files\机考霸屏桌面端`
   - 32位：`C:\Program Files (x86)\机考霸屏桌面端` 