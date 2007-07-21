#if !defined(AFX_RESPOSITORYVIEW_H__20031225_8C88_2614_5BFD_0080AD509054__INCLUDED_)
#define AFX_RESPOSITORYVIEW_H__20031225_8C88_2614_5BFD_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define IDC_SFOLDERS     100
#define IDC_SFILES       101
#define WM_APP_ENUMDONE  WM_APP + 44


typedef struct tagFILEINFO
{
   CString sFilename;
   CString sStatus;
   CString sVersion;
} FILEINFO;


class CFileEnumThread : 
   public CThreadImpl<CFileEnumThread>, 
   public ILineCallback
{
public:
   HWND m_hWnd;
   long m_lTimeout;
   CString m_sCommand;
   CString m_sSourceType;
   CSimpleArray<FILEINFO> m_aResult;
   FILEINFO m_Info;

   void RunCommand(HWND hWnd, LPCTSTR pstrCommand, LONG lTimeout);

   DWORD Run();

   HRESULT _ParseCvsLine(BSTR bstr);
   HRESULT _ParseSubversionLine(BSTR bstr);

   // ILineCallback

   HRESULT STDMETHODCALLTYPE OnIncomingLine(BSTR bstr);

   // IUnknown

   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);
};


class CRepositoryView : public CWindowImpl<CRepositoryView>
{
public:
   CImageListCtrl m_FolderImages;
   CImageListCtrl m_FileImages;
   CTreeViewCtrl m_ctrlFolders;
   CListViewCtrl m_ctrlFiles;
   CStatic m_ctrlBuilding;
   CFileEnumThread m_thread;
   CString m_sSelPath;
   COLORREF m_clrWarning;

   BEGIN_MSG_MAP(CRepositoryView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_APP_ENUMDONE, OnFileEnumDone)
      MESSAGE_HANDLER(WM_COMPACTING, OnViewOpens)
      MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
      NOTIFY_HANDLER(IDC_SFOLDERS, TVN_SELCHANGED, OnSelChanged)
      NOTIFY_HANDLER(IDC_SFOLDERS, TVN_ITEMEXPANDED, OnItemExpanded)      
      NOTIFY_HANDLER(IDC_SFILES, LVN_KEYDOWN, OnListKeyDown)
      NOTIFY_HANDLER(IDC_SFILES, LVN_ITEMCHANGED, OnListSelected)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnViewOpens(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnFileEnumDone(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSelChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnItemExpanded(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnListKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnListSelected(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

   // Implementation

   CString _GetItemPath(HTREEITEM hItem) const;
	void _ShowWaitingMessage(UINT nRes, COLORREF clrText);
   HTREEITEM _FindItemInTree(HTREEITEM hItem, LPCTSTR pstrName) const;
   bool _AddShellIcon(CImageListHandle& iml, LPCTSTR pstrExtension, DWORD dwFileAttribs, DWORD dwMoreFlags = 0) const;
};


#endif // !defined(AFX_RESPOSITORYVIEW_H__20031225_8C88_2614_5BFD_0080AD509054__INCLUDED_)
