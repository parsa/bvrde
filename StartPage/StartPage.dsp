# Microsoft Developer Studio Project File - Name="StartPage" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=StartPage - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "StartPage.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "StartPage.mak" CFG="StartPage - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "StartPage - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "StartPage - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "StartPage - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "STARTPAGE_EXPORTS" /Yu"stdafx.h" /FD /c
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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x1B000000" /dll /machine:I386 /out:"../Bin/StartPage.pkg"

!ELSEIF  "$(CFG)" == "StartPage - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "STARTPAGE_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../Bin/StartPage.pkg" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "StartPage - Win32 Release"
# Name "StartPage - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\StartPage.cpp
# End Source File
# Begin Source File

SOURCE=.\StartPage.def
# End Source File
# Begin Source File

SOURCE=.\StartPage.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\View.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\IDE\BVRDE_SDK.h
# End Source File
# Begin Source File

SOURCE=.\EventMonitor.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\View.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\background.gif
# End Source File
# Begin Source File

SOURCE=.\res\btm_blackbar.gif
# End Source File
# Begin Source File

SOURCE=.\res\btm_blackbar2.gif
# End Source File
# Begin Source File

SOURCE=.\res\btn.gif
# End Source File
# Begin Source File

SOURCE=.\res\BVRDEDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\CSS.css
# End Source File
# Begin Source File

SOURCE=".\res\footer-separator.gif"
# End Source File
# Begin Source File

SOURCE=.\res\headers.gif
# End Source File
# Begin Source File

SOURCE=.\res\index.html
# End Source File
# Begin Source File

SOURCE=.\res\index_de.html
# End Source File
# Begin Source File

SOURCE=.\res\linkangle.gif
# End Source File
# Begin Source File

SOURCE=.\res\lnk_arrow.gif
# End Source File
# Begin Source File

SOURCE=.\res\lnk_bullet1.gif
# End Source File
# Begin Source File

SOURCE=.\res\logo.gif
# End Source File
# Begin Source File

SOURCE=".\res\mid-backangle.gif"
# End Source File
# Begin Source File

SOURCE=.\res\row1.gif
# End Source File
# Begin Source File

SOURCE=.\res\row2_backangle.gif
# End Source File
# Begin Source File

SOURCE=.\res\row2_bg_nosearch.gif
# End Source File
# Begin Source File

SOURCE=.\res\row3_backangle.gif
# End Source File
# Begin Source File

SOURCE=.\res\row3_frontangle.gif
# End Source File
# Begin Source File

SOURCE=".\res\sidebar-downslash.gif"
# End Source File
# Begin Source File

SOURCE=".\res\sidebar-topdownslash.gif"
# End Source File
# Begin Source File

SOURCE=".\res\sidebar-upslash.gif"
# End Source File
# Begin Source File

SOURCE=".\res\spacer(1).gif"
# End Source File
# Begin Source File

SOURCE=.\res\spacer.gif
# End Source File
# Begin Source File

SOURCE=.\res\style.css
# End Source File
# End Group
# End Target
# End Project
