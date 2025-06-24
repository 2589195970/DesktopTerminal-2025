@echo off
setlocal enabledelayedexpansion

:: ===============================================
:: Windows Qt依赖部署脚本
:: ===============================================
echo [INFO] 开始部署Qt依赖...

:: 检查参数
if "%1"=="" (
    echo [ERROR] 用法: deploy-windows.bat ^<exe文件路径^> [部署目录]
    echo 示例: deploy-windows.bat build\zdf-exam-desktop.exe deploy
    pause
    exit /b 1
)

set EXE_PATH=%1
set DEPLOY_DIR=%2
if "%DEPLOY_DIR%"=="" set DEPLOY_DIR=deploy

:: 检查exe文件是否存在
if not exist "%EXE_PATH%" (
    echo [ERROR] 找不到exe文件: %EXE_PATH%
    pause
    exit /b 1
)

:: 自动检测Qt安装路径
set QT_PATHS=C:\Qt\5.15.2\msvc2019_64;C:\Qt\5.15.2\msvc2019;C:\Qt\5.12.12\msvc2019_64;C:\Qt\Tools\5.15.2\msvc2019_64;%QTDIR%

for %%p in (%QT_PATHS%) do (
    if exist "%%p\bin\windeployqt.exe" (
        set QT_DIR=%%p
        goto qt_found
    )
)

echo [ERROR] 未找到Qt安装或windeployqt工具
echo 请确保Qt安装完整并设置QT_DIR环境变量
pause
exit /b 1

:qt_found
echo [OK] 找到Qt安装: %QT_DIR%
set PATH=%QT_DIR%\bin;%PATH%

:: 创建部署目录
if not exist "%DEPLOY_DIR%" mkdir "%DEPLOY_DIR%"

:: 复制exe文件到部署目录
echo [INFO] 复制主程序...
copy "%EXE_PATH%" "%DEPLOY_DIR%\" >nul
if %errorlevel% neq 0 (
    echo [ERROR] 复制exe文件失败
    pause
    exit /b 1
)

:: 获取exe文件名
for %%F in ("%EXE_PATH%") do set EXE_NAME=%%~nxF

:: 使用windeployqt部署依赖
echo [INFO] 部署Qt依赖库...
windeployqt --dir "%DEPLOY_DIR%" --release --no-translations --no-system-d3d-compiler --no-opengl-sw "%DEPLOY_DIR%\%EXE_NAME%"
if %errorlevel% neq 0 (
    echo [ERROR] windeployqt部署失败
    pause
    exit /b 1
)

:: 检查Qt WebEngine特殊依赖
echo [INFO] 检查WebEngine依赖...

:: 检查QtWebEngineProcess.exe
if not exist "%DEPLOY_DIR%\QtWebEngineProcess.exe" (
    echo [INFO] 复制QtWebEngineProcess.exe...
    copy "%QT_DIR%\bin\QtWebEngineProcess.exe" "%DEPLOY_DIR%\" >nul
)

:: 检查resources目录
if not exist "%DEPLOY_DIR%\resources" (
    echo [INFO] 复制WebEngine资源...
    if exist "%QT_DIR%\resources" (
        xcopy /E /I /Y "%QT_DIR%\resources" "%DEPLOY_DIR%\resources" >nul
    )
)

:: 检查translations目录中的qtwebengine_locales
if not exist "%DEPLOY_DIR%\translations\qtwebengine_locales" (
    echo [INFO] 复制WebEngine本地化文件...
    if exist "%QT_DIR%\translations\qtwebengine_locales" (
        mkdir "%DEPLOY_DIR%\translations\qtwebengine_locales" 2>nul
        xcopy /E /I /Y "%QT_DIR%\translations\qtwebengine_locales" "%DEPLOY_DIR%\translations\qtwebengine_locales" >nul
    )
)

:: 复制项目资源文件
echo [INFO] 复制项目资源...
if exist "resources" (
    mkdir "%DEPLOY_DIR%\resources" 2>nul
    copy "resources\*.*" "%DEPLOY_DIR%\resources\" >nul 2>&1
)

