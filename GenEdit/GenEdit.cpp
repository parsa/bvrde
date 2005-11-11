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

/**
 * The GenEdit component has one exported method: Bvrde_CreateScintillaView.
 * This method registers and creates the default editor, which has the syntax hightlighting
 * ability, find/goto dialags, auto-indent, auto-completion for some languages etc etc.
 * In fairness this is just a wrapper around the Scintilla control and a default framework 
 * around the features it exposes.
 */
HWND GENEDIT_API Bvrde_CreateScintillaView(HWND hWndParent, 
                                           IDevEnv* pDevEnv, 
                                           LPCTSTR pstrFilename,
                                           LPCTSTR pstrLanguage)
{
   ATLASSERT(::IsWindow(hWndParent));
   ATLASSERT(pDevEnv);

   static HMODULE hScintilla = ::LoadLibrary(CScintillaCtrl::GetLibraryName());
   ATLASSERT(hScintilla); hScintilla;

   CScintillaView* pView = new CScintillaView(pDevEnv);
   ATLASSERT(pView);
   if( pView == NULL ) return NULL;
   HWND hWnd = pView->Create(hWndParent, CWindow::rcDefault, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_CLIENTEDGE);
   ATLASSERT(::IsWindow(hWnd));
   if( !::IsWindow(hWnd) ) {
      delete pView;
      return NULL;
   }
   pView->SetFilename(pstrFilename);
   pView->SetLanguage(pstrLanguage);

   return hWnd;
}

