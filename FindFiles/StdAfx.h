// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__3CA6C19E_C03E_4F90_AD90_B9871AE45386__INCLUDED_)
#define AFX_STDAFX_H__3CA6C19E_C03E_4F90_AD90_B9871AE45386__INCLUDED_

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

#include "atlwinmisc.h"
#include "Thread.h"

#include "../IDE/BVRDE_SDK.h"
extern IDevEnv* _pDevEnv;

#define ID_EDIT_FINDFILES   41000

#define FR_REGEXP      0x02000000
#define FR_SUBFOLDERS  0x04000000


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__3CA6C19E_C03E_4F90_AD90_B9871AE45386__INCLUDED_)
