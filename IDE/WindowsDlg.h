#if !defined(AFX_WINDOWSDLG_H__20030521_DD2F_92DB_E973_0080AD509054__INCLUDED_)
#define AFX_WINDOWSDLG_H__20030521_DD2F_92DB_E973_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CWindowsDlg : public CDialogImpl<CWindowsDlg>
{
public:
   enum { IDD = IDD_WINDOWS };

   CWindowsDlg(HWND hWnd, HWND hWndClient)
   {
      m_wndMDI = hWnd;
      m_wndMDI.m_hWndMDIClient = hWndClient;
   }

   CListViewCtrl m_ctrlList;
   CImageListCtrl m_ctrlImages;
   CMDIWindow m_wndMDI;

   BEGIN_MSG_MAP(CChooseSolutionDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_ID_HANDLER(IDC_ACTIVATE, OnActivate)
      COMMAND_ID_HANDLER(IDC_CLOSE, OnItemClose)
      NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnItemOpen)
      NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ATLASSERT(m_wndMDI.IsWindow());

      m_ctrlImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 100);

      m_ctrlList = GetDlgItem(IDC_LIST);
      m_ctrlList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
      m_ctrlList.AddColumn(_T(""), 0);
      m_ctrlList.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      m_ctrlList.SetImageList(m_ctrlImages, LVSIL_SMALL);

      HWND hWnd = ::GetWindow(m_wndMDI.m_hWndMDIClient, GW_CHILD);
      int iImage = 0;
      while( ::IsWindow(hWnd) ) {
         // Create icon
         HICON hIcon = (HICON) ::GetClassLong(hWnd, GCL_HICONSM);
         if( hIcon == NULL ) hIcon = (HICON) ::GetClassLong(hWnd, GCL_HICON);
         m_ctrlImages.AddIcon(hIcon);
         // Create list item
         CWindowText sText = hWnd;
         int iItem = m_ctrlList.InsertItem(m_ctrlList.GetItemCount(), sText, iImage);
         m_ctrlList.SetItemData(iItem, (LPARAM) hWnd);
         hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
         iImage++;
      }

      _UpdateButtons();

      CenterWindow();
      return 0;
   }
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnActivate(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      int iItem = m_ctrlList.GetNextItem(-1, LVIS_SELECTED);
      if( iItem != -1 ) m_wndMDI.MDIActivate( (HWND) m_ctrlList.GetItemData(iItem) );
      EndDialog(wID);
      return 0;
   }
   LRESULT OnItemClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      int iItem = m_ctrlList.GetNextItem(-1, LVIS_SELECTED);
      while( iItem != -1 ) {
         ::PostMessage( (HWND) m_ctrlList.GetItemData(iItem), WM_CLOSE, 0, 0L );
         iItem = m_ctrlList.GetNextItem(iItem, LVIS_SELECTED);
      }
      iItem = m_ctrlList.GetNextItem(-1, LVIS_SELECTED);
      while( iItem != -1 ) {
         m_ctrlList.DeleteItem(iItem);
         iItem = m_ctrlList.GetNextItem(-1, LVIS_SELECTED);
      }
      _UpdateButtons();
      return 0;
   }
   LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      _UpdateButtons();
      return 0;
   }
   LRESULT OnItemOpen(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      PostMessage(WM_COMMAND, MAKEWPARAM(IDC_ACTIVATE, 0));
      return 0;
   }

   // Implementations

   void _UpdateButtons()
   {
      CWindow(GetDlgItem(IDC_ACTIVATE)).EnableWindow(m_ctrlList.GetSelectedCount()==1);
      CWindow(GetDlgItem(IDC_CLOSE)).EnableWindow(m_ctrlList.GetSelectedCount()>0);
   }
};


#endif // !defined(AFX_WINDOWSDLG_H__20030521_DD2F_92DB_E973_0080AD509054__INCLUDED_)

