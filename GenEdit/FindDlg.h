#if !defined(AFX_FINDDLG_H__20030315_6106_2356_E402_0080AD509054__INCLUDED_)
#define AFX_FINDDLG_H__20030315_6106_2356_E402_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MruCombo.h"


class CFindDlg : 
   public CDialogImpl<CFindDlg>
{
public:
   enum { IDD = IDD_FIND };

   IDevEnv* m_pDevEnv;
   FINDREPLACEA& m_fr;
   CScintillaCtrl m_ctrlScintilla;
   CMruComboCtrl m_ctrlFindText;
   CButton m_ctrlUp;
   CButton m_ctrlDown;
   CButton m_ctrlMatchWholeWord;
   CButton m_ctrlMatchCase;
   CButton m_ctrlRegExp;
   CButton m_ctrlWrap;

   CFindDlg(IDevEnv* /*pDevEnv*/, HWND /*hWndScintilla*/, FINDREPLACEA& /*fr*/);

   BEGIN_MSG_MAP(CFindDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      COMMAND_CODE_HANDLER(CBN_EDITCHANGE, OnChange)
      COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_ID_HANDLER(IDC_MARKALL, OnOK)   
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

   void _UpdateButtons();
};


#endif // !defined(AFX_FINDDLG_H__20030315_6106_2356_E402_0080AD509054__INCLUDED_)
