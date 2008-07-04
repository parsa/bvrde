// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
#if !defined(AFX_STDAFX_H__7E722C37_7E96_4A31_908D_697F4A60C393__INCLUDED_)
#define AFX_STDAFX_H__7E722C37_7E96_4A31_908D_697F4A60C393__INCLUDED_

#pragma once

#define WINVER            0x0500
#define _WIN32_WINNT      0x0500
#define _WIN32_IE         0x0501
#define _RICHEDIT_VER     0x0200

#define NOCRYPT                         // Disable include of wincrypt.h
#define _WINSOCKAPI_                    // Don't include WinSOCK v1
#define _CRT_NONSTDC_NO_WARNINGS        // Don't complain so much
#define _CRT_SECURE_NO_DEPRECATE        // We used to compile this on MSVC 6
#define _CRT_NON_CONFORMING_SWPRINTFS   // Really we do...
#define _WTL_NEW_PAGE_NOTIFY_HANDLERS   // WTL uses new property page notification


#include <atlbase.h>
#include <atlapp.h>

extern CComModule _Module;

#include <atlcom.h>
#include <atlwin.h>

#include <atlmisc.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atldlgs.h>
#include <atlscrl.h>
#include <atlctrlw.h>

#include <mmsystem.h>

#include <wininet.h>

#include <winsock2.h>

#include "Thread.h"
#include "NetApiWrappers.h"

#include "atlwfile.h"
#include "atlwinmisc.h"
#include "atlgdix.h"
#include "atlctrlsext.h"
#include "atldataobj.h"

#include "atlctrlxp.h"
#include "atlctrlxp2.h"

#include "atlscintilla.h"

#include "../IDE/BVRDE_SDK.h"
extern IDevEnv* _pDevEnv;

#include "Globals.h"

#if _MSC_VER >= 1300
   #pragma warning(disable : 4100 4189 4996)
#endif


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__7E722C37_7E96_4A31_908D_697F4A60C393__INCLUDED_)
