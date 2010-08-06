// RemoteProject.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include "Project.h"
#include "WizardSheet.h"
#include "ViewSerializer.h"


CComModule _Module;
WSAInit _wsaini;
IDevEnv* _pDevEnv;


/////////////////////////////////////////////////////////////////////////////
// DllMain

EXTERN_C
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{  
   if( dwReason == DLL_PROCESS_ATTACH ) 
   {
     _Module.Init(NULL, hInstance);
     ::DisableThreadLibraryCalls(hInstance);
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
      static CAdvancedEditOptionsPage s_pageCppAdvanced;

      static CString sTitle(MAKEINTRESOURCE(IDS_TREE_ADVANCED));

      s_pageCppAdvanced.SetTitle(static_cast<LPCTSTR>(sTitle));
      s_pageCppAdvanced.m_sLanguage = _T("cpp");
      s_pageCppAdvanced.m_pArc = pArc;

      pManager->SetWizardGroup(CString(MAKEINTRESOURCE(IDS_TREE_CPP)));
      pManager->AddWizardPage(s_pageCppAdvanced.IDD, s_pageCppAdvanced);

      return TRUE;
   }
};


/////////////////////////////////////////////////////////////////////////////
// Plugin interface

EXTERN_C
BOOL WINAPI Plugin_Initialize(IDevEnv* pDevEnv)
{
   _pDevEnv = pDevEnv;

   ::LoadLibrary(CRichEditCtrl::GetLibraryName());
   AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES);

   CRemoteProject::InitializeToolBars();

   _pDevEnv->SetProperty(_T("file.extension.c"), _T("C Source"));
   _pDevEnv->SetProperty(_T("file.extension.cc"), _T("C Source"));
   _pDevEnv->SetProperty(_T("file.extension.cpp"), _T("C++ Source"));
   _pDevEnv->SetProperty(_T("file.extension.cxx"), _T("C++ Source"));
   _pDevEnv->SetProperty(_T("file.extension.h"), _T("C/C++ Header"));
   _pDevEnv->SetProperty(_T("file.extension.hh"), _T("C/C++ Header"));
   _pDevEnv->SetProperty(_T("file.extension.hxx"), _T("C/C++ Header"));
   _pDevEnv->SetProperty(_T("file.extension.hpp"), _T("C/C++ Header"));
   _pDevEnv->SetProperty(_T("file.extension.inl"), _T("C/C++ Source"));
   _pDevEnv->SetProperty(_T("file.extension.inc"), _T("C/C++ Source"));
   _pDevEnv->SetProperty(_T("file.extension.bas"), _T("BASIC File"));
   _pDevEnv->SetProperty(_T("file.extension.vbs"), _T("VBScript Script File"));
   _pDevEnv->SetProperty(_T("file.extension.java"), _T("Java Source"));
   _pDevEnv->SetProperty(_T("file.extension.txt"), _T("Text Document"));
   _pDevEnv->SetProperty(_T("file.extension.py"), _T("Python Script File"));
   _pDevEnv->SetProperty(_T("file.extension.pl"), _T("Perl Script File"));
   _pDevEnv->SetProperty(_T("file.extension.php"), _T("PHP Web Script"));
   _pDevEnv->SetProperty(_T("file.extension.asp"), _T("ASP Web Script"));

   _pDevEnv->ReserveUIRange(40000, 40000 + 200);

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
   return PLUGIN_PROJECT | PLUGIN_FILETYPE;
}

EXTERN_C
IProject* WINAPI Plugin_CreateProject()
{
   return new CRemoteProject();
}

EXTERN_C
BOOL WINAPI Plugin_DestroyProject(IProject* pProject)
{
   if( pProject == NULL ) return FALSE;
   delete pProject;
   return TRUE;
}

EXTERN_C
UINT WINAPI Plugin_QueryAcceptFile(LPCTSTR pstrFilename)
{
   // If a filename looks like it's from a remote server (UNIX syntax)
   // we'll try to snag it at any cost...
   if( pstrFilename[0] == '/' ) return 80;   
   // We support all file types!!!
   return 20;
}

EXTERN_C
IView* WINAPI Plugin_CreateView(LPCTSTR pstrFilename, IProject* pProject, IElement* pParent)
{
   if( pstrFilename == NULL ) return NULL;

   BOOL bRemoteFile = (pstrFilename[0] == '/');

   CRemoteProject* pCppProject = NULL;
   // Only accept IProject attachment if it comes from another
   // "Remote C++" plugin. Otherwise we don't know how to handle
   // internal messaging (eg. debugging).
   if( bRemoteFile && pProject == NULL ) {
      pProject = _pDevEnv->GetSolution()->GetActiveProject();
   }
   if( pProject != NULL ) {
      TCHAR szType[64] = { 0 };
      pProject->GetClass(szType, 63);
      if( _tcscmp(szType, _T(PLUGIN_NAME)) == 0 ) pCppProject = (CRemoteProject*) pProject;
   }

   // Create view
   CTextFile* pView = CreateViewFromFilename(_pDevEnv, 
      pstrFilename, 
      pCppProject,
      pProject,
      pParent);
   if( pView == NULL ) return NULL;

   CViewSerializer arc;
   arc.Add(_T("name"), ::PathFindFileName(pstrFilename));
   arc.Add(_T("filename"), pstrFilename);
   arc.Add(_T("location"), bRemoteFile ? _T("remote") : _T("local"));
   pView->Load(&arc);

   return pView;
}

