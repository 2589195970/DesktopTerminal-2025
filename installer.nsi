; NSIS 安装脚本
!define APPNAME "机考霸屏桌面端"
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
  !define OUTPUT_FILE "Output\qt-shell-setup-x86.exe"
  !define ARCH_SUFFIX " (32位)"
!else
  !define INSTALL_DIR "$PROGRAMFILES64\${COMPANYNAME}\${APPNAME}"
  !define OUTPUT_FILE "Output\qt-shell-setup.exe"
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
!define MUI_ICON "resources\logo.ico"
!define MUI_UNICON "resources\logo.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "resources\installer-banner.bmp"

; 页面
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "qt-shell\LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; 语言
!insertmacro MUI_LANGUAGE "SimpChinese"

; 安装部分
Section "主程序" SecMain
    SetOutPath "$INSTDIR"
    
    ; 复制主程序和依赖
    File /r "deploy\*.*"
    
    ; 复制配置文件
    CreateDirectory "$INSTDIR\resources"
    File /oname=resources\config.json "resources\config.json"
    
    ; 写注册表
    WriteRegStr HKLM "Software\${COMPANYNAME}\${APPNAME}" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}${ARCH_SUFFIX}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayIcon" "$INSTDIR\qt-shell.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Publisher" "${COMPANYNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONPATCH}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoRepair" 1
    
    ; 创建卸载程序
    WriteUninstaller "$INSTDIR\uninstall.exe"
    
    ; 创建开始菜单快捷方式
    CreateDirectory "$SMPROGRAMS\${APPNAME}${ARCH_SUFFIX}"
    CreateShortcut "$SMPROGRAMS\${APPNAME}${ARCH_SUFFIX}\${APPNAME}${ARCH_SUFFIX}.lnk" "$INSTDIR\qt-shell.exe"
    CreateShortcut "$SMPROGRAMS\${APPNAME}${ARCH_SUFFIX}\卸载.lnk" "$INSTDIR\uninstall.exe"
    
    ; 创建桌面快捷方式
    CreateShortcut "$DESKTOP\${APPNAME}${ARCH_SUFFIX}.lnk" "$INSTDIR\qt-shell.exe"
SectionEnd

; 卸载部分
Section "Uninstall"
    ; 删除文件
    Delete "$INSTDIR\*.*"
    RMDir /r "$INSTDIR"
    
    ; 读取架构后缀
    ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName"
    ${StrRep} $0 $0 "${APPNAME}" ""
    
    ; 删除快捷方式
    Delete "$SMPROGRAMS\${APPNAME}$0\*.*"
    RMDir "$SMPROGRAMS\${APPNAME}$0"
    Delete "$DESKTOP\${APPNAME}$0.lnk"
    
    ; 删除注册表项
    DeleteRegKey HKLM "Software\${COMPANYNAME}\${APPNAME}"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
SectionEnd

; 字符串替换函数
Function StrRep
  Exch $R1 ; $R1=substring to replace
  Exch
  Exch $R2 ; $R2=string to replace in
  Push $R3 ; $R3=replacement string
  Push $R4 ; $R4=counter
  Push $R5 ; $R5=len(R1)
  Push $R6 ; $R6=len(R3)
  Push $R7 ; $R7=temp
  StrCpy $R4 0
  StrLen $R5 $R1
  StrLen $R6 $R3
  
  loop:
    StrCpy $R7 $R2 $R5 $R4
    StrCmp $R7 $R1 found
    StrCmp $R4 $R2 done
    IntOp $R4 $R4 + 1
    Goto loop
  
  found:
    StrCpy $R7 $R2 $R4
    IntOp $R8 $R4 + $R5
    StrCpy $R9 $R2 "" $R8
    StrCpy $R2 $R7$R3$R9
    IntOp $R4 $R4 + $R6
    Goto loop
  
  done:
    Pop $R7
    Pop $R6
    Pop $R5
    Pop $R4
    Pop $R3
    Push $R2
    Exch
    Pop $R1
FunctionEnd 