#if !defined(AFX_GOTODLG_H__20030315_6B4B_5887_4870_0080AD509054__INCLUDED_)
#define AFX_GOTODLG_H__20030315_6B4B_5887_4870_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CGotoDlg : 
   public CDialogImpl<CGotoDlg>
{
public:
   enum { IDD = IDD_GOTO };

   CEdit m_ctrlLine;
   CScintillaCtrl m_ctrlScintilla;

   CGotoDlg(HWND hWndScintilla);

   BEGIN_MSG_MAP(CGotoDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

   void _UpdateButtons();
};


#endif // !defined(AFX_GOTODLG_H__20030315_6B4B_5887_4870_0080AD509054__INCLUDED_)

