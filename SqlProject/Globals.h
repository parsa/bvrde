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

void AppendRtfText(CRichEditCtrl ctrlEdit, LPCTSTR pstrText, DWORD dwMask = 0, DWORD dwEffects = 0, COLORREF clrText = 0);
void GenerateError(IDevEnv* pDevEnv, UINT nErr);
CString GetSystemErrorText(DWORD dwErr);
BOOL MergeMenu(HMENU hMenu, HMENU hMenuSource, UINT nPosition);
CString ToString(long lValue);



#endif // !defined(AFX_GLOBALS_H__20030727_693D_586E_4B88_0080AD509054__INCLUDED_)

