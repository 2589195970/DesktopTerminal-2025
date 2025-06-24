@echo off
setlocal enabledelayedexpansion

:: ===============================================
:: Windows 安装包生成脚本
:: ===============================================
echo [INFO] 开始生成Windows安装包...

:: 检查NSIS是否安装
makensis /VERSION >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] 未找到NSIS，请先安装NSIS
    echo 下载地址: https://nsis.sourceforge.io/Download
    echo 或使用: choco install nsis
    pause
    exit /b 1
)
echo [OK] NSIS已安装

:: 检查installer.nsi是否存在
if not exist "installer.nsi" (
    echo [ERROR] 找不到installer.nsi文件
    echo 请确保在项目根目录下执行此脚本
    pause
    exit /b 1
)

:: 检查deploy目录是否存在
if not exist "deploy" (
    echo [ERROR] 找不到deploy目录
    echo 请先运行build-windows.bat或deploy-windows.bat
    pause
    exit /b 1
)

:: 检查关键文件
if not exist "deploy\zdf-exam-desktop.exe" (
    echo [ERROR] deploy目录中缺少主程序文件
    echo 请先运行build-windows.bat进行构建和部署
    pause
    exit /b 1
)

:: 创建输出目录
if not exist "Output" mkdir "Output"

:: 检查资源文件
echo [INFO] 检查资源文件...
if not exist "resources\logo.ico" (
    echo [WARNING] 缺少logo.ico文件，安装包图标可能显示异常
    if exist "resources\logo.png" (
        echo [INFO] 找到logo.png，建议转换为ico格式
    )
)

:: 显示NSIS脚本信息
echo [INFO] NSIS脚本信息:
findstr "APPNAME\|VERSIONMAJOR\|VERSIONMINOR" installer.nsi 2>nul

:: 生成64位安装包
echo.
echo [INFO] 生成64位安装包...
echo ============================================
makensis -DARCH=x64 installer.nsi
if %errorlevel% neq 0 (
    echo [ERROR] 生成64位安装包失败
    echo 请检查installer.nsi文件和deploy目录内容
    set PACKAGE_FAILED=1
) else (
    echo [OK] 64位安装包生成成功
)

:: 生成32位安装包
echo.
echo [INFO] 生成32位安装包...
echo ============================================
makensis -DARCH=x86 installer.nsi
if %errorlevel% neq 0 (
    echo [WARNING] 生成32位安装包失败（可能是架构不匹配）
    echo 如果只需要64位版本，可以忽略此警告
) else (
    echo [OK] 32位安装包生成成功
)

:: 检查生成结果
echo.
echo [INFO] 检查生成的安装包...
if exist "Output\*.exe" (
    echo ============================================
    echo [SUCCESS] 安装包生成完成！
    echo ============================================
    echo.
    echo 生成的安装包：
    for %%f in (Output\*.exe) do (
        echo   - %%f (%%~zf bytes)
    )
    echo.
    
    :: 显示安装包详细信息
    echo 安装包详细信息：
    for %%f in (Output\*.exe) do (
        echo ----------------------------------------
        echo 文件: %%~nxf
        echo 大小: %%~zf bytes
        echo 路径: %%~dpf
        echo 修改时间: %%~tf
        echo ----------------------------------------
    )
    
    :: 生成校验和文件
    echo [INFO] 生成校验和文件...
    echo # 安装包校验和 > Output\checksums.txt
    echo # 生成时间: %date% %time% >> Output\checksums.txt
    echo. >> Output\checksums.txt
    
    for %%f in (Output\*.exe) do (
        echo 正在计算 %%~nxf 的校验和...
        certutil -hashfile "%%f" SHA256 | findstr /v "SHA256" | findstr /v "CertUtil" >> Output\checksums.txt
        echo %%~nxf >> Output\checksums.txt
        echo. >> Output\checksums.txt
    )
    
    echo [OK] 校验和文件已生成: Output\checksums.txt
    
) else (
    echo [ERROR] 未找到生成的安装包文件
    set PACKAGE_FAILED=1
)

:: 创建发布说明模板
echo [INFO] 创建发布说明模板...
echo # 智多分机考桌面端 发布包 > Output\README.txt
echo. >> Output\README.txt
echo 生成时间: %date% %time% >> Output\README.txt
echo. >> Output\README.txt
echo ## 系统要求 >> Output\README.txt
echo - Windows 7/10/11 (64位推荐) >> Output\README.txt
echo - Visual C++ Redistributable 2015-2019 >> Output\README.txt
echo - 至少 4GB RAM >> Output\README.txt
echo - 至少 200MB 可用磁盘空间 >> Output\README.txt
echo. >> Output\README.txt
echo ## 安装说明 >> Output\README.txt
echo 1. 右键点击安装包，选择"以管理员身份运行" >> Output\README.txt
echo 2. 按照安装向导完成安装 >> Output\README.txt
echo 3. 安装完成后，桌面会出现快捷方式 >> Output\README.txt
echo. >> Output\README.txt
echo ## 配置说明 >> Output\README.txt
echo - 配置文件位置: 安装目录\config.json >> Output\README.txt
echo - 首次运行前请修改配置文件中的URL和密码 >> Output\README.txt
echo. >> Output\README.txt
echo ## 联系支持 >> Output\README.txt
echo 如遇问题，请联系技术支持 >> Output\README.txt

if defined PACKAGE_FAILED (
    echo.
    echo [ERROR] 安装包生成过程中出现错误
    echo 请检查上述错误信息并重试
    pause
    exit /b 1
)

:: 安全检查建议
echo.
echo ============================================
echo [INFO] 发布前检查清单
echo ============================================
echo.
echo 安全检查：
echo [ ] 安装包已通过病毒扫描
echo [ ] 在干净的测试环境中验证安装和运行
echo [ ] 配置文件中的默认密码已修改
echo [ ] 版本号和应用名称正确
echo.
echo 分发准备：
echo [ ] 准备数字签名证书（推荐）
echo [ ] 准备发布说明文档
echo [ ] 测试不同Windows版本的兼容性
echo.
echo 生成的文件已保存在 Output 目录中
echo 建议对安装包进行数字签名以提高用户信任度
echo.

pause