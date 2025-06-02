# Git 设置检查清单

## 准备阶段
- [ ] 确认已安装 Git：`git --version`
- [ ] 确认已有 GitHub 账号
- [ ] 安装 GitHub CLI（可选）：`brew install gh`

## 本地初始化
- [ ] 创建 `.gitignore` 文件（已完成 ✓）
- [ ] 运行 `git init`
- [ ] 配置用户信息：
  ```bash
  git config user.name "您的名字"
  git config user.email "您的邮箱"
  ```
- [ ] 添加文件：`git add .`
- [ ] 首次提交：`git commit -m "Initial commit"`

## GitHub 设置
- [ ] 登录 GitHub.com
- [ ] 创建新仓库 `DesktopTerminal-2025`
- [ ] 选择 Private 或 Public
- [ ] **不要**初始化 README、.gitignore 或 LICENSE

## 连接和推送
- [ ] 添加远程仓库：
  ```bash
  git remote add origin https://github.com/YOUR_USERNAME/DesktopTerminal-2025.git
  ```
- [ ] 设置主分支：`git branch -M main`
- [ ] 首次推送：`git push -u origin main`

## 验证 GitHub Actions
- [ ] 访问仓库的 Actions 页面
- [ ] 确认看到 "Build Windows Installer" 工作流
- [ ] 创建测试标签：`git tag v0.0.1 && git push origin v0.0.1`
- [ ] 等待构建完成（约10-15分钟）
- [ ] 下载生成的安装包测试

## 后续维护
- [ ] 定期提交更改：`git add . && git commit -m "更新说明"`
- [ ] 推送更新：`git push`
- [ ] 发布新版本：`git tag vX.Y.Z && git push --tags`

## 故障排除

### 认证失败
使用 Personal Access Token 代替密码：
1. GitHub Settings → Developer settings
2. Personal access tokens → Tokens (classic)
3. Generate new token
4. 选择 `repo` 范围
5. 使用生成的 token 作为密码

### 推送被拒绝
```bash
# 如果远程有更新
git pull origin main --rebase
git push

# 如果是首次推送
git push -u origin main --force
```

### Actions 没有运行
1. 检查 `.github/workflows/build-windows.yml` 是否已提交
2. 检查仓库 Settings → Actions 是否已启用
3. 手动触发：Actions → Build Windows Installer → Run workflow 