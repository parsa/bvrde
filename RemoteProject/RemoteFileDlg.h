#if !defined(AFX_FTPFILEDLG_H__20030315_311D_8B14_24C9_0080AD509054__INCLUDED_)
#define AFX_FTPFILEDLG_H__20030315_311D_8B14_24C9_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CFileManager;


/**
 * @class CRemoteFileDlg
 * Displays a File Dialog similar to the standard Windows one - execpt that
 * it display files and folders from a remote server connection.
 */
class CRemoteFileDlg : 
   public CDialogImpl<CRemoteFileDlg>,
   public CDialogResize<CRemoteFileDlg>
{
public:
   enum { IDD = IDD_FILEBROWSE };

   HWND m_hWndParent;
   CFileManager* m_pFileManager;
   CString m_sSeparator;
   CString m_sDefExt;
   OPENFILENAME m_ofn;
   LPCTSTR m_pstrFilter;
   DWORD m_dwFlags;
   CSimpleArray<CString> m_aFilters;
   CString m_sOrigPath;
   CString m_sPath;
   TCHAR m_cBuffer[2048];
   bool m_bInside;

   CImageListCtrl m_FolderImages;
   CImageListCtrl m_FileImages;
   CEdit m_ctrlFilename;
   CStatic m_ctrlNoConnection;
   CListViewCtrl m_ctrlList;
   CComboBoxEx m_ctrlFolder;
   CComboBox m_ctrlTypes;
   CIcon m_BackIcon;
   CIcon m_UpIcon;
   CButton m_ctrlBack;
   CButton m_ctrlUp;
   CButton m_ctrlOK;

   CRemoteFileDlg(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
      CFileManager* pFileManager,
      LPCTSTR pstrDefExt = NULL,
      DWORD dwFlags = OFN_NOCHANGEDIR,
      LPCTSTR pstrFilter = NULL);

   BEGIN_DLGRESIZE_MAP(CRemoteFileDlg)
      DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
      DLGRESIZE_CONTROL(IDC_FILENAME, DLSZ_SIZE_X | DLSZ_MOVE_Y)
      DLGRESIZE_CONTROL(IDC_TYPES, DLSZ_SIZE_X | DLSZ_MOVE_Y)
      DLGRESIZE_CONTROL(IDC_FILENAME_LABEL, DLSZ_MOVE_Y)
      DLGRESIZE_CONTROL(IDC_TYPES_LABEL, DLSZ_MOVE_Y)
      DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
      DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
   END_DLGRESIZE_MAP()

   BEGIN_MSG_MAP(CRemoteFileDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
      COMMAND_ID_HANDLER(IDC_UP, OnUp)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_HANDLER(IDC_FOLDER, CBN_SELCHANGE, OnFolderChanged)
      COMMAND_HANDLER(IDC_FILENAME, EN_CHANGE, OnFilenameChanged)
      COMMAND_HANDLER(IDC_TYPES, CBN_SELCHANGE, OnTypeChanged)
      NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
      NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnItemOpen)
      CHAIN_MSG_MAP( CDialogResize<CRemoteFileDlg> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFolderChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFilenameChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnTypeChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnItemOpen(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

   // Implementation

   bool _PopulateView(LPCTSTR pstrPath);
   bool _AddShellIcon(CImageListHandle& iml, LPCTSTR pstrExtension, DWORD dwFileAttribs, DWORD dwMoreFlags = 0) const;
   bool _GetSelectedFilters(int iIndex, CSimpleArray<CString>& aFilters) const;
   bool _GetSelectedFiles(CSimpleArray<CString>& aFiles) const;
   void _UpdateButtons();

   static int CALLBACK _ListSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
};


#endif // !defined(AFX_FTPFILEDLG_H__20030315_311D_8B14_24C9_0080AD509054__INCLUDED_)

