# Microsoft Developer Studio Project File - Name="SqlProject" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=SqlProject - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SqlProject.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SqlProject.mak" CFG="SqlProject - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SqlProject - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SqlProject - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SqlProject - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SQLPROJECT_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\Source\Atl\Include" /I "..\Source\Wtl\Include" /I "..\Source\Wtl\Controls" /I "..\Source\Cpp\Include" /D "NDEBUG" /D "_USRDLL" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_ATL_DLL" /Yu"stdafx.h" /FD /c
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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x13000000" /dll /machine:I386 /out:"../Bin/SqlProject.pkg"

!ELSEIF  "$(CFG)" == "SqlProject - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SQLPROJECT_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\Source\Atl\Include" /I "..\Source\Wtl\Include" /I "..\Source\Wtl\Controls" /I "..\Source\Cpp\Include" /D "_DEBUG" /D "_USRDLL" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_ATL_DLL" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x406 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../Bin/SqlProject.pkg" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "SqlProject - Win32 Release"
# Name "SqlProject - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Database"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Source\CPP\Include\cOledb.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\Container.cpp
# End Source File
# Begin Source File

SOURCE=.\DbOperations.cpp
# End Source File
# Begin Source File

SOURCE=.\Files.cpp
# End Source File
# Begin Source File

SOURCE=.\Globals.cpp
# End Source File
# Begin Source File

SOURCE=.\Project.cpp
# End Source File
# Begin Source File

SOURCE=.\QueryThread.cpp
# End Source File
# Begin Source File

SOURCE=.\ResultView.cpp
# End Source File
# Begin Source File

SOURCE=.\SchemaView.cpp
# End Source File
# Begin Source File

SOURCE=.\ScintillaView.cpp
# End Source File
# Begin Source File

SOURCE=.\SqlProject.cpp
# End Source File
# Begin Source File

SOURCE=.\SqlProject.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
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

SOURCE=..\Source\WTL\Include\atlgdix.h
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

SOURCE=..\Source\WTL\Include\atlscintilla.h
# End Source File
# Begin Source File

SOURCE=..\GenEdit\GenEdit.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\SciLexer.h
# End Source File
# Begin Source File

SOURCE=..\Source\WTL\Include\Scintilla.h
# End Source File
# End Group
# Begin Group "Database Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Source\CPP\Include\cDb.h
# End Source File
# Begin Source File

SOURCE=..\Source\CPP\Include\cOledb.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\IDE\BVRDE_SDK.h
# End Source File
# Begin Source File

SOURCE=.\Container.h
# End Source File
# Begin Source File

SOURCE=.\DbOperations.h
# End Source File
# Begin Source File

SOURCE=.\Files.h
# End Source File
# Begin Source File

SOURCE=.\Globals.h
# End Source File
# Begin Source File

SOURCE=.\Project.h
# End Source File
# Begin Source File

SOURCE=.\QueryThread.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\ResultView.h
# End Source File
# Begin Source File

SOURCE=.\SchemaView.h
# End Source File
# Begin Source File

SOURCE=.\ScintillaView.h
# End Source File
# Begin Source File

SOURCE=.\SqlProject.def
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\WizardSheet.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\db.gif
# End Source File
# Begin Source File

SOURCE=.\res\info.htm
# End Source File
# Begin Source File

SOURCE=.\res\listlarge.bmp
# End Source File
# Begin Source File

SOURCE=.\res\listsmall.bmp
# End Source File
# Begin Source File

SOURCE=.\res\toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\toolimages.bmp
# End Source File
# End Group
# End Target
# End Project

