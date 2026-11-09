; Zankō 2006 — Windows installer (NSIS 3). Build: makensis -DVER=0.6.0 installer/zanko2006.nsi
; Per-user install (no administrator prompt) so the launcher can update the game silently.
; Saves and accounts (Data\Userdata) and the player's settings (Data\config.ini, Data\launcher.ini) are never overwritten or removed by updates.
Unicode true
!ifndef VER
  !define VER "0.6.0"
!endif
!define APP "Zankō 2006"
!define KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Zanko2006"
Name "${APP}"
OutFile "..\pkg\Zanko2006_Setup_v${VER}.exe"
InstallDir "$LOCALAPPDATA\Zanko2006"
InstallDirRegKey HKCU "Software\Zanko2006" "InstallDir"
RequestExecutionLevel user
SetCompressor /SOLID lzma
Icon "..\art\icon.ico"
UninstallIcon "..\art\icon.ico"
BrandingText "${APP} v${VER}"
VIProductVersion "${VER}.0"
VIAddVersionKey "ProductName" "${APP}"
VIAddVersionKey "FileDescription" "${APP} installer"
VIAddVersionKey "FileVersion" "${VER}"
VIAddVersionKey "ProductVersion" "${VER}"
VIAddVersionKey "LegalCopyright" "FACKERSK"

!include "MUI2.nsh"
!include "FileFunc.nsh"
!define MUI_ICON "..\art\icon.ico"
!define MUI_UNICON "..\art\icon.ico"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TITLE "${APP}  v${VER}"
!define MUI_WELCOMEPAGE_TEXT "Japan, 2006. The bright, green future did not arrive.$\r$\n$\r$\nThis installs ${APP} for your Windows account. No administrator rights are needed. The launcher keeps the game up to date by itself.$\r$\n$\r$\nSaves and accounts are kept when you update or reinstall."
!define MUI_FINISHPAGE_RUN "$INSTDIR\Root\Launcher.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Start the launcher"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Var UPDATE
Function .onInit
  StrCpy $UPDATE 0
  ${GetParameters} $0
  ${GetOptions} $0 "/UPDATE" $1
  IfErrors +2 0
    StrCpy $UPDATE 1
  StrCmp $UPDATE 1 0 done
    Sleep 2500   ; launched by the launcher: give it time to close so Launcher.exe can be replaced
  done:
FunctionEnd

Section "Game" SecGame
  SectionIn RO
  SetOverwrite on
  SetOutPath "$INSTDIR\Root"
  File "..\pkg\Zanko2006\Root\Zanko.exe"
  File "..\pkg\Zanko2006\Root\Launcher.exe"
  File "..\pkg\Zanko2006\Root\ModMaker.exe"
  SetOutPath "$INSTDIR"
  File /r /x Userdata /x config.ini /x launcher.ini "..\pkg\Zanko2006\Data"
  File /r "..\pkg\Zanko2006\Textures"
  File /r "..\pkg\Zanko2006\Mods"
  File "..\pkg\Zanko2006\README.txt"
  SetOverwrite off   ; the player's own settings stay as they are
  SetOutPath "$INSTDIR\Data"
  File "..\pkg\Zanko2006\Data\config.ini"
  File "..\pkg\Zanko2006\Data\launcher.ini"
  SetOverwrite on
  CreateDirectory "$INSTDIR\Data\Userdata"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKCU "Software\Zanko2006" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "Software\Zanko2006" "Version" "${VER}"
  WriteRegStr HKCU "${KEY}" "DisplayName" "${APP}"
  WriteRegStr HKCU "${KEY}" "DisplayVersion" "${VER}"
  WriteRegStr HKCU "${KEY}" "Publisher" "FACKERSK"
  WriteRegStr HKCU "${KEY}" "DisplayIcon" "$INSTDIR\Root\Launcher.exe"
  WriteRegStr HKCU "${KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKCU "${KEY}" "URLInfoAbout" "https://github.com/FACKERSK/Zanko-2006"
  WriteRegDWORD HKCU "${KEY}" "NoModify" 1
  WriteRegDWORD HKCU "${KEY}" "NoRepair" 1
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKCU "${KEY}" "EstimatedSize" "$0"
SectionEnd

Section "Desktop shortcut" SecDesk
  StrCmp $UPDATE 1 +3   ; updates keep whatever shortcuts the player chose
    SetOutPath "$INSTDIR\Root"
    CreateShortcut "$DESKTOP\${APP}.lnk" "$INSTDIR\Root\Launcher.exe" "" "$INSTDIR\Root\Launcher.exe" 0
SectionEnd

Section "Start menu shortcuts" SecStart
  StrCmp $UPDATE 1 done
    SetOutPath "$INSTDIR\Root"
    CreateDirectory "$SMPROGRAMS\${APP}"
    CreateShortcut "$SMPROGRAMS\${APP}\${APP}.lnk" "$INSTDIR\Root\Launcher.exe" "" "$INSTDIR\Root\Launcher.exe" 0
    CreateShortcut "$SMPROGRAMS\${APP}\${APP} Mod Maker.lnk" "$INSTDIR\Root\ModMaker.exe" "" "$INSTDIR\Root\ModMaker.exe" 0
    CreateShortcut "$SMPROGRAMS\${APP}\Uninstall ${APP}.lnk" "$INSTDIR\Uninstall.exe"
  done:
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecGame} "The game, the launcher and their data (required)."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesk} "A ${APP} icon on the desktop."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecStart} "${APP} and its uninstaller in the Start menu."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Function .onInstSuccess
  StrCmp $UPDATE 1 0 +3
    SetOutPath "$INSTDIR\Root"
    Exec '"$INSTDIR\Root\Launcher.exe"'   ; silent update finished: reopen the launcher
FunctionEnd

Section "Uninstall"
  RMDir /r "$INSTDIR\Root"
  RMDir /r "$INSTDIR\Versions"
  RMDir /r "$INSTDIR\Textures"
  RMDir /r "$INSTDIR\Mods"
  RMDir /r "$INSTDIR\Data\Models"
  RMDir /r "$INSTDIR\Data\Audio"
  RMDir /r "$INSTDIR\Data\UI"
  Delete "$INSTDIR\Data\news.txt"
  Delete "$INSTDIR\Data\version.txt"
  Delete "$INSTDIR\Data\update_manifest.txt"
  Delete "$INSTDIR\README.txt"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$SMPROGRAMS\${APP}\${APP}.lnk"
  Delete "$SMPROGRAMS\${APP}\Uninstall ${APP}.lnk"
  RMDir "$SMPROGRAMS\${APP}"
  Delete "$DESKTOP\${APP}.lnk"
  DeleteRegKey HKCU "${KEY}"
  DeleteRegKey HKCU "Software\Zanko2006"
  ; saves, accounts and settings in Data\Userdata stay; the folder goes only when it is empty
  RMDir "$INSTDIR\Data"
  RMDir "$INSTDIR"
SectionEnd
