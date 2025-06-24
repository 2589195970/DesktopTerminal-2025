@echo off
setlocal enabledelayedexpansion

:: ===============================================
:: Windows 构建依赖检查脚本
:: ===============================================
echo ============================================
echo 智多分机考桌面端 - 构建依赖检查
echo ============================================
echo.

set TOTAL_CHECKS=0
set PASSED_CHECKS=0
set FAILED_CHECKS=0
set WARNING_CHECKS=0

:: ===============================================
:: 1. 操作系统检查
:: ===============================================
echo [CHECK 1] 操作系统环境
set /a TOTAL_CHECKS+=1

for /f "tokens=2 delims==" %%i in ('wmic os get Version /value 2^>nul') do set OS_VERSION=%%i
if defined OS_VERSION (
    echo [OK] Windows版本: %OS_VERSION%
    set /a PASSED_CHECKS+=1
) else (
    echo [ERROR] 无法检测Windows版本
    set /a FAILED_CHECKS+=1
)

:: 检查架构
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
    echo [OK] 64位系统
) else if "%PROCESSOR_ARCHITECTURE%"=="x86" (
    echo [WARNING] 32位系统 - 建议使用64位系统以获得更好性能
    set /a WARNING_CHECKS+=1
) else (
    echo [WARNING] 未知架构: %PROCESSOR_ARCHITECTURE%
    set /a WARNING_CHECKS+=1
)
echo.

:: ===============================================
:: 2. Visual Studio环境检查
:: ===============================================
echo [CHECK 2] Visual Studio环境
set /a TOTAL_CHECKS+=1

set VS_FOUND=0

:: 检查VS2022
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    echo [OK] 找到 Visual Studio 2022 Enterprise
    set VS_FOUND=1
) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    echo [OK] 找到 Visual Studio 2022 Professional
    set VS_FOUND=1
) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    echo [OK] 找到 Visual Studio 2022 Community
    set VS_FOUND=1
)

:: 检查VS2019
if %VS_FOUND%==0 (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
        echo [OK] 找到 Visual Studio 2019 Enterprise
        set VS_FOUND=1
    ) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" (
        echo [OK] 找到 Visual Studio 2019 Professional
        set VS_FOUND=1
    ) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
        echo [OK] 找到 Visual Studio 2019 Community
        set VS_FOUND=1
    )
)

if %VS_FOUND%==1 (
    set /a PASSED_CHECKS+=1
    
    :: 检查Windows SDK
    echo [INFO] 检查Windows SDK...
    reg query "HKLM\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows\v10.0" /v InstallationFolder >nul 2>&1
    if %errorlevel%==0 (
        echo [OK] Windows 10 SDK已安装
    ) else (
        echo [WARNING] 未检测到Windows 10 SDK
        set /a WARNING_CHECKS+=1
    )
) else (
    echo [ERROR] 未找到Visual Studio 2019/2022
    echo [ERROR] 请安装Visual Studio并选择"使用C++的桌面开发"工作负载
    set /a FAILED_CHECKS+=1
)
echo.

:: ===============================================
:: 3. CMake检查
:: ===============================================
echo [CHECK 3] CMake
set /a TOTAL_CHECKS+=1

cmake --version >nul 2>&1
if %errorlevel%==0 (
    for /f "tokens=3" %%i in ('cmake --version 2^>nul ^| findstr "cmake version"') do set CMAKE_VERSION=%%i
    echo [OK] CMake版本: %CMAKE_VERSION%
    
    :: 检查版本是否满足要求
    for /f "tokens=1,2,3 delims=." %%a in ("%CMAKE_VERSION%") do (
        set CMAKE_MAJOR=%%a
        set CMAKE_MINOR=%%b
    )
    
    if %CMAKE_MAJOR% gtr 3 (
        set CMAKE_OK=1
    ) else if %CMAKE_MAJOR%==3 (
        if %CMAKE_MINOR% geq 14 (
            set CMAKE_OK=1
        ) else (
            set CMAKE_OK=0
        )
    ) else (
        set CMAKE_OK=0
    )
    
    if !CMAKE_OK!==1 (
        set /a PASSED_CHECKS+=1
    ) else (
        echo [ERROR] CMake版本过低，需要3.14或更高版本
        set /a FAILED_CHECKS+=1
    )
) else (
    echo [ERROR] 未找到CMake
    echo [ERROR] 请从 https://cmake.org/download/ 下载安装
    set /a FAILED_CHECKS+=1
)
echo.

:: ===============================================
:: 4. Qt环境检查
:: ===============================================
echo [CHECK 4] Qt环境
set /a TOTAL_CHECKS+=1

set QT_FOUND=0
set QT_PATHS=C:\Qt\5.15.2\msvc2019_64;C:\Qt\5.15.2\msvc2019;C:\Qt\5.12.12\msvc2019_64;C:\Qt\Tools\5.15.2\msvc2019_64;%QTDIR%

for %%p in (%QT_PATHS%) do (
    if exist "%%p\bin\qmake.exe" (
        set QT_DIR=%%p
        set QT_FOUND=1
        goto qt_check_found
    )
)

