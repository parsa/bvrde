
#include "StdAfx.h"
#include "resource.h"

#include "RemoteDirView.h"

#include "Project.h"
#include "FileProxy.h"


////////////////////////////////////////////////////////////////7
//
//

void CRemoteDirView::Init(CRemoteProject* pProject)
{
   ATLASSERT(pProject);
   m_pProject = pProject;
   m_pFileManager = &pProject->m_FileManager;
   m_sPath = m_pFileManager->GetParam(_T("Path"));
   m_sSeparator = m_pFileManager->GetParam(_T("Separator"));
   ATLASSERT(!m_sPath.IsEmpty());
   ATLASSERT(!m_sSeparator.IsEmpty());
}

void CRemoteDirView::Release()
{
   m_pProject = NULL;
   m_pFileManager = NULL;
}

LRESULT CRemoteDirView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   SetFont(AtlGetDefaultGuiFont());

   // Create controls
   DWORD dwStyle;
   RECT rcFolders = { 0, 0, 200, 200 };
   dwStyle = CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_TABSTOP;
   m_ctrlFolders.Create(m_hWnd, rcFolders, NULL, dwStyle, 0, IDC_SFOLDERS);
   m_ctrlFolders.SetExtendedStyle(CBES_EX_PATHWORDBREAKPROC, CBES_EX_PATHWORDBREAKPROC);
   dwStyle = LVS_SHAREIMAGELISTS | LVS_SINGLESEL | LVS_REPORT | LVS_AUTOARRANGE | LVS_NOSORTHEADER | LVS_SORTASCENDING | WS_CHILD | WS_VISIBLE | WS_TABSTOP;
   m_ctrlFiles.Create(m_hWnd, rcDefault, NULL, dwStyle, WS_EX_CLIENTEDGE, IDC_SFILES);
   dwStyle = BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE;
   m_ctrlDirUp.Create(m_hWnd, rcDefault, _T(".."), dwStyle, 0, IDC_SBUTTON);
   RECT rcNoConnection = { 10, 60, 180, 220 };
   dwStyle = SS_LEFT | WS_CHILD;
   m_ctrlNoConnection.Create(m_hWnd, rcNoConnection, CString(MAKEINTRESOURCE(IDS_NOCONNECTION)), dwStyle);
   m_ctrlNoConnection.SetFont(AtlGetDefaultGuiFont());

   // Prepare images
   int nSmallCx = ::GetSystemMetrics(SM_CXSMICON);
   int nSmallCy = ::GetSystemMetrics(SM_CYSMICON);

   if( !m_FolderImages.IsNull() ) m_FolderImages.Destroy();
   m_FolderImages.Create(nSmallCx, nSmallCy, ILC_COLOR32 | ILC_MASK, 4, 0);
   if( m_FolderImages.IsNull() ) return -1;
   _AddShellIcon(m_FolderImages, _T(""), FILE_ATTRIBUTE_DIRECTORY);
   _AddShellIcon(m_FolderImages, _T(""), FILE_ATTRIBUTE_DIRECTORY, SHGFI_OPENICON);
   _AddShellIcon(m_FolderImages, _T("C:\\"), 0);
   m_ctrlFolders.SetImageList(m_FolderImages);

   if( !m_FileImages.IsNull() ) m_FileImages.Destroy();
   m_FileImages.Create(nSmallCx, nSmallCy, ILC_COLOR32 | ILC_MASK, 8, 0);
   if( m_FileImages.IsNull() ) return -1;
   _AddShellIcon(m_FileImages, _T(""), FILE_ATTRIBUTE_DIRECTORY);
   _AddShellIcon(m_FileImages, _T(".txt"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".dat"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".tmp"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".xml"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".html"), FILE_ATTRIBUTE_NORMAL);
   m_ctrlFiles.SetImageList(m_FileImages, LVSIL_SMALL);

   // Prepare ListView
   TCHAR szTitle[128] = { 0 };
   ::LoadString(_Module.GetResourceInstance(), IDS_FILES, szTitle, 127);
   m_ctrlFiles.InsertColumn(0, szTitle, LVCFMT_LEFT, 200, 0);

   SendMessage(WM_SIZE);

   return 0;
}

