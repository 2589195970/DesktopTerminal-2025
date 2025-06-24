@echo off
setlocal enabledelayedexpansion

:: ===============================================
:: Windows 构建清理脚本
:: ===============================================
echo [INFO] 清理构建产物和临时文件...

set CLEANED_ITEMS=0

:: 清理构建目录
if exist "build" (
    echo [INFO] 删除 build 目录...
    rmdir /s /q "build" 2>nul
    if exist "build" (
        echo [WARNING] 无法完全删除 build 目录，可能有文件被占用
        echo 尝试强制删除...
        rd /s /q "build" 2>nul
    )
    if not exist "build" (
        echo [OK] build 目录已删除
        set /a CLEANED_ITEMS+=1
    )
)

:: 清理部署目录
if exist "deploy" (
    echo [INFO] 删除 deploy 目录...
    rmdir /s /q "deploy" 2>nul
    if exist "deploy" (
        echo [WARNING] 无法完全删除 deploy 目录，可能有文件被占用
        echo 尝试强制删除...
        rd /s /q "deploy" 2>nul
    )
    if not exist "deploy" (
        echo [OK] deploy 目录已删除
        set /a CLEANED_ITEMS+=1
    )
)

:: 清理输出目录
if exist "Output" (
    echo [INFO] 删除 Output 目录...
    rmdir /s /q "Output" 2>nul
    if exist "Output" (
        echo [WARNING] 无法完全删除 Output 目录，可能有文件被占用
        echo 尝试强制删除...
        rd /s /q "Output" 2>nul
    )
    if not exist "Output" (
        echo [OK] Output 目录已删除
        set /a CLEANED_ITEMS+=1
    )
)

:: 清理Windows特定构建目录
if exist "build-win" (
    echo [INFO] 删除 build-win 目录...
    rmdir /s /q "build-win" 2>nul
    if not exist "build-win" (
        echo [OK] build-win 目录已删除
        set /a CLEANED_ITEMS+=1
    )
)

:: 清理CMake缓存文件
echo [INFO] 清理CMake缓存文件...
if exist "CMakeCache.txt" (
    del /f /q "CMakeCache.txt" 2>nul
    if not exist "CMakeCache.txt" (
        echo [OK] CMakeCache.txt 已删除
        set /a CLEANED_ITEMS+=1
    )
)

if exist "CMakeFiles" (
    rmdir /s /q "CMakeFiles" 2>nul
    if not exist "CMakeFiles" (
        echo [OK] CMakeFiles 目录已删除
        set /a CLEANED_ITEMS+=1
    )
)

:: 清理Visual Studio相关文件
echo [INFO] 清理Visual Studio产物...
for %%f in (*.vcxproj *.vcxproj.filters *.vcxproj.user *.sln) do (
    if exist "%%f" (
        del /f /q "%%f" 2>nul
        if not exist "%%f" (
            echo [OK] 已删除 %%f
            set /a CLEANED_ITEMS+=1
        )
    )
)

:: 清理编译产物
echo [INFO] 清理编译产物...
for %%f in (*.obj *.lib *.exp *.pdb *.ilk) do (
    if exist "%%f" (
        del /f /q "%%f" 2>nul
        if not exist "%%f" (
            echo [OK] 已删除 %%f
            set /a CLEANED_ITEMS+=1
        )
    )
)

:: 清理Qt相关临时文件
echo [INFO] 清理Qt临时文件...
for %%f in (*.qm *.qrc.cpp moc_*.cpp moc_*.h ui_*.h qrc_*.cpp) do (
    if exist "%%f" (
        del /f /q "%%f" 2>nul
        if not exist "%%f" (
            echo [OK] 已删除 %%f
            set /a CLEANED_ITEMS+=1
        )
    )
)

:: 清理日志文件（可选）
set /p CLEAN_LOGS="是否清理日志文件？(y/N): "
if /i "%CLEAN_LOGS%"=="y" (
    echo [INFO] 清理日志文件...
    for %%f in (*.log) do (
        if exist "%%f" (
            del /f /q "%%f" 2>nul
            if not exist "%%f" (
                echo [OK] 已删除 %%f
                set /a CLEANED_ITEMS+=1
            )
        )
    )
    
    :: 清理子目录中的日志
    if exist "zdf-exam-desktop" (
        for %%f in (zdf-exam-desktop\*.log) do (
            if exist "%%f" (
                del /f /q "%%f" 2>nul
                if not exist "%%f" (
                    echo [OK] 已删除 %%f
                    set /a CLEANED_ITEMS+=1
                )
            )
        )
    )
)

:: 清理备份文件
echo [INFO] 清理备份文件...
for %%f in (*.bak *.backup *.orig *~) do (
    if exist "%%f" (
        del /f /q "%%f" 2>nul
        if not exist "%%f" (
            echo [OK] 已删除 %%f
            set /a CLEANED_ITEMS+=1
        )
    )
)

:: 清理临时目录
if exist "temp" (
    echo [INFO] 删除 temp 目录...
    rmdir /s /q "temp" 2>nul
    if not exist "temp" (
        echo [OK] temp 目录已删除
        set /a CLEANED_ITEMS+=1
    )
)

if exist "tmp" (
    echo [INFO] 删除 tmp 目录...
    rmdir /s /q "tmp" 2>nul
    if not exist "tmp" (
        echo [OK] tmp 目录已删除
        set /a CLEANED_ITEMS+=1
    )
)

:: 强制垃圾回收（尝试释放文件句柄）
echo [INFO] 尝试释放文件句柄...
taskkill /f /im "QtWebEngineProcess.exe" 2>nul >nul
taskkill /f /im "zdf-exam-desktop.exe" 2>nul >nul

:: 清理完成报告
echo.
echo ============================================
echo [SUCCESS] 清理完成！
echo ============================================
echo.
if %CLEANED_ITEMS% gtr 0 (
    echo 总共清理了 %CLEANED_ITEMS% 个项目
    echo.
    echo 已清理的内容：
    echo - 构建产物 (build, deploy, Output目录)
    echo - CMake缓存文件
    echo - Visual Studio项目文件
    echo - 编译生成的临时文件
    echo - Qt元对象编译文件
    if /i "%CLEAN_LOGS%"=="y" echo - 日志文件
    echo - 备份和临时文件
) else (
    echo 没有找到需要清理的文件
)
echo.
echo 项目已重置为干净状态，可以重新开始构建
echo.

:: 显示当前目录状态
echo 当前目录内容：
dir /b
echo.

pause