// Sql.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "resource.h"

#include "Project.h"
#include "Globals.h"
#include "WizardSheet.h"


IDevEnv* _pDevEnv;
CComModule _Module;
COledbSystem _DbSystem;



/////////////////////////////////////////////////////////////////////////////
// Wizard/Property sheet 

class CWizardListener : public IWizardListener
{
public:
   BOOL OnInitProperties(IWizardManager* pManager, IElement* pElement)
   {
      return TRUE;
   }
   BOOL OnInitWizard(IWizardManager* pManager, IProject* pProject, LPCTSTR pstrName)
   {
      return TRUE;
   }
   BOOL OnInitOptions(IWizardManager* pManager, ISerializable* pArc)
   {
      static CAdvancedEditOptionsPage s_pageSqlAdvanced;

      static CString sTitle(MAKEINTRESOURCE(IDS_TREE_ADVANCED));

      s_pageSqlAdvanced.SetTitle((LPCTSTR)sTitle);
      s_pageSqlAdvanced.m_sLanguage = _T("sql");
      s_pageSqlAdvanced.m_pArc = pArc;

      pManager->SetWizardGroup(CString(MAKEINTRESOURCE(IDS_TREE_SQL)));
      pManager->AddWizardPage(s_pageSqlAdvanced.IDD, s_pageSqlAdvanced);

      return TRUE;
   }
};


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
   _pDevEnv->ReserveUIRange(40400, 40500);

   AtlAxWinInit();
   AtlInitCommonControls(ICC_TAB_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES);
   ::LoadLibrary(CRichEditCtrl::GetLibraryName());

   CSqlProject::InitializeToolBars();

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
   return PLUGIN_PROJECT;
}

EXTERN_C
IProject* WINAPI Plugin_CreateProject()
{
   return new CSqlProject();
}

EXTERN_C
BOOL WINAPI Plugin_DestroyProject(IProject* pProject)
{
   if( pProject == NULL ) return FALSE;
   delete pProject;
   return TRUE;
}

