// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__28DE29B0_0F1F_4DDF_BB14_5EC81FDBF38C__INCLUDED_)
#define AFX_STDAFX_H__28DE29B0_0F1F_4DDF_BB14_5EC81FDBF38C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WINVER            0x0400
#define _WIN32_WINNT      0x0400
#define _WIN32_IE         0x0501
#define _RICHEDIT_VER     0x0200

#define _WTL_NEW_PAGE_NOTIFY_HANDLERS
#define _WTL_USE_MDI

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlcom.h>
#include <atlhost.h>
#include <atlwin.h>
#include <atlctl.h>

#include <atlmisc.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atldlgs.h>
#include <atlctrlw.h>

#include <shlwapi.h>

#include "atlwfile.h"
#include "atlgdix.h"
#include "atlctrlsext.h"
#include "atlwinmisc.h"
#include "atlcollections.h"

#include "BVRDE_SDK.h"
#include "Plugin.h"
#include "Globals.h"
#include "ObjectModel.h"

class CSolution;

extern IDevEnv* g_pDevEnv;
extern CSimpleArray<CPlugin> g_aPlugins;
extern CSolution* g_pSolution;


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__28DE29B0_0F1F_4DDF_BB14_5EC81FDBF38C__INCLUDED_)
