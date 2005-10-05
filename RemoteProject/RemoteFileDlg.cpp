
#include "StdAfx.h"
#include "resource.h"

#include "RemoteFileDlg.h"
#include "FileProxy.h"

#pragma code_seg( "DIALOGS" )


CRemoteFileDlg::CRemoteFileDlg(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
   CFileManager* pFileManager,
   LPCTSTR pstrDefExt /*= NULL*/,
   DWORD dwFlags /*= OFN_NOCHANGEDIR*/,
   LPCTSTR pstrFilter /*= NULL*/)
{
   m_pFileManager = pFileManager;
   m_pstrFilter = pstrFilter;
   m_sDefExt = pstrDefExt;
   m_dwFlags = dwFlags;
   ::ZeroMemory(&m_ofn, sizeof(m_ofn));
   m_bInside = false;
   m_pstrBuffer = NULL;
}

CRemoteFileDlg::~CRemoteFileDlg()
{
   if( m_pstrBuffer != NULL ) free(m_pstrBuffer);
}

// Message Handlers

LRESULT CRemoteFileDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pFileManager);

   m_sSeparator = m_pFileManager->GetParam(_T("Separator"));
   ATLASSERT(!m_sSeparator.IsEmpty());

   m_sOrigPath = m_ofn.lpstrInitialDir;
   if( m_sOrigPath.Right(1) != m_sSeparator ) m_sOrigPath += m_sSeparator;

   m_ctrlFilename = GetDlgItem(IDC_FILENAME);
   m_ctrlTypes = GetDlgItem(IDC_TYPES);
   m_ctrlList = GetDlgItem(IDC_LIST);
   m_ctrlNoConnection = GetDlgItem(IDC_NOCONNECTION);
   m_ctrlFolder = GetDlgItem(IDC_FOLDER);
   m_ctrlOK = GetDlgItem(IDOK);

   // We support multiple selections
   if( m_dwFlags & OFN_ALLOWMULTISELECT ) m_ctrlList.ModifyStyle(LVS_SINGLESEL, 0);

   // Prepare images
   int nSmallCx = ::GetSystemMetrics(SM_CXSMICON);
   int nSmallCy = ::GetSystemMetrics(SM_CYSMICON);

   m_FolderImages.Create(nSmallCx, nSmallCy, ILC_COLOR32 | ILC_MASK, 4, 0);
   if( m_FolderImages.IsNull() ) return -1;
   _AddShellIcon(m_FolderImages, _T(""), FILE_ATTRIBUTE_DIRECTORY);
   _AddShellIcon(m_FolderImages, _T(""), FILE_ATTRIBUTE_DIRECTORY, SHGFI_OPENICON);
   _AddShellIcon(m_FolderImages, _T("C:\\"), FILE_ATTRIBUTE_DIRECTORY);
   m_ctrlFolder.SetImageList(m_FolderImages);

   m_FileImages.Create(nSmallCx, nSmallCy, ILC_COLOR32 | ILC_MASK, 8, 0);
   if( m_FileImages.IsNull() ) return -1;
   _AddShellIcon(m_FileImages, _T(""), FILE_ATTRIBUTE_DIRECTORY);
   _AddShellIcon(m_FileImages, _T(".txt"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".exe"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".tmp"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".xml"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".bat"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".html"), FILE_ATTRIBUTE_NORMAL);
   m_ctrlList.SetImageList(m_FileImages, LVSIL_SMALL);

   // Buttons
   m_BackIcon.LoadIcon(IDI_BACK, 16, 16);
   m_UpIcon.LoadIcon(IDI_UP, 16, 16);
   m_ctrlBack = GetDlgItem(IDC_BACK);
   m_ctrlBack.SetIcon(m_BackIcon);
   m_ctrlUp = GetDlgItem(IDC_UP);
   m_ctrlUp.SetIcon(m_UpIcon);

   // Prepare filter
   if( m_pstrFilter ) {
      LPCTSTR pstrFilter = m_pstrFilter;
      while( *pstrFilter ) {
         CString sName = pstrFilter;
         m_ctrlTypes.AddString(sName);
         pstrFilter += ::lstrlen(pstrFilter) + 1;
         CString sFilter = pstrFilter;
         m_aFilters.Add(sFilter);
         pstrFilter += ::lstrlen(pstrFilter) + 1;
      }
      m_ctrlTypes.SetCurSel(0);
   }

   // Initial folder listing
   _PopulateView(m_sOrigPath);
   _UpdateButtons();

   DlgResize_Init(true, true);
   return TRUE;
}

