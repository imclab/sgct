!include MUI2.nsh
!include EnvVarUpdate.nsh
!include FileAssociation.nsh

!define SGCT_VERSION "0.9.2"

; The name of the installer
Name "SGCT ${SGCT_VERSION} msvc9 x86 installer"

!define REALMSG "$\nOriginal non-restricted account type: $2"
!define env_hklm 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
!define MUI_COMPONENTSPAGE_NODESC
!define MUI_WELCOMEPAGE_TEXT "Simple Graphics Cluster Toolkit (SGCT) is a static cross-platform C++ library for developing OpenGL applications."
!define MUI_WELCOMEFINISHPAGE_BITMAP "sgct.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH

Section
	ClearErrors
	UserInfo::GetName
	
	Pop $0
	UserInfo::GetAccountType
	Pop $1
	# GetOriginalAccountType will check the tokens of the original user of the
	# current thread/process. If the user tokens were elevated or limited for
	# this process, GetOriginalAccountType will return the non-restricted
	# account type.
	# On Vista with UAC, for example, this is not the same value when running
	# with `RequestExecutionLevel user`. GetOriginalAccountType will return
	# "admin" while GetAccountType will return "user".
	UserInfo::GetOriginalAccountType
	Pop $2
	StrCmp $1 "Admin" 0 +2
		Goto done
	StrCmp $1 "Power" 0 +3
		MessageBox MB_OK "Administrator permission is required to install this software.$\nThe installer will now exit."
		Quit
	StrCmp $1 "User" 0 +3
		MessageBox MB_OK "Administrator permission is required to install this software.$\nThe installer will now exit."
		Quit
	StrCmp $1 "Guest" 0 +3
		MessageBox MB_OK "Administrator permission is required to install this software.$\nThe installer will now exit."
		Quit
	MessageBox MB_OK "Administrator permission is required to install this software.$\nThe installer will now exit."
		Quit
		
	done:
SectionEnd

; The file to write
OutFile "..\..\bin\installers\SGCT_${SGCT_VERSION}_msvc9_setup.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\SGCT"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\SGCT" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages

!define MUI_ICON "C_transparent.ico"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "License.rtf"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

;--------------------------------

