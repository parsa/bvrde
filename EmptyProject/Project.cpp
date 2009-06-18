
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"
#include "WizardSheet.h"
#include "ViewSerializer.h"


EXTERN_C IView* WINAPI Plugin_CreateView(LPCTSTR, IProject*, IElement*);


// Constructor / destructor

CEmptyProject::CEmptyProject() :
   m_bLoaded(false),
   m_bIsDirty(true)
{
}

CEmptyProject::~CEmptyProject()
{
   Close();
}

// IProject

BOOL CEmptyProject::Initialize(IDevEnv* pEnv, LPCTSTR pstrPath)
{
   m_bLoaded = true;

   m_wndMain = pEnv->GetHwnd(IDE_HWND_MAIN);
   ATLASSERT(m_wndMain.IsWindow());

   m_sPath = pstrPath;

   _InitializeData();

   _pDevEnv->AddAppListener(this);
   _pDevEnv->AddTreeListener(this);
   _pDevEnv->AddWizardListener(this);

   return TRUE;
}

BOOL CEmptyProject::Close()
{
   if( !m_bLoaded ) return TRUE;
   m_bLoaded = false;

   _pDevEnv->RemoveAppListener(this);
   _pDevEnv->RemoveTreeListener(this);
   _pDevEnv->RemoveIdleListener(this);
   _pDevEnv->RemoveWizardListener(this);

   Reset();

   return TRUE;
}

BOOL CEmptyProject::GetName(LPTSTR pstrName, UINT cchMax) const
{
   _tcsncpy(pstrName, m_sName, cchMax);
   return TRUE;
}

BOOL CEmptyProject::SetName(LPCTSTR pstrName)
{
   ATLASSERT(!::IsBadStringPtr(pstrName,-1));
   m_sName = pstrName;
   m_bIsDirty = true;
   return TRUE;
}

BOOL CEmptyProject::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Project"), cchMax);
   return TRUE;
}

BOOL CEmptyProject::GetClass(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T(PLUGIN_NAME), cchMax);
   return TRUE;
}

BOOL CEmptyProject::IsDirty() const
{
   if( !m_bLoaded ) return FALSE;
   if( m_bIsDirty ) return TRUE;
   return FALSE;
}

IDispatch* CEmptyProject::GetDispatch()
{
   return _pDevEnv->CreateStdDispatch(_T("Project"), this);
}

IElement* CEmptyProject::GetParent() const
{
   return _pDevEnv->GetSolution();
}

INT CEmptyProject::GetItemCount() const
{
   return m_aFiles.GetSize();
}

IView* CEmptyProject::GetItem(INT iIndex)
{
   if( iIndex < 0 || iIndex > m_aFiles.GetSize() ) return NULL;
   return m_aFiles[iIndex];
}

BOOL CEmptyProject::Reset()
{
   m_sName.Empty();
   // Close the view's nicely before killing them off
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) m_aFiles[i]->CloseView();
   m_aFiles.RemoveAll();
   m_bIsDirty = false;
   return TRUE;
}

BOOL CEmptyProject::Load(ISerializable* pArc)
{
   Reset();

   pArc->Read(_T("name"), m_sName.GetBufferSetLength(128), 128);
   m_sName.ReleaseBuffer();

   if( !_LoadSettings(pArc) ) return FALSE;
   if( !_LoadFiles(pArc, this) ) return FALSE;
   return TRUE;
}

BOOL CEmptyProject::Save(ISerializable* pArc)
{
   pArc->Write(_T("name"), m_sName);
   pArc->Write(_T("type"), _T(PLUGIN_NAME));

   if( !_SaveSettings(pArc) ) return FALSE;
   if( !_SaveFiles(pArc, this) ) return FALSE;

   m_bIsDirty = false;
   return TRUE;
}

BOOL CEmptyProject::CreateProject()
{
   CString sName;
   GetName(sName.GetBufferSetLength(128), 128);
   sName.ReleaseBuffer();

   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   HTREEITEM hRoot = ctrlTree.GetRootItem();
   HTREEITEM hItem = ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
         sName, 
         6, 6, 
         0, 0,
         (LPARAM) this,
         hRoot, TVI_LAST);
   _PopulateTree(ctrlTree, this, hItem);
   ctrlTree.Expand(hRoot);
   ctrlTree.Expand(hItem);
   
   return TRUE;
}