LRESULT CRemoteFileDlg::OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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

LRESULT CRemoteFileDlg::OnUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CString sPath = m_sPath.Left(m_sPath.GetLength() - 1);
   int pos = sPath.ReverseFind(m_sSeparator.GetAt(0));
   if( pos >= 0 ) sPath = sPath.Left(pos + 1);
   _PopulateView(sPath);
   m_ctrlUp.SetButtonStyle(BS_PUSHBUTTON);
   return 0;
}

LRESULT CRemoteFileDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( !m_ctrlOK.IsWindowEnabled() ) return 0;

   CWaitCursor cursor;

   // Is selected item a directory? 
   // We need to navigate into the subfolder then...
   if( m_ctrlList.GetSelectedCount() == 1 ) {
      int iItem = m_ctrlList.GetNextItem(-1, LVNI_SELECTED);
      CString sFilename;
      m_ctrlList.GetItemText(iItem, 0, sFilename);
      if( sFilename.GetLength() == 0 ) return 0;

      CString sText = CWindowText(m_ctrlFilename);
      if( sText == sFilename ) {
         DWORD dwAttribs = m_ctrlList.GetItemData(iItem);
         if( dwAttribs & FILE_ATTRIBUTE_DIRECTORY ) {
            m_sPath += sFilename;
            m_sPath += m_sSeparator;
            _PopulateView(m_sPath);
            _UpdateButtons();
            return 0;
         }
      }
   }

   // Create result. The path is the first entry.
   // First find all the selected files...
   CString sFiles = CWindowText(m_ctrlFilename);
   CSimpleArray<CString> aFiles;
   while( !sFiles.IsEmpty() ) {
      sFiles.TrimLeft(_T(" \""));
      if( sFiles.IsEmpty() ) break;
      CString sFilename = sFiles.SpanExcluding(_T("\""));
      int iLen = sFilename.GetLength();
      if( iLen == 0 ) break;
      if( !m_sDefExt.IsEmpty() && sFilename.Find('.') < 0 ) sFilename += _T(".") + m_sDefExt;
      aFiles.Add(sFilename);
      sFiles = sFiles.Mid(iLen + 1);
   }
   sFiles.TrimRight();
   if( !sFiles.IsEmpty() ) {
      if( !m_sDefExt.IsEmpty() && sFiles.Find('.') < 0 ) sFiles += _T(".") + m_sDefExt;
      aFiles.Add(sFiles);
   }
   // ... then figure out the size of the return-buffer
   DWORD dwBufLen = m_sPath.GetLength() + 1;
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      dwBufLen += aFiles[i].GetLength() + 1;
   }
   dwBufLen += 3;
   // ... then generate the return-buffer, which is returned as a SZ-array.
   LPTSTR pstr = m_pstrBuffer = (LPTSTR) malloc(dwBufLen * sizeof(TCHAR));
   ::lstrcpy(pstr, m_sPath);
   pstr += ::lstrlen(pstr) + 1;
   for( int j = 0; j < aFiles.GetSize(); j++ ) {
      ::lstrcpy(pstr, aFiles[j]);
      pstr += ::lstrlen(pstr) + 1;
   }
   *pstr = '\0';

   m_ofn.lpstrFile = m_pstrBuffer;

   // Change back to original path
   if( m_dwFlags & OFN_NOCHANGEDIR ) m_pFileManager->SetCurPath(m_sOrigPath);

   EndDialog(wID);
   return 0;
}

LRESULT CRemoteFileDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   // Change back to original path
   if( m_dwFlags & OFN_NOCHANGEDIR ) m_pFileManager->SetCurPath(m_sOrigPath);

   EndDialog(wID);
   return 0;
}

