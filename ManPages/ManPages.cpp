// ManPages.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include "Frame.h"


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
      //
      AtlAxWinInit();
      AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);
   }
   else if( dwReason == DLL_PROCESS_DETACH ) 
   {
     _Module.Term();
   }
   return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Interface

IDevEnv* _pDevEnv;
CComModule _Module;


BOOL APIENTRY ManPages_ShowHelp(IDevEnv* pDevEnv, LPCWSTR pstrKeyword, LPCWSTR pstrLanguage)
{      
   ATLASSERT(pDevEnv);
   ATLASSERT(!::IsBadStringPtr(pstrKeyword,-1));

   _pDevEnv = pDevEnv;
   
   CFrameWindow* pFrame = NULL;
   HWND hWnd = ::FindWindow(_T("BVRDE_ManPage"), NULL);
   if( hWnd ) {
      CWinProp prop = hWnd;
      prop.GetProperty(_T("ManPage"), pFrame);
   }
   else {
      pFrame = new CFrameWindow();
      if( pFrame == NULL ) return FALSE;
      CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
      CWinProp prop = pFrame->Create(wndMain, CWindow::rcDefault);
      prop.SetProperty(_T("Manpage"), pFrame);     
   }
   if( pFrame == NULL ) return FALSE;
   if( pFrame->SetPage(pstrKeyword, pstrLanguage, -1) ) pFrame->ShowWindow(SW_SHOW);
   return TRUE;
}