void CEmptyProject::ActivateProject()
{
   _pDevEnv->AddIdleListener(this);
   _pDevEnv->AddCommandListener(this);
}

void CEmptyProject::DeactivateProject()
{
   _pDevEnv->RemoveIdleListener(this);
   _pDevEnv->RemoveCommandListener(this);
}

void CEmptyProject::ActivateUI()
{
   CMenu menu;
   menu.LoadMenu(IDR_MAIN);
   ATLASSERT(menu.IsMenu());
   CMenuHandle menuMain = _pDevEnv->GetMenuHandle(IDE_HWND_MAIN);
   CMenuHandle menuFile = menuMain.GetSubMenu(MENUPOS_FILE_FB);
   MergeMenu(menuFile.GetSubMenu(1), menu.GetSubMenu(1), 2);
}

void CEmptyProject::DeactivateUI()
{
}

// IAppMessageListener

LRESULT CEmptyProject::OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   // Process message in ATL-style message map
   LRESULT lResult = 0;
   bHandled = ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult);
   return lResult;
}

BOOL CEmptyProject::PreTranslateMessage(MSG* pMsg)
{
   if( _pDevEnv->GetSolution()->GetFocusProject() != this ) return FALSE;
   if( !m_accel.IsNull() && m_accel.TranslateAccelerator(m_wndMain, pMsg) ) return TRUE;
   return FALSE;
}

// IIdleListener

void CEmptyProject::OnIdle(IUpdateUI* pUIBase)
{
   CString sFileType;
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement ) {
      pElement->GetType(sFileType.GetBufferSetLength(64), 64);
      sFileType.ReleaseBuffer();
   }

   pUIBase->UIEnable(ID_PROJECT_SET_DEFAULT, TRUE);

   pUIBase->UIEnable(ID_FILE_ADD_FOLDER,  sFileType == _T("Project") || sFileType == _T("Folder"));
   pUIBase->UIEnable(ID_FILE_ADD_LOCAL, sFileType == _T("Project") || sFileType == _T("Folder"));
   pUIBase->UIEnable(ID_FILE_DELETE, TRUE);
   pUIBase->UIEnable(ID_FILE_RENAME, TRUE);

   pUIBase->UIEnable(ID_VIEW_OPEN, pElement != NULL);
   pUIBase->UIEnable(ID_VIEW_OPENWITH, FALSE);
   pUIBase->UIEnable(ID_VIEW_PROPERTIES, pElement != NULL);
}

void CEmptyProject::OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax)
{
   AtlLoadString(wID, pstrText, cchMax);
}

// ITreeMessageListener

LRESULT CEmptyProject::OnTreeMessage(LPNMHDR pnmh, BOOL& bHandled)
{
   if( _GetSelectedTreeElement() == NULL ) return 0;
   // Process message in ATL-style message map
   LRESULT lResult = 0;
   bHandled = ProcessWindowMessage(pnmh->hwndFrom, WM_NOTIFY, (WPARAM) pnmh->idFrom, (LPARAM) pnmh, lResult);
   return lResult;
}

// IWizardListener

BOOL CEmptyProject::OnInitWizard(IWizardManager* pManager, IProject* pProject, LPCTSTR pstrName)
{
   if( pProject != this ) return TRUE;

   static CMessagePage s_pageMessage;
   static CString sProviderTitle(MAKEINTRESOURCE(IDS_WIZARD_TITLE_MESSAGE));
   static CString sProviderSubTitle(MAKEINTRESOURCE(IDS_WIZARD_SUBTITLE_MESSAGE));
   s_pageMessage.SetHeaderTitle(sProviderTitle);
   s_pageMessage.SetHeaderSubTitle(sProviderSubTitle);

   pManager->AddWizardPage(s_pageMessage.IDD, s_pageMessage);

   SetName(pstrName);
   return TRUE;
}

BOOL CEmptyProject::OnInitProperties(IWizardManager* /*pManager*/, IElement* /*pElement*/)
{
   return TRUE;
}

BOOL CEmptyProject::OnInitOptions(IWizardManager* /*pManager*/, ISerializable* /*pArc*/)
{
   return TRUE;
}

