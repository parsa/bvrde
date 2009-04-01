// WebFiles.Html : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "resource.h"

#include "Files.h"
#include "WizardSheet.h"


CComModule _Module;
IDevEnv* _pDevEnv;
CToolBarCtrl m_ctrlToolbar;


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
      static CAdvancedEditOptionsPage s_pageHtmlAdvanced;
      static CAdvancedEditOptionsPage s_pagePhpAdvanced;
      static CAdvancedEditOptionsPage s_pageAspAdvanced;
      static CAdvancedEditOptionsPage s_pageXmlAdvanced;

      static CString sAdvanced(MAKEINTRESOURCE(IDS_TREE_ADVANCED));

      s_pageHtmlAdvanced.SetTitle(static_cast<LPCTSTR>(sAdvanced));
      s_pageHtmlAdvanced.m_sLanguage = _T("html");
      s_pageHtmlAdvanced.m_pArc = pArc;
      s_pagePhpAdvanced.SetTitle(static_cast<LPCTSTR>(sAdvanced));
      s_pagePhpAdvanced.m_sLanguage = _T("php");
      s_pagePhpAdvanced.m_pArc = pArc;
      s_pageAspAdvanced.SetTitle(static_cast<LPCTSTR>(sAdvanced));
      s_pageAspAdvanced.m_sLanguage = _T("asp");
      s_pageAspAdvanced.m_pArc = pArc;
      s_pageXmlAdvanced.SetTitle(static_cast<LPCTSTR>(sAdvanced));
      s_pageXmlAdvanced.m_sLanguage = _T("xml");
      s_pageXmlAdvanced.m_pArc = pArc;

      pManager->SetWizardGroup(CString(MAKEINTRESOURCE(IDS_TREE_HTML)));
      pManager->AddWizardPage(s_pagePhpAdvanced.IDD, s_pagePhpAdvanced);

      pManager->SetWizardGroup(CString(MAKEINTRESOURCE(IDS_TREE_PHP)));
      pManager->AddWizardPage(s_pageAspAdvanced.IDD, s_pageAspAdvanced);

      pManager->SetWizardGroup(CString(MAKEINTRESOURCE(IDS_TREE_ASP)));
      pManager->AddWizardPage(s_pageHtmlAdvanced.IDD, s_pageHtmlAdvanced);

      pManager->SetWizardGroup(CString(MAKEINTRESOURCE(IDS_TREE_XML)));
      pManager->AddWizardPage(s_pageXmlAdvanced.IDD, s_pageXmlAdvanced);

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

   _pDevEnv->SetProperty(_T("file.extension.html"), _T("HTML File"));
   _pDevEnv->SetProperty(_T("file.extension.aspx"), _T("ASPX File"));
   _pDevEnv->SetProperty(_T("file.extension.htm"), _T("HTML File"));
   _pDevEnv->SetProperty(_T("file.extension.php"), _T("PHP File"));
   _pDevEnv->SetProperty(_T("file.extension.asp"), _T("ASP File"));
   _pDevEnv->SetProperty(_T("file.extension.xml"), _T("XML File"));
   _pDevEnv->SetProperty(_T("file.extension.xsl"), _T("XML File"));
   _pDevEnv->SetProperty(_T("file.extension.xslt"), _T("XML File"));

   static CWizardListener wizard;
   _pDevEnv->AddWizardListener(&wizard);

   _pDevEnv->ReserveUIRange(43500, 43500 + 100);

   // Create HTML Editor toolbar
   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
   m_ctrlToolbar = CFrameWindowImplBase<>::CreateSimpleToolBarCtrl(wndMain, IDR_HTML, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
   _pDevEnv->AddToolBar(m_ctrlToolbar, _T("Web"), CString(MAKEINTRESOURCE(IDS_CAPTION_TOOLBAR)));
   AddCommandBarImages(IDR_HTML);

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
   if( pstrExt == NULL ) return FALSE;
   LPCTSTR pstrExtensions[] =
   {
      _T(".PHTML"),
      _T(".HTML"),
      _T(".HTM"),
      _T(".XML"),
      _T(".XSLT"),
      _T(".XSL"),
      _T(".PHP"),
      _T(".PHP3"),
      _T(".PHP4"),
      _T(".PHP5"),
      _T(".TPL"),
      _T(".ASP"),
      _T(".ASPX"),
      _T(".JSP"),
      _T(".PY"),
   };
   LPCTSTR* ppstr = pstrExtensions;
   for( int i = 0; i < sizeof(pstrExtensions) / sizeof(LPCTSTR); i++ ) {
      if( _tcsicmp(pstrExt, *ppstr) == 0 ) return 40;
      ppstr++;
   }
   return 0;
}

EXTERN_C
IView* WINAPI Plugin_CreateView(LPCTSTR pstrFilename, IProject* pProject, IElement* pParent)
{
   LPCTSTR pstrExt = ::PathFindExtension(pstrFilename);
   if( pstrExt == NULL ) return NULL;
   enum
   {
      TYPE_NONE,
      TYPE_HTML,
      TYPE_PHP,
      TYPE_ASP,
      TYPE_XML,
   } iType = TYPE_NONE;
   if( _tcsicmp(pstrExt, _T(".PHTML")) == 0 ) iType = TYPE_PHP;
   if( _tcsicmp(pstrExt, _T(".HTML")) == 0 ) iType = TYPE_HTML;
   if( _tcsicmp(pstrExt, _T(".HTM")) == 0 ) iType = TYPE_HTML;
   if( _tcsicmp(pstrExt, _T(".ASP")) == 0 ) iType = TYPE_ASP;
   if( _tcsicmp(pstrExt, _T(".ASPX")) == 0 ) iType = TYPE_ASP;
   if( _tcsicmp(pstrExt, _T(".PHP")) == 0 ) iType = TYPE_PHP;
   if( _tcsicmp(pstrExt, _T(".PHP3")) == 0 ) iType = TYPE_PHP;
   if( _tcsicmp(pstrExt, _T(".PHP4")) == 0 ) iType = TYPE_PHP;
   if( _tcsicmp(pstrExt, _T(".PHP5")) == 0 ) iType = TYPE_PHP;
   if( _tcsicmp(pstrExt, _T(".JSP")) == 0 ) iType = TYPE_HTML;
   if( _tcsicmp(pstrExt, _T(".TPL")) == 0 ) iType = TYPE_HTML;
   if( _tcsicmp(pstrExt, _T(".XML")) == 0 ) iType = TYPE_XML;
   if( _tcsicmp(pstrExt, _T(".XSL")) == 0 ) iType = TYPE_XML;
   if( _tcsicmp(pstrExt, _T(".XSLT")) == 0 ) iType = TYPE_XML;
   if( _tcsicmp(pstrExt, _T(".PY")) == 0 ) iType = TYPE_HTML;

   if( iType == TYPE_HTML ) return new CHtmlView(pProject, pParent, pstrFilename);
   if( iType == TYPE_XML ) return new CXmlView(pProject, pParent, pstrFilename);
   if( iType == TYPE_PHP ) return new CPhpView(pProject, pParent, pstrFilename);
   if( iType == TYPE_ASP ) return new CAspView(pProject, pParent, pstrFilename);

   return NULL;
}