:qt_check_found
if %QT_FOUND%==1 (
    echo [OK] 找到Qt安装: %QT_DIR%
    
    :: 检查Qt版本
    for /f "tokens=4" %%i in ('"%QT_DIR%\bin\qmake.exe" -query QT_VERSION 2^>nul') do set QT_VERSION=%%i
    if defined QT_VERSION (
        echo [OK] Qt版本: %QT_VERSION%
    ) else (
        echo [WARNING] 无法获取Qt版本信息
        set /a WARNING_CHECKS+=1
    )
    
    :: 检查关键Qt模块
    echo [INFO] 检查Qt模块...
    if exist "%QT_DIR%\bin\Qt5WebEngine.dll" (
        echo [OK] Qt5WebEngine模块已安装
    ) else (
        echo [ERROR] 缺少Qt5WebEngine模块
        echo [ERROR] 请重新安装Qt并确保包含WebEngine模块
        set /a FAILED_CHECKS+=1
        goto qt_module_error
    )
    
    if exist "%QT_DIR%\bin\Qt5WebEngineWidgets.dll" (
        echo [OK] Qt5WebEngineWidgets模块已安装
    ) else (
        echo [ERROR] 缺少Qt5WebEngineWidgets模块
        set /a FAILED_CHECKS+=1
        goto qt_module_error
    )
    
    :: 检查windeployqt工具
    if exist "%QT_DIR%\bin\windeployqt.exe" (
        echo [OK] windeployqt工具可用
    ) else (
        echo [WARNING] 缺少windeployqt工具
        set /a WARNING_CHECKS+=1
    )
    
    set /a PASSED_CHECKS+=1
    goto qt_check_done
    
    :qt_module_error
    echo [ERROR] Qt模块检查失败
    goto qt_check_done
    
) else (
    echo [ERROR] 未找到Qt安装
    echo [ERROR] 请安装Qt 5.12+并确保包含以下模块：
    echo [ERROR] - Qt WebEngine
    echo [ERROR] - Qt WebEngine Widgets  
    echo [ERROR] - MSVC 2019编译器支持
    set /a FAILED_CHECKS+=1
)

:qt_check_done
echo.

:: ===============================================
:: 5. NSIS检查 (可选)
:: ===============================================
echo [CHECK 5] NSIS (安装包生成工具)
set /a TOTAL_CHECKS+=1

makensis /VERSION >nul 2>&1
if %errorlevel%==0 (
    for /f "tokens=2" %%i in ('makensis /VERSION 2^>nul') do set NSIS_VERSION=%%i
    echo [OK] NSIS版本: %NSIS_VERSION%
    set /a PASSED_CHECKS+=1
) else (
    echo [WARNING] 未找到NSIS
    echo [INFO] NSIS用于生成Windows安装包，非必需但推荐安装
    echo [INFO] 下载地址: https://nsis.sourceforge.io/Download
    echo [INFO] 或使用: choco install nsis
    set /a WARNING_CHECKS+=1
)
echo.

:: ===============================================
:: 6. Git检查
:: ===============================================
echo [CHECK 6] Git
set /a TOTAL_CHECKS+=1

git --version >nul 2>&1
if %errorlevel%==0 (
    for /f "tokens=1,2,3" %%i in ('git --version 2^>nul') do set GIT_VERSION=%%k
    echo [OK] Git版本: %GIT_VERSION%
    set /a PASSED_CHECKS+=1
) else (
    echo [WARNING] 未找到Git
    echo [INFO] Git用于版本控制，推荐安装
    set /a WARNING_CHECKS+=1
)
echo.

:: ===============================================
:: 7. 磁盘空间检查
:: ===============================================
echo [CHECK 7] 磁盘空间
set /a TOTAL_CHECKS+=1

for /f "tokens=3" %%i in ('dir /-c "%CD%" 2^>nul ^| findstr "bytes free"') do set FREE_BYTES=%%i
if defined FREE_BYTES (
    set /a FREE_MB=%FREE_BYTES% / 1048576
    echo [INFO] 当前目录可用空间: !FREE_MB! MB
    
    if !FREE_MB! gtr 1024 (
        echo [OK] 磁盘空间充足
        set /a PASSED_CHECKS+=1
    ) else (
        echo [WARNING] 磁盘空间不足，推荐至少1GB可用空间
        set /a WARNING_CHECKS+=1
    )
) else (
    echo [WARNING] 无法检测磁盘空间
    set /a WARNING_CHECKS+=1
)
echo.

:: ===============================================
:: 8. 网络连接检查
:: ===============================================
echo [CHECK 8] 网络连接 (用于下载依赖)
set /a TOTAL_CHECKS+=1

ping -n 1 github.com >nul 2>&1
if %errorlevel%==0 (
    echo [OK] 网络连接正常
    set /a PASSED_CHECKS+=1
) else (
    echo [WARNING] 无法连接到github.com
    echo [INFO] 网络连接用于下载依赖和子模块，请确保网络畅通
    set /a WARNING_CHECKS+=1
)
echo.

:: ===============================================
:: 结果汇总
:: ===============================================
echo ============================================
echo 依赖检查结果汇总
echo ============================================
echo.
echo 总检查项: %TOTAL_CHECKS%
echo 通过: %PASSED_CHECKS%
echo 警告: %WARNING_CHECKS%
echo 失败: %FAILED_CHECKS%
echo.

if %FAILED_CHECKS%==0 (
    if %WARNING_CHECKS%==0 (
        echo [SUCCESS] 所有依赖检查通过！
        echo 你可以开始构建项目了。
        echo.
        echo 构建命令:
        echo   build-windows.bat
    ) else (
        echo [SUCCESS] 基本依赖满足，但有一些警告项
        echo 你可以开始构建，但建议处理上述警告以获得更好的体验。
        echo.
        echo 构建命令:
        echo   build-windows.bat
    )
) else (
    echo [ERROR] 依赖检查失败！
    echo 请先解决上述错误，然后重新运行此检查脚本。
    echo.
    echo 快速修复指南:
    if %VS_FOUND%==0 echo - 安装Visual Studio 2019/2022社区版
    if %QT_FOUND%==0 echo - 安装Qt 5.12+并包含WebEngine模块
    cmake --version >nul 2>&1
    if %errorlevel% neq 0 echo - 从cmake.org下载安装CMake
)

echo.
echo ============================================
echo.

pause