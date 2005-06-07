
#include "StdAfx.h"
#include "resource.h"

#include "RepositoryView.h"


////////////////////////////////////////////////////////////////7
// CFileEnumThread
//

void CFileEnumThread::RunCommand(HWND hWnd, LPCTSTR pstrCommand, LONG lTimeout)
{
   ATLASSERT(::IsWindow(hWnd));
   ATLASSERT(!::IsBadStringPtr(pstrCommand,-1));
   if( _tcslen(pstrCommand) == 0 ) return;
   Stop();
   // Assign variables
   m_hWnd = hWnd;
   m_sCommand = pstrCommand;
   m_lTimeout = lTimeout;
   // Figure out which source control product we'll meet
   TCHAR szCommand[200] = { 0 };
   _pDevEnv->GetProperty(_T("sourcecontrol.type"), szCommand, 199);
   m_sSourceType = szCommand;
   Start();
}

DWORD CFileEnumThread::Run()
{
   CCoInitialize cominit(COINIT_MULTITHREADED);

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

   return 0;
}

// ILineCallback

HRESULT CFileEnumThread::OnIncomingLine(BSTR bstr)
{
   if( bstr == NULL ) return S_OK;
   if( ShouldStop() ) return E_ABORT;
   if( m_sSourceType == "cvs" )
   {
      // CVS output will generally look like this:
      //   ===================================================================
      //   File: filename.cpp      Status: Up-to-date
      //
      //   Working revision:   1.3
      //   Repository revision:   1.3   /home/CVS/myfolder/filename.cpp,v   
      LPOLESTR p = wcsstr(bstr, L"================");
      if( p != NULL ) {
         FILEINFO empty;
         m_Info = empty;
      }
      p = wcsstr(bstr, L"Repository revision:");
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
         for( int i = 0; i < m_aResult.GetSize(); i++ ) if( m_aResult[i].sFilename == sFile ) return S_OK;
         m_Info.sFilename = sFile;
         return m_aResult.Add(m_Info) ? S_OK : E_OUTOFMEMORY;
      }
      p = wcsstr(bstr, L"Status: ");
      if( p != NULL ) {
         m_Info.sStatus = p + 8;
      }
      p = wcsstr(bstr, L"Working revision: ");
      if( p != NULL ) {
         m_Info.sVersion = _T("");
         p += 18;
         while( *p == ' ' ) p++;
         while( *p != '\0' && *p != ' ' ) m_Info.sVersion += *p++;
      }
   }
   if( m_sSourceType == "subversion" )
   {
      // Subversion output will generally look like this:
      //   $ svn status --show-updates --verbose wc
      //    M           965       938 sally        wc/bar.c
      //          *     965       922 harry        wc/foo.c
      //   A  +         965       687 harry        wc/qax.c
      //                965       687 harry        wc/zig.c
      if( wcslen(bstr) < 15 ) return S_OK;
      switch( bstr[0] ) {
      case ' ':
      case 'A':
      case 'D':
      case 'M':
      case 'C':
      case '!':
         WCHAR szStatus[8] = { 0 };
         wcsncpy(szStatus, bstr, 6);
         FILEINFO empty;
         m_Info = empty;
         if( wcschr(szStatus, 'C') != NULL ) m_Info.sStatus += _T("Conflict ");
         if( wcschr(szStatus, 'D') != NULL ) m_Info.sStatus += _T("Modified ");
         if( wcschr(szStatus, 'M') != NULL ) m_Info.sStatus += _T("Added ");
         if( wcschr(szStatus, 'C') != NULL ) m_Info.sStatus += _T("Deleted ");
         if( wcschr(szStatus, 'L') != NULL ) m_Info.sStatus += _T("Locked ");
         if( wcschr(szStatus, '*') != NULL ) m_Info.sStatus = _T("Out-of-date");
         if( wcschr(szStatus, '!') != NULL ) m_Info.sStatus = _T("Ignored");
         LPCWSTR p = bstr + 10;
         while( *p == ' ' ) p++;
         while( *p != '\0' && *p != ' ' ) m_Info.sVersion += *p++;
         p = wcsrchr(bstr, ' ');
         if( p != NULL ) m_Info.sFilename = p + 1;
         m_Info.sFilename.TrimRight();
         return m_aResult.Add(m_Info) ? S_OK : E_OUTOFMEMORY;
      }
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


/////////////////////////////////////////////////////////////////////////
// CTagElement

class CTagElement : public IElement
{
public:
   FILEINFO* m_pInfo;
   
   CTagElement(FILEINFO* pFile) : m_pInfo(pFile) 
   {
   }
   BOOL Load(ISerializable* /*pArc*/)
   {
      return FALSE;
   }
   BOOL Save(ISerializable* pArc)
   {
      ATLASSERT(!::IsBadReadPtr(m_pInfo,sizeof(FILEINFO)));
      pArc->Write(_T("type"), _T("Revision Info"));
      pArc->Write(_T("file"), m_pInfo->sFilename);
      pArc->Write(_T("filename"), ::PathFindFileName(m_pInfo->sFilename));
      pArc->Write(_T("status"), m_pInfo->sStatus);
      pArc->Write(_T("version"), m_pInfo->sVersion);
      return TRUE;
   }
   BOOL GetName(LPTSTR pstrName, UINT cchMax) const
   {
      return _tcsncpy(pstrName, ::PathFindFileName(m_pInfo->sFilename), cchMax) > 0;
   }
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const
   {
      return _tcsncpy(pstrType, _T("Revision Info"), cchMax) > 0;
   }
   IDispatch* GetDispatch()
   {
      return NULL;
   }
};


////////////////////////////////////////////////////////////////7
// CRepositoryView
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
   m_ctrlBuilding.Create(m_hWnd, rcBuilding, _T(""), dwStyle);
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

   m_clrWarning = RGB(0,0,200);

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
   dc.SetTextColor(m_clrWarning);
   dc.SetBkMode(TRANSPARENT);
   return (LRESULT) ::GetSysColorBrush(COLOR_WINDOW);
}

