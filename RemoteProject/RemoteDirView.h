#if !defined(AFX_REMOTEDIRVIEW_H__20050306_7DEB_A0A7_F00F_0080AD509054__INCLUDED_)
#define AFX_REMOTEDIRVIEW_H__20050306_7DEB_A0A7_F00F_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Forward declare
class CRemoteProject;
class CFileManager;

#define IDC_SFOLDERS     100
#define IDC_SFILES       101
#define IDC_SBUTTON      102


class CRemoteDirView : 
   public CWindowImpl<CRemoteDirView>,
   public IIdleListener,
   public CRawDropSource
{
public:
   CImageListCtrl m_FolderImages;    // Images for folder combobox
   CImageListCtrl m_FileImages;      // Images for file
   CComboBoxEx m_ctrlFolders;
   CListViewCtrl m_ctrlFiles;
   CStatic m_ctrlNoConnection;
   CButton m_ctrlDirUp;

   CRemoteProject* m_pProject;       // Reference to project
   CFileManager* m_pFileManager;     // Reference to file protocol implementation
   CString m_sPath;                  // Current path (full)
   CString m_sSeparator;             // Directory separator character
   bool m_bWin32Path;                // Is this a local Win32 system?

   void Init(CRemoteProject* pProject);
   void Detach();

   BEGIN_MSG_MAP(CRemoteDirView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      MESSAGE_HANDLER(WM_COMPACTING, OnViewOpens)
      MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
      COMMAND_ID_HANDLER(IDC_SBUTTON, OnUp)
      COMMAND_HANDLER(IDC_SFOLDERS, CBN_SELCHANGE, OnSelChanged)
      NOTIFY_HANDLER(IDC_SFILES, LVN_BEGINDRAG, OnBeginDrag)
      NOTIFY_HANDLER(IDC_SFILES, LVN_ITEMACTIVATE, OnItemOpen)
      NOTIFY_HANDLER(IDC_SFILES, LVN_KEYDOWN, OnListKeyDown)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnViewOpens(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnSelChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnItemOpen(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnBeginDrag(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnListKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

   // Implementation

   void _UpdateButtons();
   bool _PopulateView(LPCTSTR pstrPath);
   CString _GetItemPath(HTREEITEM hItem) const;
   bool _AddShellIcon(CImageListHandle& iml, LPCTSTR pstrExtension, DWORD dwFileAttribs, DWORD dwMoreFlags = 0) const;
   static int CALLBACK _ListSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);
};


#endif // !defined(AFX_REMOTEDIRVIEW_H__20050306_7DEB_A0A7_F00F_0080AD509054__INCLUDED_)
