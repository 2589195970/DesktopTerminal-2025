# 配置文件使用指南

## 概述

zdf-exam-desktop支持通过外部配置文件动态修改应用设置，无需重新编译或打包程序。

## 配置文件位置

程序会按照以下优先级顺序查找配置文件：

1. **可执行文件同目录** (最高优先级)
   - Windows: `C:\Program Files\zdf-exam-desktop\config.json`
   - Linux: `/usr/local/bin/config.json`
   - macOS: `/Applications/zdf-exam-desktop.app/Contents/MacOS/config.json`

2. **用户配置目录**
   - Windows: `%APPDATA%\zdf-exam-desktop\config.json`
   - Linux: `~/.config/zdf-exam-desktop\config.json`
   - macOS: `~/Library/Application Support/zdf-exam-desktop\config.json`

3. **系统配置目录** (仅 Linux/Unix)
   - `/etc/zdf-exam-desktop/config.json`

4. **内置资源目录** (向后兼容)
   - `resources/config.json`

## 配置文件格式

配置文件采用 JSON 格式，包含以下字段：

```json
{
  "url": "http://stu.sdzdf.com/",
  "exitPassword": "sdzdf@2025",
  "appName": "智多分机考桌面端",
  "iconPath": "logo.svg",
  "appVersion": "1.0.0"
}
```

### 字段说明

- **url**: 应用启动时加载的网址
- **exitPassword**: 退出应用所需的密码（按 F10 或 \ 键时需要输入）
- **appName**: 应用窗口标题
- **iconPath**: 应用图标路径（可选）
- **appVersion**: 应用版本号（可选）

## 使用方法

### 方法一：修改现有配置文件

1. 首次运行程序时，如果没有找到配置文件，程序会自动在可执行文件目录创建默认配置文件
2. 使用文本编辑器打开 `config.json`
3. 修改相应的配置项
4. 保存文件并重新启动程序

### 方法二：创建自定义配置文件

1. 在上述任意配置文件位置创建 `config.json` 文件
2. 复制默认配置内容并修改
3. 保存文件并启动程序

## 示例场景

### 场景1：更换考试系统地址
```json
{
  "url": "http://exam.school.edu.cn/",
  "exitPassword": "admin123",
  "appName": "学校考试系统"
}
```

### 场景2：多考场配置
为不同考场创建不同的配置文件：

**考场A配置 (config-room-a.json)**
```json
{
  "url": "http://exam.school.edu.cn/room-a",
  "exitPassword": "roomA2025",
  "appName": "考场A考试系统"
}
```

**考场B配置 (config-room-b.json)**
```json
{
  "url": "http://exam.school.edu.cn/room-b",
  "exitPassword": "roomB2025",
  "appName": "考场B考试系统"
}
```

使用时只需将对应的配置文件重命名为 `config.json` 即可。

## 注意事项

1. 配置文件必须是有效的 JSON 格式，否则程序将无法启动
2. 必须包含 `url`、`exitPassword` 和 `appName` 三个必填字段
3. 修改配置文件后需要重新启动程序才能生效
4. 建议定期备份配置文件
5. 退出密码建议使用复杂的字符组合，确保安全性

## 故障排除

### 程序无法启动
- 检查配置文件是否为有效的 JSON 格式
- 确保所有必填字段都已正确填写
- 查看程序目录下的日志文件了解详细错误信息

### 配置不生效
- 确认配置文件放置在正确的位置
- 检查是否有多个配置文件存在（程序会使用优先级最高的）
- 重新启动程序

### 忘记退出密码
- 找到正在使用的配置文件
- 使用文本编辑器查看或修改 `exitPassword` 字段
- 如果无法确定配置文件位置，可以删除所有配置文件，程序会创建新的默认配置 