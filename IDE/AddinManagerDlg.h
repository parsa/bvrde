#if !defined(AFX_ADDINMANAGERDLG_H__20030525_1C1D_CA14_67C9_0080AD509054__INCLUDED_)
#define AFX_ADDINMANAGERDLG_H__20030525_1C1D_CA14_67C9_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CAddinManagerDlg : public CDialogImpl<CAddinManagerDlg>
{
public:
   enum { IDD = IDD_ADDIN_MANAGER };

   CAddinManagerDlg(CMainFrame* pMainFrame) :
      m_pMainFrame(pMainFrame)
   {
   }

   CMainFrame* m_pMainFrame;
   CImageListCtrl m_ctrlImages;
   CCheckListViewCtrl m_ctrlList;
   CStatic m_ctrlDescription;
   
   BOOL m_bChanged;

   BEGIN_MSG_MAP(CChooseSolutionDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ATLASSERT(m_pMainFrame);

      m_ctrlImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 100);

      COLORREF clrBack = BlendRGB(::GetSysColor(COLOR_WINDOW), RGB(0,0,0), 6);

      m_ctrlList.SubclassWindow(GetDlgItem(IDC_LIST));
      m_ctrlList.SetBkColor(clrBack);
      m_ctrlList.SetExtendedListViewStyle(LVS_EX_CHECKBOXES | LVS_EX_SUBITEMIMAGES);
      m_ctrlList.AddColumn(CString(MAKEINTRESOURCE(IDS_ADDIN)), 0);
      m_ctrlList.AddColumn(CString(MAKEINTRESOURCE(IDS_TYPE)), 1);
      m_ctrlList.SetImageList(m_ctrlImages, LVSIL_SMALL);

      m_ctrlDescription = GetDlgItem(IDC_DESCRIPTION);

      for( int i = 0; i < g_aPlugins.GetSize(); i++ ) {
         const CPlugin& plugin = g_aPlugins[i];
         SHFILEINFO sfi = { 0 };
         ::SHGetFileInfo(plugin.GetFilename(), 
                         FILE_ATTRIBUTE_NORMAL, 
                         &sfi, sizeof(sfi), 
                         SHGFI_ICON | SHGFI_SHELLICONSIZE | SHGFI_SMALLICON);
         m_ctrlImages.AddIcon(sfi.hIcon);
         // Create list item
         CString sText = plugin.GetName();
         long lType = plugin.GetType();
         int iItem = m_ctrlList.InsertItem(m_ctrlList.GetItemCount(), sText, -1);
         m_ctrlList.SetItemData(iItem, (LPARAM) i);
         m_ctrlList.SetCheckState(iItem, plugin.IsMarkedActive());
         // Change text and icon on the first subitem
         CString sType;
         if( (lType & PLUGIN_PROJECT) != 0 ) {
            if( !sType.IsEmpty() ) sType += _T(" / ");
            sType += CString(MAKEINTRESOURCE(IDS_TYPE_PROJECT));
         }
         if( (lType & PLUGIN_FILETYPE) != 0 ) {
            if( !sType.IsEmpty() ) sType += _T(" / ");
            sType += CString(MAKEINTRESOURCE(IDS_TYPE_FILETYPE));
         }
         if( (lType & PLUGIN_EXTENSION) != 0 ) {
            if( !sType.IsEmpty() ) sType += _T(" / ");
            sType += CString(MAKEINTRESOURCE(IDS_TYPE_EXTENSION));
         }
         m_ctrlList.SetItemText(iItem, 1, sType);
         LVITEM itm = { 0 };
         itm.iItem = iItem;
         itm.iSubItem = 1;
         itm.mask = LVIF_IMAGE;
         itm.iImage = i;
         m_ctrlList.SetItem(&itm);
      }
      int cxChar = (int) LOWORD(GetDialogBaseUnits());
      m_ctrlList.SetColumnWidth(0, cxChar * 25);
      m_ctrlList.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
      m_ctrlList.SelectItem(0);

      m_bChanged = FALSE;

      BOOL bDummy = FALSE;
      OnItemChanged(0, NULL, bDummy);

      CenterWindow();

      return TRUE;
   }
   LRESULT OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
   {
      if( lParam != (LPARAM) (HWND) m_ctrlDescription ) {
         bHandled = FALSE;
         return 0;
      }
      CDCHandle dc = (HDC) wParam;
      DefWindowProc();
      dc.SetTextColor(::GetSysColor(COLOR_3DDKSHADOW));
      dc.SetBkMode(TRANSPARENT);
      return (LRESULT) ::GetSysColorBrush(COLOR_BTNFACE);
   }
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      if( m_bChanged ) {
         // Delete registry key that holds list of items and recreate it...
         CRegKey regParent;
         if( regParent.Create(HKEY_CURRENT_USER, REG_BVRDE) == ERROR_SUCCESS ) {
            regParent.DeleteSubKey(_T("Disabled Plugins"));
            regParent.Close();
         }
         CRegKey reg;
         if( reg.Create(HKEY_CURRENT_USER, REG_BVRDE _T("\\Disabled Plugins")) == ERROR_SUCCESS ) {
            int nCount = m_ctrlList.GetItemCount();
            for( int i = 0; i < nCount; i++ ) {
               int iIndex = (int) m_ctrlList.GetItemData(i);
               BOOL bActive = m_ctrlList.GetCheckState(i);
               CPlugin& plugin = g_aPlugins[iIndex];
               plugin.ChangeActiveState(bActive);
               if( bActive ) continue;
               TCHAR szName[MAX_PATH];
               _tcscpy(szName, plugin.GetFilename());
               ::PathStripPath(szName);
               ::PathRemoveExtension(szName);
               reg.SetValue(1, szName);
            }
            reg.Close();
         }
         m_pMainFrame->_ShowMessageBox(m_hWnd, IDS_RESTART, IDS_CAPTION_MESSAGE, MB_ICONINFORMATION);
      }
      EndDialog(wID);
      return 0;
   }
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      LPNMLISTVIEW pNMLV = (LPNMLISTVIEW) pnmh;
      CString sDescription;
      int iCurSel = m_ctrlList.GetSelectedIndex();
      if( iCurSel >= 0 ) sDescription = g_aPlugins[m_ctrlList.GetItemData(iCurSel)].GetDescription();
      m_ctrlDescription.SetWindowText(sDescription);
      if( pNMLV != NULL && (pNMLV->uOldState & LVIS_STATEIMAGEMASK) != (pNMLV->uNewState & LVIS_STATEIMAGEMASK) ) m_bChanged = TRUE;
      _UpdateButtons();
      return 0;
   }

   // Implementations

   void _UpdateButtons()
   {
      CWindow(GetDlgItem(IDOK)).EnableWindow(m_bChanged);
   }
};


#endif // !defined(AFX_ADDINMANAGERDLG_H__20030525_1C1D_CA14_67C9_0080AD509054__INCLUDED_)
