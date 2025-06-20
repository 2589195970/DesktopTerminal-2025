# Windows 7兼容性启动脚本使用说明

## 文件说明

### `run-windows7-compat.bat`
Windows 7系统专用启动脚本，用于手动测试WebEngine黑屏修复方案。

## 使用方法

### 1. 下载脚本到Windows 7系统
将 `run-windows7-compat.bat` 文件复制到Windows 7目标机器上的任意位置。

### 2. 确认程序安装路径
脚本默认程序路径为：
```
C:\Program Files (x86)\智多分\智多分-机考桌面端\zdf-exam-desktop.exe
```

如果程序安装在其他位置，请编辑脚本修改 `PROGRAM_PATH` 变量。

### 3. 运行脚本
- 双击 `run-windows7-compat.bat` 文件
- 或在命令提示符中运行该脚本

### 4. 观察输出
脚本会显示：
- 设置的环境变量
- 程序查找结果
- 启动状态

## 环境变量说明

脚本设置的兼容性环境变量：

| 变量名 | 值 | 作用 |
|--------|-----|------|
| `QTWEBENGINE_DISABLE_GPU` | 1 | 禁用GPU加速 |
| `QTWEBENGINE_DISABLE_SANDBOX` | 1 | 禁用沙箱模式 |
| `QT_OPENGL` | software | 强制软件OpenGL |
| `QSG_RHI_PREFER_SOFTWARE_RENDERER` | 1 | 首选软件渲染器 |
| `QTWEBENGINE_CHROMIUM_FLAGS` | 多个标志 | Chromium兼容性参数 |

## 故障排除

### 问题1：找不到程序文件
**症状**：脚本显示"错误：未找到程序文件！"
**解决**：
1. 确认程序已正确安装
2. 检查安装路径是否正确
3. 修改脚本中的 `PROGRAM_PATH` 变量

### 问题2：程序仍然黑屏
**可能原因**：
1. 图形驱动程序问题
2. DirectX版本过旧
3. 内存不足

**解决步骤**：
1. 更新图形驱动程序
2. 安装最新的DirectX
3. 关闭其他程序释放内存
4. 重启系统后再试

### 问题3：程序崩溃
**检查步骤**：
1. 查看Windows事件日志
2. 检查 `log/` 目录下的日志文件
3. 确认VC++ 2017运行时已安装

## 日志文件位置

程序运行后会在以下位置生成日志：
```
[程序安装目录]\log\
├── app.log        # 应用程序日志
├── startup.log    # 启动日志  
├── config.log     # 配置加载日志
└── exit.log       # 退出尝试日志
```

关注日志中的关键信息：
- "检测到Windows 7系统，启用兼容模式"
- "应用Windows 7 WebEngine黑屏修复补丁"
- 任何错误或警告信息

## 性能说明

使用软件渲染模式可能会导致：
- 启动时间稍长（5-10秒）
- 网页滚动略显卡顿
- CPU使用率相对较高

这是为了确保在Windows 7系统上正常运行的必要妥协。