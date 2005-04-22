#if !defined(AFX_ARGUMENTSDLG_H__20050326_347B_78FA_7317_0080AD509054__INCLUDED_)
#define AFX_ARGUMENTSDLG_H__20050326_347B_78FA_7317_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



class CRunArgumentsDlg : 
   public CDialogImpl<CRunArgumentsDlg>
{
public:
   enum { IDD = IDD_RUNARGUMENTS };

   CRemoteProject* m_pProject;

   void Init(CRemoteProject* pProject)
   {
      m_pProject = pProject;
   }

   BEGIN_MSG_MAP(CRunArgumentsDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnOk)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnChange);
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ATLASSERT(m_pProject);
      CenterWindow();
      CWindow(GetDlgItem(IDC_ARGUMENTS)).SetWindowText(m_pProject->m_DebugManager.GetParam(_T("AppArgs")));
      CWindow(GetDlgItem(IDOK)).EnableWindow(FALSE);
      return FALSE;
   }
   LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {      
      m_pProject->m_DebugManager.SetParam(_T("AppArgs"), CWindowText(GetDlgItem(IDC_ARGUMENTS)));
      EndDialog(wID);
      return 0;
   }
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CWindow(GetDlgItem(IDOK)).EnableWindow(TRUE);
      return 0;
   }
};


#endif // !defined(AFX_ARGUMENTSDLG_H__20050326_347B_78FA_7317_0080AD509054__INCLUDED_)

