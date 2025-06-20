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
cd zdf-exam-desktop
mkdir build && cd build
cmake ..
cmake --build .
```

### 4. 退出说明
- 按 F10 弹出密码输入框，输入 123456 可安全退出

### 5. Windows 7 兼容性优化

本程序针对Windows 7低内存环境进行了特殊优化：

#### 自动检测功能
- 自动检测Windows 7系统版本
- 自动检测系统内存大小
- 在4GB及以下内存环境下自动启用超保守模式

#### 渐进式启动
- 在低内存环境下，程序首先显示加载界面
- 延迟3秒后再加载实际网页内容
- 避免用户看到长时间黑屏，改善用户体验

#### 内存优化措施
- 启用单进程模式，减少内存占用
- 禁用不必要的Chrome功能和扩展
- 设置更严格的内存限制（128MB堆空间）
- 定期监控内存使用并触发垃圾回收

#### 配置选项
可在 `config.json` 中自定义低内存模式设置：
```json
{
  "lowMemoryMode": {
    "enabled": "auto",              // auto/true/false
    "memoryThresholdMB": 4096,      // 内存检测阈值
    "progressiveLoading": true,     // 是否启用渐进式加载
    "progressiveLoadingDelay": 3000 // 延迟加载时间(毫秒)
  }
}
```

#### 日志记录
- 程序会在日志中记录系统内存信息
- 记录是否启用了超保守模式
- 记录内存监控和回收情况 

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
   cd zdf-exam-desktop
   .\build-windows.bat
   ```

4. 脚本会根据系统环境生成安装包：
   - `zdf-exam-desktop-setup.exe` (64位系统)
   - `zdf-exam-desktop-setup-x86.exe` (适用于32位系统)

## 安装包说明

生成的 Windows 安装包具有以下特性：

1. 支持 32 位和 64 位 Windows 系统
2. 自动安装所有必要的 Qt 依赖库
3. 创建开始菜单和桌面快捷方式
4. 提供标准的卸载功能
5. 安装路径：
   - `C:\Program Files\zdf-exam-desktop` (64位系统)
   - `C:\Program Files (x86)\zdf-exam-desktop` (32位系统) 

## 编译问题修复记录

### 2024年6月修复 - QWebEngineSettings和Logger问题

1. **缺少头文件修复**
   - 添加 QWebEngineSettings 头文件
   - 添加 QWebEnginePage 头文件
   - 添加 QWebEngineProfile 头文件

2. **Logger类修改**
   - Logger类现在继承自QObject
   - 添加Q_OBJECT宏以支持信号槽
   - 将flushAllLogBuffers函数移入timerFlushLogBuffers槽函数
   - 修复connect语法，使用恰当的父子关系
   
3. **QWebEngineSettings用法修改**
   - 从page()->settings()改为使用QWebEngineSettings::globalSettings()
   
4. **弃用API修复**
   - 将Qt::AA_MacPluginApplication替换为Qt::AA_PluginApplication
   
5. **构建系统改进**
   - 在CMake中添加Qt5::Core和Qt5::WebEngine库依赖 