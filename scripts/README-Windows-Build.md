# Windows 构建脚本使用说明

## 概述

这套Windows构建脚本提供了完整的自动化构建、部署和打包解决方案，让你能够轻松地将智多分机考桌面端编译成可分发的Windows安装包。

## 脚本列表

### 1. `check-deps.bat` - 环境依赖检查
**用途**: 检查构建环境是否满足要求
**运行时机**: 首次构建前，或遇到构建问题时

```bat
scripts\check-deps.bat
```

**检查项目**:
- ✅ Windows版本和架构
- ✅ Visual Studio 2019/2022安装
- ✅ CMake版本
- ✅ Qt 5.12+安装及WebEngine模块
- ✅ NSIS安装包生成工具
- ✅ Git版本控制
- ✅ 磁盘空间
- ✅ 网络连接

### 2. `build-windows.bat` - 一键构建主脚本
**用途**: 自动完成从源码到安装包的完整构建流程
**运行时机**: 需要完整构建时

```bat
build-windows.bat
```

**执行流程**:
1. 环境检查和配置
2. 清理旧构建文件
3. CMake配置和编译
4. Qt依赖部署
5. 资源文件复制
6. 部署验证
7. 生成NSIS安装包

### 3. `scripts\deploy-windows.bat` - 依赖部署脚本
**用途**: 将编译好的exe文件及其依赖部署到指定目录
**运行时机**: 只需要部署，不需要重新编译时

```bat
scripts\deploy-windows.bat build\zdf-exam-desktop.exe [deploy目录]
```

**功能**:
- 自动检测Qt安装路径
- 使用windeployqt部署Qt依赖
- 复制WebEngine特殊依赖
- 复制项目资源文件
- 创建运行时检查脚本
- 验证部署完整性

### 4. `scripts\package-windows.bat` - 安装包生成脚本
**用途**: 将已部署的文件打包成Windows安装包
**运行时机**: 部署完成后，需要生成安装包时

```bat
scripts\package-windows.bat
```

**功能**:
- 生成64位和32位安装包
- 自动计算文件校验和
- 创建发布说明文档
- 提供安全检查清单

### 5. `scripts\clean-build.bat` - 构建清理脚本
**用途**: 清理所有构建产物，重置项目到干净状态
**运行时机**: 遇到构建问题需要重新开始时，或发布前清理时

```bat
scripts\clean-build.bat
```

**清理内容**:
- build、deploy、Output目录
- CMake缓存文件
- Visual Studio项目文件
- 编译临时文件
- Qt元对象文件
- 日志文件（可选）

## 完整构建流程

### 新手推荐流程

1. **环境检查**
   ```bat
   scripts\check-deps.bat
   ```
   确保所有依赖都已正确安装

2. **一键构建**
   ```bat
   build-windows.bat
   ```
   自动完成所有构建步骤

3. **测试验证**
   ```bat
   deploy\zdf-exam-desktop.exe
   ```
   运行生成的程序进行测试

### 高级用户流程

1. **环境检查**
   ```bat
   scripts\check-deps.bat
   ```

2. **清理环境**（如果需要）
   ```bat
   scripts\clean-build.bat
   ```

3. **编译程序**
   ```bat
   mkdir build && cd build
   cmake ..\zdf-exam-desktop -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
   nmake
   cd ..
   ```

4. **部署依赖**
   ```bat
   scripts\deploy-windows.bat build\zdf-exam-desktop.exe
   ```

5. **生成安装包**
   ```bat
   scripts\package-windows.bat
   ```

## 输出文件说明

### 构建产物

```
项目根目录/
├── build/                    # 编译产物
│   └── zdf-exam-desktop.exe  # 编译生成的可执行文件
├── deploy/                   # 部署产物
│   ├── zdf-exam-desktop.exe  # 主程序
│   ├── Qt5*.dll              # Qt依赖库
│   ├── platforms/            # Qt平台插件
│   ├── resources/            # WebEngine资源
│   ├── config.json           # 配置文件
│   ├── check-runtime.bat     # 运行时检查脚本
│   └── run.bat              # 启动脚本
└── Output/                   # 安装包产物
    ├── 智多分-机考桌面端-setup.exe      # 64位安装包
    ├── 智多分-机考桌面端-setup-x86.exe  # 32位安装包
    ├── checksums.txt         # 校验和文件
    └── README.txt            # 发布说明
```

### 文件用途

- **deploy目录**: 可直接在Windows机器上运行的完整程序包
- **Output目录**: 用于分发的安装包，包含自动安装程序
- **checksums.txt**: 文件完整性验证
- **README.txt**: 给最终用户的安装说明

## 常见问题解决

### 1. "未找到Visual Studio环境"
**解决方案**:
- 安装Visual Studio 2019或2022社区版
- 确保选择"使用C++的桌面开发"工作负载
- 重启命令提示符

### 2. "未找到Qt安装"
**解决方案**:
- 安装Qt 5.12或更高版本
- 确保选择MSVC 2019编译器版本
- 必须包含Qt WebEngine模块
- 设置QT_DIR环境变量

### 3. "CMake配置失败"
**解决方案**:
- 检查CMake版本是否为3.14+
- 确保Qt路径正确设置
- 删除build目录重新尝试

### 4. "windeployqt部署失败"
**解决方案**:
- 确保Qt安装完整
- 检查PATH环境变量包含Qt\bin目录
- 使用完整的Qt安装而非在线安装器的部分安装

### 5. "生成的程序无法运行"
**解决方案**:
- 运行deploy\check-runtime.bat检查运行时环境
- 安装Visual C++ Redistributable 2015-2019
- 确保目标机器为64位Windows（如果使用64位版本）

### 6. "NSIS生成安装包失败"
**解决方案**:
- 安装NSIS: `choco install nsis` 或从官网下载
- 确保deploy目录存在且包含所有必要文件
- 检查resources目录中的图标文件

## 高级配置

### 自定义构建选项

编辑CMakeLists.txt可以修改以下选项:
- 编译器优化级别
- 静态链接选项
- 调试信息包含

### 自定义安装包

编辑installer.nsi可以修改:
- 应用程序名称和版本
- 安装目录结构
- 快捷方式创建
- 注册表项
- 卸载程序行为

### 自动化集成

这些脚本可以集成到:
- CI/CD流水线
- 定时构建任务
- 版本发布流程
- 自动测试系统

## 版本管理建议

1. **版本号管理**: 
   - 更新installer.nsi中的版本常量
   - 更新CMakeLists.txt中的项目版本
   - 创建Git标签标记发布版本

2. **发布清单**:
   ```bat
   # 更新版本号
   # 运行完整测试
   scripts\clean-build.bat
   scripts\check-deps.bat
   build-windows.bat
   # 测试生成的安装包
   # 创建Git标签
   git tag v1.0.0
   git push --tags
   ```

3. **文档更新**:
   - 更新README.md
   - 更新配置文件示例
   - 更新用户使用手册

## 技术支持

如果遇到其他问题:
1. 运行check-deps.bat查看详细环境信息
2. 检查构建过程中的错误消息
3. 查阅docs/windows-packaging-guide.md
4. 在项目仓库提交Issue并附上详细的错误信息

---
*最后更新: 2025-06-24*