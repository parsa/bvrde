#if !defined(AFX_PASSWORDDLG_H__20031126_E21E_000E_2DDB_0080AD509054__INCLUDED_)
#define AFX_PASSWORDDLG_H__20031126_E21E_000E_2DDB_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CPasswordDlg : 
   public CDialogImpl<CPasswordDlg>
{
public:
   enum { IDD = IDD_PASSWORD };

   CFont m_font;
   CStatic m_ctrlTitle;
   CEdit m_ctrlPassword;
   CString m_sPassword;

   CString GetPassword() const
   {
      return m_sPassword;
   }

   BEGIN_MSG_MAP(CPasswordDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
      COMMAND_ID_HANDLER(IDOK, OnClose)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnChange);
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CLogFont lf = GetFont();
      lf.MakeBolder();
      lf.MakeLarger(6);
      m_font.CreateFontIndirect(&lf);
      m_ctrlTitle = GetDlgItem(IDC_TITLE);
      m_ctrlTitle.SetFont(m_font);
      m_ctrlPassword = GetDlgItem(IDC_PASSWORD);
      m_ctrlPassword.SetLimitText(99);
      return 0;
   }
   LRESULT OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
   {
      if( (HWND) lParam != m_ctrlTitle ) {
         bHandled = FALSE;
         return 0;
      }
      CDCHandle dc( (HDC) wParam );
      dc.SetBkMode(TRANSPARENT);
      return (LRESULT) (HBRUSH) ( AtlGetStockBrush(WHITE_BRUSH) );
   }
   LRESULT OnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      m_sPassword = CWindowText(m_ctrlPassword);
      EndDialog(wID);
      return 0;
   }
   LRESULT OnChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CWindow(GetDlgItem(IDOK)).EnableWindow(m_ctrlPassword.GetWindowTextLength()>0);
      return 0;
   }
};


#endif // !defined(AFX_PASSWORDDLG_H__20031126_E21E_000E_2DDB_0080AD509054__INCLUDED_)

