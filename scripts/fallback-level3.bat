@echo off
chcp 65001 >nul 2>&1
echo ========================================
echo Level 3 最终方案 - 系统诊断和降级
echo ========================================
echo.

echo [诊断] 正在检查系统环境...
echo.

echo 1. 检查操作系统版本:
ver
echo.

echo 2. 检查内存状态:
systeminfo | findstr "Physical Memory"
echo.

echo 3. 检查VC++运行时:
reg query "HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86" /v Version 2>nul
if %errorlevel%==0 (
    echo ✓ VC++ 2017 Runtime (x86) 已安装
) else (
    echo ✗ VC++ 2017 Runtime (x86) 未安装
    echo   请从微软官网下载安装
)
echo.

echo 4. 检查.NET Framework:
reg query "HKLM\SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full" /v Version 2>nul
if %errorlevel%==0 (
    echo ✓ .NET Framework 4.x 已安装
) else (
    echo ✗ .NET Framework 4.x 未安装
)
echo.

echo 5. 检查DirectX:
dxdiag /whql:off /t %TEMP%\dxdiag.txt
findstr "DirectX Version" %TEMP%\dxdiag.txt
del %TEMP%\dxdiag.txt 2>nul
echo.

echo ========================================
echo 请选择解决方案：
echo ========================================
echo 1. 使用系统浏览器替代方案
echo 2. 使用最小依赖模式
echo 3. 生成详细错误报告
echo 4. 创建系统信息报告
echo.
set /p solution=请选择 (1-4): 

if "%solution%"=="1" goto :browser
if "%solution%"=="2" goto :minimal
if "%solution%"=="3" goto :errorlog
if "%solution%"=="4" goto :sysinfo

:browser
echo.
echo [方案1] 系统浏览器替代方案
echo 这将打开默认浏览器访问考试网站
echo.
set /p url=请输入考试网站URL: 
if not "%url%"=="" (
    start "" "%url%"
    echo 已在默认浏览器中打开考试网站
)
goto :end

:minimal
echo.
echo [方案2] 最小依赖模式
echo 尝试禁用所有高级功能...
set "QTWEBENGINE_DISABLE_GPU=1"
set "QTWEBENGINE_DISABLE_SANDBOX=1" 
set "QT_OPENGL=software"
set "QTWEBENGINE_CHROMIUM_FLAGS=--no-sandbox --single-process --disable-gpu --disable-web-security --disable-extensions --disable-plugins --disable-images --disable-javascript"

set /p PROGRAM_PATH=请输入程序完整路径： 
if exist "%PROGRAM_PATH%" (
    start "" "%PROGRAM_PATH%"
)
goto :end

:errorlog
echo.
echo [方案3] 生成错误报告
echo 正在收集错误信息...

set /p PROGRAM_PATH=请输入程序完整路径： 
if exist "%PROGRAM_PATH%" (
    echo 启动程序并记录错误...
    echo 程序启动时间: %DATE% %TIME% > error_report.txt
    echo 系统版本: >> error_report.txt
    ver >> error_report.txt
    echo. >> error_report.txt
    echo 环境变量: >> error_report.txt
    set >> error_report.txt
    
    "%PROGRAM_PATH%" 2>&1 | tee error_output.txt
    
    echo 错误报告已生成：error_report.txt 和 error_output.txt
)
goto :end

:sysinfo
echo.
echo [方案4] 生成系统信息报告
msinfo32 /report system_info.txt
echo 系统信息报告已生成：system_info.txt
echo 请将此文件发送给技术支持

:end
echo.
echo 如果所有方案都无效，建议：
echo 1. 升级到Windows 10系统
echo 2. 使用其他设备进行考试
echo 3. 联系技术支持获取专门的解决方案
echo.
pause