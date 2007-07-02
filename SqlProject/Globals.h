#if !defined(AFX_GLOBALS_H__20030727_693D_586E_4B88_0080AD509054__INCLUDED_)
#define AFX_GLOBALS_H__20030727_693D_586E_4B88_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//////////////////////////////////////////////////////////////
//

#define PLUGIN_NAME        "Data Links"
#define PLUGIN_DESCRIPTION "Manages SQL Connections (OLE DB Projects)."


//////////////////////////////////////////////////////////////
//

CString ToString(long lValue);
CString GetSystemErrorText(DWORD dwErr);
void GenerateError(IDevEnv* pDevEnv, HWND hWnd, UINT nErr);
BOOL MergeMenu(HMENU hMenu, HMENU hMenuSource, UINT nPosition);
void AppendRtfText(CRichEditCtrl ctrlEdit, LPCTSTR pstrText, DWORD dwMask = 0, DWORD dwEffects = 0, COLORREF clrText = 0);


#endif // !defined(AFX_GLOBALS_H__20030727_693D_586E_4B88_0080AD509054__INCLUDED_)

