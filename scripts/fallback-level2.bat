@echo off
chcp 65001 >nul 2>&1
echo ========================================
echo Level 2 备用方案 - 渲染模式切换
echo ========================================
echo.

echo 请选择渲染模式：
echo 1. DirectX Software (WARP)
echo 2. OpenGL Software  
echo 3. ANGLE D3D11
echo 4. 纯软件渲染
echo 5. 兼容模式（最安全）
echo.
set /p choice=请选择 (1-5): 

if "%choice%"=="1" goto :warp
if "%choice%"=="2" goto :opengl
if "%choice%"=="3" goto :angle
if "%choice%"=="4" goto :software
if "%choice%"=="5" goto :compat
echo 无效选择，使用默认兼容模式
goto :compat

:warp
echo [Level 2A] DirectX WARP 模式
set "QT_OPENGL=angle"
set "QT_ANGLE_PLATFORM=warp"
set "QTWEBENGINE_CHROMIUM_FLAGS=--use-angle=d3d11 --use-gl=angle"
goto :run

:opengl
echo [Level 2B] OpenGL Software 模式
set "QT_OPENGL=software"
set "QTWEBENGINE_CHROMIUM_FLAGS=--use-gl=swiftshader"
goto :run

:angle
echo [Level 2C] ANGLE D3D11 模式
set "QT_OPENGL=angle"
set "QT_ANGLE_PLATFORM=d3d11"
set "QTWEBENGINE_CHROMIUM_FLAGS=--use-angle=d3d11"
goto :run

:software
echo [Level 2D] 纯软件渲染模式
set "QT_OPENGL=software"
set "QSG_RHI_BACKEND=software"
set "QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu --use-gl=swiftshader --single-process"
goto :run

:compat
echo [Level 2E] 最大兼容模式
set "QT_OPENGL=software"
set "QSG_RHI_BACKEND=software"
set "QTWEBENGINE_DISABLE_GPU=1"
set "QTWEBENGINE_DISABLE_SANDBOX=1"
set "QTWEBENGINE_CHROMIUM_FLAGS=--no-sandbox --disable-gpu --single-process --disable-web-security"

:run
echo.
set /p PROGRAM_PATH=请输入程序完整路径： 

if exist "%PROGRAM_PATH%" (
    echo [启动] 使用选定的渲染模式...
    start "" "%PROGRAM_PATH%"
) else (
    echo [错误] 程序文件不存在
)

echo.
echo 如果仍有问题，请尝试 fallback-level3.bat
pause