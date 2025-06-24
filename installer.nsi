; NSIS 安装脚本（请以 UTF-8 无 BOM 保存）
Unicode true

; 包含必要的头文件
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"
!include "x64.nsh"

; ─────────────────────────────────────────────
; 常量定义
; ─────────────────────────────────────────────
!define APPNAME        "智多分-机考桌面端"
!define COMPANYNAME    "智多分"
!define DESCRIPTION    "教育考试场景专用安全浏览器"
!define VERSIONMAJOR   1
!define VERSIONMINOR   0
!define VERSIONPATCH   0

; 架构：默认为 x64，可在命令行 -DARCH=x86 覆盖
!ifndef ARCH
  !define ARCH "x64"
!endif

!if "${ARCH}" == "x86"
  !define INSTALL_DIR "$PROGRAMFILES32\${COMPANYNAME}\${APPNAME}"
  !define OUTPUT_FILE "Output\${APPNAME}-setup-x86.exe"
!else
  !define INSTALL_DIR "$PROGRAMFILES64\${COMPANYNAME}\${APPNAME}"
  !define OUTPUT_FILE "Output\${APPNAME}-setup.exe"
!endif

; ─────────────────────────────────────────────
; 界面与全局设置
; ─────────────────────────────────────────────
Name "${APPNAME}"
OutFile "${OUTPUT_FILE}"
InstallDir "${INSTALL_DIR}"
InstallDirRegKey HKLM "Software\${COMPANYNAME}\${APPNAME}" "InstallDir"
RequestExecutionLevel admin

; MUI2
!define MUI_ICON    "resources\logo.ico"
!define MUI_UNICON  "resources\logo.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; 必须在页面设置之后添加语言设置
!insertmacro MUI_LANGUAGE "SimpChinese"

; 语言字符串定义
LangString MSG_InstallDone ${LANG_SIMPCHINESE} "安装完成！桌面快捷方式已创建。"
LangString MSG_UninstallDone ${LANG_SIMPCHINESE} "卸载完成，相关文件已全部移除。"

; ─────────────────────────────────────────────
; 安装前检查
; ─────────────────────────────────────────────
Function .onInit
    ; 检查管理员权限
    UserInfo::GetAccountType
    Pop $R0
    ${If} $R0 != "admin"
        MessageBox MB_ICONSTOP "此程序需要管理员权限才能安装，请右键选择'以管理员身份运行'。"
        SetErrorLevel 740 ; ERROR_ELEVATION_REQUIRED
        Quit
    ${EndIf}
    
    ; 检查系统架构
    ${If} "${ARCH}" == "x64"
        ${IfNot} ${RunningX64}
            MessageBox MB_ICONSTOP "此安装包仅适用于64位Windows系统。"
            Quit
        ${EndIf}
    ${EndIf}
    
    ; 检查操作系统版本
    ${GetWindowsVersion} $R0 $R1
    ${If} $R0 == "95"
    ${OrIf} $R0 == "98"
    ${OrIf} $R0 == "ME"
    ${OrIf} $R0 == "NT"
        MessageBox MB_ICONSTOP "不支持的操作系统版本，需要Windows 7或更高版本。"
        Quit
    ${EndIf}
    
    ; 检查是否已安装
    ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString"
    ${If} $R0 != ""
        MessageBox MB_YESNO|MB_ICONQUESTION "检测到已安装${APPNAME}，是否要先卸载旧版本？" IDYES uninstall_old IDNO continue_install
        uninstall_old:
            ExecWait '$R0 /S _?=$INSTDIR'
            Delete $R0
            RMDir "$INSTDIR"
        continue_install:
    ${EndIf}
FunctionEnd