LRESULT CRepositoryView::OnViewOpens(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlBuilding.ShowWindow(SW_HIDE);
   if( m_thread.IsRunning() ) return 0;
   // Show idle message
   if( m_thread.m_aResult.GetSize() == 0 ) _ShowWaitingMessage(IDS_BUILDING, RGB(0,0,200));
   // Build entire structure
   TCHAR szCommand[200] = { 0 };
   _pDevEnv->GetProperty(_T("sourcecontrol.browse.all"), szCommand, 199);
	if( _tcslen(szCommand) == 0 ) _ShowWaitingMessage(IDS_NOTCONFIGURED, RGB(200,0,0));
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

   m_ctrlFiles.DeleteAllItems();

   for( int i = 0; i < m_thread.m_aResult.GetSize(); i++ ) {
      CString sFilename = m_thread.m_aResult[i].sFilename;
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
            int iItem = m_ctrlFiles.InsertItem(LVIF_TEXT | LVIF_IMAGE, m_ctrlFiles.GetItemCount(), sPart, 0, 0, 0, 0L);
            m_ctrlFiles.SetItemData(iItem, i);
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

LRESULT CRepositoryView::OnListSelected(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   int iIndex = m_ctrlFiles.GetSelectedIndex();
   if( iIndex < 0 ) return 0;
   int iItem = m_ctrlFiles.GetItemData(iIndex);
   if( iItem >= m_thread.m_aResult.GetSize() ) return 0;
   FILEINFO info = m_thread.m_aResult[iItem];
   CTagElement prop = &info;
   _pDevEnv->ShowProperties(&prop, FALSE);
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

void CRepositoryView::_ShowWaitingMessage(UINT nRes, COLORREF clrText)
{
   m_clrWarning = clrText;
   m_ctrlFolders.DeleteAllItems();
   m_ctrlFiles.DeleteAllItems();
   m_ctrlBuilding.SetWindowText(CString(MAKEINTRESOURCE(nRes)));
   m_ctrlBuilding.ShowWindow(SW_SHOW);
   m_ctrlBuilding.Invalidate();
}

