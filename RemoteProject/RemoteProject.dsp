# Microsoft Developer Studio Project File - Name="RemoteProject" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=RemoteProject - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RemoteProject.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RemoteProject.mak" CFG="RemoteProject - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RemoteProject - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RemoteProject - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "RemoteProject - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "REMOTEPROJECT_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\Source\Atl\Include" /I "..\Source\Wtl\Include" /I "..\Source\Wtl\Controls" /I "..\Source\Cpp\Include" /I "..\Source\Scintilla" /D "NDEBUG" /D "_USRDLL" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_ATL_DLL" /Yu"stdafx.h" /FD /Zm200 /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x406 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x18000000" /dll /machine:I386 /def:".\RemoteProject.def" /out:"../Bin/RemoteProject.pkg"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "RemoteProject - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "REMOTEPROJECT_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\Source\Atl\Include" /I "..\Source\Wtl\Include" /I "..\Source\Wtl\Controls" /I "..\Source\Cpp\Include" /I "..\Source\Scintilla" /D "_DEBUG" /D "_USRDLL" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_ATL_DLL" /FR /Yu"stdafx.h" /FD /GZ /Zm200 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x406 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /def:".\RemoteProject.def" /out:"../Bin/RemoteProject.pkg" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /map

!ENDIF 

# Begin Target

# Name "RemoteProject - Win32 Release"
# Name "RemoteProject - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "View Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BreakpointView.cpp
# End Source File
# Begin Source File

SOURCE=.\ClassView.cpp
# End Source File
# Begin Source File

SOURCE=.\DisasmView.cpp
# End Source File
# Begin Source File

SOURCE=.\MemoryView.cpp
# End Source File
# Begin Source File

SOURCE=.\RegisterView.cpp
# End Source File
# Begin Source File

SOURCE=.\RemoteDirView.cpp
# End Source File
# Begin Source File

SOURCE=.\ScintillaView.cpp
# End Source File
# Begin Source File

SOURCE=.\StackView.cpp
# End Source File
# Begin Source File

SOURCE=.\TelnetView.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadView.cpp
# End Source File
# Begin Source File

SOURCE=.\VariableView.cpp
# End Source File
# Begin Source File

SOURCE=.\WatchView.cpp
# End Source File
# End Group
# Begin Group "Dialog Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\QuickWatchDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\RemoteFileDlg.cpp
# End Source File
# End Group
# Begin Group "Protocol Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ComSpecProtocol.cpp
# End Source File
# Begin Source File

SOURCE=.\FileProtocol.cpp
# End Source File
# Begin Source File

SOURCE=.\FtpProtocol.cpp
# End Source File
# Begin Source File

SOURCE=.\RloginProtocol.cpp
# End Source File
# Begin Source File

SOURCE=.\SftpProtocol.cpp
# End Source File
# Begin Source File

SOURCE=.\SshProtocol.cpp
# End Source File
# Begin Source File

SOURCE=.\TelnetProtocol.cpp
# End Source File
# End Group
# Begin Group "Tag Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\LexInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\TagInfo.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\Commands.cpp
# End Source File
# Begin Source File

SOURCE=.\CompileManager.cpp
# End Source File
# Begin Source File

SOURCE=.\DebugManager.cpp
# End Source File
# Begin Source File

SOURCE=.\FileProxy.cpp
# End Source File
# Begin Source File

SOURCE=.\Files.cpp
# End Source File
# Begin Source File

SOURCE=.\Globals.cpp
# End Source File
# Begin Source File

SOURCE=.\MiInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectModel.cpp
# End Source File
# Begin Source File

SOURCE=.\Project.cpp
# End Source File
# Begin Source File

SOURCE=.\RemoteProject.cpp
# End Source File
# Begin Source File

SOURCE=.\RemoteProject.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\RemoteProject.rc
# End Source File
# Begin Source File

SOURCE=.\ShellProxy.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TagProxy.cpp
# End Source File
# Begin Source File

SOURCE=.\Turnpike.cpp
# End Source File
# Begin Source File

SOURCE=.\ViewSerializer.cpp
# End Source File
# Begin Source File

SOURCE=.\WizardSheet.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Source\WTL\Include\atlctrlsext.h
# End Source File
# Begin Source File

SOURCE=..\Source\ATL\Include\atldataobj.h
# End Source File
# Begin Source File

SOURCE=..\Source\ATL\Include\atldispa.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\atlgdix.h
# End Source File
# Begin Source File

SOURCE=..\Source\ATL\Include\atlwfile.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\atlwinmisc.h
# End Source File
# Begin Source File