LRESULT CRemoteDirView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   CWindowRect rcCombo = m_ctrlFolders;
   CClientRect rcClient = m_hWnd;
   int cxy = (rcCombo.bottom - rcCombo.top);
   m_ctrlFolders.MoveWindow(rcClient.left, rcClient.top, rcClient.right - cxy, rcClient.top + cxy + 200);
   m_ctrlDirUp.MoveWindow(rcClient.right - cxy, rcClient.top, cxy, rcClient.top + cxy);
   m_ctrlFiles.MoveWindow(rcClient.left, rcClient.top + cxy, rcClient.right, rcClient.bottom - cxy);
   m_ctrlFiles.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
   return 0;
}

LRESULT CRemoteDirView::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   if( (HWND) lParam != m_ctrlNoConnection ) {
      bHandled = FALSE;
      return 0;
   }
   CDCHandle dc = (HDC) wParam;
   dc.SetTextColor(RGB(200,0,0));
   dc.SetBkMode(TRANSPARENT);
   return (LRESULT) ::GetSysColorBrush(COLOR_WINDOW);
}

LRESULT CRemoteDirView::OnViewOpens(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   _PopulateView(m_sPath);
   return 0;
}

LRESULT CRemoteDirView::OnUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( !m_ctrlDirUp.IsWindowEnabled() ) return 0;
   CString sPath = m_sPath.Left(m_sPath.GetLength() - 1);
   int pos = sPath.ReverseFind(m_sSeparator.GetAt(0));
   if( pos >= 0 ) sPath = sPath.Left(pos + 1);
   _PopulateView(sPath);
   return 0;
}

LRESULT CRemoteDirView::OnSelChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iIndex = m_ctrlFolders.GetCurSel();
   if( iIndex < 0 ) return 0;
   CString sPath;
   for( int i = 0; i <= iIndex; i++ ) {
      TCHAR szName[300] = { 0 };
      COMBOBOXEXITEM cbi = { 0 };
      cbi.mask = CBEIF_TEXT;
      cbi.iItem = i;
      cbi.pszText = szName;
      cbi.cchTextMax = (sizeof(szName) / sizeof(TCHAR)) - 1;
      if( i > 0 ) m_ctrlFolders.GetItem(&cbi);
      sPath += szName;
      sPath += m_sSeparator;
   }
   _PopulateView(sPath);
   return 0;
}


LRESULT CRemoteDirView::OnBeginDrag(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   return 0;
}

LRESULT CRemoteDirView::OnItemOpen(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   CWaitCursor cursor;
  
   // Is selected item a directory? 
   // We need to navigate into the subfolder then...
   if( m_ctrlFiles.GetSelectedCount() != 1 ) return 0;
   int iItem = m_ctrlFiles.GetSelectedIndex();
   CString sFilename;
   m_ctrlFiles.GetItemText(iItem, 0, sFilename);
   if( sFilename.GetLength() == 0 ) return 0;
   DWORD dwAttribs = m_ctrlFiles.GetItemData(iItem);
   if( (dwAttribs & FILE_ATTRIBUTE_DIRECTORY) != 0 ) {
      m_sPath += sFilename;
      _PopulateView(m_sPath);
   }
   else {
      ATLASSERT(m_pProject);
      m_pProject->OpenView(m_sPath + sFilename, 0);
   }
   return 0;
}

LRESULT CRemoteDirView::OnListKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   LPNMLVKEYDOWN lpNMLVD = (LPNMLVKEYDOWN) pnmh;
   if( lpNMLVD->wVKey == VK_BACK ) PostMessage(WM_COMMAND, MAKEWPARAM(IDC_SBUTTON, 0));
   bHandled = FALSE;
   return 0;
}

