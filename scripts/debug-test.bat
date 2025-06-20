@echo off
:: 简化的调试脚本
echo ===========================================
echo Windows 7 兼容性调试脚本
echo ===========================================
echo.

echo 第1步：设置环境变量
set QTWEBENGINE_DISABLE_GPU=1
set QTWEBENGINE_DISABLE_SANDBOX=1
set QT_OPENGL=software
echo 完成！
echo.

echo 第2步：检查程序路径
echo 请在下面手动输入程序的完整路径：
echo 示例：C:\Program Files (x86)\智多分\智多分-机考桌面端\zdf-exam-desktop.exe
echo.
set /p PROGRAM_PATH=请输入程序路径： 

echo.
echo 第3步：验证路径
if exist "%PROGRAM_PATH%" (
    echo ✓ 找到程序文件
) else (
    echo ✗ 程序文件不存在
    echo 请检查路径是否正确
    pause
    exit /b 1
)

echo.
echo 第4步：启动程序
echo 正在启动：%PROGRAM_PATH%
echo.
start "" "%PROGRAM_PATH%"

echo 程序启动完成！
echo 请观察程序是否正常显示网页内容
echo.
pause