#if !defined(AFX_OPENFILESVIEW_H__20070208_CDC1_8925_AE96_0080AD509054__INCLUDED_)
#define AFX_OPENFILESVIEW_H__20070208_CDC1_8925_AE96_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class COpenFilesView : 
   public CWindowImpl<COpenFilesView>,
   public IViewMessageListener
{
public:
   DECLARE_WND_CLASS_EX(_T("BVRDE_OpenFiles"), 0, COLOR_WINDOW)

   CListViewCtrl m_ctrlList;
   CImageListCtrl m_Images;

   enum 
   { 
      WM_USER_VIEW_CREATE = WM_USER + 4,
      WM_USER_VIEW_DESTROY,
   };

   BEGIN_MSG_MAP(COpenFilesView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
      MESSAGE_HANDLER(WM_USER_VIEW_CREATE, OnViewCreated)
      MESSAGE_HANDLER(WM_USER_VIEW_DESTROY, OnViewDestroyed)
      NOTIFY_HANDLER(IDC_LIST, NM_RCLICK, OnRClick)
      NOTIFY_HANDLER(IDC_LIST, LVN_ITEMACTIVATE, OnItemDblClick) 
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_Images.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 100);

      DWORD dwStyle = WS_BORDER | WS_CHILD | WS_VISIBLE | LVS_SINGLESEL | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SORTASCENDING;
      m_ctrlList.Create(m_hWnd, rcDefault, NULL, dwStyle, 0, IDC_LIST);
      m_ctrlList.SetExtendedListViewStyle(LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_UNDERLINEHOT);
      m_ctrlList.AddColumn(_T(""), 0);
      m_ctrlList.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      m_ctrlList.SetImageList(m_Images, LVSIL_SMALL);

      g_pDevEnv->AddViewListener(this);
      return 0;
   }
   LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      g_pDevEnv->RemoveViewListener(this);
      bHandled = FALSE;
      return 0;
   }
   LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      return TRUE; // Children fills entire client area
   }
   LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      if( !m_ctrlList.IsWindow() ) return 0;
      RECT rc = { 0 };
      GetClientRect(&rc);
      m_ctrlList.MoveWindow(&rc);
      m_ctrlList.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      return 0;
   }
   LRESULT OnItemDblClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
   {
      int iCurSel = m_ctrlList.GetSelectedIndex();
      if( iCurSel < 0 ) return 0;
      IView* pView = (IView*) m_ctrlList.GetItemData(iCurSel);
      ATLTRY( pView->OpenView(0) );
      return 0;
   }
   LRESULT OnRClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
   {
      return 0;
   }
   LRESULT OnViewCreated(UINT uMsg, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      CWindow wndFrame = (HWND) lParam;
      if( !wndFrame.IsWindow() ) return 0;
      CWinProp prop = wndFrame;
      IView* pView = NULL;
      prop.GetProperty(_T("View"), pView);
      if( pView == NULL ) return 0;
      // BUG: Cleanup
      int nItems = m_ctrlList.GetItemCount();
      while( nItems == 0 && m_Images.GetImageCount() > 0 ) m_Images.Remove(0);
      if( m_Images.GetImageCount() > 300 ) m_Images.Remove(0);
      // Capture icon...
      HICON hIcon = wndFrame.GetIcon(FALSE);
      if( hIcon == NULL ) hIcon = (HICON) ::GetClassLong(wndFrame, GCL_HICONSM);
      if( hIcon == NULL ) hIcon = (HICON) ::GetClassLong(wndFrame, GCL_HICON);
      int iImage = m_Images.AddIcon(hIcon);
      // Create list item
      TCHAR szName[200] = { 0 };
      pView->GetName(szName, (sizeof(szName) / sizeof(TCHAR)) - 1);
      int iItem = m_ctrlList.InsertItem(m_ctrlList.GetItemCount(), szName, iImage);
      m_ctrlList.SetItemData(iItem, (DWORD_PTR) pView);
      return 0;
   }
   LRESULT OnViewDestroyed(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      int nCount = m_ctrlList.GetItemCount();
      for( int i = nCount - 1; i >= 0; i-- ) {
         if( m_ctrlList.GetItemData(i) == (DWORD_PTR) lParam ) m_ctrlList.DeleteItem(i);
      }
      return 0;
   }

   // IViewMessageListener

   LRESULT OnViewMessage(IView* pView, MSG* pMsg, BOOL& bHandled)
   {
      // Delay the update of our list, since the view may need
      // a little time to update the new MDIChild icon.
      bHandled = FALSE;
      if( !IsWindow() ) return 0;
      if( pMsg->message == WM_CREATE ) PostMessage(WM_USER_VIEW_CREATE, 0, (LPARAM) pMsg->hwnd);
      if( pMsg->message == WM_CLOSE ) PostMessage(WM_USER_VIEW_DESTROY, 0, (LPARAM) pView);
      return 0;
   }
};


#endif // !defined(AFX_OPENFILESVIEW_H__20070208_CDC1_8925_AE96_0080AD509054__INCLUDED_)