// ICustomCommandListener

void CEmptyProject::OnUserCommand(LPCTSTR /*pstrCommand*/, BOOL& bHandled)
{
   bHandled = FALSE;
}

void CEmptyProject::OnMenuCommand(LPCTSTR /*pstrType*/, LPCTSTR /*pstrCommand*/, LPCTSTR /*pstrArguments*/, LPCTSTR /*pstrPath*/, int /*iFlags*/, BOOL& bHandled)
{
   bHandled = FALSE;
}

// Message handlers

LRESULT CEmptyProject::OnFileRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
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
      //       since this instance has been nuked!
   }
   else {
      // BUG: Leaks all the child views.
      // TODO: Recursivly remove all children
      IView* pView = static_cast<IView*>(pElement);
      pView->CloseView();
      m_aFiles.Remove(pView);
      m_bIsDirty = true;
   }
   return 0;
}

LRESULT CEmptyProject::OnFileRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   HTREEITEM hItem = NULL;
   if( _GetSelectedTreeElement(&hItem) == NULL ) { bHandled = FALSE; return 0; }
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   ctrlTree.SelectItem(hItem);
   ctrlTree.EditLabel(hItem);
   return 0;
}

LRESULT CEmptyProject::OnFileAddFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   HTREEITEM hItem = { 0 };
   IElement* pElement = _GetSelectedTreeElement(&hItem);
   if( pElement == NULL ) { bHandled = FALSE; return 0; }
   // Create new object
   IView* pFolder = new CFolderFile(this, this, pElement);
   if( pFolder == NULL ) return 0;
   CString sName(MAKEINTRESOURCE(IDS_NEW_FOLDER));
   pFolder->SetName(sName);
   m_aFiles.Add(pFolder);
   // Add new tree item
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   int iImage = 0;
   HTREEITEM hNew = ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
         sName, 
         iImage, iImage, 
         0, 0,
         (LPARAM) pFolder,
         hItem, TVI_LAST);
   ctrlTree.Expand(hItem);
   ctrlTree.SelectItem(hNew);
   ctrlTree.EditLabel(hNew);
   m_bIsDirty = true;
   return 0;
}

LRESULT CEmptyProject::OnFileAddLocal(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   // Need to make sure the project file itself has been saved
   if( m_aFiles.GetSize() == 0 ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE_ALL, 0));

   // Find the tree-item, which will be our new parent
   HTREEITEM hItem = NULL;
   IElement* pElement = _GetSelectedTreeElement(&hItem);
   if( pElement == NULL )  { bHandled = FALSE; return 0; }

   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);

   // Browse for filename
   CString sFilter(MAKEINTRESOURCE(IDS_FILTER_FILES));
   for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == '|' ) sFilter.SetAt(i, '\0');
   TCHAR szBuffer[MAX_PATH * 10] = { 0 };
   DWORD dwStyle = OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ENABLESIZING | OFN_ALLOWMULTISELECT;
   CFileDialog dlg(TRUE, _T(""), NULL, dwStyle, sFilter, m_wndMain);
   dlg.m_ofn.Flags &= ~OFN_ENABLEHOOK;
   dlg.m_ofn.lpstrFile = szBuffer;
   dlg.m_ofn.nMaxFile = sizeof(szBuffer) / sizeof(TCHAR);
   dlg.m_ofn.lpstrInitialDir = m_sPath;
   if( dlg.DoModal() != IDOK ) return 0;

   // Compute paths and names
   CString sPath = dlg.m_ofn.lpstrFile;
   if( sPath.Right(1) != _T("\\") ) sPath += '\\';
   HTREEITEM hFirst = NULL;

   TCHAR szFilename[MAX_PATH] = { 0 };
   TCHAR *src = &szBuffer[dlg.m_ofn.nFileOffset];
   TCHAR *dst = &szFilename[dlg.m_ofn.nFileOffset];
   _tcsncpy(szFilename, szBuffer, dlg.m_ofn.nFileOffset);
   szFilename[dlg.m_ofn.nFileOffset - 1] = '\\';
   while (*src != '\0') {
      _tcscpy(dst, src);
      src = &src[_tcslen(src) + 1];

      TCHAR szName[MAX_PATH];
      _tcscpy(szName, szFilename);
      ::PathStripPath(szName);

      TCHAR szPath[MAX_PATH];
      _tcscpy(szPath, m_sPath);
      ::PathRemoveBackslash(szPath);

      TCHAR szRelativeFilename[MAX_PATH] = { 0 };
      ::PathRelativePathTo(szRelativeFilename, szPath, FILE_ATTRIBUTE_DIRECTORY, szFilename, FILE_ATTRIBUTE_NORMAL);
      if( _tcslen(szRelativeFilename) == 0 ) _tcscpy(szRelativeFilename, szFilename);

      if( !_CheckProjectFile(szName) ) return 0;

      // Create new object
      IView* pView = _pDevEnv->CreateView(szFilename, this, pElement);
      if( pView == NULL ) return 0;

      // Load some default properties
      CViewSerializer arc;
      arc.Add(_T("name"), szName);
      arc.Add(_T("filename"), szRelativeFilename);
      arc.Add(_T("location"), _T("local"));
      pView->Load(&arc);

      // Add view to collection
      m_aFiles.Add(pView);

      // Add new tree item
      int iImage = _GetElementImage(pView);
      HTREEITEM hNew = ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
            szName, 
            iImage, iImage, 
            0, 0,
            (LPARAM) pView,
            hItem, TVI_LAST);
      if( hFirst == NULL ) hFirst = hNew;
   }
   ctrlTree.Expand(hItem);
   if( hFirst != NULL ) ctrlTree.SelectItem(hFirst);

   m_bIsDirty = true;
   return 0;
}

