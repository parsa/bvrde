// FileManager.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "resource.h"

#include "EventMonitor.h"


CShellModule _Module;
IDevEnv* _pDevEnv;
CEventMonitor _Monitor;


#define PLUGIN_NAME        "File Manager"
#define PLUGIN_DESCRIPTION "Adds a File Explorer browser for local files."


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
     AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);
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
   _pDevEnv->ReserveUIRange(40900, 40901);
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
   hMenu = ::GetSubMenu(hMenu, 2);
   hMenu = ::GetSubMenu(hMenu, 2);
   TCHAR szTitle[128] = { 0 };
   ::LoadString(_Module.GetResourceInstance(), IDS_TITLE, szTitle, 127);
   ::AppendMenu(hMenu, MF_STRING, ID_VIEW_FILEMANAGER, szTitle);
}