SOURCE=.\cryptlib_wrapper.h
# End Source File
# Begin Source File

SOURCE=..\Source\CPP\Include\NetApiWrappers.h
# End Source File
# Begin Source File

SOURCE=..\Source\CPP\Include\Thread.h
# End Source File
# End Group
# Begin Group "Controls"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Source\WTL\Controls\AcListBox.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\atlscintilla.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\CmdEdit.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\CoolTabCtrls.h
# End Source File
# Begin Source File

SOURCE=.\DockManager.h
# End Source File
# Begin Source File

SOURCE=..\GenEdit\GenEdit.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Controls\MruCombo.h
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
# Begin Source File

SOURCE=..\Source\WTL\Include\SciLexer.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\Scintilla.h
# End Source File
# End Group
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ArgumentsDlg.h
# End Source File
# Begin Source File

SOURCE=.\AttachProcessDlg.h
# End Source File
# Begin Source File

SOURCE=.\BreakpointInfoDlg.h
# End Source File
# Begin Source File

SOURCE=.\CryptLibDlg.h
# End Source File
# Begin Source File

SOURCE=.\PasswordDlg.h
# End Source File
# Begin Source File

SOURCE=.\QuickWatchDlg.h
# End Source File
# Begin Source File

SOURCE=.\RemoteFileDlg.h
# End Source File
# End Group
# Begin Group "Views"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BreakpointView.h
# End Source File
# Begin Source File

SOURCE=.\ClassView.h
# End Source File
# Begin Source File

SOURCE=.\DisasmView.h
# End Source File
# Begin Source File

SOURCE=.\MemoryView.h
# End Source File
# Begin Source File

SOURCE=.\RegisterView.h
# End Source File
# Begin Source File

SOURCE=.\RemoteDirView.h
# End Source File
# Begin Source File

SOURCE=.\ScintillaView.h
# End Source File
# Begin Source File

SOURCE=.\StackView.h
# End Source File
# Begin Source File

SOURCE=.\TelnetView.h
# End Source File
# Begin Source File

SOURCE=.\ThreadView.h
# End Source File
# Begin Source File

SOURCE=.\VariableView.h
# End Source File
# Begin Source File

SOURCE=.\WatchView.h
# End Source File
# End Group
# Begin Group "Protocols"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ComSpecProtocol.h
# End Source File
# Begin Source File

SOURCE=.\FileProtocol.h
# End Source File
# Begin Source File

SOURCE=.\FtpProtocol.h
# End Source File
# Begin Source File

SOURCE=.\RloginProtocol.h
# End Source File
# Begin Source File

SOURCE=.\SftpProtocol.h
# End Source File
# Begin Source File

SOURCE=.\SshProtocol.h
# End Source File
# Begin Source File

SOURCE=.\TelnetProtocol.h
# End Source File
# End Group
# Begin Group "Tags"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\LexInfo.h
# End Source File
# Begin Source File

SOURCE=.\TagInfo.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\IDE\BVRDE_SDK.h
# End Source File
# Begin Source File

SOURCE=.\Commands.h
# End Source File
# Begin Source File

SOURCE=.\CompileManager.h
# End Source File
# Begin Source File

SOURCE=.\DebugManager.h
# End Source File
# Begin Source File

SOURCE=.\FileProxy.h
# End Source File
# Begin Source File

SOURCE=.\Files.h
# End Source File
# Begin Source File

SOURCE=.\Globals.h
# End Source File
# Begin Source File

SOURCE=.\MiInfo.h
# End Source File
# Begin Source File

SOURCE=.\ObjectModel.h
# End Source File
# Begin Source File

SOURCE=.\Project.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\ShellProxy.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TagProxy.h
# End Source File
# Begin Source File

SOURCE=.\ViewSerializer.h
# End Source File
# Begin Source File

SOURCE=.\WizardSheet.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\back.ico
# End Source File
# Begin Source File

SOURCE=.\Res\back.ico
# End Source File
# Begin Source File

SOURCE=.\back1.ico
# End Source File
# Begin Source File

SOURCE=.\Res\bookmarks.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\build.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\classview.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\debug.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\Res\key.ico
# End Source File
# Begin Source File

SOURCE=.\Res\minus.ico
# End Source File
# Begin Source File

SOURCE=.\Res\plus.ico
# End Source File
# Begin Source File

SOURCE=.\Res\refresh.ico
# End Source File
# Begin Source File

SOURCE=.\Res\search.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\toolimages.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\up.ico
# End Source File
# End Group
# End Target
# End Project