LRESULT CRemoteFileDlg::OnFolderChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{      
   int iIndex = m_ctrlFolder.GetCurSel();
   if( iIndex < 0 ) return 0;
   CString sPath;
   for( int i = 0; i <= iIndex; i++ ) {
      TCHAR szName[MAX_PATH] = { 0 };
      COMBOBOXEXITEM cbi = { 0 };
      cbi.mask = CBEIF_TEXT;
      cbi.iItem = i;
      cbi.pszText = szName;
      cbi.cchTextMax = (sizeof(szName) / sizeof(TCHAR)) - 1;
      if( i > 0 ) m_ctrlFolder.GetItem(&cbi);
      sPath += szName;
      sPath += m_sSeparator;
   }
   _PopulateView(sPath);
   _UpdateButtons();
   return 0;
}

LRESULT CRemoteFileDlg::OnFilenameChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{      
   m_bInside = true;

   CString sFilenames = CWindowText(m_ctrlFilename);
   CSimpleArray<CString> aFiles;
   _GetSelectedFiles(aFiles);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      if( sFilenames.Find(aFiles[i]) < 0 ) {
         // Remove selected file if it's not here anymore
         LV_FINDINFO fi = { 0 };
         fi.flags = LVFI_STRING;
         fi.psz = aFiles[i];
         int iIndex = m_ctrlList.FindItem(&fi, -1);
         if( iIndex >= 0 ) m_ctrlList.SetItemState(iIndex, 0, LVIS_FOCUSED|LVIS_SELECTED);
      }
   }
   _UpdateButtons();

   m_bInside = false;
   return 0;
}

LRESULT CRemoteFileDlg::OnTypeChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   m_ctrlFilename.SetWindowText(_T(""));
   _PopulateView(m_sPath);
   _UpdateButtons();
   return 0;
}

LRESULT CRemoteFileDlg::OnItemOpen(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   BOOL bDummy;
   OnOK(0, IDOK, NULL, bDummy);
   return 0;
}

LRESULT CRemoteFileDlg::OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   if( m_bInside ) return 0;

   CString sFilenames;
   CSimpleArray<CString> aFiles;
   _GetSelectedFiles(aFiles);
   int nCount = aFiles.GetSize();
   for( int i = 0; i < nCount; i++ ) {
      if( i > 0 ) sFilenames += ' ';
      if( nCount > 1 ) sFilenames += '\"';
      sFilenames += aFiles[i];
      if( nCount > 1 ) sFilenames += '\"';
   }
   m_ctrlFilename.SetWindowText(sFilenames);
   _UpdateButtons();
   return 0;
}

// Implementation

void CRemoteFileDlg::_UpdateButtons()
{
   int nSelCount = m_ctrlList.GetSelectedCount();
   BOOL bHasFilename = m_ctrlFilename.GetWindowTextLength() > 0;
   BOOL bFolderSelected = FALSE;
   int iIndex = m_ctrlList.GetNextItem(-1, LVNI_SELECTED);
   while( iIndex != -1 ) {
      LVITEM lvi = { 0 };
      lvi.iItem = iIndex;
      lvi.mask = LVIF_PARAM;
      m_ctrlList.GetItem(&lvi);
      if( lvi.lParam & FILE_ATTRIBUTE_DIRECTORY ) bFolderSelected = TRUE;
      iIndex = m_ctrlList.GetNextItem(iIndex, LVNI_SELECTED);
   }
   m_ctrlOK.EnableWindow(bHasFilename && (nSelCount <= 1 || !bFolderSelected));
   m_ctrlUp.EnableWindow(m_ctrlFolder.GetCurSel() >= 1);
}

