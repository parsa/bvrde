
#include "StdAfx.h"
#include "resource.h"

#include "RemoteDirView.h"

#include "Project.h"
#include "FileProxy.h"


////////////////////////////////////////////////////////////////7
// CRemoteDirView

CRemoteDirView::~CRemoteDirView()
{
   if( IsWindow() ) /* scary */
      DestroyWindow();
}

void CRemoteDirView::Init(CRemoteProject* pProject)
{
   ATLASSERT(pProject);
   m_pProject = pProject;
   m_pFileManager = &pProject->m_FileManager;
   m_sPath = m_pFileManager->GetParam(_T("Path"));
   m_sSeparator = m_pFileManager->GetParam(_T("Separator"));
   m_bWin32Path = (m_sPath.Mid(1, 2) == _T(":\\"));
   ATLASSERT(!m_sPath.IsEmpty());
   ATLASSERT(!m_sSeparator.IsEmpty());
}

void CRemoteDirView::Detach()
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
   RECT rcNoConnection = { 10, 60, 180, 260 };
   dwStyle = SS_LEFT | WS_CHILD;
   m_ctrlNoConnection.Create(m_hWnd, rcNoConnection, _T(""), dwStyle);
   m_ctrlNoConnection.SetFont(AtlGetDefaultGuiFont());

   // Prepare images
   int nSmallCx = ::GetSystemMetrics(SM_CXSMICON);
   int nSmallCy = ::GetSystemMetrics(SM_CYSMICON);

   if( !m_FolderImages.IsNull() ) m_FolderImages.Destroy();
   m_FolderImages.Create(nSmallCx, nSmallCy, ILC_COLOR32 | ILC_MASK, 4, 0);
   if( m_FolderImages.IsNull() ) return (LRESULT) -1;
   _AddShellIcon(m_FolderImages, _T("C:\\XXX"), FILE_ATTRIBUTE_DIRECTORY);
   _AddShellIcon(m_FolderImages, _T("C:\\XXX"), FILE_ATTRIBUTE_DIRECTORY, SHGFI_OPENICON);
   _AddShellIcon(m_FolderImages, m_bWin32Path ? _T("C:\\") : _T("X:\\"), 0);
   m_ctrlFolders.SetImageList(m_FolderImages);

   if( !m_FileImages.IsNull() ) m_FileImages.Destroy();
   m_FileImages.Create(nSmallCx, nSmallCy, ILC_COLOR32 | ILC_MASK, 8, 0);
   if( m_FileImages.IsNull() ) return (LRESULT) -1;
   _AddShellIcon(m_FileImages, _T("C:\\XXX"), FILE_ATTRIBUTE_DIRECTORY);
   _AddShellIcon(m_FileImages, _T(".txt"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".cpp"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".tmp"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".xml"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".html"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".bat"), FILE_ATTRIBUTE_NORMAL);
   _AddShellIcon(m_FileImages, _T(".exe"), FILE_ATTRIBUTE_NORMAL);
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

LRESULT CRemoteDirView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   int iItem = m_ctrlFiles.GetSelectedIndex();
   if( iItem < 0 ) return 0;
   // Load and show menu
   DWORD dwPos = ::GetMessagePos();
   POINT ptPos = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
   CMenu menu;
   menu.LoadMenu(IDR_REMOTEDIR);
   CMenuHandle submenu = menu.GetSubMenu(0);
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, ptPos, FALSE, this);
   // Handle result locally
   switch( nCmd ) {
   case ID_REMOTEDIR_OPEN:
      {
         m_ctrlFiles.SelectItem(iItem);
         OnItemOpen(0, NULL, bHandled);
      }
      break;
   }
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
   int iPos = sPath.ReverseFind(m_sSeparator.GetAt(0));
   if( iPos >= 0 ) sPath = sPath.Left(iPos + 1);
   _PopulateView(sPath);
   return 0;
}

