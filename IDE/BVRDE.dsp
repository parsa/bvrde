# Microsoft Developer Studio Project File - Name="BVRDE" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=BVRDE - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "BVRDE.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "BVRDE.mak" CFG="BVRDE - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "BVRDE - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "BVRDE - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "BVRDE - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Temp"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /I "..\Source\Atl\Include" /I "..\Source\Wtl\Include" /I "..\Source\Wtl\Controls" /I "..\Source\Cpp\Include" /D "NDEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_ATL_DLL" /Yu"stdafx.h" /FD /Zm200 /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x406 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:"../Bin/BVRDE.exe"
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "BVRDE - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Temp"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\Source\Atl\Include" /I "..\Source\Wtl\Include" /I "..\Source\Wtl\Controls" /I "..\Source\Cpp\Include" /D "_DEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_ATL_DLL" /FR /Yu"stdafx.h" /FD /GZ /Zm200 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x406 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../Bin/BVRDE.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "BVRDE - Win32 Release"
# Name "BVRDE - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BVRDE.cpp
# End Source File
# Begin Source File

SOURCE=.\BVRDE.rc
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\CommandView.cpp
# End Source File
# Begin Source File

SOURCE=.\DevEnv.cpp
# End Source File
# Begin Source File

SOURCE=.\Dialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\LoadSave.cpp
# End Source File
# Begin Source File

SOURCE=.\Macro.cpp
# End Source File
# Begin Source File

SOURCE=.\mainfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectModel.cpp
# End Source File
# Begin Source File

SOURCE=.\Options.cpp
# End Source File
# Begin Source File

SOURCE=.\OutputView.cpp
# End Source File
# Begin Source File

SOURCE=.\Plugin.cpp
# End Source File
# Begin Source File

SOURCE=.\RegSerializer.cpp
# End Source File
# Begin Source File

SOURCE=.\Solution.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\UpdateUI.cpp
# End Source File
# Begin Source File

SOURCE=.\WizardSheet.cpp
# End Source File
# Begin Source File

SOURCE=.\XmlSerializer.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Source\ATL\Include\atlcmdline.h
# End Source File
# Begin Source File

SOURCE=..\Source\ATL\Include\atlcollections.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\atldib.h
# End Source File
# Begin Source File

SOURCE=..\Source\ATL\Include\atldispa.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\atlgdix.h
# End Source File
# Begin Source File

SOURCE=..\Source\ATL\Include\atlscript.h
# End Source File
# Begin Source File

SOURCE=..\Source\ATL\Include\atlwfile.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\atlwinmisc.h
# End Source File
# Begin Source File

SOURCE=..\Source\CPP\Include\Thread.h
# End Source File
# End Group
# Begin Group "Controls"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Source\WTL\Include\atlctrlsext.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\atlctrlxp.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\atlctrlxp2.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\atldock.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\atldock2.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\AutoHideXP.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\ColorCombo.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\CoolTabCtrls.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\DlgTabCtrl.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\FontCombo.h
# End Source File
# Begin Source File

SOURCE=.\MDIContainer.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\MenuShadows.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\PropertyGrid.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\PropertyItem.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\PropertyItemEditors.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\PropertyItemImpl.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\PropertyList.h
# End Source File
# End Group
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AboutDlg.h
# End Source File
# Begin Source File

SOURCE=.\AddinManagerDlg.h
# End Source File
# Begin Source File

SOURCE=.\ArgumentPromptDlg.h
# End Source File
# Begin Source File

SOURCE=.\ChooseSolutionDlg.h
# End Source File
# Begin Source File

SOURCE=.\CustomizeDlg.h
# End Source File
# Begin Source File

SOURCE=.\ExternalToolsDlg.h
# End Source File
# Begin Source File

SOURCE=.\FileMissingDlg.h
# End Source File
# Begin Source File

SOURCE=.\KeyboardDlg.h
# End Source File
# Begin Source File

SOURCE=.\LayoutDlg.h
# End Source File
# Begin Source File

SOURCE=.\MacroBindDlg.h
# End Source File
# Begin Source File

