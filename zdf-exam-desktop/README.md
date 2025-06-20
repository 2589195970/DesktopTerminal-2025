# 智多分机考桌面端

## 项目概述
本项目为智多分机考系统的桌面端应用，基于Qt WebEngine开发，提供全屏考试环境。

## 主要功能
- 全屏Web考试界面，防止考生退出
- F10快捷键密码退出机制
- 禁用常见系统快捷键（Alt+Tab、Ctrl+Alt+Del等）
- 系统资源监控和日志记录
- 操作记录（每次F10尝试都记录）
- **Windows 7兼容性优化**（v1.2.1+）
- **老旧CPU性能优化**（v1.2.1+）

## 系统要求

### 最低配置
- **操作系统**: Windows 7 SP1 (x86/x64)、Windows 8.1、Windows 10/11
- **CPU**: Intel Core 2代及以上（Sandy Bridge+）、AMD同等级别
- **内存**: 2GB RAM（推荐4GB+）
- **网络**: 稳定的互联网连接

### 推荐配置
- **操作系统**: Windows 10/11 (x64)
- **CPU**: Intel Core 6代及以上（Skylake+）
- **内存**: 4GB+ RAM
- **网络**: 宽带连接

### 特殊兼容性说明
- **Windows 7 + 老旧CPU**: 自动检测CPU架构并启用兼容模式
  - 支持Intel 2-4代CPU（Sandy Bridge、Ivy Bridge、Haswell）
  - 自动禁用硬件加速和WebGL以提升兼容性
  - 低内存环境自动优化（≤4GB内存）
- **1核2线程环境优化**（v1.2.2+）: 针对单核心双线程CPU的特殊优化
  - 自动检测并限制Chrome线程数：`--max-threads=2`
  - 大幅减少定时器频率：日志30秒、维护5秒、内存检查200秒
  - 禁用后台进程和网络活动以减少线程竞争
  - 超保守模式下延长启动等待时间至8秒
- **性能优化**: 针对老旧硬件的特殊优化
  - CPU使用率优化：减少定时器频率
  - 内存监控：200秒间隔检查，低频率垃圾回收
  - 焦点和全屏检查：每15秒检查一次（而非每秒）

## 技术架构

### 核心技术栈
- **Qt 5.9.9**: 跨平台框架，兼容Windows 7
- **Qt WebEngine**: 基于Chromium 56的Web渲染引擎
- **Visual Studio 2015**: 编译工具链（x86架构）
- **CMake**: 构建系统

### Windows 7兼容性技术
1. **CPU架构检测**
   - 自动识别Intel 2-4代处理器
   - 检测Haswell、Ivy Bridge、Sandy Bridge架构
   - 根据CPU能力自动调整WebEngine参数

2. **内存管理优化**
   - 系统内存自动检测
   - 低内存环境（≤4GB）启用保守模式
   - JavaScript垃圾回收优化

3. **Chrome/WebEngine参数优化**
   ```
   老旧CPU模式：
   --no-sandbox --single-process --disable-webgl 
   --disable-webgl2 --disable-3d-apis --force-cpu-draw 
   --use-gl=disabled --disable-accelerated-video-processing
   --disable-features=WebRTC --renderer-process-limit=1
   --disable-smooth-scrolling
   
   标准Windows 7模式：
   --no-sandbox --single-process --max_old_space_size=256
   --disable-smooth-scrolling
   ```

## 构建说明

### 开发环境配置
```bash
# 安装依赖
# - Qt 5.9.9 (win32_msvc2015)
# - Visual Studio 2015 或 Build Tools
# - Windows SDK 8.1

# 构建项目
cd zdf-exam-desktop
mkdir build && cd build
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
nmake
```

### 部署配置
```bash
# Qt依赖部署
windeployqt --dir deploy zdf-exam-desktop.exe

# 复制资源文件
copy resources\*.* deploy\resources\
```

## 配置文件说明

### config.json基本配置
```json
{
  "url": "http://stu.sdzdf.com/",
  "exitPassword": "123456",
  "appName": "zdf-exam-desktop",
  "disableHardwareAcceleration": false,
  "lowMemoryMode": {
    "enabled": true,
    "threshold": 4096,
    "progressiveLoading": true,
    "progressiveLoadingDelay": 3000
  }
}
```

### 低内存模式参数
- `enabled`: 是否启用低内存模式
- `threshold`: 内存阈值（MB），低于此值启用优化
- `progressiveLoading`: 渐进式加载，避免黑屏
- `progressiveLoadingDelay`: 加载延迟时间（毫秒）

## 日志系统

### 日志文件类型
- `app.log`: 应用程序运行日志
- `config.log`: 配置文件操作日志  
- `exit.log`: F10退出尝试记录
- `startup.log`: 启动过程日志

### 操作记录格式
```
[2025-01-14 10:30:45] 热键退出尝试: 密码正确，退出
[2025-01-14 10:25:30] 热键退出尝试: 密码错误
[2025-01-14 10:20:15] 热键退出尝试: 取消输入
```

## 兼容性测试

### 测试环境
- **Windows 7 Ultimate SP1 (x86)**
  - Intel i7-4790 (Haswell)
  - 2GB RAM
  - ✅ 正常运行，启用超保守模式

- **Windows 10 (x64)**
  - Intel i7-6700 (Skylake)  
  - 8GB RAM
  - ✅ 正常运行，标准模式

### 已知问题解决
1. **黑屏问题**: 通过渐进式加载和CPU架构检测解决
2. **启动缓慢**: Chrome参数优化，减少渲染负载
3. **CPU占用高**: 定时器频率优化，内存检查间隔增加

## 版本历史

### v1.2.1 (2025-01-14)
- 🐛 修复变量重定义编译错误
- ⚡ CPU性能优化：减少定时器频率和内存检查频率
- 🔧 简化Chrome参数，提升老旧硬件兼容性
- 📝 完善兼容性文档

### v1.2.0 (2025-01-14)
- 🎯 新增CPU架构自动检测
- 🛡️ Windows 7 + 老旧CPU全面兼容性支持
- 💾 低内存环境优化
- 📊 系统硬件信息日志记录

### v1.1.0 (2025-01-14)
- 🔄 新增渐进式加载，解决黑屏问题
- 💾 低内存模式支持
- 📈 内存监控和垃圾回收机制
- 🚀 Windows 7基础兼容性优化

### v1.0.0 (2025-01-13)
- 🎉 初始版本发布
- 🔒 基础安全控制功能
- 📝 操作日志记录
- 🚫 快捷键拦截机制

## 故障排除

### 常见问题

**Q: Windows 7上出现黑屏？**
A: 程序会自动检测硬件并启用兼容模式。如仍有问题，请检查：
- 是否为老旧CPU（i7-4xxx及更早）
- 内存是否不足4GB
- 查看日志确认是否启用了超保守模式

**Q: CPU占用率过高？**  
A: v1.2.1已优化CPU使用：
- 定时器频率从1.5秒调整为3秒
- 内存检查从每15秒改为每60秒
- 减少不必要的系统调用

**Q: 程序启动慢？**
A: 老旧硬件上启动时间较长是正常现象：
- 渐进式加载会延迟3秒显示内容
- Chrome引擎初始化需要时间
- 可通过调整`progressiveLoadingDelay`参数优化

## 许可证
本项目采用 MIT 许可证。详见 [LICENSE](LICENSE.txt) 文件。

## 技术支持
如遇问题，请查看日志文件并提供以下信息：
- 操作系统版本和架构
- CPU型号和内存大小  
- 错误日志内容
- 复现步骤 