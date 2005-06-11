#if !defined(AFX_GLOBALS_H__20030315_A892_7CB2_47EF_0080AD509054__INCLUDED_)
#define AFX_GLOBALS_H__20030315_A892_7CB2_47EF_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//////////////////////////////////////////////////////////////
//

class CTextFile;
class CEmptyProject;


//////////////////////////////////////////////////////////////
//

#define PLUGIN_NAME        "Empty Project"
#define PLUGIN_DESCRIPTION "An initially empty project.  Doesn't allow compile/debug commands, but acts as a file placeholder."


//////////////////////////////////////////////////////////////
//

CString ToString(long lValue);
CString GetSystemErrorText(DWORD dwErr);
void GenerateError(IDevEnv* pDevEnv, UINT nErr);
void AppendRtfText(CRichEditCtrl ctrlEdit, LPCTSTR pstrText, DWORD dwMask = 0, DWORD dwEffects = 0, COLORREF clrText = 0);
BOOL MergeMenu(HMENU hMenu, HMENU hMenuSource, UINT nPosition);

CString GetFileTypeFromFilename(LPCTSTR pstrFilename);
CTextFile* CreateViewFromFilename(LPCTSTR pstrFilename, CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent);


#endif // !defined(AFX_GLOBALS_H__20030315_A892_7CB2_47EF_0080AD509054__INCLUDED_)

