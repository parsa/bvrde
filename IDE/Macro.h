#if !defined(AFX_MACRO_H__20030713_74FA_FE92_6D06_0080AD509054__INCLUDED_)
#define AFX_MACRO_H__20030713_74FA_FE92_6D06_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame;

#include "atlscript.h"


class ATL_NO_VTABLE CMacro :
   public CComObjectRootEx<CComSingleThreadModel>,
   public IActiveScriptSiteImpl<CMacro>,
   public IActiveScriptSiteWindowImpl<CMacro>
{
public:
   void Init(CMainFrame* pFrame, CApplicationOM* pApp);
   bool RunMacroFromFile(LPCTSTR pstrFilename, LPCTSTR pstrFunction);
   bool RunMacroFromScript(CComBSTR bstrData, CComBSTR bstrFunction, DWORD dwFlags = 0, VARIANT* pvarResult = NULL, EXCEPINFO* pExcepInfo = NULL);

   CMainFrame* m_pMainFrame;
   EXCEPINFO* m_pExcepInfo;
   CGlobalsOM m_Globals;
   HWND m_hWnd;

   BEGIN_COM_MAP(CMacro)
      COM_INTERFACE_ENTRY(IActiveScriptSite)
      COM_INTERFACE_ENTRY(IActiveScriptSiteWindow)
   END_COM_MAP()

   STDMETHOD(GetItemInfo)(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown** ppiunkItem, ITypeInfo** ppti);
   STDMETHOD(GetDocVersionString)(BSTR* pbstrVersion);
   STDMETHOD(OnScriptError)(IActiveScriptError* pScriptError);
};



#endif // !defined(AFX_MACRO_H__20030713_74FA_FE92_6D06_0080AD509054__INCLUDED_)

