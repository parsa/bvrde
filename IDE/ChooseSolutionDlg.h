#if !defined(AFX_CHOOSESOLUTIONDLG_H__20030227_BBF5_4046_F3FF_0080AD509054__INCLUDED_)
#define AFX_CHOOSESOLUTIONDLG_H__20030227_BBF5_4046_F3FF_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CChooseSolutionDlg : public CDialogImpl<CChooseSolutionDlg>
{
public:
   enum { IDD = IDD_CHOOSESOLUTION };

   CButton m_ctrlBlankSolution;
   CButton m_ctrlWizard;
   CButton m_ctrlOpen;
   CListViewCtrl m_ctrlList;
   CImageListCtrl m_Images;
   CSimpleArray<CString> m_aFiles;

   int m_iType;
   CString m_sFilename;

   BEGIN_MSG_MAP(CChooseSolutionDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnChanged)
      NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
      NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnItemOpen)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_iType = 0;
      m_sFilename = "";

      m_ctrlBlankSolution = GetDlgItem(IDC_RADIO1);
      m_ctrlWizard = GetDlgItem(IDC_RADIO2);
      m_ctrlOpen = GetDlgItem(IDC_RADIO3);
      
      m_ctrlList = GetDlgItem(IDC_LIST);
      m_Images.Create(IDB_PROJECTS, 32, 64, RGB(0,128,128));
      m_ctrlList.SetImageList(m_Images, LVSIL_NORMAL);
      _FillList();

      if( m_ctrlList.GetItemCount() > 0 ) {
         m_ctrlOpen.Click();
      }
      else {
         m_ctrlWizard.Click();
      }

      CenterWindow(GetParent());

      return TRUE;
   }
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      if( m_ctrlBlankSolution.GetCheck() == BST_CHECKED ) m_iType = 0;
      else if( m_ctrlWizard.GetCheck() == BST_CHECKED ) m_iType = 1;
      else if( m_ctrlOpen.GetCheck() == BST_CHECKED ) {
         int iIndex = m_ctrlList.GetSelectedIndex();
         if( iIndex == -1 ) return 0;
         m_iType = 2;
         m_sFilename = m_aFiles[ m_ctrlList.GetItemData(iIndex) ];
      }
      EndDialog(wID);
      return 0;
   }
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      BOOL bEnable = TRUE;
      if( m_ctrlOpen.GetCheck() == BST_CHECKED && m_ctrlList.GetSelectedCount() == 0 ) bEnable = FALSE;
      CWindow(GetDlgItem(IDOK)).EnableWindow(bEnable);
      return 0;
   }
   LRESULT OnItemOpen(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      m_ctrlOpen.Click();
      BOOL bDummy;
      OnOK(0, IDOK, NULL, bDummy);
      return 0;
   }
   LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      m_ctrlOpen.Click();
      BOOL bDummy;
      OnChanged(0, 0, NULL, bDummy);
      return 0;
   }

   // Implementation

   void _FillList()
   {
      m_ctrlList.DeleteAllItems();
      m_aFiles.RemoveAll();

      CString sPattern;
      sPattern.Format(_T("%s%s\\*.sln"), CModulePath(), _T(SOLUTIONDIR));
      CFindFile ff;
      for( BOOL bRes = ff.FindFile(sPattern); bRes; bRes = ff.FindNextFile() ) {
         if( ff.IsDots() ) continue;
         if( ff.IsDirectory() ) continue;
         CString sIcon;
         sIcon.Format(_T("%sProjects\\%s.ico"), CModulePath(), ff.GetFileTitle());
         CIcon icon;
         icon.LoadIcon( (LPCTSTR) sIcon );
         int iImage = 0;
         if( !icon.IsNull() ) iImage = m_Images.AddIcon(icon);
         int iIndex = m_ctrlList.InsertItem(0, ff.GetFileTitle(), iImage);
         m_ctrlList.SetItemData(iIndex, m_aFiles.GetSize());
         m_aFiles.Add(ff.GetFilePath());
      }
      ff.Close();
      m_ctrlList.SelectItem(0);
      BOOL bDummy;
      OnChanged(0, 0, NULL, bDummy);
   }
};


#endif // !defined(AFX_CHOOSESOLUTIONDLG_H__20030227_BBF5_4046_F3FF_0080AD509054__INCLUDED_)

