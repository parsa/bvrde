// FindFiles.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "resource.h"

#include "EventMonitor.h"


CComModule _Module;
IDevEnv* _pDevEnv;
CEventMonitor _Monitor;


#define PLUGIN_NAME        "Find in Files"
#define PLUGIN_DESCRIPTION "Searches for a string in multiple files."


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
// Plugin interface

EXTERN_C
BOOL WINAPI Plugin_Initialize(IDevEnv* pDevEnv)
{
   _pDevEnv = pDevEnv;
   _pDevEnv->AddAppListener(&_Monitor);
   _pDevEnv->AddIdleListener(&_Monitor);
   _pDevEnv->ReserveUIRange(ID_EDIT_FINDFILES, ID_EDIT_FINDFILES + 1);

   HMODULE hRichEdit = ::LoadLibrary(CRichEditCtrl::GetLibraryName());
   ATLASSERT(hRichEdit);

   return TRUE;
}

EXTERN_C
BOOL WINAPI Plugin_GetName(LPTSTR pstrName, UINT cchMax)
{
   ::lstrcpyn(pstrName, _T(PLUGIN_NAME), cchMax);
   return TRUE;
}

EXTERN_C
BOOL WINAPI Plugin_GetDescription(LPTSTR pstrDescription, UINT cchMax)
{
   ::lstrcpyn(pstrDescription, _T(PLUGIN_DESCRIPTION), cchMax);
   return TRUE;
}

EXTERN_C
LONG WINAPI Plugin_GetType()
{   
   return PLUGIN_EXTENSION;
}

EXTERN_C
VOID WINAPI Plugin_SetMenu(HMENU hMenu)
{
   // Modify "Edit" menu
   hMenu = ::GetSubMenu(hMenu, 1);
   int nCount = ::GetMenuItemCount(hMenu);   
   for( int i = 0; i < nCount; i++ ) {
      // Find the position of the "Find" menu item and
      // insert new item right after.
      if( ::GetMenuItemID(hMenu, i) == ID_EDIT_FIND ) {
         TCHAR szTitle[128] = { 0 };
         ::LoadString(_Module.GetResourceInstance(), IDS_TITLE, szTitle, 127);
         MENUITEMINFO mii = { 0 };
         mii.cbSize = sizeof(mii);
         mii.fMask = MIIM_ID | MIIM_TYPE;
         mii.fType = MFT_STRING;
         mii.wID = ID_EDIT_FINDFILES;
         mii.dwTypeData = szTitle;
         ::InsertMenuItem(hMenu, i + 1, TRUE, &mii);
         break;
      }
   }
}

