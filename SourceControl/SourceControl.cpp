// SourceControl.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "resource.h"

#include "EventMonitor.h"
#include "WizardSheet.h"
#include "Globals.h"


CComModule _Module;
IDevEnv* _pDevEnv;
CEventMonitor _Monitor;
CScCommands _Commands;



#define PLUGIN_NAME        "Source Control"
#define PLUGIN_DESCRIPTION "Adds integration with Source Control (CVS)."


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
   }
   if( dwReason == DLL_PROCESS_DETACH ) 
   {
      _Module.Term();
   }
   return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Wizard/Property sheet

class CWizardListener : public IWizardListener
{
public:
   BOOL OnInitProperties(IWizardManager* /*pManager*/, IElement* /*pElement*/)
   {
      return TRUE;
   }

   BOOL OnInitWizard(IWizardManager* /*pManager*/, IProject* /*pProject*/, LPCTSTR /*pstrName*/)
   {
      return TRUE;
   }

   BOOL OnInitOptions(IWizardManager* pManager, ISerializable* pArc)
   {
      pManager->AddWizardGroup(CString(MAKEINTRESOURCE(IDS_TREE_ROOT)), CString(MAKEINTRESOURCE(IDS_TREE_SOURCECONTROL)));
      static CMainOptionsPage s_pageMainOptions;
      static CDiffOptionsPage s_pageDiffOptions;
      static CString sMainTitle = MAKEINTRESOURCE(IDS_TREE_COMMANDS);
      static CString sDiffTitle = MAKEINTRESOURCE(IDS_TREE_DIFF);
      s_pageMainOptions.SetTitle((LPCTSTR)sMainTitle);
      s_pageMainOptions.m_pArc = pArc;
      pManager->AddWizardPage(s_pageMainOptions.IDD, s_pageMainOptions);
      s_pageDiffOptions.SetTitle((LPCTSTR)sDiffTitle);
      s_pageDiffOptions.m_pArc = pArc;
      pManager->AddWizardPage(s_pageDiffOptions.IDD, s_pageDiffOptions);
      return TRUE;
   }
};


/////////////////////////////////////////////////////////////////////////////
// Plugin interface

EXTERN_C
BOOL WINAPI Plugin_Initialize(IDevEnv* pDevEnv)
{
   _pDevEnv = pDevEnv;

   _pDevEnv->AddAppListener(&_Monitor);
   _pDevEnv->AddIdleListener(&_Monitor);

   _pDevEnv->ReserveUIRange(42000, 42099);

   _Commands.Init();

   static CWizardListener wizard;
   _pDevEnv->AddWizardListener(&wizard);

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
   // Here we customize the standard application menu
   hMenu = ::GetSubMenu(hMenu, MENUPOS_VIEW_FB);
   hMenu = ::GetSubMenu(hMenu, 2);
   TCHAR szTitle[128] = { 0 };
   AtlLoadString(IDS_REPOSITORY, szTitle, 127);
   ::AppendMenu(hMenu, MF_STRING, ID_VIEW_REPOSITORY, szTitle);
}

EXTERN_C
VOID WINAPI Plugin_SetPopupMenu(IElement* pElement, HMENU hMenu)
{
   // Here we customize the popup menu for a popup-element of the "Folder"-type or
   // for a project or project file.
   if( pElement == NULL ) return;
   bool bFound = false;
   // We'll accept everything that identifies itself as a folder
   if( !bFound ) {
      TCHAR szType[32] = { 0 };
      pElement->GetType(szType, 31);
      if( _tcscmp(szType, _T("Folder")) == 0 ) bFound = true;
   }
   // We'll also accept elements that return a filename
   if( !bFound ) {
      CComDispatchDriver dd = pElement->GetDispatch();
      if( dd == NULL ) return;
      CComVariant v;
      dd.GetPropertyByName(OLESTR("Filename"), &v);
      if( v.vt == VT_BSTR && ::SysStringLen(v.bstrVal) > 0 ) bFound = true;
   }
   // ...and projects that have an ExecCommand command
   if( !bFound ) {
      CComDispatchDriver dd = pElement->GetDispatch();
      if( dd == NULL ) return;
      DISPID DispId = 0;
      if( SUCCEEDED(dd.GetIDOfName(L"ExecCommand", &DispId)) ) bFound = true;
   }
   if( !bFound ) return;
   // User can disable integration through settings
   TCHAR szBuffer[32] = { 0 };
   _pDevEnv->GetProperty(_T("sourcecontrol.enable"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("true")) != 0 ) return;
   // Finally we can merge the menu
   HMENU hSrcMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_SOURCECONTROL));
   ::MergeMenu(hMenu, hSrcMenu, ::GetMenuItemCount(hMenu) - 2);
   ::DestroyMenu(hSrcMenu);
   // Remember last element because we're going to
   // process the command on this one later.
   // BUG: No we can't expect this to survive!!
   _Commands.SetCurElement(pElement);
}

