#if !defined(AFX_CHOOSESOLUTIONDLG_H__20030227_BBF5_4046_F3FF_0080AD509054__INCLUDED_)
#define AFX_CHOOSESOLUTIONDLG_H__20030227_BBF5_4046_F3FF_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CChooseSolutionDlg : public CDialogImpl<CChooseSolutionDlg>
{
public:
   enum { IDD = IDD_CHOOSESOLUTION };

   CMainFrame* m_pMainFrame;
   CButton m_ctrlBlankSolution;
   CButton m_ctrlWizard;
   CButton m_ctrlOpen;
   CListViewCtrl m_ctrlList;
   CImageListCtrl m_LargeImages;
   CImageListCtrl m_SmallImages;
   CSimpleArray<CString> m_aFiles;

   enum
   {
      SOLUTION_BLANK = 0,
      SOLUTION_WIZARD,
      SOLUTION_FILE,
   } m_SelectType;
   CString m_sFilename;

   CChooseSolutionDlg()
   {
      m_pMainFrame = NULL;
      m_sFilename.Empty();
      m_SelectType = SOLUTION_BLANK;
   }
   void Init(CMainFrame* pMainFrame)
   {
      m_pMainFrame = pMainFrame;
   }

   BEGIN_MSG_MAP(CChooseSolutionDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_ID_HANDLER(IDC_BROWSE, OnBrowse)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnChanged)
      NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
      NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnItemOpen)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlBlankSolution = GetDlgItem(IDC_RADIO1);
      m_ctrlWizard = GetDlgItem(IDC_RADIO2);
      m_ctrlOpen = GetDlgItem(IDC_RADIO3);
      
      m_LargeImages.Create(IDB_PROJECTS32, 32, 100, RGB(0,128,128));
      m_SmallImages.Create(IDB_PROJECTS16, 16, 100, RGB(0,128,128));

      m_ctrlList = GetDlgItem(IDC_LIST);
      m_ctrlList.AddColumn(_T("XXX"), 0);
      m_ctrlList.SetImageList(m_LargeImages, LVSIL_NORMAL);
      m_ctrlList.SetImageList(m_SmallImages, LVSIL_SMALL);

      _FillList();

      if( m_ctrlList.GetItemCount() > 0 ) m_ctrlOpen.Click();
      else m_ctrlWizard.Click();

      CenterWindow(GetParent());

      return TRUE;
   }
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      if( m_ctrlBlankSolution.GetCheck() == BST_CHECKED ) m_SelectType = SOLUTION_BLANK;
      else if( m_ctrlWizard.GetCheck() == BST_CHECKED ) m_SelectType = SOLUTION_WIZARD;
      else if( m_ctrlOpen.GetCheck() == BST_CHECKED ) {
         int iIndex = m_ctrlList.GetSelectedIndex();
         if( iIndex == -1 ) return 0;
         m_SelectType = SOLUTION_FILE;
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
   LRESULT OnBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CString sFilter(MAKEINTRESOURCE(IDS_FILTER_SOLUTION));
      for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == _T('|') ) sFilter.SetAt(i, _T('\0'));
      CFileDialog dlg(TRUE, _T("sln"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, sFilter);
      dlg.m_ofn.Flags &= ~OFN_ENABLEHOOK;
      if( dlg.DoModal() != IDOK ) return 0;
      m_SelectType = SOLUTION_FILE;
      m_sFilename = dlg.m_ofn.lpstrFile;
      EndDialog(IDOK);
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
      if( m_ctrlOpen.GetCheck() != BST_CHECKED ) m_ctrlOpen.Click();
      PostMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0));
      return 0;
   }
   LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      if( m_ctrlOpen.GetCheck() != BST_CHECKED ) m_ctrlOpen.Click();
      SendMessage(WM_COMMAND, MAKEWPARAM(0, BN_CLICKED));
      return 0;
   }

   // Implementation

   void _FillList()
   {
      ATLASSERT(m_pMainFrame);

      m_ctrlList.DeleteAllItems();
      m_aFiles.RemoveAll();

      // Include all the local projects
      CString sPattern;
      sPattern.Format(_T("%s%s\\*.sln"), CModulePath(), _T(SOLUTIONDIR));
      CFindFile ff;
      for( BOOL bRes = ff.FindFile(sPattern); bRes; bRes = ff.FindNextFile() ) {
         if( ff.IsDots() ) continue;
         if( ff.IsDirectory() ) continue;
         _AddListItem(ff.GetFilePath());
      }
      ff.Close();
      // ... then add file history as well
      for( int i = 0; i < m_pMainFrame->m_mru.m_arrDocs.GetSize(); i++ ) {
         CString sFilename = m_pMainFrame->m_mru.m_arrDocs[i].szDocName;
         // We compare the projects by filename (not including path)
         // to make sure we only add history items once
         TCHAR szFile1[MAX_PATH];
         _tcscpy(szFile1, sFilename);
         ::PathStripPath(szFile1);
         bool bFound = false;
         for( int j = 0; !bFound && j < m_aFiles.GetSize(); j++ ) {
            TCHAR szFile2[MAX_PATH];            
            _tcscpy(szFile2, m_aFiles[j]);
            ::PathStripPath(szFile2);
            if( _tcsicmp(szFile1, szFile2) == 0 ) bFound = true;
         }
         if( !bFound ) _AddListItem(sFilename);
      }
      // Should we display in icon view instead
      const int MAX_FILES_IN_ICONVIEW = 9;
      if( m_ctrlList.GetItemCount() > MAX_FILES_IN_ICONVIEW ) m_ctrlList.SetViewType(LVS_REPORT);
      m_ctrlList.SetColumnWidth(0, LVSCW_AUTOSIZE);
      // Select first item
      m_ctrlList.SelectItem(0);
      BOOL bDummy;
      OnChanged(0, 0, NULL, bDummy);
   }

   void _AddListItem(LPCTSTR pstrFilename)
   {
      TCHAR szName[MAX_PATH];
      _tcscpy(szName, pstrFilename);
      ::PathStripPath(szName);
      ::PathRemoveExtension(szName);
      TCHAR szIconName[MAX_PATH];
      _tcscpy(szIconName, pstrFilename);
      ::PathRenameExtension(szIconName, _T(".ico"));
      CIcon icon;
      icon.LoadIcon(szIconName);
      int iImage = 0;
      if( !icon.IsNull() ) {
         iImage = m_LargeImages.AddIcon(icon);
         m_SmallImages.AddIcon(icon);
      }
      int iIndex = m_ctrlList.InsertItem(0, szName, iImage);
      m_ctrlList.SetItemData(iIndex, m_aFiles.GetSize());
      CString sFilename = pstrFilename;
      m_aFiles.Add(sFilename);
   }
};


#endif // !defined(AFX_CHOOSESOLUTIONDLG_H__20030227_BBF5_4046_F3FF_0080AD509054__INCLUDED_)

