// EmptyProject.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include "Project.h"
#include "ViewSerializer.h"


CComModule _Module;
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
     //
     AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES);
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
   
   _pDevEnv->SetProperty(_T("file.extension.c"), _T("C Source"));
   _pDevEnv->SetProperty(_T("file.extension.cpp"), _T("C++ Source"));
   _pDevEnv->SetProperty(_T("file.extension.cxx"), _T("C++ Source"));
   _pDevEnv->SetProperty(_T("file.extension.h"), _T("C/C++ Header"));
   _pDevEnv->SetProperty(_T("file.extension.hxx"), _T("C/C++ Header"));
   _pDevEnv->SetProperty(_T("file.extension.hpp"), _T("C/C++ Header"));
   _pDevEnv->SetProperty(_T("file.extension.inl"), _T("C/C++ Source"));
   _pDevEnv->SetProperty(_T("file.extension.bas"), _T("BASIC File"));
   _pDevEnv->SetProperty(_T("file.extension.vbs"), _T("VBScript Script File"));
   _pDevEnv->SetProperty(_T("file.extension.java"), _T("Java Source"));
   _pDevEnv->SetProperty(_T("file.extension.txt"), _T("Text Document"));

   _pDevEnv->ReserveUIRange(43000, 43000 + 200);

   CEmptyProject::InitializeToolBars();
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
   return new CEmptyProject();
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
   // We support all file types!!!
   return 10;
}

EXTERN_C
IView* WINAPI Plugin_CreateView(LPCTSTR pstrFilename, IProject* pProject, IElement* pParent)
{
   if( pstrFilename == NULL ) return NULL;

   // Only accept IProject attachment if it comes from another
   // "Empty Project" plugin. Otherwise we don't know how to handle
   // internal messaging (eg. debugging).
   CEmptyProject* pLocalProject = NULL;
   if( pProject ) {
      TCHAR szType[64] = { 0 };
      pProject->GetClass(szType, 63);
      if( _tcscmp(szType, _T(PLUGIN_NAME)) != 0 ) pLocalProject = (CEmptyProject*) pProject;
   }

   // Create view
   CTextFile* pView = CreateViewFromFilename(pstrFilename, pLocalProject, pProject, pParent);
   if( pView == NULL ) return NULL;

   CViewSerializer arc;
   arc.Add(_T("name"), ::PathFindFileName(pstrFilename));
   arc.Add(_T("filename"), pstrFilename);
   pView->Load(&arc);

   return pView;
}

