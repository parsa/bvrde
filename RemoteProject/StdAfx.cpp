// stdafx.cpp : source file that includes just the standard includes
//   RemoteProject.pch will be the pre-compiled header
//   stdafx.obj will contain the pre-compiled type information

#include "StdAfx.h"

// Shell helper functions (IE4)
#pragma comment(lib, "shlwapi.lib")

// Internet functions
#pragma comment(lib, "wininet.lib")

// WinSOCKET functions
#pragma comment(lib, "ws2_32.lib")

// Delay load
#pragma comment(lib, "Delayimp.lib")
#pragma comment(linker, "/DelayLoad:wininet.dll")
#pragma comment(linker, "/DelayLoad:ws2_32.dll")

#if _MSC_VER < 1300
   // Setting this linker switch causes segment size to be set to 512 bytes
   #pragma comment(linker, "/OPT:NOWIN98")
#endif

