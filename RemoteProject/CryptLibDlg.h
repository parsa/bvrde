#if !defined(AFX_CRYPTLIBDLG_H__20030717_0D20_C30B_18E4_0080AD509054__INCLUDED_)
#define AFX_CRYPTLIBDLG_H__20030717_0D20_C30B_18E4_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CCryptLibDlg : 
   public CDialogImpl<CCryptLibDlg>
{
public:
   enum { IDD = IDD_CRYPTLIB };

   CFont m_font;
   CStatic m_ctrlTitle;
   CHyperLink m_ctrlLink;

   BEGIN_MSG_MAP(CCryptLibDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
      COMMAND_ID_HANDLER(IDOK, OnClose)
      COMMAND_ID_HANDLER(IDCANCEL, OnClose)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CLogFont lf = GetFont();
      lf.MakeBolder();
      lf.MakeLarger(6);
      m_font.CreateFontIndirect(&lf);
      m_ctrlTitle = GetDlgItem(IDC_TITLE);
      m_ctrlTitle.SetFont(m_font);
      m_ctrlLink.SubclassWindow(GetDlgItem(IDC_LINK));
      return TRUE;
   }
   LRESULT OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
   {
      if( (HWND) lParam != m_ctrlTitle ) {
         bHandled = FALSE;
         return 0;
      }
      CDCHandle dc( (HDC) wParam );
      dc.SetBkMode(TRANSPARENT);
      return (LRESULT) (HBRUSH) AtlGetStockBrush(WHITE_BRUSH);
   }
   LRESULT OnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
};


#endif // !defined(AFX_CRYPTLIBDLG_H__20030717_0D20_C30B_18E4_0080AD509054__INCLUDED_)

