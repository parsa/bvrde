// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__43159474_A2DD_4CED_B49D_CEE7C1471EF3__INCLUDED_)
#define AFX_STDAFX_H__43159474_A2DD_4CED_B49D_CEE7C1471EF3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WINVER            0x0400
#define _WIN32_WINNT      0x0400
#define _WIN32_IE         0x0501
#define _RICHEDIT_VER     0x0200

#include <atlbase.h>
#include <atlapp.h>

extern CComModule _Module;

#include <atlcom.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atlctrlx.h>

#include "atlwfile.h"
#include "atldispa.h"
#include "atlscript.h"
#include "atlwinmisc.h"

#include "../IDE/BVRDE_SDK.h"
extern IDevEnv* _pDevEnv;

#include <mshtmhst.h>
#include <urlmon.h>

#include "ASPParser.h"
#include "ScriptHandler.h"


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__43159474_A2DD_4CED_B49D_CEE7C1471EF3__INCLUDED_)