LRESULT CRemoteDirView::OnSelChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iIndex = m_ctrlFolders.GetCurSel();
   if( iIndex < 0 ) return 0;
   // Cannot select the root folder (Host name)
   if( iIndex == 0 ) {
      m_ctrlFolders.SetCurSel(1);
      iIndex = m_ctrlFolders.GetCurSel();
   }
   CString sPath;
   for( int i = 1; i <= iIndex; i++ ) {
      TCHAR szName[300] = { 0 };
      COMBOBOXEXITEM cbi = { 0 };
      cbi.mask = CBEIF_TEXT;
      cbi.iItem = i;
      cbi.pszText = szName;
      cbi.cchTextMax = (sizeof(szName) / sizeof(TCHAR)) - 1;
      m_ctrlFolders.GetItem(&cbi);
      sPath += szName;
      if( sPath.Right(1) != m_sSeparator ) sPath += m_sSeparator;
   }
   _PopulateView(sPath);
   return 0;
}

LRESULT CRemoteDirView::OnBeginDrag(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   if( m_ctrlFiles.GetSelectedCount() != 1 ) return 0;
   int iItem = m_ctrlFiles.GetSelectedIndex();
   CString sFilename;
   m_ctrlFiles.GetItemText(iItem, 0, sFilename);
   if( sFilename.GetLength() == 0 ) return 0;
   DWORD dwAttribs = m_ctrlFiles.GetItemData(iItem);
   if( (dwAttribs & FILE_ATTRIBUTE_DIRECTORY) != 0 ) return 0;
   USES_CONVERSION;
   CComObject<CSimpleDataObj>* pObj = NULL;
   if( FAILED( CComObject<CSimpleDataObj>::CreateInstance(&pObj) ) ) return 0;
   if( FAILED( pObj->SetHDropData(T2CA(m_sPath + sFilename)) ) ) return 0;
   DWORD dwEffect = 0;
   ::DoDragDrop(pObj, this, DROPEFFECT_COPY, &dwEffect);
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
      _PopulateView(m_sPath + sFilename);
   }
   else {
      ATLASSERT(m_pProject);
      m_pProject->OpenView(m_sPath + sFilename, 1, FINDVIEW_FULLPATH | FINDVIEW_ALLPROJECTS | FINDVIEW_DEPENDENCIES, true);
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
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, CString(MAKEINTRESOURCE(IDS_STATUS_ENUMFILES)));

   if( m_pFileManager == NULL ) {
      m_ctrlFolders.ResetContent();
      m_ctrlFiles.DeleteAllItems();
      m_ctrlDirUp.EnableWindow(FALSE);
      m_ctrlNoConnection.SetWindowText(CString(MAKEINTRESOURCE(IDS_NOCONNECTION)));
      m_ctrlNoConnection.ShowWindow(SW_SHOWNOACTIVATE);
      m_ctrlNoConnection.Invalidate();
      return false;
   }

   CString sPath = pstrPath;
   CString sHost = m_pFileManager->GetParam(_T("Host"));

   CString sHostName;
   sHostName.Format(_T("[%s]"), sHost);
   if( m_bWin32Path ) sHostName.LoadString(IDS_LOCALDRIVE);

   if( sPath.Right(1) != m_sSeparator ) sPath += m_sSeparator;

   // NOTE: We're reusing the shared FTP/SFTP connection so we cannot
   //       actually leave the path changed (since the opening of views may rely
   //       on cur.dir in some protocols). We'll remember the original path
   //       and restore it on exit.
   CString sOldPath = m_pFileManager->GetParam(_T("Path"));
   m_pFileManager->SetCurPath(sPath);

   CSimpleArray<WIN32_FIND_DATA> aFiles;
   if( !m_pFileManager->EnumFiles(aFiles, false) ) {
      DWORD dwErr = ::GetLastError();
      m_ctrlFiles.DeleteAllItems();
      m_ctrlDirUp.EnableWindow(FALSE);
      CString sMsg = CString(MAKEINTRESOURCE(IDS_NOCONNECTION));
      sMsg += _T("\r\n\r\n\r\n");
      sMsg += GetSystemErrorText(dwErr);
      m_ctrlNoConnection.SetWindowText(sMsg);
      m_ctrlNoConnection.ShowWindow(SW_SHOWNOACTIVATE);
      m_ctrlNoConnection.Invalidate();
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

      // Translate filename into listview image.
      // Since this is usually on a remote drive we'll not attempt
      // to use the local Win32 icon-association methods.
      CString sFileType = GetFileTypeFromFilename(sFilename);
      typedef struct tagFILEIMAGE {
         LPCTSTR pstr;
         int iImage;
      } FILEIMAGE;
      static FILEIMAGE ppstrFileTypes[] = {
         { _T("cpp"),      2 },
         { _T("header"),   2 },
         { _T("text"),     3 },
         { _T("makefile"), 6 },
         { _T("xml"),      4 },
         { _T("html"),     5 },
         { _T("asp"),      5 },
         { _T("php"),      5 },
         { _T("bash"),     6 },
         { NULL, NULL }
      };
      static FILEIMAGE ppstrFileExt[] = {
         { _T(".TXT"),     1 },
         { _T(".CFG"),     1 },
         { _T(".LOG"),     1 },
         { _T(".INI"),     1 },
         { _T(".CONFIG"),  1 },
         { _T(".CMD"),     6 },
         { _T(".BAT"),     6 },
         { _T(".SH"),      6 },
         { _T(".EXE"),     7 },
         { NULL, NULL }
      };
      int j;
      int iImage = 3;
      for( j = 0; ppstrFileTypes[j].pstr != NULL; j++ ) {
         if( sFileType == ppstrFileTypes[j].pstr ) iImage = ppstrFileTypes[j].iImage;
      }
      for( j = 0; ppstrFileExt[j].pstr != NULL; j++ ) {
         if( sFilename.Find(ppstrFileExt[j].pstr) > 0 ) iImage = ppstrFileExt[j].iImage;
      }
      if( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ) iImage = 0;
      
      UINT uState = 0;
      if( fd.cFileName[0] == '.' ) uState = LVIS_CUT;
      if( fd.cFileName[0] == '$' ) uState = LVIS_CUT;

      m_ctrlFiles.InsertItem(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_STATE, 
         i, 
         fd.cFileName, 
         uState, LVIS_CUT, 
         iImage, 
         fd.dwFileAttributes);
   }

   m_ctrlFiles.SortItemsEx(_ListSortProc, (LPARAM) this);

   CString sBuildPath;
   CString sName = sHostName;
   for( int iIndex = 0; ; iIndex++ ) {      
      COMBOBOXEXITEM cbi = { 0 };
      cbi.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_INDENT | CBEIF_TEXT;
      cbi.iItem = iIndex;
      cbi.pszText = (LPTSTR) (LPCTSTR) sName;
      cbi.iImage = iIndex == 0 ? 2 : 0;
      cbi.iSelectedImage = iIndex == 0 ? 2 : 1;
      cbi.iIndent = iIndex;
      m_ctrlFolders.InsertItem(&cbi);

      int iPos = sPath.Find(m_sSeparator, sBuildPath.GetLength());
      if( iPos < 0 ) break;
      if( iIndex == 0 && (iPos == 0 || m_bWin32Path) ) iPos += 1; // Make sure we capture the / or C:\ separator in the root
      sName = sPath.Mid(sBuildPath.GetLength(), iPos - sBuildPath.GetLength());

      sBuildPath += sName;
      if( sBuildPath.Right(1) != m_sSeparator ) sBuildPath += m_sSeparator;
   }

   m_ctrlFolders.SetCurSel(m_ctrlFolders.GetCount() - 1);
   m_ctrlFiles.SetFocus();
   m_ctrlFiles.SelectItem(0);

   m_pFileManager->SetCurPath(sOldPath);

   m_sPath = sPath;

   _UpdateButtons();

   return true;
}

void CRemoteDirView::_UpdateButtons()
{
   m_ctrlDirUp.EnableWindow(m_ctrlFolders.GetCurSel() > 1);
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

// IIdleListener

void CRemoteDirView::OnIdle(IUpdateUI* pUIBase)
{   
   BOOL bEnabled = FALSE;
   int iItem = m_ctrlFiles.GetSelectedIndex();
   if( iItem >= 0 ) {
      DWORD dwAttribs = m_ctrlFiles.GetItemData(iItem);
      bEnabled = ((dwAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0);
   }
   pUIBase->UIEnable(ID_REMOTEDIR_OPEN, bEnabled);
}

void CRemoteDirView::OnGetMenuText(UINT /*wID*/, LPTSTR /*pstrText*/, int /*cchMax*/)
{
}

