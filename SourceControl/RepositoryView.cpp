
#include "StdAfx.h"
#include "resource.h"

#include "RepositoryView.h"


////////////////////////////////////////////////////////////////7
//
//

void CFileEnumThread::RunCommand(HWND hWnd, LPCTSTR pstrCommand, LONG lTimeout)
{
   ATLASSERT(::IsWindow(hWnd));
   ATLASSERT(!::IsBadStringPtr(pstrCommand,-1));
   if( _tcslen(pstrCommand) == 0 ) return;
   Stop();
   m_hWnd = hWnd;
   m_sCommand = pstrCommand;
   m_lTimeout = lTimeout;
   Start();
}

DWORD CFileEnumThread::Run()
{
   ::CoInitialize(NULL);

   // Execute prompt through the project's scripting mode
   ISolution* pSolution = _pDevEnv->GetSolution();
   if( pSolution == NULL ) return 0;
   IProject* pProject = pSolution->GetActiveProject();
   if( pProject == NULL ) return 0;
   IDispatch* pDisp = pProject->GetDispatch();   
   CComDispatchDriver dd = pDisp;
   CComVariant vRet;
   dd.GetPropertyByName(OLESTR("CurDir"), &vRet);
   CString sCD;
   sCD.Format(_T("cd %s"), vRet.bstrVal);
   CComBSTR bstrFileEnum = m_sCommand;
   CComVariant aParams[3];
   aParams[2] = sCD;
   aParams[1] = static_cast<ILineCallback*>(this);
   aParams[0] = 1000;
   dd.InvokeN(OLESTR("ExecCommand"), aParams, 3);
   aParams[2] = bstrFileEnum;
   aParams[0] = m_lTimeout;
   dd.InvokeN(OLESTR("ExecCommand"), aParams, 3);

   ::PostMessage(m_hWnd, WM_APP_ENUMDONE, 0, 0L);

   ::CoUninitialize();
   return 0;
}

// ILineCallback

HRESULT CFileEnumThread::OnIncomingLine(BSTR bstr)
{
   if( bstr == NULL ) return S_OK;
   if( ShouldStop() ) return E_ABORT;
   // CVS output will generally look like this:
   //   ===================================================================
   //   File: filename.cpp      Status: Up-to-date
   //
   //   Working revision:   1.3
   //   Repository revision:   1.3   /home/CVS/myfolder/filename.cpp,v
   LPOLESTR p = wcsstr(bstr, L"Repository revision:");
   if( p != NULL ) {
      if( wcslen(p) < 31 ) return S_OK;
      CString sFile = p;
      int iPos = sFile.ReverseFind(' ');
      if( iPos > 0 ) sFile = sFile.Mid(iPos + 1);
      iPos = sFile.ReverseFind(',');
      if( iPos > 0 ) sFile = sFile.Left(iPos);
      sFile.TrimLeft();
      sFile.TrimRight();
      // Add if not exists...
      for( int i = 0; i < m_aResult.GetSize(); i++ ) if( m_aResult[i] == sFile ) return S_OK;
      return m_aResult.Add(sFile) ? S_OK : E_OUTOFMEMORY;
   }
   return S_OK;
}

// IUnknown

HRESULT CFileEnumThread::QueryInterface(REFIID riid, void** ppvObject)
{
   if( riid == __uuidof(ILineCallback) || riid == IID_IUnknown ) {
      *ppvObject = static_cast<ILineCallback*>(this);
      return S_OK;
   }
   return E_NOINTERFACE;
}

ULONG CFileEnumThread::AddRef(void)
{
   return 1;
}

ULONG CFileEnumThread::Release(void)
{
   return 1;
}


////////////////////////////////////////////////////////////////7
//
//

