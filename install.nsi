; Script generated by the HM NIS Edit Script Wizard.

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "GnssView"
!define PRODUCT_PUBLISHER "HDGNSS"
!define PRODUCT_WEB_SITE "http://www.hdgnss.com"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "resources\icon.ico"
; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "license.txt"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Start menu page
var ICONS_GROUP
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "hdgnss"
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\GnssView.exe"
!insertmacro MUI_PAGE_FINISH

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------


Name "${PRODUCT_NAME}"
OutFile "${PRODUCT_NAME}_setup.exe"
InstallDir "$PROGRAMFILES64\hdgnss"
;InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite try
  SetOutPath "$INSTDIR"
  File "build\Release\GnssView.exe"
  File "build\Release\D3Dcompiler_47.dll"
  File "build\Release\Qt6Core.dll"
  File "build\Release\Qt6OpenGL.dll"
  File "build\Release\Qt6SerialPort.dll"
  File "build\Release\Qt6Gui.dll"
  File "build\Release\Qt6OpenGLWidgets.dll"
  File "build\Release\Qt6Svg.dll"
  File "build\Release\opengl32sw.dll"
  File "build\Release\Qt6Charts.dll"
  File "build\Release\Qt6Network.dll"
  File "build\Release\Qt6Widgets.dll"
  SetOutPath "$INSTDIR\iconengines"
  File "build\Release\iconengines\qsvgicon.dll"
  SetOutPath "$INSTDIR\imageformats"
  File "build\Release\imageformats\qgif.dll"
  File "build\Release\imageformats\qico.dll"
  File "build\Release\imageformats\qjpeg.dll"
  File "build\Release\imageformats\qsvg.dll"
  SetOutPath "$INSTDIR\networkinformation"
  File "build\Release\networkinformation\qnetworklistmanager.dll"
  SetOutPath "$INSTDIR\platforms"
  File "build\Release\platforms\qwindows.dll"
  SetOutPath "$INSTDIR\styles"
  File "build\Release\styles\qmodernwindowsstyle.dll"
  SetOutPath "$INSTDIR\tls"
  File "build\Release\tls\qcertonlybackend.dll"
  File "build\Release\tls\qschannelbackend.dll"
  SetOutPath "$INSTDIR\generic"
  File "build\Release\generic\qtuiotouchplugin.dll"

; Shortcuts
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
  CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\GnssView.lnk" "$INSTDIR\GnssView.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd






