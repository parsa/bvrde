
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"
#include "Files.h"

#pragma code_seg( "MISC" )

#include "Globals.h"
#include "WizardSheet.h"


CSqlProject::CSqlProject() :
   m_bLoaded(false), 
   m_bIsDirty(true)
{
}

CSqlProject::~CSqlProject()
{
   Close();
}

BOOL CSqlProject::Initialize(IDevEnv* pEnv, LPCTSTR pstrPath)
{
   _pDevEnv = pEnv;
   m_wndMain = pEnv->GetHwnd(IDE_HWND_MAIN);

   CSqlProject::InitializeToolBars();

   if( m_accel.IsNull() ) m_accel.LoadAccelerators(IDR_ACCELERATOR);

   _pDevEnv->AddAppListener(this);
   _pDevEnv->AddTreeListener(this);
   _pDevEnv->AddWizardListener(this);

   _pDevEnv->ShowToolBar(m_ctrlToolbar, TRUE, TRUE);

   m_bLoaded = true;
   return TRUE;
}

BOOL CSqlProject::Load(ISerializable* pArc)
{
   pArc->Read(_T("name"), m_sName.GetBufferSetLength(128), 128);
   m_sName.ReleaseBuffer();

   if( !_LoadSettings(pArc) ) return FALSE;
   if( !_LoadConnections(pArc) ) return FALSE;

   m_bIsDirty = false;
   return TRUE;
}

BOOL CSqlProject::Save(ISerializable* pArc)
{
   pArc->Write(_T("name"), m_sName);
   pArc->Write(_T("type"), _T(PLUGIN_NAME));

   if( !_SaveSettings(pArc) ) return FALSE;
   if( !_SaveConnections(pArc) ) return FALSE;

   m_bIsDirty = false;
   return TRUE;
}

BOOL CSqlProject::GetName(LPTSTR pstrName, UINT cchMax) const
{
   _tcsncpy(pstrName, m_sName, cchMax);
   return TRUE;
}

BOOL CSqlProject::SetName(LPCTSTR pstrName)
{
   ATLASSERT(!::IsBadStringPtr(pstrName,-1));
   m_sName = pstrName;
   m_bIsDirty = true;
   return TRUE;
}

BOOL CSqlProject::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Project"), cchMax);
   return TRUE;
}

IElement* CSqlProject::GetParent() const
{
   return _pDevEnv->GetSolution();
}

IDispatch* CSqlProject::GetDispatch()
{
   return _pDevEnv->CreateStdDispatch(_T("Project"), this);
}

BOOL CSqlProject::GetClass(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T(PLUGIN_NAME), cchMax);
   return TRUE;
}

BOOL CSqlProject::IsDirty() const
{
   if( m_bIsDirty ) return TRUE;
   for( int i = 0; i < m_aViews.GetSize(); i++ ) if( m_aViews[i]->IsDirty() ) return TRUE;
   return FALSE;
}

BOOL CSqlProject::Close()
{
   if( !m_bLoaded ) return TRUE;

   _pDevEnv->RemoveAppListener(this);
   _pDevEnv->RemoveTreeListener(this);
   _pDevEnv->RemoveIdleListener(this);
   _pDevEnv->RemoveWizardListener(this);

   // Close the views nicely
   for( int i = 0; i < m_aViews.GetSize(); i++ ) m_aViews[i]->CloseView();

   m_bLoaded = false;
   return TRUE;
}

BOOL CSqlProject::CreateProject()
{
   TCHAR szName[MAX_PATH + 1] = { 0 };
   GetName(szName, MAX_PATH);
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   HTREEITEM hRoot = ctrlTree.GetRootItem();
   HTREEITEM hProject = ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
      szName, 
      IDE_TREEIMAGE_PROJECT, IDE_TREEIMAGE_PROJECT, 
      0, 0,
      (LPARAM) this,
      hRoot, 
      TVI_LAST);
   for( int i = 0; i < m_aViews.GetSize(); i++ ) 
   {
      m_aViews[i]->GetName(szName, MAX_PATH);
      HTREEITEM hItem = ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
         szName, 
         IDE_TREEIMAGE_DATABASE, IDE_TREEIMAGE_DATABASE, 
         0, 0,
         (LPARAM) m_aViews[i],
         hProject, 
         TVI_LAST);
   }
   ctrlTree.Expand(hRoot);
   ctrlTree.Expand(hProject);
   return TRUE;
}

void CSqlProject::ActivateProject()
{
   _pDevEnv->AddIdleListener(this);
   _pDevEnv->AddCommandListener(this);
}

void CSqlProject::DeactivateProject()
{
   _pDevEnv->RemoveIdleListener(this);
   _pDevEnv->RemoveCommandListener(this);
}

void CSqlProject::ActivateUI()
{
   CMenu menu;
   menu.LoadMenu(IDR_MAIN);
   ATLASSERT(menu.IsMenu());
   CMenuHandle menuMain = _pDevEnv->GetMenuHandle(IDE_HWND_MAIN);
   CMenuHandle menuFile = menuMain.GetSubMenu(MENUPOS_FILE_FB);
   CMenuHandle menuView = menuMain.GetSubMenu(MENUPOS_VIEW_FB);
   MergeMenu(menuFile.GetSubMenu(SUBMENUPOS_FILE_ADD_FB), menu.GetSubMenu(3), 2);
   MergeMenu(menuMain, menu.GetSubMenu(0), 3);
   MergeMenu(menuView.GetSubMenu(SUBMENUPOS_VIEW_VIEWS_FB), menu.GetSubMenu(2), 0);
}

void CSqlProject::DeactivateUI()
{
}

IView* CSqlProject::GetItem(INT iIndex)
{
   if( iIndex < 0 || iIndex >= m_aViews.GetSize() ) return NULL;
   return m_aViews[iIndex];
}

INT CSqlProject::GetItemCount() const
{
   return m_aViews.GetSize();
}

// IAppMessageListener

LRESULT CSqlProject::OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   LRESULT lResult = 0;
   bHandled = ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult);
   return lResult;
}

BOOL CSqlProject::PreTranslateMessage(MSG* pMsg)
{
   if( _pDevEnv->GetSolution()->GetFocusProject() != this ) return FALSE;
   if( !m_accel.IsNull() && m_accel.TranslateAccelerator(m_wndMain, pMsg) ) return TRUE;
   return FALSE;
}

// IIdleListener

void CSqlProject::OnIdle(IUpdateUI* pUIBase)
{
   CString sFileType;
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement ) {
      pElement->GetType(sFileType.GetBufferSetLength(64), 64);
      sFileType.ReleaseBuffer();
   }

   pUIBase->UIEnable(ID_ADD_DATABASE, sFileType == _T("Project"));      

   if( pElement ) {
      pUIBase->UIEnable(ID_VIEW_OPEN, TRUE);
      pUIBase->UIEnable(ID_FILE_RENAME, TRUE);
      pUIBase->UIEnable(ID_FILE_DELETE, TRUE);
      pUIBase->UIEnable(ID_VIEW_PROPERTIES, TRUE);
      pUIBase->UIEnable(ID_PROJECT_SET_DEFAULT, TRUE);
   }
}

void CSqlProject::OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax)
{
   AtlLoadString(wID, pstrText, cchMax);
}

// ITreeMessageListener

LRESULT CSqlProject::OnTreeMessage(LPNMHDR pnmh, BOOL& bHandled)
{
   if( _GetSelectedTreeElement() == NULL ) return 0;
   LRESULT lResult = 0;
   bHandled = ProcessWindowMessage(pnmh->hwndFrom, WM_NOTIFY, (WPARAM) pnmh->idFrom, (LPARAM) pnmh, lResult);
   return lResult;
}

// ICustomCommandListener

void CSqlProject::OnUserCommand(LPCTSTR pstrCommand, BOOL& bHandled)
{
   bHandled = FALSE;
   if( _tcsncmp(pstrCommand, _T("sql "), 4) == 0 ) 
   {
      IProject* pProject = _pDevEnv->GetSolution()->GetActiveProject();
      if( pProject == this ) bHandled = TRUE;
   }
   if( _tcsicmp(pstrCommand, _T("help")) == 0 ) 
   {
      CRichEditCtrl ctrlEdit = _pDevEnv->GetHwnd(IDE_HWND_COMMANDVIEW);
      AppendRtfText(ctrlEdit, CString(MAKEINTRESOURCE(IDS_HELP)));
   }
}

void CSqlProject::OnMenuCommand(LPCTSTR pstrType, LPCTSTR pstrCommand, LPCTSTR pstrArguments, LPCTSTR /*pstrPath*/, int /*iFlags*/, BOOL& bHandled)
{
   bHandled = FALSE;
   if( _tcscmp(pstrType, _T("sql")) != 0 ) return;
   CString sCommandline;
   sCommandline.Format(_T("sql %s %s"), pstrCommand, pstrArguments);
   OnUserCommand(sCommandline, bHandled);
}

// IWizardListener

BOOL CSqlProject::OnInitProperties(IWizardManager* pManager, IElement* pElement)
{
   if( pElement != this ) return TRUE;
   m_pWizardView = NULL;
   return TRUE;
}

BOOL CSqlProject::OnInitWizard(IWizardManager* pManager, IProject* pProject, LPCTSTR pstrName)
{
   if( pProject != this ) return TRUE;

   static CProviderPage s_pageProvider;
   static CString sProviderTitle(MAKEINTRESOURCE(IDS_WIZARD_TITLE_PROVIDER));
   static CString sProviderSubTitle(MAKEINTRESOURCE(IDS_WIZARD_SUBTITLE_PROVIDER));
   s_pageProvider.m_pProject = this;
   s_pageProvider.SetHeaderTitle(sProviderTitle);
   s_pageProvider.SetHeaderSubTitle(sProviderSubTitle);

   static CConnectionPage s_pageConnection;
   static CString sConnectionTitle(MAKEINTRESOURCE(IDS_WIZARD_TITLE_CONNECTION));
   static CString sConnectionSubTitle(MAKEINTRESOURCE(IDS_WIZARD_SUBTITLE_CONNECTION));
   s_pageConnection.m_pProject = this;
   s_pageConnection.SetHeaderTitle(sConnectionTitle);
   s_pageConnection.SetHeaderSubTitle(sConnectionSubTitle);

   m_pWizardView = NULL;

   pManager->AddWizardPage(s_pageProvider.IDD, s_pageProvider);
   pManager->AddWizardPage(s_pageConnection.IDD, s_pageConnection);

   SetName(pstrName);
   return TRUE;
}

BOOL CSqlProject::OnInitOptions(IWizardManager* pManager, ISerializable* pArc)
{
   return TRUE;
}

// Implementation

bool CSqlProject::_LoadSettings(ISerializable* pArc)
{
   if( !pArc->ReadGroupBegin(_T("Settings")) ) return true;
   if( !pArc->ReadGroupEnd() ) return false;
   return TRUE;
}

bool CSqlProject::_SaveSettings(ISerializable* pArc)
{
   if( !pArc->WriteGroupBegin(_T("Settings")) ) return false;
   if( !pArc->WriteGroupEnd() ) return false;
   return TRUE;
}

bool CSqlProject::_LoadConnections(ISerializable* pArc)
{
   if( !pArc->ReadGroupBegin(_T("Connections")) ) return true;
   while( pArc->ReadGroupBegin(_T("Connection")) ) {
      CView* pView = new CView(this, this);
      if( pView == NULL ) return false;
      if( !pView->Load(pArc) ) return false;
      m_aViews.Add(pView);
      if( !pArc->ReadGroupEnd() ) return false;
   }
   if( !pArc->ReadGroupEnd() ) return false;
   return TRUE;
}

bool CSqlProject::_SaveConnections(ISerializable* pArc)
{
   if( !pArc->WriteGroupBegin(_T("Connections")) ) return false;
   for( int i = 0; i < m_aViews.GetSize(); i++ ) {
      if( !pArc->WriteGroupBegin(_T("Connection")) ) return false;
      if( !m_aViews[i]->Save(pArc) ) return false;
      if( !pArc->WriteGroupEnd() ) return false;
   }
   if( !pArc->WriteGroupEnd() ) return false;
   return TRUE;
}

IElement* CSqlProject::_GetSelectedTreeElement(HTREEITEM* phItem /*= NULL*/) const
{
   // Get the active/selected tree-item
   if( phItem ) *phItem = NULL;
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   HTREEITEM hItem = ctrlTree.GetDropHilightItem();
   if( hItem == NULL ) hItem = ctrlTree.GetSelectedItem();
   if( hItem == NULL ) return NULL;
   if( phItem ) *phItem = hItem;
   // Let's check that this tree-item actually belongs to this project
   LPARAM lParam = ctrlTree.GetItemData(hItem);
   if( lParam == NULL ) return NULL;
   bool bLocal = false;
   if( lParam == (LPARAM) this ) bLocal = true;
   for( int i = 0; i < m_aViews.GetSize(); i++ ) if( (LPARAM) m_aViews[i] == lParam ) bLocal = true;
   if( !bLocal ) return NULL;
   return (IElement*) lParam;
}

void CSqlProject::InitializeToolBars()
{
   // NOTE: The toolbars are static members of this
   //       class so we need to initialize them once only.
   if( m_ctrlToolbar.IsWindow() ) return;

   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);

   m_ctrlToolbar = CFrameWindowImplBase<>::CreateSimpleToolBarCtrl(wndMain, IDR_TOOLBAR, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

   _pDevEnv->AddToolBar(m_ctrlToolbar, _T("SQL"), CString(MAKEINTRESOURCE(IDS_CAPTION_TOOLBAR)));

   _AddCommandBarImages(IDR_TOOLIMAGES);
}

bool CSqlProject::_AddCommandBarImages(UINT nRes)
{
   CImageListCtrl Images;
   Images.Create(16, 16, ILC_COLOR | ILC_MASK, 16, 1);
   CBitmap bmp;
   bmp.LoadBitmap(IDR_TOOLIMAGES);
   ATLASSERT(!bmp.IsNull());
   if( bmp.IsNull() ) return false;

   HRSRC hRsrc = ::FindResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(nRes), (LPTSTR) RT_TOOLBAR);
   ATLASSERT(hRsrc);
   if( hRsrc == NULL ) return false;
   HGLOBAL hGlobal = ::LoadResource(_Module.GetResourceInstance(), hRsrc);
   ATLASSERT(hGlobal);
   if( hGlobal == NULL ) return false;

   struct _ToolBarData   // toolbar resource data
   {
      WORD wVersion;
      WORD wWidth;
      WORD wHeight;
      WORD wItemCount;
      WORD* GetItems() { return (WORD*)(this + 1); }
   };
   _ToolBarData* pData = (_ToolBarData*) ::LockResource(hGlobal);
   if( pData == NULL ) return FALSE;
   ATLASSERT(pData->wVersion==1);

   if( Images.Add(bmp, RGB(192,192,192)) == -1) return false;

   // Fill the array with command IDs
   WORD* pItems = pData->GetItems();
   int nItems = pData->wItemCount;
   for( int i = 0; i < nItems; i++ ) {
      CIcon icon = Images.GetIcon(i);
      ATLASSERT(!icon.IsNull());
      _pDevEnv->AddCommandBarImage(pItems[i], icon);
   }
   return true;
}

// Message handlers

LRESULT CSqlProject::OnFileRemove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   HTREEITEM hItem = NULL;
   IElement* pElement = _GetSelectedTreeElement(&hItem);
   if( pElement == NULL ) { bHandled = FALSE; return 0; }
   // Remove tree item
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   ctrlTree.DeleteItem(hItem);
   // Remove entire project or file
   if( pElement == this ) {
      ISolution* pSolution = _pDevEnv->GetSolution();
      pSolution->RemoveProject(this);
      // NOTE: Never use class-scope variables from this point
      //       since the object has been nuked!
   }
   else {
      CView* pView = static_cast<CView*>(pElement);
      pView->CloseView();
      m_aViews.Remove(pView);
      m_bIsDirty = true;
   }
   return 0;
}

LRESULT CSqlProject::OnFileRename(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   HTREEITEM hItem = NULL;
   if( _GetSelectedTreeElement(&hItem) == NULL ) { bHandled = FALSE; return 0; }
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   ctrlTree.SelectItem(hItem);
   ctrlTree.EditLabel(hItem);
   return 0;
}