LRESULT CRepositoryView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   SetFont(AtlGetDefaultGuiFont());

   // Create controls
   DWORD dwStyle;
   dwStyle = TVS_HASBUTTONS | TVS_INFOTIP | TVS_HASLINES | TVS_SHOWSELALWAYS | WS_CHILD | WS_VISIBLE | WS_TABSTOP;
   m_ctrlFolders.Create(m_hWnd, rcDefault, NULL, dwStyle, WS_EX_CLIENTEDGE, IDC_SFOLDERS);
   dwStyle = LVS_SHAREIMAGELISTS | LVS_SINGLESEL | LVS_REPORT | LVS_AUTOARRANGE | LVS_NOSORTHEADER | WS_CHILD | WS_VISIBLE | WS_TABSTOP;
   m_ctrlFiles.Create(m_hWnd, rcDefault, NULL, dwStyle, WS_EX_CLIENTEDGE, IDC_SFILES);
   RECT rcBuilding = { 10, 20, 180, 120 };
   dwStyle = SS_LEFT | WS_CHILD;
   m_ctrlBuilding.Create(m_hWnd, rcBuilding, CString(MAKEINTRESOURCE(IDS_BUILDING)), dwStyle);
   m_ctrlBuilding.SetFont(AtlGetDefaultGuiFont());

   // Prepare images
   int nSmallCx = ::GetSystemMetrics(SM_CXSMICON);
   int nSmallCy = ::GetSystemMetrics(SM_CYSMICON);

   if( !m_FolderImages.IsNull() ) m_FolderImages.Destroy();
   m_FolderImages.Create(nSmallCx, nSmallCy, ILC_COLOR32 | ILC_MASK, 4, 0);
   if( m_FolderImages.IsNull() ) return -1;
   _AddShellIcon(m_FolderImages, _T(""), FILE_ATTRIBUTE_DIRECTORY);
   _AddShellIcon(m_FolderImages, _T(""), FILE_ATTRIBUTE_DIRECTORY, SHGFI_OPENICON);
   _AddShellIcon(m_FolderImages, _T("C:\\"), 0);
   m_ctrlFolders.SetImageList(m_FolderImages, TVSIL_NORMAL);

   if( !m_FileImages.IsNull() ) m_FolderImages.Destroy();
   m_FileImages.Create(nSmallCx, nSmallCy, ILC_COLOR32 | ILC_MASK, 8, 0);
   if( m_FileImages.IsNull() ) return -1;
   _AddShellIcon(m_FileImages, _T(".tmp"), FILE_ATTRIBUTE_NORMAL);
   m_ctrlFiles.SetImageList(m_FileImages, LVSIL_SMALL);

   // Prepare ListView
   TCHAR szTitle[128] = { 0 };
   ::LoadString(_Module.GetResourceInstance(), IDS_FILES, szTitle, 127);
   m_ctrlFiles.InsertColumn(0, szTitle, LVCFMT_LEFT, 200, 0);

   return 0;
}

LRESULT CRepositoryView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   CClientRect rcClient = m_hWnd;
   int cy = (rcClient.bottom - rcClient.top) / 2;
   m_ctrlFolders.MoveWindow(rcClient.left, rcClient.top, rcClient.right, rcClient.top + cy);
   m_ctrlFiles.MoveWindow(rcClient.left, rcClient.top + cy, rcClient.right, rcClient.bottom - cy);
   m_ctrlFiles.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
   return 0;
}

LRESULT CRepositoryView::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   if( (HWND) lParam != m_ctrlBuilding ) {
      bHandled = FALSE;
      return 0;
   }
   CDCHandle dc = (HDC) wParam;
   dc.SetTextColor(RGB(0,0,200));
   dc.SetBkMode(TRANSPARENT);
   return (LRESULT) ::GetSysColorBrush(COLOR_WINDOW);
}

LRESULT CRepositoryView::OnViewOpens(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlBuilding.ShowWindow(SW_HIDE);
   if( m_thread.IsRunning() ) return 0;
   // Show idle message
   if( m_thread.m_aResult.GetSize() == 0 ) {
      m_ctrlBuilding.ShowWindow(SW_SHOW);
      m_ctrlFolders.DeleteAllItems();
      m_ctrlFiles.DeleteAllItems();
   }
   // Build entire structure
   TCHAR szCommand[200] = { 0 };
   _pDevEnv->GetProperty(_T("sourcecontrol.browse.all"), szCommand, 199);
   m_thread.RunCommand(m_hWnd, szCommand, 8000L);
   return 0;
}

LRESULT CRepositoryView::OnFileEnumDone(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   CWaitCursor cursor;
  
   m_ctrlBuilding.ShowWindow(SW_HIDE);

   CWindowRedraw files = m_ctrlFiles;
   CWindowRedraw folders = m_ctrlFolders;

   if( m_ctrlFolders.GetRootItem() == NULL ) {
      TCHAR szType[128] = { 0 };
      _pDevEnv->GetProperty(_T("sourcecontrol.type"), szType, 127);
      m_ctrlFolders.InsertItem(szType, 2, 2, TVI_ROOT, TVI_LAST);
   }

   for( int i = 0; i < m_thread.m_aResult.GetSize(); i++ ) {
      CString sFilename = m_thread.m_aResult[i];
      // Take the return string, muffle it from C:\temp\... to /temp/...
      // and build the tree structure. We're adding nodes to the tree always.
      sFilename.Replace('\\', '/');
      int iPos = sFilename.Find(':');
      if( iPos >= 0 ) sFilename = sFilename.Mid(iPos + 1);
      HTREEITEM hParent = m_ctrlFolders.GetRootItem();
      while( !sFilename.IsEmpty() ) {
         CString sPart = sFilename.SpanExcluding(_T("/"));
         sFilename = sFilename.Mid(sPart.GetLength() + 1);
         if( sPart.IsEmpty() ) continue;
         if( sFilename.IsEmpty() ) 
         {
            CString sPath = _GetItemPath(hParent);
            if( m_sSelPath.IsEmpty() ) m_sSelPath = sPath;
            if( sPath != m_sSelPath ) continue;
            m_ctrlFiles.InsertItem(LVIF_TEXT | LVIF_IMAGE, m_ctrlFiles.GetItemCount(), sPart, 0, 0, 0, 0L);
            m_ctrlFolders.Expand(hParent);
            m_ctrlFolders.SelectItem(hParent);
         }
         else 
         {
            HTREEITEM hItem = _FindItemInTree(hParent, sPart);
            if( hItem != NULL ) hParent = hItem;
            else hParent = m_ctrlFolders.InsertItem(sPart, 0, 0, hParent, TVI_LAST);
         }
      }
   }

   return 0;
}

