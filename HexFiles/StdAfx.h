// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__461F0D2A_FD70_4708_ADDF_D1E5B8FB485A__INCLUDED_)
#define AFX_STDAFX_H__461F0D2A_FD70_4708_ADDF_D1E5B8FB485A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WINVER            0x0400
#define _WIN32_WINNT      0x0400
#define _WIN32_IE         0x0501
#define _RICHEDIT_VER     0x0200

#define _WTL_NEW_PAGE_NOTIFY_HANDLERS
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NON_CONFORMING_SWPRINTFS

#define PROGID_XMLDOMDocument L"MSXML2.DOMDocument"


#include <atlbase.h>
#include <atlapp.h>

extern CComModule _Module;

#include <atlcom.h>
#include <atlwin.h>
#include <atlframe.h>
#include <atlhost.h>
#include <atlctl.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atldlgs.h>

#include <mshtml.h>
#include <msxml2.h>
#include <ExDispid.h>

#include "../IDE/BVRDE_SDK.h"
extern IDevEnv* _pDevEnv;

#include "Globals.h"

#include "atlgdix.h"
#include "atlwfile.h"
#include "atlctrlsext.h"
#include "atlwinmisc.h"

#include "atlscintilla.h"
#include "CoolTabCtrls.h"


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__461F0D2A_FD70_4708_ADDF_D1E5B8FB485A__INCLUDED_)
