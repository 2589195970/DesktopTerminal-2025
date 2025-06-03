; NSIS 安装脚本
; 使用 Unicode 版本
Unicode true

!define APPNAME "zdf-exam-desktop"
!define COMPANYNAME "智多分"
!define DESCRIPTION "教育考试场景专用安全浏览器"
!define VERSIONMAJOR 1
!define VERSIONMINOR 0
!define VERSIONPATCH 0

; 架构设置 - 默认为x64，可通过命令行参数 -DARCH=x86 覆盖
!ifndef ARCH
  !define ARCH "x64"
!endif

; 根据架构设置安装路径和输出文件名
!if "${ARCH}" == "x86"
  !define INSTALL_DIR "$PROGRAMFILES32\${COMPANYNAME}\${APPNAME}"
  !define OUTPUT_FILE "Output\zdf-exam-desktop-setup-x86.exe"
  !define ARCH_SUFFIX " (32位)"
!else
  !define INSTALL_DIR "$PROGRAMFILES64\${COMPANYNAME}\${APPNAME}"
  !define OUTPUT_FILE "Output\zdf-exam-desktop-setup.exe"
  !define ARCH_SUFFIX ""
!endif

; 引入现代UI
!include "MUI2.nsh"

; 安装程序基本设置
Name "${APPNAME}${ARCH_SUFFIX}"
OutFile "${OUTPUT_FILE}"
InstallDir "${INSTALL_DIR}"
InstallDirRegKey HKLM "Software\${COMPANYNAME}\${APPNAME}" "InstallDir"
RequestExecutionLevel admin

; 界面设置
!define MUI_ICON "resources\simple_icon.ico"
!define MUI_UNICON "resources\simple_icon.ico"
; 暂时不使用 installer-banner.bmp，因为可能有格式问题
; !define MUI_WELCOMEFINISHPAGE_BITMAP "resources\installer-banner.bmp"

; 页面定义 - 不包含许可协议页面
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; 卸载页面
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; 语言设置
!insertmacro MUI_LANGUAGE "SimpChinese"

; 安装部分
Section "主程序" SecMain
    SetOutPath "$INSTDIR"
    
    ; 复制主程序和依赖
    File /r "deploy\*.*"
    
    ; 复制配置文件和图标
    CreateDirectory "$INSTDIR\resources"
    
    ; 只在配置文件不存在时才复制默认配置
    IfFileExists "$INSTDIR\config.json" +2
        File /oname=config.json "resources\config.json"
    
    ; 始终复制资源文件夹中的配置（作为备份/参考）
    File /oname=resources\config.json.default "resources\config.json"
    File /oname=resources\simple_icon.ico "resources\simple_icon.ico"
    
    ; 写注册表
    WriteRegStr HKLM "Software\${COMPANYNAME}\${APPNAME}" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}${ARCH_SUFFIX}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayIcon" "$INSTDIR\zdf-exam-desktop.exe,0"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Publisher" "${COMPANYNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONPATCH}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoRepair" 1
    
    ; 创建卸载程序
    WriteUninstaller "$INSTDIR\uninstall.exe"
    
    ; 创建开始菜单快捷方式
    CreateDirectory "$SMPROGRAMS\${APPNAME}${ARCH_SUFFIX}"
    CreateShortcut "$SMPROGRAMS\${APPNAME}${ARCH_SUFFIX}\${APPNAME}${ARCH_SUFFIX}.lnk" "$INSTDIR\zdf-exam-desktop.exe" "" "$INSTDIR\resources\simple_icon.ico" 0
    CreateShortcut "$SMPROGRAMS\${APPNAME}${ARCH_SUFFIX}\卸载.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\resources\simple_icon.ico" 0
    
    ; 创建桌面快捷方式
    CreateShortcut "$DESKTOP\${APPNAME}${ARCH_SUFFIX}.lnk" "$INSTDIR\zdf-exam-desktop.exe" "" "$INSTDIR\resources\simple_icon.ico" 0
    
    ; 显示安装完成消息
    MessageBox MB_OK "安装完成！桌面快捷方式已创建。"
SectionEnd

; 卸载部分
Section "Uninstall"
    ; 删除文件
    Delete "$INSTDIR\*.*"
    RMDir /r "$INSTDIR"
    
    ; 删除快捷方式 - 直接使用架构后缀
    !if "${ARCH}" == "x86"
        Delete "$SMPROGRAMS\${APPNAME} (32位)\*.*"
        RMDir "$SMPROGRAMS\${APPNAME} (32位)"
        Delete "$DESKTOP\${APPNAME} (32位).lnk"
    !else
        Delete "$SMPROGRAMS\${APPNAME}\*.*"
        RMDir "$SMPROGRAMS\${APPNAME}"
        Delete "$DESKTOP\${APPNAME}.lnk"
    !endif
    
    ; 删除注册表项
    DeleteRegKey HKLM "Software\${COMPANYNAME}\${APPNAME}"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
SectionEnd 