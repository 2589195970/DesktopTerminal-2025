@echo off
chcp 65001 >nul 2>&1
:: Windows 7兼容模式启动脚本
:: 用于手动测试WebEngine黑屏修复方案

echo ========================================
echo 智多分机考桌面端 - Windows 7兼容模式
echo ========================================
echo.

echo [调试] 脚本开始执行...
echo [调试] 当前目录: %CD%
echo.

echo [步骤1] 正在设置Windows 7兼容性环境变量...

:: 禁用GPU和沙箱
set "QTWEBENGINE_DISABLE_GPU=1"
set "QTWEBENGINE_DISABLE_SANDBOX=1"

:: 强制软件渲染
set "QT_OPENGL=software"
set "QSG_RHI_PREFER_SOFTWARE_RENDERER=1"

:: Chromium兼容性标志
set "QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu --disable-gpu-compositing --disable-gpu-sandbox --single-process --in-process-gpu --disable-dev-shm-usage --no-sandbox"

echo [完成] 环境变量设置完成！
echo.
echo [信息] 当前环境变量：
echo   QTWEBENGINE_DISABLE_GPU=%QTWEBENGINE_DISABLE_GPU%
echo   QTWEBENGINE_DISABLE_SANDBOX=%QTWEBENGINE_DISABLE_SANDBOX%
echo   QT_OPENGL=%QT_OPENGL%
echo   QSG_RHI_PREFER_SOFTWARE_RENDERER=%QSG_RHI_PREFER_SOFTWARE_RENDERER%
echo.

:: 检查多个可能的程序路径
echo [步骤2] 搜索程序文件...

set "PROGRAM_PATH="
set "SEARCH_PATH1=C:\Program Files (x86)\智多分\智多分-机考桌面端\zdf-exam-desktop.exe"
set "SEARCH_PATH2=C:\Program Files\智多分\智多分-机考桌面端\zdf-exam-desktop.exe"
set "SEARCH_PATH3=%USERPROFILE%\Desktop\智多分-机考桌面端\zdf-exam-desktop.exe"
set "SEARCH_PATH4=%CD%\zdf-exam-desktop.exe"

echo [检查] 路径1: %SEARCH_PATH1%
if exist "%SEARCH_PATH1%" (
    echo [找到] 程序位于路径1
    set "PROGRAM_PATH=%SEARCH_PATH1%"
    goto :found
)

echo [检查] 路径2: %SEARCH_PATH2%
if exist "%SEARCH_PATH2%" (
    echo [找到] 程序位于路径2
    set "PROGRAM_PATH=%SEARCH_PATH2%"
    goto :found
)

echo [检查] 路径3: %SEARCH_PATH3%
if exist "%SEARCH_PATH3%" (
    echo [找到] 程序位于路径3
    set "PROGRAM_PATH=%SEARCH_PATH3%"
    goto :found
)

echo [检查] 路径4: %SEARCH_PATH4%
if exist "%SEARCH_PATH4%" (
    echo [找到] 程序位于路径4
    set "PROGRAM_PATH=%SEARCH_PATH4%"
    goto :found
)

:: 如果都没找到
echo [错误] 未找到程序文件！
echo.
echo 已检查的路径：
echo   1. %SEARCH_PATH1%
echo   2. %SEARCH_PATH2%
echo   3. %SEARCH_PATH3%
echo   4. %SEARCH_PATH4%
echo.
echo 请确认程序已正确安装，或将此脚本复制到程序目录中运行
echo.
echo 按任意键退出...
pause >nul
exit /b 1

:found
echo [步骤3] 启动程序...
echo [命令] "%PROGRAM_PATH%"
echo.

start "" "%PROGRAM_PATH%"

echo [完成] 程序启动命令已执行
echo [注意] 如果程序没有显示，请检查任务管理器中是否有进程
echo.
echo 按任意键退出...
pause >nul