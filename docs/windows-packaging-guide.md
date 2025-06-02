# Windows 打包指南

## 一、技术可行性分析

### 项目技术栈
- **Qt 5** + **Qt WebEngine**（基于Chromium）
- **CMake** 构建系统
- **C++** 开发语言
- 跨平台设计，无Windows特定代码

### 打包方案选择
1. **NSIS**（推荐）：开源、功能强大、文件小
2. **Inno Setup**：简单易用，适合中小型项目
3. **WiX Toolset**：生成标准MSI包，适合企业部署
4. **Qt Installer Framework**：Qt官方方案，支持在线更新

## 二、在macOS上打包Windows exe的问题

### 主要障碍
1. **无法交叉编译**：Qt应用包含平台特定的二进制代码
2. **缺少Windows SDK**：需要Windows特定的头文件和库
3. **工具链限制**：windeployqt等工具只能在Windows运行

### 解决方案

#### 方案1：GitHub Actions（推荐）
- 免费、自动化、无需本地Windows环境
- 已创建 `.github/workflows/build-windows.yml`
- 推送tag触发自动构建：`git tag v1.0.0 && git push --tags`

#### 方案2：虚拟机
```bash
# 1. 安装 Parallels Desktop 或 VMware Fusion
# 2. 安装 Windows 10/11
# 3. 在虚拟机中安装开发环境
# 4. 共享文件夹进行开发
```

#### 方案3：远程Windows机器
- AWS EC2 Windows实例
- Azure Windows虚拟机
- 物理Windows服务器

## 三、Windows下直接打包的问题和解决方案

### 常见问题

#### 1. Qt WebEngine 依赖问题
**问题**：Qt WebEngine需要大量运行时文件（约150MB）
**解决**：
```bat
:: 确保复制所有WebEngine资源
xcopy /E /I /Y "%QT_DIR%\resources" "deploy\resources"
xcopy /E /I /Y "%QT_DIR%\translations\qtwebengine_locales" "deploy\translations\qtwebengine_locales"
```

#### 2. Visual C++ 运行时缺失
**问题**：目标机器可能没有安装VC++运行时
**解决**：
- 在安装包中包含 VC++ Redistributable
- 或使用静态链接（会增大文件体积）

#### 3. 缺少系统DLL
**问题**：某些Windows系统DLL可能缺失
**解决**：
```bat
:: 使用 Dependency Walker 检查依赖
:: 或使用 windeployqt 的详细模式
windeployqt --list mapping deploy\qt-shell.exe
```

#### 4. 权限问题
**问题**：应用需要管理员权限
**解决**：
- 在NSIS脚本中设置：`RequestExecutionLevel admin`
- 或创建应用程序清单文件

#### 5. 防病毒软件误报
**问题**：新编译的exe可能被误报为病毒
**解决**：
- 代码签名（需要购买证书）
- 提交到主流杀软厂商白名单
- 使用知名打包工具

### 环境准备清单

#### 必需软件
1. **Visual Studio 2019/2022**
   - 勾选"使用C++的桌面开发"
   - 包含Windows SDK

2. **Qt 5.15.2**
   - 选择MSVC 2019 64-bit
   - 包含Qt WebEngine模块

3. **CMake**（3.14+）
   - 添加到系统PATH

4. **NSIS**（可选，用于创建安装包）

#### 环境变量设置
```bat
:: 设置Qt目录
set QT_DIR=C:\Qt\5.15.2\msvc2019_64

:: 添加到PATH（可选）
set PATH=%QT_DIR%\bin;%PATH%
```

### 构建步骤

#### 使用提供的脚本
```bat
:: 直接运行构建脚本
build-windows.bat
```

#### 手动构建
```bat
:: 1. 创建构建目录
mkdir build && cd build

:: 2. 配置CMake
cmake ..\qt-shell -G "NMake Makefiles" -DCMAKE_PREFIX_PATH="%QT_DIR%"

:: 3. 编译
nmake

:: 4. 部署依赖
cd ..
mkdir deploy
copy build\qt-shell.exe deploy\
windeployqt --dir deploy --release deploy\qt-shell.exe

:: 5. 创建安装包
makensis installer.nsi
```

### 部署检查清单

#### 必需文件
- [x] qt-shell.exe（主程序）
- [x] Qt5Core.dll, Qt5Gui.dll, Qt5Widgets.dll
- [x] Qt5WebEngine.dll, Qt5WebEngineCore.dll
- [x] Qt5WebEngineWidgets.dll
- [x] platforms/qwindows.dll
- [x] imageformats/*.dll
- [x] QtWebEngineProcess.exe
- [x] resources/（WebEngine资源）
- [x] translations/qtwebengine_locales/
- [x] resources/config.json（配置文件）

#### 测试要点
1. 在没有Qt的干净Windows系统测试
2. 测试不同Windows版本（7/10/11）
3. 测试32位和64位系统
4. 检查防病毒软件兼容性
5. 验证全屏和热键功能

### 优化建议

#### 减小安装包体积
1. 使用UPX压缩exe和dll
2. 移除不需要的Qt模块
3. 使用7-zip压缩算法（NSIS支持）

#### 提升启动速度
1. 延迟加载Qt WebEngine
2. 使用预编译头文件
3. 优化资源加载

#### 增强安全性
1. 代码签名
2. 启用DEP和ASLR
3. 使用安全的编译选项

## 四、故障排除

### 问题：找不到Qt5Core.dll
```bat
:: 检查部署
windeployqt --list missing deploy\qt-shell.exe

:: 手动复制
copy "%QT_DIR%\bin\Qt5Core.dll" deploy\
```

### 问题：WebEngine页面空白
```bat
:: 确保复制了所有WebEngine资源
xcopy /E /I /Y "%QT_DIR%\resources" "deploy\resources"

:: 检查进程
:: 应该能看到 QtWebEngineProcess.exe
```

### 问题：中文显示乱码
- 确保源代码使用UTF-8编码
- 检查配置文件编码
- 使用 `QTextCodec::setCodecForLocale`

### 问题：无法创建全屏窗口
- 检查显卡驱动
- 尝试兼容模式运行
- 检查多显示器设置

## 五、发布清单

- [ ] 版本号更新（CMakeLists.txt, installer.nsi）
- [ ] 更新README和文档
- [ ] 运行完整测试
- [ ] 创建Git标签
- [ ] 构建Release版本
- [ ] 病毒扫描
- [ ] 上传到发布平台
- [ ] 更新下载链接 