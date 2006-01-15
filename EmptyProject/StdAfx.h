// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
#if !defined(AFX_STDAFX_H__7E722C37_7E96_4A31_908D_697F4A60C393__INCLUDED_)
#define AFX_STDAFX_H__7E722C37_7E96_4A31_908D_697F4A60C393__INCLUDED_

#pragma once

#define WINVER            0x0400
#define _WIN32_WINNT      0x0400
#define _WIN32_IE         0x0501
#define _RICHEDIT_VER     0x0200

#define NOCRYPT				// Disable include of wincrypt.h
#define _WINSOCKAPI_       // Don't include WinSOCK v1
#define _CRT_SECURE_NO_DEPRECATE
#define _WTL_NEW_PAGE_NOTIFY_HANDLERS

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

#include "Thread.h"

#include "atlwfile.h"
#include "atlwinmisc.h"
#include "atlgdix.h"
#include "atlctrlsext.h"

#include "atlscintilla.h"

#include "../IDE/BVRDE_SDK.h"
extern IDevEnv* _pDevEnv;

#include "Globals.h"


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__7E722C37_7E96_4A31_908D_697F4A60C393__INCLUDED_)