:: 复制配置文件到exe目录（最高优先级）
if exist "resources\config.json" (
    copy "resources\config.json" "%DEPLOY_DIR%\" >nul
    echo [OK] 已复制配置文件到部署目录
)

:: 验证关键文件
echo [INFO] 验证部署文件...
set MISSING_FILES=0

set REQUIRED_DLLS=Qt5Core.dll Qt5Gui.dll Qt5Widgets.dll Qt5WebEngine.dll Qt5WebEngineCore.dll Qt5WebEngineWidgets.dll Qt5Network.dll

for %%d in (%REQUIRED_DLLS%) do (
    if not exist "%DEPLOY_DIR%\%%d" (
        echo [WARNING] 缺少: %%d
        set MISSING_FILES=1
    )
)

:: 检查platforms目录
if not exist "%DEPLOY_DIR%\platforms\qwindows.dll" (
    echo [WARNING] 缺少platforms\qwindows.dll
    set MISSING_FILES=1
)

:: 创建默认配置文件（如果不存在）
if not exist "%DEPLOY_DIR%\config.json" (
    echo [INFO] 创建默认配置文件...
    echo {"url":"https://example.com","exitPassword":"admin123","appName":"智多分机考桌面端"} > "%DEPLOY_DIR%\config.json"
)

:: 创建运行时库检查脚本
echo [INFO] 创建运行时检查脚本...
echo @echo off > "%DEPLOY_DIR%\check-runtime.bat"
echo echo [INFO] 检查运行时环境... >> "%DEPLOY_DIR%\check-runtime.bat"
echo. >> "%DEPLOY_DIR%\check-runtime.bat"
echo echo 正在检查Visual C++ Redistributable... >> "%DEPLOY_DIR%\check-runtime.bat"
echo reg query "HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" /v Version ^>nul 2^>^&1 >> "%DEPLOY_DIR%\check-runtime.bat"
echo if %%errorlevel%% neq 0 ( >> "%DEPLOY_DIR%\check-runtime.bat"
echo     echo [WARNING] 未检测到Visual C++ 2015-2019 Redistributable >> "%DEPLOY_DIR%\check-runtime.bat"
echo     echo 请从Microsoft官网下载安装 >> "%DEPLOY_DIR%\check-runtime.bat"
echo ^) else ( >> "%DEPLOY_DIR%\check-runtime.bat"
echo     echo [OK] Visual C++ Redistributable已安装 >> "%DEPLOY_DIR%\check-runtime.bat"
echo ^) >> "%DEPLOY_DIR%\check-runtime.bat"
echo. >> "%DEPLOY_DIR%\check-runtime.bat"
echo echo 按任意键继续... >> "%DEPLOY_DIR%\check-runtime.bat"
echo pause ^>nul >> "%DEPLOY_DIR%\check-runtime.bat"

:: 创建启动脚本
echo [INFO] 创建启动脚本...
echo @echo off > "%DEPLOY_DIR%\run.bat"
echo cd /d "%%~dp0" >> "%DEPLOY_DIR%\run.bat"
echo echo 启动智多分机考桌面端... >> "%DEPLOY_DIR%\run.bat"
echo start "" "%EXE_NAME%" >> "%DEPLOY_DIR%\run.bat"

echo.
echo ============================================
echo [SUCCESS] 依赖部署完成！
echo ============================================
echo.
echo 部署目录: %DEPLOY_DIR%
echo 主程序: %DEPLOY_DIR%\%EXE_NAME%
echo.
if %MISSING_FILES% equ 1 (
    echo [WARNING] 检测到缺少某些文件，请检查上述警告信息
    echo 如果运行出现问题，请确保：
    echo 1. Qt安装完整
    echo 2. 目标机器安装了Visual C++ Redistributable
    echo.
) else (
    echo [OK] 所有关键文件已正确部署
    echo.
)
echo 测试方法：
echo 1. 将%DEPLOY_DIR%目录复制到目标机器
echo 2. 运行check-runtime.bat检查运行时环境
echo 3. 运行%EXE_NAME%或run.bat启动应用
echo.