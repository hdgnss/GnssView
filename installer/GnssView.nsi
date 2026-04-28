!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

Unicode True
RequestExecutionLevel admin

!ifndef APP_NAME
  !define APP_NAME "GnssView"
!endif

!ifndef APP_VERSION
  !define APP_VERSION "0.1.0"
!endif

!ifndef COMPANY_NAME
  !define COMPANY_NAME "hdgnss"
!endif

!ifndef INSTALL_DIR
  !define INSTALL_DIR "C:\Program Files\hdgnss"
!endif

!ifndef STAGING_DIR
  !error "STAGING_DIR must be defined"
!endif

!ifndef OUTPUT_EXE
  !error "OUTPUT_EXE must be defined"
!endif

Name "${APP_NAME}"
OutFile "${OUTPUT_EXE}"
InstallDir "${INSTALL_DIR}"
InstallDirRegKey HKLM "Software\${COMPANY_NAME}\${APP_NAME}" "InstallDir"
ShowInstDetails show
ShowUninstDetails show
SetCompressor /SOLID lzma
SetOverwrite on
BrandingText "${COMPANY_NAME}"

!define MUI_ABORTWARNING
!define MUI_ICON "${STAGING_DIR}\assets\gnssview-icon.ico"
!define MUI_UNICON "${STAGING_DIR}\assets\gnssview-icon.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "English"

Var CheckOutput

Function EnsureGnssViewClosed
  nsExec::ExecToStack 'cmd /c tasklist /FI "IMAGENAME eq GnssView.exe" | find /I "GnssView.exe"'
  Pop $0
  Pop $CheckOutput

  ${If} $0 == 0
    MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION "${APP_NAME} is running. Setup must close it before continuing. Click OK to close it automatically, or Cancel to exit setup." IDCANCEL cancel
    nsExec::ExecToStack 'cmd /c taskkill /IM GnssView.exe /F'
    Pop $1
    Pop $CheckOutput
    ${If} $1 != 0
      MessageBox MB_ICONSTOP|MB_OK "Unable to close ${APP_NAME}. Please exit it manually and try again.$\r$\n$\r$\n$CheckOutput"
      Abort
    ${EndIf}
    Sleep 1000
  ${EndIf}

  Return

cancel:
  Abort
FunctionEnd

Function un.EnsureGnssViewClosed
  nsExec::ExecToStack 'cmd /c tasklist /FI "IMAGENAME eq GnssView.exe" | find /I "GnssView.exe"'
  Pop $0
  Pop $CheckOutput

  ${If} $0 == 0
    MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION "${APP_NAME} is running. Uninstall must close it before continuing. Click OK to close it automatically, or Cancel to exit uninstaller." IDCANCEL cancel
    nsExec::ExecToStack 'cmd /c taskkill /IM GnssView.exe /F'
    Pop $1
    Pop $CheckOutput
    ${If} $1 != 0
      MessageBox MB_ICONSTOP|MB_OK "Unable to close ${APP_NAME}. Please exit it manually and try again.$\r$\n$\r$\n$CheckOutput"
      Abort
    ${EndIf}
    Sleep 1000
  ${EndIf}

  Return

cancel:
  Abort
FunctionEnd

Function .onInit
  ${If} ${RunningX64}
    SetRegView 64
    StrCpy $INSTDIR "${INSTALL_DIR}"
  ${EndIf}
  Call EnsureGnssViewClosed
FunctionEnd

Function un.onInit
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  Call un.EnsureGnssViewClosed
FunctionEnd

Section "Application Files" SEC_APP
  SectionIn RO
  SetOutPath "$INSTDIR"
  File /r "${STAGING_DIR}\*"

  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateDirectory "$SMPROGRAMS\${COMPANY_NAME}"
  CreateShortcut "$SMPROGRAMS\${COMPANY_NAME}\${APP_NAME}.lnk" "$INSTDIR\GnssView.exe" "" "$INSTDIR\GnssView.exe" 0
  CreateShortcut "$SMPROGRAMS\${COMPANY_NAME}\Uninstall ${APP_NAME}.lnk" "$INSTDIR\Uninstall.exe"

  WriteRegStr HKLM "Software\${COMPANY_NAME}\${APP_NAME}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${COMPANY_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "EstimatedSize" "$0"
SectionEnd

Section /o "Desktop Shortcut" SEC_DESKTOP
  CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\GnssView.exe" "" "$INSTDIR\GnssView.exe" 0
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\${COMPANY_NAME}\${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\${COMPANY_NAME}\Uninstall ${APP_NAME}.lnk"
  RMDir "$SMPROGRAMS\${COMPANY_NAME}"

  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
  DeleteRegKey HKLM "Software\${COMPANY_NAME}\${APP_NAME}"
SectionEnd
