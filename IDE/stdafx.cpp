// stdafx.cpp : source file that includes just the standard includes
//   BVRDE.pch will be the pre-compiled header
//   stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#if (_ATL_VER < 0x0700)
#include <atlimpl.cpp>
#endif //(_ATL_VER < 0x0700)

// Shell helper functions (IE4)
#pragma comment(lib, "shlwapi.lib")

#ifndef _DEBUG
   #if _MSC_VER < 1300
      // Setting this linker switch causes segment size to be set to 512 bytes
      #pragma comment(linker, "/OPT:NOWIN98")
      // Helpful linker hint
      #pragma comment(linker, "/RELEASE")
      // Remove function padding
      #pragma optimize("gsy", on)
   #endif
#endif
