#if !defined(AFX_BREAKPOINTINFODLG_H__20070707_A4F2_0792_274F_0080AD509054__INCLUDED_)
#define AFX_BREAKPOINTINFODLG_H__20070707_A4F2_0792_274F_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CBreakpointInfoDlg : 
   public CDialogImpl<CBreakpointInfoDlg>
{
public:
   enum { IDD = IDD_BREAKPOINTINFO };

   CRemoteProject* m_pProject;
   CBreakpointView::BREAKINFO m_Info;
   CFont m_fontBold;

   CButton m_ctrlIgnoreCountCheck;
   CButton m_ctrlConditionCheck;
   CEdit m_ctrlIgnoreCount;
   CEdit m_ctrlCondition;

   void Init(CRemoteProject* pProject, const CBreakpointView::BREAKINFO Info)
   {
      m_pProject = pProject;
      m_Info = Info;
   }

   BEGIN_MSG_MAP(CBreakpointInfoDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnOk)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnClicked);
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ATLASSERT(m_pProject);
      CenterWindow();
      CLogFont lf = AtlGetDefaultGuiFont();
      lf.MakeBolder();
      m_fontBold.CreateFontIndirect(&lf);
      CWindow(GetDlgItem(IDC_FRAME1)).SetFont(m_fontBold);
      CWindow(GetDlgItem(IDC_FRAME2)).SetFont(m_fontBold);
      CWindow(GetDlgItem(IDC_FRAME3)).SetFont(m_fontBold);
      m_ctrlIgnoreCountCheck = GetDlgItem(IDC_IGNORECOUNT_CHECK);
      m_ctrlIgnoreCount = GetDlgItem(IDC_IGNORECOUNT);
      m_ctrlConditionCheck = GetDlgItem(IDC_CONDITION_CHECK);
      m_ctrlCondition = GetDlgItem(IDC_CONDITION);
      SetDlgItemInt(IDC_IGNORECOUNT, m_Info.iIgnoreCount, FALSE);
      m_ctrlCondition.SetWindowText(m_Info.sCondition);
      m_ctrlIgnoreCountCheck.SetCheck(m_Info.iIgnoreCount > 0 ? BST_CHECKED : BST_UNCHECKED);
      m_ctrlConditionCheck.SetCheck(!m_Info.sCondition.IsEmpty() ? BST_CHECKED : BST_UNCHECKED);
      CString sInfo;
      CString sTemp;
      sTemp.Format(IDS_BREAKPOINTHIT, m_Info.iHitCount);
      sInfo += sTemp;
      if( m_Info.bTemporary ) {
         sTemp.LoadString(IDS_BREAKPOINTTEMP);
         sInfo += sTemp;
      }
      SetDlgItemText(IDC_INFORMATION, sInfo);
      _UpdateButtons();
      return FALSE;
   }

   LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      // Set or clear "Ignore count"
      if( m_ctrlIgnoreCountCheck.GetCheck() == BST_CHECKED ) {
         m_pProject->m_DebugManager.DoDebugCommandV(_T("-break-after %d %d"), m_Info.iBrkNr, _ttoi(CWindowText(m_ctrlIgnoreCount)));
      }
      else if( m_Info.iIgnoreCount > 0 ) {
         m_pProject->m_DebugManager.DoDebugCommandV(_T("-break-after %d %d"), m_Info.iBrkNr, 0);
      }
      // Set or clear "Break condition"
      if( m_ctrlConditionCheck.GetCheck() == BST_CHECKED ) {
         m_pProject->m_DebugManager.DoDebugCommandV(_T("-break-condition %d %s"), m_Info.iBrkNr, CWindowText(m_ctrlCondition));
      }
      else if( !m_Info.sCondition.IsEmpty() ) {
         m_pProject->m_DebugManager.DoDebugCommandV(_T("-break-condition %d"), m_Info.iBrkNr);
      }
      m_pProject->m_DebugManager.DoDebugCommand(_T("-break-list"));
      EndDialog(wID);
      return 0;
   }

   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   
   LRESULT OnClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      _UpdateButtons();
      return 0;
   }

   // Implementation

   void _UpdateButtons()
   {
      m_ctrlIgnoreCount.EnableWindow(m_ctrlIgnoreCountCheck.GetCheck() == BST_CHECKED);
      m_ctrlCondition.EnableWindow(m_ctrlConditionCheck.GetCheck() == BST_CHECKED);
   }
};


#endif // !defined(AFX_BREAKPOINTINFODLG_H__20070707_A4F2_0792_274F_0080AD509054__INCLUDED_)

