// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__3DFF06CB_76F6_4908_9E75_40E306C7C241__INCLUDED_)
#define AFX_STDAFX_H__3DFF06CB_76F6_4908_9E75_40E306C7C241__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WINVER            0x0400
#define _WIN32_WINNT      0x0400
#define _WIN32_IE         0x0501

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NON_CONFORMING_SWPRINTFS


#include <atlbase.h>
#include <atlapp.h>

#include <shellapi.h>

extern CComModule _Module;

#include <atlcom.h>
#include <atlwin.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlctrlx.h>

#include <commdlg.h>

#include "../IDE/BVRDE_SDK.h"

#define FR_INSEL 0x20000000
#define FR_WRAP  0x40000000

#include "atlwinmisc.h"
#include "atlscintilla.h"

#define SCE_C_FUNCTIONCLASS  20
#define SCE_C_TYPEDEFCLASS   21


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__3DFF06CB_76F6_4908_9E75_40E306C7C241__INCLUDED_)
