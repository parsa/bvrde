#if !defined(AFX_ARGUMENTPROMPTDLG_H__20050306_232D_F4E4_2759_0080AD509054__INCLUDED_)
#define AFX_ARGUMENTPROMPTDLG_H__20050306_232D_F4E4_2759_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CArgumentPromptDlg : 
   public CDialogImpl<CArgumentPromptDlg>
{
public:
   enum { IDD = IDD_ARGUMENTPROMPT };

   CEdit m_ctrlCommand;
   CEdit m_ctrlArguments;

   CString m_sCommand;
   CString m_sArguments;

   BEGIN_MSG_MAP(CArgumentPromptDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CenterWindow();
      
      m_ctrlCommand = GetDlgItem(IDC_COMMAND);
      m_ctrlArguments = GetDlgItem(IDC_ARGUMENTS);

      m_ctrlCommand.SetWindowText(m_sCommand);
      m_ctrlArguments.SetWindowText(m_sArguments);

      m_ctrlArguments.SetFocus();
      m_ctrlArguments.SetSel(0, 0);
      int iPos = m_sArguments.Find('?');
      if( iPos > 0 ) m_ctrlArguments.SetSel(iPos, iPos + 1);
      
      return FALSE;
   }

   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      m_sCommand = CWindowText(m_ctrlCommand);
      m_sArguments = CWindowText(m_ctrlArguments);
      EndDialog(wID);
      return 0;
   }
   
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
};


#endif // !defined(AFX_ARGUMENTPROMPTDLG_H__20050306_232D_F4E4_2759_0080AD509054__INCLUDED_)

