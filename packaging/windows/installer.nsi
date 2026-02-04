; ScratchRobin Windows Installer
; NSIS Script

!define PRODUCT_NAME "ScratchRobin"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "Dalton Calford"
!define PRODUCT_WEB_SITE "https://github.com/DaltonCalford/ScratchRobin"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\scratchrobin.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

SetCompressor lzma

; MUI Settings
!include "MUI2.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME

; License page
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"

; Directory page
!insertmacro MUI_PAGE_DIRECTORY

; Instfiles page
!insertmacro MUI_PAGE_INSTFILES

; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\scratchrobin.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; Installer sections
Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  
  ; Main executable
  File "..\..\build-windows\Release\scratchrobin.exe"
  
  ; DLLs
  File "..\..\build-windows\Release\*.dll"
  
  ; Create shortcuts
  CreateDirectory "$SMPROGRAMS\ScratchRobin"
  CreateShortcut "$SMPROGRAMS\ScratchRobin\ScratchRobin.lnk" "$INSTDIR\scratchrobin.exe"
  CreateShortcut "$DESKTOP\ScratchRobin.lnk" "$INSTDIR\scratchrobin.exe"
  
  ; Store installation folder
  WriteRegStr HKCU "Software\ScratchRobin" "" $INSTDIR
  
  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\scratchrobin.exe"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

Section -AdditionalIcons
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortcut "$SMPROGRAMS\ScratchRobin\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortcut "$SMPROGRAMS\ScratchRobin\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section -Post
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\scratchrobin.exe"
SectionEnd

; Uninstaller sections
Section Uninstall
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$INSTDIR\scratchrobin.exe"
  Delete "$INSTDIR\*.dll"
  
  Delete "$SMPROGRAMS\ScratchRobin\Uninstall.lnk"
  Delete "$SMPROGRAMS\ScratchRobin\Website.lnk"
  Delete "$SMPROGRAMS\ScratchRobin\ScratchRobin.lnk"
  Delete "$DESKTOP\ScratchRobin.lnk"
  
  RMDir "$SMPROGRAMS\ScratchRobin"
  RMDir "$INSTDIR"
  
  DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  DeleteRegKey HKCU "Software\ScratchRobin"
SectionEnd