LRESULT CEmptyProject::OnViewOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) { bHandled = FALSE; return 0; }
   if( pElement == this ) return 0;

   CWaitCursor cursor;

   CString sStatus;
   sStatus.Format(IDS_STATUS_OPENFILE);
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus, FALSE);

   IView* pView = static_cast<IView*>(pElement);
   if( !pView->OpenView(0) ) GenerateError(_pDevEnv, NULL, IDS_ERR_OPENVIEW);
   return 0;
}


LRESULT CEmptyProject::OnProjectSetDefault(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) { bHandled = FALSE; return 0; };
   IProject* pProject = static_cast<IProject*>(pElement);
   _pDevEnv->GetSolution()->SetActiveProject(pProject);
   return 0;
}

LRESULT CEmptyProject::OnTreeLabelBegin(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) return TRUE;
   CString sType;
   pElement->GetType(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();
   if( pElement == this ) return FALSE;
   if( sType == _T("Project") ) return FALSE;
   if( sType == _T("Folder") ) return FALSE;
   return TRUE;
}

LRESULT CEmptyProject::OnTreeLabelEdit(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
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

LRESULT CEmptyProject::OnTreeKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
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

LRESULT CEmptyProject::OnTreeDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
   return OnViewOpen(0, 0, NULL, bHandled);
}

LRESULT CEmptyProject::OnTreeRClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
   bHandled = FALSE;
   HTREEITEM hItem = NULL;
   IElement* pElement = _GetSelectedTreeElement(&hItem);
   if( pElement == NULL ) return 0;
   CString sType;
   pElement->GetType(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();
   UINT nRes = 0;
   if( sType == _T("Folder") ) nRes = IDR_FOLDER;
   else if( sType == _T("Project") ) nRes = IDR_PROJECT;
   else nRes = IDR_TEXT;
   CMenu menu;
   menu.LoadMenu(nRes);
   CMenuHandle submenu = menu.GetSubMenu(0);
   DWORD dwPos = ::GetMessagePos();
   POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
   _pDevEnv->ShowPopupMenu(pElement, submenu, pt, TRUE, this);
   bHandled = TRUE;
   return 0;
}

// Operations

BOOL CEmptyProject::GetPath(LPTSTR pstrPath, UINT cchMax) const
{
   _tcsncpy(pstrPath, m_sPath, cchMax);
   return TRUE;
}

bool CEmptyProject::OpenView(LPCTSTR pstrFilename, long lLineNum)
{
   IView* pView = FindView(pstrFilename, false);
   if( pView == NULL ) return false;
   return pView->OpenView(lLineNum) == TRUE;
}