; ─────────────────────────────────────────────
; 安装部分
; ─────────────────────────────────────────────
Section "主程序" SecMain
    SetOutPath "$INSTDIR"
    
    ; 检查deploy目录是否存在
    ${If} ${FileExists} "deploy\zdf-exam-desktop.exe"
        DetailPrint "正在安装应用程序文件..."
        ; 拷贝应用及依赖
        File /r "deploy\*.*"
    ${Else}
        MessageBox MB_ICONSTOP "安装包损坏：找不到应用程序文件。请重新下载安装包。"
        Abort
    ${EndIf}

    ; 资源目录
    CreateDirectory "$INSTDIR\resources"
    DetailPrint "正在配置应用程序..."
    
    ; config.json：仅首次安装时复制，保护用户配置
    ${If} ${FileExists} "$INSTDIR\config.json"
        DetailPrint "保留现有配置文件"
    ${Else}
        ${If} ${FileExists} "resources\config.json"
            File /oname=config.json "resources\config.json"
            DetailPrint "已安装默认配置文件"
        ${Else}
            ; 创建基本配置文件
            FileOpen $0 "$INSTDIR\config.json" w
            FileWrite $0 '{"url":"https://example.com","exitPassword":"admin123","appName":"智多分机考桌面端"}'
            FileClose $0
            DetailPrint "已创建默认配置文件"
        ${EndIf}
    ${EndIf}
    
    ; 始终复制一份默认配置作为备份
    ${If} ${FileExists} "resources\config.json"
        File /oname=resources\config.json.default "resources\config.json"
    ${EndIf}
    
    ; 复制图标文件
    ${If} ${FileExists} "resources\logo.ico"
        File /oname=resources\logo.ico "resources\logo.ico"
    ${ElseIf} ${FileExists} "resources\logo.png"
        File /oname=resources\logo.png "resources\logo.png"
        DetailPrint "使用PNG格式图标"
    ${Else}
        DetailPrint "警告：未找到应用图标文件"
    ${EndIf}

    ; 注册表写入
    WriteRegStr HKLM "Software\${COMPANYNAME}\${APPNAME}" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName"       "${APPNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayVersion"    "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONPATCH}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Publisher"         "${COMPANYNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayIcon"       "$INSTDIR\zdf-exam-desktop.exe,0"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString"   "$INSTDIR\uninstall.exe"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoRepair" 1

    ; 卸载器
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; 开始菜单
    CreateDirectory "$SMPROGRAMS\${APPNAME}"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\zdf-exam-desktop.exe" "" "$INSTDIR\resources\logo.ico" 0
    CreateShortcut "$SMPROGRAMS\${APPNAME}\卸载.lnk"      "$INSTDIR\uninstall.exe"        "" "$INSTDIR\resources\logo.ico" 0

    ; 桌面快捷方式
    ${If} ${FileExists} "$INSTDIR\resources\logo.ico"
        CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\zdf-exam-desktop.exe" "" "$INSTDIR\resources\logo.ico" 0
    ${Else}
        CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\zdf-exam-desktop.exe"
    ${EndIf}
    
    ; 安装完成后验证
    DetailPrint "正在验证安装..."
    ${If} ${FileExists} "$INSTDIR\zdf-exam-desktop.exe"
    ${AndIf} ${FileExists} "$INSTDIR\Qt5Core.dll"
    ${AndIf} ${FileExists} "$INSTDIR\config.json"
        DetailPrint "安装验证成功"
        
        ; 检查VC++运行时
        ClearErrors
        ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Version"
        ${If} ${Errors}
            MessageBox MB_YESNO|MB_ICONQUESTION "未检测到Visual C++ Redistributable，这可能导致程序无法运行。$\n$\n是否现在打开Microsoft官网下载页面？" IDYES open_vc_download IDNO skip_vc_download
            open_vc_download:
                ExecShell "open" "https://aka.ms/vs/17/release/vc_redist.x64.exe"
            skip_vc_download:
        ${EndIf}
        
        MessageBox MB_OK "$(MSG_InstallDone)$\n$\n注意：首次运行前请修改配置文件中的URL和密码设置。"
    ${Else}
        MessageBox MB_ICONSTOP "安装验证失败！某些关键文件可能未正确安装。"
    ${EndIf}
SectionEnd

; ─────────────────────────────────────────────
; 卸载部分
; ─────────────────────────────────────────────
Section "Uninstall"
    ; 删除文件与目录
    RMDir /r "$INSTDIR"

    ; 删除快捷方式
    RMDir /r "$SMPROGRAMS\${APPNAME}"
    Delete "$DESKTOP\${APPNAME}.lnk"

    ; 删除注册表
    DeleteRegKey HKLM "Software\${COMPANYNAME}\${APPNAME}"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

    MessageBox MB_OK "$(MSG_UninstallDone)"
SectionEnd
