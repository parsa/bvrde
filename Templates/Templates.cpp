// Templates.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"


CComModule _Module;
IDevEnv* _pDevEnv;

#include "NewFileDlg.h"


/////////////////////////////////////////////////////////////////////////////
// DLL Main

BOOL APIENTRY DllMain(HINSTANCE hInstance, 
                      DWORD dwReason, 
                      LPVOID /*lpReserved*/)
{
   if( dwReason == DLL_PROCESS_ATTACH ) 
   {
      _Module.Init(NULL, hInstance);
      ::DisableThreadLibraryCalls(hInstance);
   }
   else if( dwReason == DLL_PROCESS_DETACH ) 
   {
     _Module.Term();
   }
   return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Exported functions

EXTERN_C
BOOL APIENTRY Templates_NewFile(HWND hWndParent, IDevEnv* pDevEnv, ISolution* pSolution, IProject* pProject, IView* pView)
{   
   CNewFileDlg dlg;
   dlg.Init(pDevEnv, pSolution, pProject, pView);
   return dlg.DoModal(hWndParent) == IDOK;
}

EXTERN_C
BOOL APIENTRY Templates_NewProject(HWND hWndParent, IDevEnv* pDevEnv, ISolution* pSolution, IProject* pProject)
{   
   ATLASSERT(false);  // TODO: Implement this...
   return FALSE;
}

EXTERN_C
BOOL APIENTRY Templates_RunWizard(HWND hWndParent, LPCTSTR pstrName, IDevEnv* pDevEnv, ISolution* pSolution, IProject* pProject, IView* pView)
{   
   TCHAR szIniFilename[MAX_PATH] = { 0 };
   ::wsprintf(szIniFilename, _T("%sTemplates\\Wizards.ini"), CModulePath());
   TCHAR szFilename[MAX_PATH] = { 0 };
   ::GetPrivateProfileString(pstrName, _T("File"), _T(""), szFilename, MAX_PATH, szIniFilename);
   TCHAR szDialog[MAX_PATH] = { 0 };
   ::GetPrivateProfileString(pstrName, _T("Dialog"), _T(""), szDialog, MAX_PATH, szIniFilename);
   if( ::lstrlen(szFilename) == 0 ) return FALSE;
   TCHAR szActualFile[MAX_PATH] = { 0 };
   ::wsprintf(szActualFile, _T("%sTemplates\\Wizards\\%s"), CModulePath(), szFilename);
   TCHAR szActualDialog[MAX_PATH] = { 0 };
   ::wsprintf(szActualDialog, _T("%sTemplates\\Wizards\\%s"), CModulePath(), szDialog);
   CFile f;      
   if( !f.Open(szActualFile) ) return FALSE;
   DWORD dwSize = f.GetSize();
   LPSTR lpstr = (LPSTR) malloc(dwSize + 1);
   if( lpstr == NULL ) return FALSE;
   f.Read(lpstr, dwSize);
   f.Close();
   lpstr[dwSize] = '\0';
   CComBSTR bstrScript = lpstr;
   free(lpstr);
   CParser parser;
   LPWSTR pstrScript = parser.Process(bstrScript);
   if( pstrScript == NULL ) return FALSE;
   CComObjectGlobal<CScriptHandler> script;
   script.Init(hWndParent,
      pDevEnv->GetDispatch(), 
      pSolution->GetDispatch(), 
      pProject->GetDispatch(), 
      pView->GetDispatch());
   bool bRes = script.ShowDialog(szDialog, szActualDialog);
   if( bRes ) script.Execute(pstrScript);
   free(pstrScript);
   if( !bRes ) return FALSE;
   CComBSTR bstrResult = script.GetResult();
   CComDispatchDriver spDisp = pView->GetDispatch();
   spDisp.Invoke0(L"Open");
   CComVariant vArg1 = 0L;
   CComVariant vArg2 = -1L;
   spDisp.Invoke2(L"SetSelection", &vArg1, &vArg2);
   vArg1 = bstrResult;
   spDisp.Invoke1(L"ReplaceSelection", &vArg1);
   return TRUE;
}