IView* CEmptyProject::FindView(LPCTSTR pstrFilename, bool bLocally /*= false*/) const
{
   ATLASSERT(!::IsBadStringPtr(pstrFilename,-1));
   // Prepare filename (strip path)
   TCHAR szSearchFile[256];
   _tcscpy(szSearchFile, pstrFilename);
   ::PathStripPath(szSearchFile);
   // No need to look for dummy filenames
   if( _tcslen(pstrFilename) < 2 ) return false;
   // Scan through all views and see if there's a filename-match
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      IView* pView = m_aFiles[i];
      TCHAR szFilename[MAX_PATH + 1] = { 0 };
      pView->GetFileName(szFilename, MAX_PATH);
      ::PathStripPath(szFilename);
      if( _tcsicmp(szSearchFile, szFilename) == 0 ) return pView;
   }
   if( bLocally ) return NULL;
   // Locate file in another project
   ISolution* pSolution = _pDevEnv->GetSolution();
   for( int a = 0; a < pSolution->GetItemCount(); a++ ) {
      IProject* pProject = pSolution->GetItem(a);
      for( INT b = 0; b < pProject->GetItemCount(); b++ ) {
         IView* pView = pProject->GetItem(b);
         TCHAR szFilename[MAX_PATH + 1] = { 0 };
         pView->GetFileName(szFilename, MAX_PATH);
         ::PathStripPath(szFilename);
         if( _tcsicmp(szSearchFile, szFilename) == 0 ) return pView;
      }
   }
   return NULL;
}

void CEmptyProject::InitializeToolBars()
{
}

// Implementation

void CEmptyProject::_InitializeData()
{
   // Initilize data. These control are mostly statics and
   // need only be initialized once.
   if( !m_accel.IsNull() ) return;

   m_accel.LoadAccelerators(IDR_ACCELERATOR);
}

bool CEmptyProject::_LoadSettings(ISerializable* pArc)
{
   if( !pArc->ReadGroupBegin(_T("Settings")) ) return true;
   if( !pArc->ReadGroupEnd() ) return false;
   return true;
}

bool CEmptyProject::_SaveSettings(ISerializable* pArc)
{
   if( !pArc->WriteGroupBegin(_T("Settings")) ) return false;
   if( !pArc->WriteGroupEnd() ) return false;
   return true;
}

bool CEmptyProject::_LoadFiles(ISerializable* pArc, IElement* pParent)
{
   if( !pArc->ReadGroupBegin(_T("Files")) ) return true;
   while( pArc->ReadGroupBegin(_T("File")) ) {
      CString sType;
      pArc->Read(_T("type"), sType.GetBufferSetLength(64), 64);
      sType.ReleaseBuffer();
      CString sFilename;
      pArc->Read(_T("filename"), sFilename.GetBufferSetLength(MAX_PATH), MAX_PATH);
      sFilename.ReleaseBuffer();
      // Create file object
      IView* pView = NULL;
      if( sType == _T("Folder") ) pView = new CFolderFile(this, this, pParent);
      else pView = _pDevEnv->CreateView(sFilename, this, pParent);
      ATLASSERT(pView);
      if( pView == NULL ) return false;
      // Load file properties
      if( !pView->Load(pArc) ) {
         pView->CloseView();
         return false;
      }
      // Add file to collection
      m_aFiles.Add(pView);
      // Recurse into subfiles
      if( !_LoadFiles(pArc, pView) ) {
         m_aFiles.Remove(pView);
         pView->CloseView();
         return false;
      }
      if( !pArc->ReadGroupEnd() ) return false;
   }
   if( !pArc->ReadGroupEnd() ) return false;
   return true;
}

bool CEmptyProject::_SaveFiles(ISerializable* pArc, IElement* pParent)
{
   if( !pArc->WriteGroupBegin(_T("Files")) ) return false;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      IView* pFile = m_aFiles[i];
      if( pFile->GetParent() == pParent ) {
         if( !pArc->WriteGroupBegin(_T("File")) ) return false;
         // Save file properties
         IElement* pElement = pFile;
         if( !pElement->Save(pArc) ) return false;
         // Recurse into subfiles
         TCHAR szType[64] = { 0 };
         pElement->GetType(szType, 63);
         if( _tcscmp(szType, _T("Folder")) == 0 ) {
            pFile->Save();
            if( !_SaveFiles(pArc, pFile) ) return false;
         }
         if( !pArc->WriteGroupEnd() ) return false;
      }
   }
   if( !pArc->WriteGroupEnd() ) return false;
   return true;
}

