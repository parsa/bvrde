// GenEdit.cpp : Defines the entry point for the DLL application.
//

#include "StdAfx.h"

#include "GenEdit.h"
#include "ScintillaView.h"

CComModule _Module;


/////////////////////////////////////////////////////////////////////////////
// DLL Main 

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
   switch( dwReason ) {
   case DLL_PROCESS_ATTACH:
      {
         _Module.Init(NULL, hInstance);
         ::DisableThreadLibraryCalls(hInstance);
      }
      break;
   case DLL_PROCESS_DETACH:
      {
         _Module.Term();
      }
      break;
   }
   return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Exported function

HWND GENEDIT_API Bvrde_CreateScintillaView(HWND hWndParent, 
                                           IDevEnv* pDevEnv, 
                                           LPCTSTR pstrFilename,
                                           LPCTSTR pstrLanguage)
{
   HMODULE hScintilla = ::LoadLibrary(CScintillaCtrl::GetLibraryName());
   ATLASSERT(hScintilla); hScintilla;

   CScintillaView* pView = new CScintillaView(pDevEnv);
   HWND hWnd = pView->Create(hWndParent, CWindow::rcDefault, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_CLIENTEDGE);
   if( !::IsWindow(hWnd) ) {
      delete pView;
      return NULL;
   }
   pView->SetFilename(pstrFilename);
   pView->SetLanguage(pstrLanguage);
   return hWnd;
}
