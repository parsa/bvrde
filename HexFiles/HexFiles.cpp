// WebFiles.Html : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "resource.h"

#include "Files.h"
#include "WizardSheet.h"


CComModule _Module;
IDevEnv* _pDevEnv;


BOOL APIENTRY DllMain(HINSTANCE hInstance, 
                      DWORD dwReason, 
                      LPVOID /*lpReserved*/)
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
      static CHexEditorOptionsPage s_pageHexEditor;

      static CString sTitle(MAKEINTRESOURCE(IDS_TREE_HEXEDITOR));

      s_pageHexEditor.SetTitle((LPCTSTR)sTitle);
      s_pageHexEditor.m_pArc = pArc;

      pManager->AddWizardGroup(CString(MAKEINTRESOURCE(IDS_TREE_EDITORS)), CString(MAKEINTRESOURCE(IDS_TREE_BINARY)));
      pManager->AddWizardPage(s_pageHexEditor.IDD, s_pageHexEditor);

      return TRUE;
   }
};


/////////////////////////////////////////////////////////////////////////////
// Plugin interface

EXTERN_C
BOOL WINAPI Plugin_Initialize(IDevEnv* pDevEnv)
{
   _pDevEnv = pDevEnv;

   AtlAxWinInit();
   AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES);

   _pDevEnv->SetProperty(_T("file.extension.obj"), _T("Object File"));
   _pDevEnv->SetProperty(_T("file.extension.bin"), _T("Binary File"));

   static CWizardListener wizard;
   _pDevEnv->AddWizardListener(&wizard);

   _pDevEnv->ReserveUIRange(43600, 43600 + 100);

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
   return PLUGIN_FILETYPE;
}

EXTERN_C
UINT WINAPI Plugin_QueryAcceptFile(LPCTSTR pstrFilename)
{
   LPTSTR pstrExt = ::PathFindExtension(pstrFilename);
   if( pstrExt == NULL ) return 0;
   LPCTSTR pstrExtensions[] =
   {
      _T(".OBJ"),
      _T(".DAT"),
      _T(".RES"),
      _T(".BIN"),
      _T(".EXE"),
      _T(".DLL"),
      _T(".A"),
      _T(".SO"),
   };
   LPCTSTR* ppstr = pstrExtensions;
   for( int i = 0; i < sizeof(pstrExtensions) / sizeof(LPCTSTR); i++ ) {
      if( _tcsicmp(pstrExt, *ppstr) == 0 ) return 30;
      ppstr++;
   }

   // If it's a local file and it contains primarily binary data
   // we'll suggest this editor for all file-extensions.
   // TODO: This is painfully slow! We're doing this for all
   //       files during project loading!!!!
   if( _tcslen(pstrFilename) > 3 && pstrFilename[1] == ':' ) {
      CFile f;
      if( f.Open(pstrFilename) ) {
         if( f.GetSize() > 64 ) {
            BYTE bBuffer[64];
            f.Read(bBuffer, 64);
            int iPoints = 0;
            for( int i = 0; i < 63; i++ ) {
               if( bBuffer[i] > 128 ) iPoints++;
               if( bBuffer[i] == 0 && bBuffer[i + 1] == 0 ) iPoints += 32;
            }
            if( iPoints > 32 ) return 30;
         }
      }
   }

   return 0;
}

EXTERN_C
IView* WINAPI Plugin_CreateView(LPCTSTR pstrFilename, IProject* pProject, IElement* pParent)
{
   return new CView(pProject, pParent, pstrFilename);
}