void CEmptyProject::_PopulateTree(CTreeViewCtrl& ctrlTree, 
                                   IElement* pParent, 
                                   HTREEITEM hParent) const
{
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) 
   {
      IView* pFile = m_aFiles[i];
      if( pFile->GetParent() == pParent ) {
         int iImage = _GetElementImage(pFile);
         CString sName;
         pFile->GetName(sName.GetBufferSetLength(128), 128);
         sName.ReleaseBuffer();
         HTREEITEM hNew = ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
               sName, 
               iImage, iImage, 
               0, 0,
               (LPARAM) pFile,
               hParent, TVI_LAST);
         _PopulateTree(ctrlTree, pFile, hNew);
      }
   }
   TVSORTCB tvscb = { 0 };
   tvscb.hParent = hParent;
   tvscb.lParam = (LPARAM) this;
   tvscb.lpfnCompare = _SortTreeCB;
   ctrlTree.SortChildrenCB(&tvscb);
}

int CEmptyProject::_GetElementImage(IElement* pElement) const
{
   ATLASSERT(pElement);
   TCHAR szType[64] = { 0 };
   pElement->GetType(szType, 63);
   typedef struct tagFILEIMAGE {
      LPCTSTR pstr;
      IDE_PROJECTTREE_IMAGE Image;
   } FILEIMAGE;
   static FILEIMAGE ppstrFileTypes[] = {
      { _T("Folder"),      IDE_TREEIMAGE_FOLDER_CLOSED },
      { _T("CPP"),         IDE_TREEIMAGE_CPP },
      { _T("Header"),      IDE_TREEIMAGE_HEADER },
      { _T("Makefile"),    IDE_TREEIMAGE_MAKEFILE },
      { _T("XML"),         IDE_TREEIMAGE_XML },
      { _T("HTML"),        IDE_TREEIMAGE_HTML },
      { _T("ASP Scrip"),   IDE_TREEIMAGE_HTML },
      { _T("PHP"),         IDE_TREEIMAGE_HTML },
      { _T("Java"),        IDE_TREEIMAGE_JAVA },
      { _T("BASIC"),       IDE_TREEIMAGE_BASIC },
      { _T("ShellScript"), IDE_TREEIMAGE_BASH },
      { _T("Pascal"),      IDE_TREEIMAGE_SCRIPT },
      { _T("Python"),      IDE_TREEIMAGE_SCRIPT },
      { NULL, IDE_TREEIMAGE_LAST }
   };
   for( int i = 0; ppstrFileTypes[i].pstr != NULL; i++ ) {
      if( _tcscmp(szType, ppstrFileTypes[i].pstr) == 0 ) return (int) ppstrFileTypes[i].Image;
   }
   return IDE_TREEIMAGE_TEXT;
}

IElement* CEmptyProject::_GetSelectedTreeElement(HTREEITEM* phItem /*= NULL*/) const
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
   for( int i = 0; !bLocal && i < m_aFiles.GetSize(); i++ ) if( (LPARAM) m_aFiles[i] == lParam ) bLocal = true;
   if( !bLocal ) return NULL;
   return (IElement*) lParam;
}

IElement* CEmptyProject::_GetDropTreeElement(HTREEITEM* phItem /*= NULL*/) const
{
   if( phItem ) *phItem = NULL;
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   HTREEITEM hItem = ctrlTree.GetDropHilightItem();
   if( hItem == NULL ) return NULL;
   if( phItem ) *phItem = hItem;
   LPARAM lParam = ctrlTree.GetItemData(hItem);
   if( lParam == NULL ) return NULL;
   bool bLocal = false;
   if( lParam == (LPARAM) this ) bLocal = true;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) if( (LPARAM) m_aFiles[i] == lParam ) bLocal = true;
   if( !bLocal ) return NULL;
   return (IElement*) lParam;
}

UINT CEmptyProject::_GetMenuPosFromID(HMENU hMenu, UINT ID) const
{
   UINT nCount = ::GetMenuItemCount(hMenu);
   for( UINT i = 0; i < nCount; i++ ) {
      if( ::GetMenuItemID(hMenu, i) == ID ) return i;
   }
   return (UINT) -1;
}

