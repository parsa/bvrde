// stdafx.cpp : source file that includes just the standard includes
//   StartPage.pch will be the pre-compiled header
//   stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"


// Shell helper functions (IE4)
#pragma comment(lib, "shlwapi.lib")


#if _MSC_VER < 1300
   // Setting this linker switch causes segment size to be set to 512 bytes
   #pragma comment(linker, "/OPT:NOWIN98")
#endif
