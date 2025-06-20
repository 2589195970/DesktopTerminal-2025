@echo off
chcp 65001 >nul 2>&1
echo ========================================
echo Level 1 备用方案 - 参数调优
echo ========================================
echo.

echo [Level 1] 尝试更保守的兼容性设置...

:: 更保守的环境变量设置
set "QTWEBENGINE_DISABLE_GPU=1"
set "QTWEBENGINE_DISABLE_SANDBOX=1"
set "QT_OPENGL=software"
set "QT_ANGLE_PLATFORM=warp"
set "QSG_RHI_BACKEND=software"
set "QSG_RHI_PREFER_SOFTWARE_RENDERER=1"

:: 更全面的Chromium禁用标志
set "QTWEBENGINE_CHROMIUM_FLAGS=--no-sandbox --disable-web-security --disable-features=VizDisplayCompositor --disable-background-timer-throttling --disable-renderer-backgrounding --disable-backgrounding-occluded-windows --disable-gpu --disable-gpu-compositing --disable-gpu-sandbox --single-process --in-process-gpu --disable-dev-shm-usage --disable-software-rasterizer --disable-accelerated-2d-canvas --disable-accelerated-jpeg-decoding --disable-accelerated-mjpeg-decode --disable-accelerated-video-decode --use-gl=swiftshader --disable-extensions --disable-plugins"

echo [信息] 设置了更保守的参数组合
echo.

set /p PROGRAM_PATH=请输入程序完整路径： 

if exist "%PROGRAM_PATH%" (
    echo [启动] Level 1 参数模式...
    start "" "%PROGRAM_PATH%"
) else (
    echo [错误] 程序文件不存在
)

echo.
echo 如果仍然黑屏，请尝试 fallback-level2.bat
pause