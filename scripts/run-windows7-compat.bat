@echo off
:: Windows 7兼容模式启动脚本
:: 用于手动测试WebEngine黑屏修复方案

echo ========================================
echo 智多分机考桌面端 - Windows 7兼容模式
echo ========================================
echo.

echo 正在设置Windows 7兼容性环境变量...

:: 禁用GPU和沙箱
set QTWEBENGINE_DISABLE_GPU=1
set QTWEBENGINE_DISABLE_SANDBOX=1

:: 强制软件渲染
set QT_OPENGL=software
set QSG_RHI_PREFER_SOFTWARE_RENDERER=1

:: Chromium兼容性标志
set QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu --disable-gpu-compositing --disable-gpu-sandbox --single-process --in-process-gpu --disable-dev-shm-usage --no-sandbox

echo 环境变量设置完成！
echo.
echo 当前环境变量：
echo QTWEBENGINE_DISABLE_GPU=%QTWEBENGINE_DISABLE_GPU%
echo QTWEBENGINE_DISABLE_SANDBOX=%QTWEBENGINE_DISABLE_SANDBOX%
echo QT_OPENGL=%QT_OPENGL%
echo QSG_RHI_PREFER_SOFTWARE_RENDERER=%QSG_RHI_PREFER_SOFTWARE_RENDERER%
echo.

:: 检查程序是否存在
set "PROGRAM_PATH=C:\Program Files (x86)\智多分\智多分-机考桌面端\zdf-exam-desktop.exe"
if exist "%PROGRAM_PATH%" (
    echo 找到程序：%PROGRAM_PATH%
    echo 正在启动程序...
    echo.
    "%PROGRAM_PATH%"
) else (
    echo 错误：未找到程序文件！
    echo 请检查程序是否已正确安装到：
    echo %PROGRAM_PATH%
    echo.
    echo 如果程序安装在其他位置，请修改此脚本中的PROGRAM_PATH变量
    echo.
    pause
)

echo.
echo 程序已退出
pause