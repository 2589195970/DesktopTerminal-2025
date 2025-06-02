# GitHub Actions 使用指南

## 快速开始

### 1. 首次设置
```bash
# 确保文件已提交
git add .github/workflows/build-windows.yml
git add installer.nsi
git add build-windows.bat
git commit -m "Add Windows build configuration"
git push
```

### 2. 触发构建

#### 方式一：推送版本标签（推荐）
```bash
# 创建并推送标签
git tag v1.0.0
git push origin v1.0.0

# 或一次性推送
git tag v1.0.1 -m "Bug fixes"
git push --tags
```

#### 方式二：手动触发
1. 打开 GitHub 仓库页面
2. 点击 `Actions` 标签
3. 选择 `Build Windows Installer`
4. 点击 `Run workflow` 按钮

### 3. 下载构建产物
1. 进入 `Actions` 页面
2. 点击最新的构建记录
3. 在页面底部找到 `Artifacts`
4. 下载 `windows-installer` 

## 监控构建状态

### 实时查看日志
- 构建开始后，点击进入可查看实时日志
- 每个步骤都可以展开查看详细输出

### 构建通知
在仓库设置中可以配置：
- 邮件通知
- Slack/Discord 集成
- 自定义 Webhook

## 常见问题

### Q: 构建失败怎么办？
A: 查看失败步骤的日志，常见原因：
- 代码编译错误
- 依赖下载失败（重试即可）
- 配置文件路径错误

### Q: 如何加快构建速度？
A: 可以使用缓存：
```yaml
- name: Cache Qt
  uses: actions/cache@v3
  with:
    path: ${{ runner.workspace }}/Qt
    key: ${{ runner.os }}-qt-${{ matrix.qt_version }}
```

### Q: 如何添加代码签名？
A: 使用 GitHub Secrets 存储证书：
```yaml
- name: Sign executable
  env:
    CERTIFICATE: ${{ secrets.WINDOWS_CERTIFICATE }}
    PASSWORD: ${{ secrets.CERTIFICATE_PASSWORD }}
  run: |
    # 签名脚本
```

## 进阶配置

### 自动发布到 GitHub Releases
```yaml
- name: Create Release
  uses: softprops/action-gh-release@v1
  if: startsWith(github.ref, 'refs/tags/')
  with:
    files: Output/qt-shell-setup.exe
    draft: false
    prerelease: false
```

### 矩阵构建（多配置）
```yaml
strategy:
  matrix:
    include:
      - arch: x64
        qt_arch: win64_msvc2019_64
      - arch: x86
        qt_arch: win32_msvc2019
```

### 构建状态徽章
在 README.md 中添加：
```markdown
[![Build Status](https://github.com/你的用户名/DesktopTerminal-2025/workflows/Build%20Windows%20Installer/badge.svg)](https://github.com/你的用户名/DesktopTerminal-2025/actions)
``` 