bool CRemoteDirView::_PopulateView(LPCTSTR pstrPath)
{
   CWaitCursor cursor;

   if( m_pFileManager == NULL ) {
      m_ctrlFolders.ResetContent();
      m_ctrlFiles.DeleteAllItems();
      m_ctrlDirUp.EnableWindow(FALSE);
      m_ctrlNoConnection.ShowWindow(SW_SHOW);
      return false;
   }

   CString sHost = m_pFileManager->GetParam(_T("Host"));
   CString sHostName;
   sHostName.Format(_T("[%s]"), sHost);

   m_sPath = pstrPath;
   if( m_sPath.Right(1) != m_sSeparator ) m_sPath += m_sSeparator;

   CString sOldPath = m_pFileManager->GetParam(_T("Path"));
   m_pFileManager->SetCurPath(m_sPath);

   CSimpleArray<WIN32_FIND_DATA> aFiles;
   if( !m_pFileManager->EnumFiles(aFiles) ) {
      m_ctrlFiles.DeleteAllItems();
      m_ctrlDirUp.EnableWindow(FALSE);
      m_ctrlNoConnection.ShowWindow(SW_SHOW);
      m_pFileManager->SetCurPath(sOldPath);
      return false;
   }

   CWindowRedraw redraw1 = m_ctrlFiles;
   CWindowRedraw redraw2 = m_ctrlFolders;

   m_ctrlFiles.DeleteAllItems();
   m_ctrlFolders.ResetContent();

   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      const WIN32_FIND_DATA& fd = aFiles[i];
      CString sFilename = fd.cFileName;
      sFilename.MakeUpper();
      int iImage = 3;
      if( sFilename.Find(_T(".TXT")) > 0) iImage = 1;
      if( sFilename.Find(_T(".C")) > 0 ) iImage = 2;
      if( sFilename.Find(_T(".H")) > 0 ) iImage = 2;
      if( sFilename.Find(_T(".XML")) > 0 ) iImage = 4;
      if( sFilename.Find(_T(".HTM")) > 0 ) iImage = 5;
      if( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ) iImage = 0;
      m_ctrlFiles.InsertItem(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, 
         i, 
         fd.cFileName, 
         0, 0, 
         iImage, 
         fd.dwFileAttributes);
   }

   m_ctrlFiles.SortItemsEx(_ListSortProc, (LPARAM) this);

   CString sPath;
   CString sName = sHost;
   int iIndex = 0;
   while( true ) {      
      COMBOBOXEXITEM cbi = { 0 };
      cbi.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_INDENT | CBEIF_TEXT;
      cbi.iItem = iIndex;
      cbi.pszText = (LPTSTR) (LPCTSTR) sName;
      cbi.iImage = iIndex == 0 ? 2 : 0;
      cbi.iSelectedImage = iIndex == 0 ? 2 : 1;
      cbi.iIndent = iIndex;
      m_ctrlFolders.InsertItem(&cbi);

      int pos = m_sPath.Find(m_sSeparator, sPath.GetLength() + 1);
      if( pos < 0 ) break;
      sName = m_sPath.Mid(sPath.GetLength() + 1, pos - sPath.GetLength() - 1);

      sPath += sName;
      sPath += m_sSeparator;

      iIndex++;
   }

   m_ctrlFolders.SetCurSel(m_ctrlFolders.GetCount() - 1);
   m_ctrlFiles.SetFocus();
   m_ctrlFiles.SelectItem(0);

   m_pFileManager->SetCurPath(sOldPath);

   _UpdateButtons();

   return true;
}

void CRemoteDirView::_UpdateButtons()
{
   m_ctrlDirUp.EnableWindow(m_ctrlFolders.GetCurSel() >= 1);
   m_ctrlDirUp.SetButtonStyle(BS_PUSHBUTTON);
   m_ctrlNoConnection.ShowWindow(SW_HIDE);
}

bool CRemoteDirView::_AddShellIcon(CImageListHandle& iml, LPCTSTR pstrExtension, DWORD dwFileAttribs, DWORD dwMoreFlags /*= 0*/) const
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

int CALLBACK CRemoteDirView::_ListSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   CRemoteDirView* pThis = (CRemoteDirView*) lParamSort;
   TCHAR szName1[MAX_PATH];
   TCHAR szName2[MAX_PATH];
   LVITEM lvi1 = { 0 };
   LVITEM lvi2 = { 0 };
   lvi1.iItem = lParam1;
   lvi2.iItem = lParam2;
   lvi1.pszText = szName1;
   lvi2.pszText = szName2;
   lvi1.mask = lvi2.mask = LVIF_TEXT | LVIF_PARAM;
   lvi1.cchTextMax = lvi2.cchTextMax = MAX_PATH;
   pThis->m_ctrlFiles.GetItem(&lvi1);
   pThis->m_ctrlFiles.GetItem(&lvi2);
   int lRes = (lvi2.lParam & FILE_ATTRIBUTE_DIRECTORY) - (lvi1.lParam & FILE_ATTRIBUTE_DIRECTORY);
   if( lRes != 0 ) return lRes;
   return ::lstrcmp(lvi1.pszText, lvi2.pszText);
}