int CALLBACK CEmptyProject::_SortTreeCB(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/)
{
   IView* pItem1 = reinterpret_cast<IView*>(lParam1);
   IView* pItem2 = reinterpret_cast<IView*>(lParam2);
   TCHAR szName1[128];
   TCHAR szName2[128];
   TCHAR szType1[64];
   TCHAR szType2[64];
   pItem1->GetName(szName1, 127); szName1[127] = '\0';
   pItem2->GetName(szName2, 127); szName2[127] = '\0';
   pItem1->GetType(szType1, 63); szType1[63] = '\0';
   pItem2->GetType(szType2, 63); szType2[63] = '\0';
   bool bIsFolder1 = (_tcscmp(szType1, _T("Folder")) == 0);
   bool bIsFolder2 = (_tcscmp(szType2, _T("Folder")) == 0);
   if( bIsFolder1 && !bIsFolder2 ) return -1;
   if( !bIsFolder1 && bIsFolder2 ) return 1;
   if( bIsFolder1 && bIsFolder2 ) return 0; // Folders are not sorted alphabetically!
   return _tcsicmp(szName1, szName2);
}

void CEmptyProject::_AddButtonText(CToolBarCtrl tb, UINT nID, UINT nRes)
{
#ifndef BTNS_SHOWTEXT
   const UINT BTNS_SHOWTEXT = 0x0040;
#endif  // BTNS_SHOWTEXT
   TBBUTTONINFO tbi = { 0 };
   tbi.cbSize = sizeof(TBBUTTONINFO),
   tbi.dwMask  = TBIF_STYLE;
   tb.GetButtonInfo(nID, &tbi);
   tbi.dwMask = TBIF_STYLE | TBIF_TEXT;
   tbi.fsStyle |= TBSTYLE_AUTOSIZE | BTNS_SHOWTEXT;
   CString s(MAKEINTRESOURCE(nRes));
   tbi.pszText = (LPTSTR) (LPCTSTR) s;
   tb.SetButtonInfo(nID, &tbi);
}

void CEmptyProject::_AddControlToToolbar(CToolBarCtrl tb, HWND hWnd, USHORT cx, UINT nCmdPos, bool bIsCommandId, bool bInsertBefore /*= true*/)
{
   ATLASSERT(::IsWindow(hWnd));
   static UINT s_nCmd = 45000; // Unique number for new toolbar separator
   int iIndex = nCmdPos;
   if( bIsCommandId ) iIndex = tb.CommandToIndex(nCmdPos);
   if( !bInsertBefore ) iIndex++;
   
   // Create a separator toolbar button
   TBBUTTON but = { 0 };
   but.fsStyle = TBSTYLE_SEP;
   but.fsState = TBSTATE_ENABLED;
   but.idCommand = s_nCmd;
   BOOL bRes = tb.InsertButton(iIndex, &but);
   ATLASSERT(bRes); bRes;
   TBBUTTONINFO info;
   info.cbSize = sizeof(info);
   info.dwMask = TBIF_SIZE;
   info.cx = cx;
   tb.SetButtonInfo(s_nCmd++, &info);

   // Chain the control to its new parent
   ::SetParent(hWnd, tb);
   // Position the new control on top of the separator
   RECT rc;
   tb.GetItemRect(iIndex, &rc);
   ::SetWindowPos(hWnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE);
}

bool CEmptyProject::_AddCommandBarImages(UINT nRes) const
{
   CImageListCtrl Images;
   Images.Create(16, 16, ILC_COLOR | ILC_MASK, 40, 1);
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

bool CEmptyProject::_CheckProjectFile(LPCTSTR pstrName) 
{
   // Check that the file doesn't already exist in the
   // project filelist.
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      CString sName;
      m_aFiles[i]->GetName(sName.GetBufferSetLength(MAX_PATH), MAX_PATH);
      sName.ReleaseBuffer();
      if( sName.CompareNoCase(pstrName) == 0 ) {
         ::SetLastError(0L);
         GenerateError(_pDevEnv, NULL, IDS_ERR_FILEINCLUDED);
         return false;
      }       
   }
   return true;
}

// Static members

CAccelerator CEmptyProject::m_accel;