bool CRemoteFileDlg::_PopulateView(LPCTSTR pstrPath)
{
   CWaitCursor cursor;

   CString sHost = m_pFileManager->GetParam(_T("Host"));
   CString sHostName;
   sHostName.Format(_T("[%s]"), sHost);

   m_sPath = pstrPath;

   m_pFileManager->SetCurPath(m_sPath);

   CSimpleArray<WIN32_FIND_DATA> aFiles;
   if( !m_pFileManager->EnumFiles(aFiles) ) {
      m_ctrlList.DeleteAllItems();
      m_ctrlNoConnection.ShowWindow(SW_SHOW);
      return false;
   }

   CSimpleArray<CString> aFilters;
   _GetSelectedFilters(m_ctrlTypes.GetCurSel(), aFilters);

   CWindowRedraw redraw1 = m_ctrlList;
   CWindowRedraw redraw2 = m_ctrlFolder;

   m_ctrlList.DeleteAllItems();
   m_ctrlFolder.ResetContent();

   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      const WIN32_FIND_DATA& fd = aFiles[i];
      CString sFilename = fd.cFileName;

      bool bFound = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
      for( int j = 0; !bFound && j < aFilters.GetSize(); j++ ) {
         bFound = ::PathMatchSpec(sFilename, aFilters[j]) == TRUE;
      }
      if( !bFound ) continue;

      sFilename.MakeUpper();
      int iImage = 3;
      if( sFilename.Find(_T("MAK")) == 0 ) iImage = 5;
      if( sFilename.Find(_T(".TXT")) > 0) iImage = 1;
      if( sFilename.Find(_T(".LOG")) > 0) iImage = 1;
      if( sFilename.Find(_T(".CFG")) > 0) iImage = 1;
      if( sFilename.Find(_T(".C")) > 0 ) iImage = 4;
      if( sFilename.Find(_T(".H")) > 0 ) iImage = 4;
      if( sFilename.Find(_T(".EC")) > 0 ) iImage = 4;
      if( sFilename.Find(_T(".PC")) > 0 ) iImage = 4;
      if( sFilename.Find(_T(".SH")) > 0 ) iImage = 5;
      if( sFilename.Find(_T(".MAK")) > 0 ) iImage = 5;
      if( sFilename.Find(_T(".HTM")) > 0 ) iImage = 6;
      if( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ) iImage = 0;
      m_ctrlList.InsertItem(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, 
         i, 
         fd.cFileName, 
         0, 0, 
         iImage, 
         fd.dwFileAttributes);
   }

   m_ctrlList.SortItemsEx(_ListSortProc, (LPARAM) this);

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
      m_ctrlFolder.InsertItem(&cbi);

      int pos = m_sPath.Find(m_sSeparator, sPath.GetLength() + 1);
      if( pos < 0 ) break;
      sName = m_sPath.Mid(sPath.GetLength() + 1, pos - sPath.GetLength() - 1);

      sPath += sName;
      sPath += m_sSeparator;

      iIndex++;
   }

   m_ctrlFolder.SetCurSel(m_ctrlFolder.GetCount() - 1);
   m_ctrlFilename.SetWindowText(_T(""));
   m_ctrlList.SetItemState(0, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
   m_ctrlFilename.SetFocus();

   return true;
}

bool CRemoteFileDlg::_AddShellIcon(CImageListHandle& iml, LPCTSTR pstrExtension, DWORD dwFileAttribs, DWORD dwMoreFlags /*= 0*/) const
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

bool CRemoteFileDlg::_GetSelectedFilters(int iIndex, CSimpleArray<CString>& aFilters) const
{
   CString sFilters = m_aFilters[iIndex];
   while( !sFilters.IsEmpty() ) {
      CString sToken = sFilters.SpanExcluding(_T(";"));
      if( sToken.IsEmpty() ) break;
      aFilters.Add(sToken);
      sFilters = sFilters.Mid(sToken.GetLength() + 1);
   }
   return true;
}

bool CRemoteFileDlg::_GetSelectedFiles(CSimpleArray<CString>& aFiles) const
{
   int iIndex = m_ctrlList.GetNextItem(-1, LVNI_SELECTED);
   while( iIndex != -1 ) {
      CString sFilename;
      m_ctrlList.GetItemText(iIndex, 0, sFilename);
      aFiles.Add(sFilename);
      iIndex = m_ctrlList.GetNextItem(iIndex, LVNI_SELECTED);
   }
   return true;
}

int CALLBACK CRemoteFileDlg::_ListSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   CRemoteFileDlg* pThis = (CRemoteFileDlg*) lParamSort;
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
   pThis->m_ctrlList.GetItem(&lvi1);
   pThis->m_ctrlList.GetItem(&lvi2);
   int lRes = (lvi2.lParam & FILE_ATTRIBUTE_DIRECTORY) - (lvi1.lParam & FILE_ATTRIBUTE_DIRECTORY);
   if( lRes != 0 ) return lRes;
   return ::lstrcmp(lvi1.pszText, lvi2.pszText);
}
