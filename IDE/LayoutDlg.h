#if !defined(AFX_LAYOUTDLG_H__20030525_3763_0D86_EDA6_0080AD509054__INCLUDED_)
#define AFX_LAYOUTDLG_H__20030525_3763_0D86_EDA6_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CLayoutDlg :
   public CDialogImpl<CLayoutDlg>
{
public:
   enum { IDD = IDD_LAYOUT };

   CMainFrame* m_pMainFrame;
   HWND m_hWndSheet;
   CCheckTreeViewCtrl m_ctrlTree;
   CButton m_ctrlLarge;

   void Init(HWND hWndSheet, CMainFrame* pMainFrame)
   {
      m_hWndSheet = hWndSheet;
      m_pMainFrame = pMainFrame;
   }

   BEGIN_MSG_MAP(CLayoutDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnChanged)
      NOTIFY_CODE_HANDLER(TVN_ITEMCHECKED, OnItemChanged)
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlTree.SubclassWindow(GetDlgItem(IDC_LIST));
      for( int i = 0; i < m_pMainFrame->m_aToolBars.GetSize(); i++ ) {
         CMainFrame::TOOLBAR& tb = m_pMainFrame->m_aToolBars[i];
         HTREEITEM hItem = m_ctrlTree.InsertItem(tb.szName, 0, TVI_ROOT);
         m_ctrlTree.SetCheckState(hItem, ::IsWindowVisible(tb.hWnd));
      }
      m_ctrlLarge = GetDlgItem(IDC_LARGE_ICONS);
      return TRUE;
   }
   LRESULT OnApply(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CReBarCtrl Rebar = m_pMainFrame->m_Rebar;
      HTREEITEM hItem = m_ctrlTree.GetRootItem();
      int iIndex = 0;
      while( hItem ) {
         if( iIndex >= m_pMainFrame->m_aToolBars.GetSize() ) break;
         CMainFrame::TOOLBAR& tbi = m_pMainFrame->m_aToolBars[iIndex];
         for( UINT i = 0; i < Rebar.GetBandCount(); i++ ) {
            REBARBANDINFO rbi = { 0 };
            rbi.cbSize = sizeof(REBARBANDINFO);
            rbi.fMask = RBBIM_CHILD;
            Rebar.GetBandInfo(i, &rbi);
            if( rbi.hwndChild == tbi.hWnd ) {              
               // Hide the toolbar now
               Rebar.ShowBand(i, m_ctrlTree.GetCheckState(hItem));
               // Recreate with large buttons?
               if( m_ctrlLarge.GetCheck() == BST_CHECKED ) {
                  CToolBarCtrl tb = tbi.hWnd;
                  tb.SetButtonSize(32, 32);
                  tb.AutoSize();
               }
               break;
            }
         }
         hItem = m_ctrlTree.GetNextSiblingItem(hItem);
         iIndex++;
      }
      m_pMainFrame->_SaveToolBarState();
      return 0;
   }
   LRESULT OnChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      ::PostMessage(m_hWndSheet, WM_USER_MODIFIED, 0, 0);
      return 0;
   }
   LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      ::PostMessage(m_hWndSheet, WM_USER_MODIFIED, 0, 0);
      return 0;
   }
};


#endif // !defined(AFX_LAYOUTDLG_H__20030525_3763_0D86_EDA6_0080AD509054__INCLUDED_)

