# 从macOS触发Windows构建指南

## 概述

由于你在macOS环境下开发，无法直接运行Windows构建脚本，本项目提供了通过GitHub Actions进行Windows自动构建的完整解决方案。

## 前置要求

1. **GitHub CLI工具**
   ```bash
   # 使用Homebrew安装
   brew install gh
   
   # 或下载安装
   # https://cli.github.com/
   ```

2. **GitHub认证**
   ```bash
   # 登录GitHub
   gh auth login
   
   # 验证认证状态
   gh auth status
   ```

3. **项目权限**
   - 对仓库有推送权限
   - 能够触发GitHub Actions

## 快速构建方法

### 方法1: 一键构建（推荐）

```bash
# 在项目根目录执行
./build-windows-from-macos.sh
```

这个脚本会：
- 自动检查和安装GitHub CLI
- 触发Release版本的Windows构建
- 生成完整的安装包
- 提供监控链接

### 方法2: 高级构建选项

```bash
# 使用高级脚本
./scripts/trigger-windows-build.sh

# 常用选项示例：
./scripts/trigger-windows-build.sh -t Release    # Release构建
./scripts/trigger-windows-build.sh -t Debug     # Debug构建
./scripts/trigger-windows-build.sh -r v1.0.0    # 创建发布版本
```

### 方法3: 直接使用GitHub CLI

```bash
# 手动触发工作流
gh workflow run "build-windows.yml" \
    --field build_type="Release" \
    --field create_installer="true" \
    --field upload_artifacts="true"
```

### 方法4: 通过Git标签触发

```bash
# 创建并推送版本标签
git tag v1.0.0
git push origin v1.0.0

# 这会自动触发构建并创建GitHub Release
```

## 构建流程说明

### GitHub Actions工作流程

1. **环境准备**
   - Windows Server最新版
   - Qt 5.15.2 + WebEngine
   - Visual Studio Build Tools
   - NSIS安装包生成器

2. **构建步骤**
   - 检出代码和子模块
   - 配置Qt和MSVC环境
   - CMake配置和编译
   - Qt依赖部署（windeployqt）
   - 复制项目资源文件
   - 生成NSIS安装包

3. **输出产物**
   - 便携版程序包（deploy目录）
   - Windows安装包（.exe）
   - 自动版本命名
   - SHA256校验和

### 支持的架构

- **主要支持**: Windows x64 (64位)
- **可扩展**: Windows x86 (32位) - 可在matrix中启用

## 监控构建进度

### 1. 网页监控

```bash
# 打开Actions页面
gh repo view --web
# 然后点击Actions标签
```

### 2. 命令行监控

```bash
# 查看最近的构建
gh run list --workflow=build-windows.yml

# 查看特定构建的详情
gh run view [RUN_ID]

# 实时监控最新构建
gh run watch
```

### 3. 构建状态

构建状态包括：
- ✅ **success**: 构建成功，产物可下载
- ❌ **failure**: 构建失败，需要查看日志
- 🟡 **in_progress**: 正在构建中
- ⏳ **queued**: 排队等待中

## 下载构建产物

### 1. 从GitHub Actions下载

```bash
# 列出最新构建的产物
gh run view --json artifacts

# 下载特定产物
gh run download [RUN_ID] --name [ARTIFACT_NAME]

# 下载所有产物
gh run download [RUN_ID]
```

### 2. 从GitHub Releases下载

如果通过标签触发构建，产物会自动发布到Releases页面：

```bash
# 查看所有releases
gh release list

# 下载最新release
gh release download

# 下载特定版本
gh release download v1.0.0
```

## 构建产物说明

### 文件类型

1. **安装包文件**
   - `zdf-exam-desktop-v1.0.0-x64-setup.exe` - Windows安装包
   - 包含自动安装程序
   - 自动创建桌面快捷方式
   - 检查运行时依赖

2. **便携版文件**
   - `zdf-exam-desktop-v1.0.0-x64-portable.zip` - 便携版
   - 解压即可使用
   - 包含所有依赖文件
   - 适合无安装权限的环境

### 文件结构

```
便携版解压后:
deploy/
├── zdf-exam-desktop.exe     # 主程序
├── Qt5*.dll                 # Qt运行库
├── platforms/               # Qt平台插件
├── imageformats/            # 图像格式支持
├── QtWebEngineProcess.exe   # WebEngine进程
├── resources/               # WebEngine资源
├── config.json              # 配置文件
├── check-runtime.bat        # 运行时检查
└── run.bat                  # 启动脚本
```

## 故障排除

### 1. 构建失败

```bash
# 查看详细错误日志
gh run view [RUN_ID] --log

# 查看特定步骤日志
gh run view [RUN_ID] --log --job=[JOB_NAME]
```

常见问题：
- **Qt依赖问题**: 检查Qt版本和模块配置
- **编译错误**: 检查源代码语法错误
- **资源文件缺失**: 确保resources目录包含必要文件

### 2. 认证问题

```bash
# 重新登录
gh auth logout
gh auth login

# 检查权限
gh auth status
```

### 3. 网络问题

```bash
# 检查仓库连接
gh repo view

# 测试API连接
gh api user
```

## 自动化建议

### 1. 发布流程自动化

```bash
# 创建发布脚本 (scripts/release.sh)
#!/bin/bash
VERSION=$1
git tag v$VERSION
git push origin v$VERSION
echo "Release v$VERSION 已触发构建"
```

### 2. 定期构建

可以在GitHub Actions中设置定时触发：

```yaml
on:
  schedule:
    - cron: '0 2 * * 1'  # 每周一凌晨2点构建
```

### 3. PR构建验证

当前配置已支持PR触发构建，确保代码质量。

## 高级配置

### 1. 多架构构建

编辑 `.github/workflows/build-windows.yml`:

```yaml
strategy:
  matrix:
    arch: [x64, x86]  # 启用32位构建
```

### 2. 自定义构建选项

修改CMake配置：

```yaml
- name: Configure CMake
  run: |
    cmake ../zdf-exam-desktop \
      -DCMAKE_BUILD_TYPE=${{ matrix.config }} \
      -DCUSTOM_OPTION=ON
```

### 3. 代码签名

添加代码签名步骤（需要证书）：

```yaml
- name: Sign executable
  run: |
    signtool sign /f certificate.p12 /p ${{ secrets.CERT_PASSWORD }} deploy/zdf-exam-desktop.exe
```

## 总结

通过这套解决方案，你可以在macOS环境下轻松触发Windows构建：

1. **简单场景**: 使用 `./build-windows-from-macos.sh`
2. **发布场景**: 创建Git标签自动构建和发布
3. **测试场景**: 推送代码到分支自动验证构建
4. **自定义场景**: 使用高级脚本指定构建参数

所有构建都在云端完成，产物自动生成，无需本地Windows环境。

---
*配合使用GitHub Actions，实现真正的跨平台开发体验*