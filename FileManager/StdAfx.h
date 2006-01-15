// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__948F16C3_508B_4069_B3C3_83157B9A2F44__INCLUDED_)
#define AFX_STDAFX_H__948F16C3_508B_4069_B3C3_83157B9A2F44__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WINVER            0x0400
#define _WIN32_WINNT      0x0400
#define _WIN32_IE         0x0501
#define _RICHEDIT_VER     0x0200

#define _CRT_SECURE_NO_DEPRECATE


#include <atlbase.h>
#include <atlapp.h>
#include "atlshellextbase.h"

extern CShellModule _Module;

#include "atlshellext.h"

#include <atlwin.h>
#include <atlctrls.h>

#include "atlwinmisc.h"
#include "atldataobj.h"

#include "../IDE/BVRDE_SDK.h"
extern IDevEnv* _pDevEnv;

#define ID_VIEW_FILEMANAGER   40900


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__948F16C3_508B_4069_B3C3_83157B9A2F44__INCLUDED_)