LRESULT CSqlProject::OnFileAddDatabase(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   HTREEITEM hItem = { 0 };
   IElement* pElement = _GetSelectedTreeElement(&hItem);
   if( pElement == NULL ) { bHandled = FALSE; return 0; }
   // Create new view through wizard
   _pDevEnv->ShowWizard(IDE_WIZARD_FILE, this);
   return 0;
}

LRESULT CSqlProject::OnViewOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) { bHandled = FALSE; return 0; }
   if( pElement == this ) return 0;

   CWaitCursor cursor;

   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, CString(MAKEINTRESOURCE(IDS_STATUS_OPENDATABASE)));

   IView* pView = static_cast<IView*>(pElement);
   if( !pView->OpenView(0) ) GenerateError(_pDevEnv, NULL, IDS_ERR_OPENVIEW);
   return 0;
}

LRESULT CSqlProject::OnViewProperties(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   // Find the tree-item, which will be our new parent
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) { bHandled = FALSE; return 0; }
   if( pElement == this ) { bHandled = FALSE; return 0; }
   // Bring up DataLink Configuration dialog
   CView* pView = static_cast<CView*>(pElement);
   pView->ChangeProperties(m_wndMain);
   return 0;
}

LRESULT CSqlProject::OnProjectSetDefault(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) { bHandled = FALSE; return 0; };
   IProject* pProject = static_cast<IProject*>(pElement);
   _pDevEnv->GetSolution()->SetActiveProject(pProject);
   return 0;
}

LRESULT CSqlProject::OnTreeLabelBegin(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) return TRUE;
   CString sType;
   pElement->GetType(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();
   if( pElement != this ) return FALSE;
   if( sType == _T("Project") ) return FALSE;
   if( sType == _T("Data Link") ) return FALSE;
   return TRUE;
}

LRESULT CSqlProject::OnTreeLabelEdit(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   bHandled = FALSE;
   LPNMTVDISPINFO lpNMTVDI = (LPNMTVDISPINFO) pnmh;
   if( lpNMTVDI->item.pszText == NULL ) return FALSE;
   // Commit rename action
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) return FALSE;
   if( pElement == this ) {
      SetName(lpNMTVDI->item.pszText);
   }
   else {
      IView* pView = static_cast<IView*>(pElement);
      pView->SetName(lpNMTVDI->item.pszText);
   }
   // Refresh property window (if visible)
   _pDevEnv->ShowProperties(pElement, FALSE);
   m_bIsDirty = true;
   bHandled = TRUE;
   return TRUE;
}

LRESULT CSqlProject::OnTreeKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   bHandled = FALSE;
   LPNMTVKEYDOWN lpNMTVKD = (LPNMTVKEYDOWN) pnmh;
   if( lpNMTVKD->wVKey == VK_DELETE ) {
      m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_DELETE, 0));
   }
   if( lpNMTVKD->wVKey == VK_RETURN ) {
      m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_OPEN, 0));
   }
   if( lpNMTVKD->wVKey == VK_F2 ) {
      CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
      HTREEITEM hItem = NULL;
      if( _GetSelectedTreeElement(&hItem) != NULL ) ctrlTree.EditLabel(hItem);
   }
   return TRUE;
}

LRESULT CSqlProject::OnTreeDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
   return OnViewOpen(0, 0, NULL, bHandled);
}

LRESULT CSqlProject::OnTreeRClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
   bHandled = FALSE;
   HTREEITEM hItem = NULL;
   IElement* pElement = _GetSelectedTreeElement(&hItem);
   if( pElement == NULL ) return 0;
   CString sType;
   pElement->GetType(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();
   UINT nRes = IDR_DATABASE;
   if( sType == _T("Project") ) nRes = IDR_PROJECT;
   CMenu menu;
   menu.LoadMenu(nRes);
   CMenuHandle submenu = menu.GetSubMenu(0);
   DWORD dwPos = ::GetMessagePos();
   POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
   _pDevEnv->ShowPopupMenu(pElement, submenu, pt, TRUE, this);
   bHandled = TRUE;
   return 0;
}


CToolBarCtrl CSqlProject::m_ctrlToolbar;