Section "SGCT ${SGCT_VERSION} MSVC9 x86"
	SectionIn RO
	
	; Write the installation path into the registry
	WriteRegStr HKLM "SOFTWARE\SGCT" "Install_Dir" "$INSTDIR"
	  
	; Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT" "DisplayName" "SGCT ${SGCT_VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT" "NoRepair" 1
	
	SetOutPath $INSTDIR
	writeUninstaller "$INSTDIR\uninstall.exe"
	
	File C-Student_wiki.url
	File SGCT_tutorials.url
	File C_transparent.ico
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\include"
	File /r "..\..\include\"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\lib\msvc9"
	File "..\..\lib\msvc9\sgct32.lib"
	File "..\..\lib\msvc9\sgct32_d.lib"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\config"
	File /r "..\..\config\"
	
	#add examples
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\sgct_template"
	File "..\..\src\apps\sgct_template\main.cpp"
	File "..\..\src\apps\sgct_template\sgct_template_msvc9.vcproj"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\spinning_triangle"
	File "..\..\bin\example1\msvc9\example1_msvc9.exe"
	File "..\..\src\apps\example1\main.cpp"
	File "..\..\src\apps\example1\example1_msvc9.vcproj"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\texture_example"
	File "..\..\bin\textureExample\msvc9\textureExample_msvc9.exe"
	File "..\..\src\apps\textureExample\box.png"
	File "..\..\src\apps\textureExample\main.cpp"
	File "..\..\src\apps\textureExample\textureExample_msvc9.vcproj"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\simple_shader"
	File "..\..\bin\simpleShaderExample\msvc9\simpleShaderExample_msvc9.exe"
	File "..\..\src\apps\simpleShaderExample\simple.frag"
	File "..\..\src\apps\simpleShaderExample\simple.vert"
	File "..\..\src\apps\simpleShaderExample\main.cpp"
	File "..\..\src\apps\simpleShaderExample\simpleShaderExample_msvc9.vcproj"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\height_mapping"
	File "..\..\bin\heightMappingExample\msvc9\heightMappingExample_msvc9.exe"
	File "..\..\src\apps\heightMappingExample\heightmap.frag"
	File "..\..\src\apps\heightMappingExample\heightmap.vert"
	File "..\..\src\apps\heightMappingExample\heightmap.png"
	File "..\..\src\apps\heightMappingExample\normalmap.png"
	File "..\..\src\apps\heightMappingExample\main.cpp"
	File "..\..\src\apps\heightMappingExample\heightMappingExample_msvc9.vcproj"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\simple_navigation"
	File "..\..\bin\simpleNavigationExample\msvc9\simpleNavigationExample_msvc9.exe"
	File "..\..\src\apps\simpleNavigationExample\main.cpp"
	File "..\..\src\apps\simpleNavigationExample\simpleNavigationExample_msvc9.vcproj"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\gamepad"
	File "..\..\bin\gamepadExample\msvc9\gamepadExample_msvc9.exe"
	File "..\..\src\apps\gamepadExample\main.cpp"
	File "..\..\src\apps\gamepadExample\gamepadExample_msvc9.vcproj"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\osg_example"
	File "..\..\bin\osgExample\msvc9\osgExample_msvc9.exe"
	File "..\..\src\apps\osgExample\airplane.ive"
	File "..\..\src\apps\osgExample\main.cpp"
	File "..\..\src\apps\osgExample\osgExample_msvc9.vcproj"
	
	SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\remote_app"
	File "..\..\bin\remote_app\msvc9\remote_app_msvc9.exe"
	File "..\..\bin\remote_app\SGCTRemote.exe"
	File "..\..\src\apps\SGCTRemote\main.cpp"
	File "..\..\src\apps\SGCTRemote\single_remote.xml"
	File "..\..\src\apps\SGCTRemote\remote_app.cbp"
	File "..\..\src\apps\SGCTRemote\SGCTRemote.sln"
	File /r "..\..\src\apps\SGCTRemote\SGCTRemote\"
SectionEnd

Section "SGCT environment variable"
	;remove if set
	DeleteRegValue ${env_hklm} SGCT_ROOT_DIR
	
	${EnvVarUpdate} $0 "SGCT_ROOT_DIR" "P" "HKLM" "$INSTDIR\SGCT_${SGCT_VERSION}_x86"
	
	;update env vars
	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
	
	; reboot after install
	SetRebootFlag true
SectionEnd

Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\SGCT"
  CreateShortCut "$SMPROGRAMS\SGCT\Uninstall.lnk" "$INSTDIR\uninstall.exe" ""
  
  #add examples
  SetOutPath $INSTDIR
  CreateDirectory "$SMPROGRAMS\SGCT\examples"
  
  CreateShortCut "$SMPROGRAMS\SGCT\C-Student_wiki.lnk" "$INSTDIR\C-Student_wiki.url" "" "$INSTDIR\C_transparent.ico"
  CreateShortCut "$SMPROGRAMS\SGCT\SGCT_tutorials.lnk" "$INSTDIR\SGCT_tutorials.url" "" "$INSTDIR\C_transparent.ico"
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\spinning_triangle"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\spinning_triangle.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\spinning_triangle\example1_msvc9.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\texture_example"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\texture_example.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\texture_example\textureExample_msvc9.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\simple_shader"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\simple_shader.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\simple_shader\simpleShaderExample_msvc9.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\height_mapping"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\height_mapping.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\height_mapping\heightMappingExample_msvc9.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\simple_navigation"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\simple_navigation.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\simple_navigation\simpleNavigationExample_msvc9.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\gamepad"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\gamepad.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\gamepad\gamepadExample_msvc9.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""
  
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\osg_example"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\osg_example.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\osg_example\osgExample_msvc9.exe" "-config $\"%SGCT_ROOT_DIR%\config\single.xml$\""

  CreateDirectory "$SMPROGRAMS\SGCT\examples\remote"
  SetOutPath "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\remote_app"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\remote\remote_app.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\remote_app\remote_app_msvc9.exe" "-config single_remote.xml"
  CreateShortCut "$SMPROGRAMS\SGCT\examples\remote\SGCTRemote.lnk" "$INSTDIR\SGCT_${SGCT_VERSION}_x86\examples\remote_app\SGCTRemote.exe" ""
SectionEnd

Section "OSG 3.0.1 MSVC9 x86"
	SetOutPath "$INSTDIR\osg\osg_3.0.1_MSVC9_x86"
	File /r "D:\bin\osg\OpenSceneGraph-3.0.1-VS9.0.30729-x86-release-12741\"
	
	${registerExtension} "$INSTDIR\osg\osg_3.0.1_MSVC9_x86\bin\osgviewer.exe" ".osg" "OSG file"
	${registerExtension} "$INSTDIR\osg\osg_3.0.1_MSVC9_x86\bin\osgviewer.exe" ".osgb" "OSG bin file"
	${registerExtension} "$INSTDIR\osg\osg_3.0.1_MSVC9_x86\bin\osgviewer.exe" ".osgt" "OSG text file"
	${registerExtension} "$INSTDIR\osg\osg_3.0.1_MSVC9_x86\bin\osgviewer.exe" ".ive" "OSG bin file"
SectionEnd

Section "OSG environment variables"
	;remove if set
	DeleteRegValue ${env_hklm} OSG_PATH
    DeleteRegValue ${env_hklm} OSG_ROOT
    DeleteRegValue ${env_hklm} OSGHOME
	
	${EnvVarUpdate} $0 "OSG_ROOT" "P" "HKLM" "$INSTDIR\osg\osg_3.0.1_MSVC9_x86"
	${EnvVarUpdate} $0 "OSG_PATH" "P" "HKLM" "%OSG_ROOT%\bin"
    ${EnvVarUpdate} $0 "OSGHOME" "P" "HKLM" "%OSG_ROOT%"
  
    ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "%OSG_ROOT%\bin"
	
	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
	
	; reboot after install
	SetRebootFlag true
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGCT"
  DeleteRegKey HKLM "SOFTWARE\SGCT"
  
  ; delete variable
  DeleteRegValue ${env_hklm} SGCT_ROOT_DIR
  DeleteRegValue ${env_hklm} OSG_PATH
  DeleteRegValue ${env_hklm} OSG_ROOT
  DeleteRegValue ${env_hklm} OSGHOME
	
  ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "%OSG_ROOT%\bin"
  
  ${unregisterExtension} ".osg" "OSG file"
  ${unregisterExtension} ".osgt" "OSG text file"
  ${unregisterExtension} ".osgb" "OSG bin file"
  ${unregisterExtension} ".ive" "OSG bin file"
  
  ; make sure windows knows about the change
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
	
  Delete $INSTDIR\uninstall.exe

  ; Remove directories used
  RMDir /r "$SMPROGRAMS\SGCT"
  RMDir /r "$INSTDIR"

SectionEnd