LRESULT CRepositoryView::OnSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   if( pnmh->hwndFrom != m_ctrlFolders ) return 0;
   LPNMTREEVIEW pnmtv = (LPNMTREEVIEW) pnmh;
   CString sPath = _GetItemPath(pnmtv->itemNew.hItem);
   if( sPath.IsEmpty() ) return 0;
   if( sPath == m_sSelPath ) return 0;
   CWaitCursor cursor;
   m_ctrlFiles.DeleteAllItems();
   m_sSelPath = sPath;
   TCHAR szCommand[200] = { 0 };
   _pDevEnv->GetProperty(_T("sourcecontrol.browse.single"), szCommand, 199);
   CString sCommand = szCommand;
   sCommand.Replace(_T("$PATH$"), m_sSelPath);
   m_thread.RunCommand(m_hWnd, sCommand, 1000L);
   return 0;
}

LRESULT CRepositoryView::OnItemExpanded(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   bHandled = FALSE;
   // Toggle the icon of the folder nodes.
   // They have both an open/closed state.
   LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) pnmh;
   TVITEM item = lpNMTV->itemNew;
   item.mask |= TVIF_IMAGE;
   m_ctrlFolders.GetItem(&item);
   if( item.iImage == 0 || item.iImage == 1 ) {
      item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
      item.iImage = item.iSelectedImage = (lpNMTV->action & TVE_EXPAND) != 0 ? 1 : 0;
      m_ctrlFolders.SetItem(&item);
   }
   return 0;
}

LRESULT CRepositoryView::OnListKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   LPNMLVKEYDOWN lpNMLVD = (LPNMLVKEYDOWN) pnmh;
   if( lpNMLVD->wVKey == VK_BACK ) {
      m_ctrlFolders.SelectItem(m_ctrlFolders.GetParentItem(m_ctrlFolders.GetSelectedItem()));
      m_ctrlFiles.SelectItem(0);
   }
   bHandled = FALSE;
   return 0;
}

HTREEITEM CRepositoryView::_FindItemInTree(HTREEITEM hItem, LPCTSTR pstrName) const
{
   CString sName;
   hItem = m_ctrlFolders.GetChildItem(hItem);
   while( hItem != NULL ) {
      m_ctrlFolders.GetItemText(hItem, sName);
      if( sName == pstrName ) return hItem;
      hItem = m_ctrlFolders.GetNextSiblingItem(hItem);
   }
   return NULL;
}

CString CRepositoryView::_GetItemPath(HTREEITEM hItem) const
{
   CString sName;
   CString sPath;
   CString sTemp;
   while( hItem != NULL ) {
      m_ctrlFolders.GetItemText(hItem, sName);
      sTemp.Format(_T("/%s%s"), sName, sPath);
      sPath = sTemp;
      hItem = m_ctrlFolders.GetParentItem(hItem);
   }
   return sPath;
}

bool CRepositoryView::_AddShellIcon(CImageListHandle& iml, LPCTSTR pstrExtension, DWORD dwFileAttribs, DWORD dwMoreFlags /*= 0*/) const
{
   SHFILEINFO sfi = { 0 };
   CImageListHandle hil = (HIMAGELIST) ::SHGetFileInfo(pstrExtension,
      dwFileAttribs,
      &sfi,
      sizeof(sfi),
      SHGFI_USEFILEATTRIBUTES | SHGFI_SMALLICON | SHGFI_SYSICONINDEX | dwMoreFlags);
   CIcon icon = hil.GetIcon(sfi.iIcon, ILD_TRANSPARENT);
   return iml.AddIcon(icon) == TRUE;
}