SOURCE=.\MacrosDlg.h
# End Source File
# Begin Source File

SOURCE=.\MsgBoxDlg.h
# End Source File
# Begin Source File

SOURCE=.\OptionsDlg.h
# End Source File
# Begin Source File

SOURCE=.\SoundsDlg.h
# End Source File
# Begin Source File

SOURCE=.\SplashDlg.h
# End Source File
# Begin Source File

SOURCE=.\WindowsDlg.h
# End Source File
# End Group
# Begin Group "Views"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CommandView.h
# End Source File
# Begin Source File

SOURCE=.\ExplorerView.h
# End Source File
# Begin Source File

SOURCE=.\OutputView.h
# End Source File
# Begin Source File

SOURCE=.\ProjectView.h
# End Source File
# Begin Source File

SOURCE=.\PropertiesView.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\BVRDE_SDK.h
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\DummyElement.h
# End Source File
# Begin Source File

SOURCE=.\Globals.h
# End Source File
# Begin Source File

SOURCE=.\Macro.h
# End Source File
# Begin Source File

SOURCE=.\mainfrm.h
# End Source File
# Begin Source File

SOURCE=.\ObjectModel.h
# End Source File
# Begin Source File

SOURCE=.\Plugin.h
# End Source File
# Begin Source File

SOURCE=.\RegSerializer.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Solution.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\UpdateUI.h
# End Source File
# Begin Source File

SOURCE=.\WizardSheet.h
# End Source File
# Begin Source File

SOURCE=.\XmlSerializer.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\banner.bmp
# End Source File
# Begin Source File

SOURCE=.\res\buildanim.bmp
# End Source File
# Begin Source File

SOURCE=.\res\BVRDE.exe.manifest
# End Source File
# Begin Source File

SOURCE=.\res\BVRDE.ico
# End Source File
# Begin Source File

SOURCE=.\res\BVRDEdoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\delete.ico
# End Source File
# Begin Source File

SOURCE=.\res\down.ico
# End Source File
# Begin Source File

SOURCE=.\res\droparrow.ico
# End Source File
# Begin Source File

SOURCE=.\res\error.ico
# End Source File
# Begin Source File

SOURCE=.\res\explorer.bmp
# End Source File
# Begin Source File

SOURCE=.\res\folders.bmp
# End Source File
# Begin Source File

SOURCE=.\res\fullscreen.bmp
# End Source File
# Begin Source File

SOURCE=.\res\information.ico
# End Source File
# Begin Source File

SOURCE=.\res\load1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\load1.ico
# End Source File
# Begin Source File

SOURCE=.\res\load2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\load2.ico
# End Source File
# Begin Source File

SOURCE=.\res\load3.bmp
# End Source File
# Begin Source File

SOURCE=.\res\load3.ico
# End Source File
# Begin Source File

SOURCE=.\res\new.ico
# End Source File
# Begin Source File

SOURCE=.\res\printanim.bmp
# End Source File
# Begin Source File

SOURCE=.\res\project.bmp
# End Source File
# Begin Source File

SOURCE=.\res\project.ico
# End Source File
# Begin Source File

SOURCE=.\res\project1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\projects16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\projects32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\question.ico
# End Source File
# Begin Source File

SOURCE=.\res\saveanim.bmp
# End Source File
# Begin Source File

SOURCE=.\res\splash.bmp
# End Source File
# Begin Source File

SOURCE=.\res\test.bmp
# End Source File
# Begin Source File

SOURCE=.\res\toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\toolimages.bmp
# End Source File
# Begin Source File

SOURCE=.\res\transferanim.bmp
# End Source File
# Begin Source File

SOURCE=.\res\up.ico
# End Source File
# Begin Source File

SOURCE=.\res\warning.ico
# End Source File
# Begin Source File

SOURCE=.\res\watermark.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=..\ChangeLog
# End Source File
# Begin Source File

SOURCE=..\Copyright
# End Source File
# Begin Source File

SOURCE=..\Install
# End Source File
# Begin Source File

SOURCE=..\ReadMe
# End Source File
# Begin Source File

SOURCE=..\ToDo
# End Source File
# End Target
# End Project
