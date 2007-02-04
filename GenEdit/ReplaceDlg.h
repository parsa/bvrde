#if !defined(AFX_REPLACEDLG_H__20030315_B179_BB41_9076_0080AD509054__INCLUDED_)
#define AFX_REPLACEDLG_H__20030315_B179_BB41_9076_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MruCombo.h"


class CReplaceDlg : 
   public CDialogImpl<CReplaceDlg>
{
public:
   enum { IDD = IDD_REPLACE };

   IDevEnv* m_pDevEnv;
   FINDREPLACEA& m_fr;
   CScintillaCtrl m_ctrlScintilla;
   CMruComboCtrl m_ctrlFindText;
   CMruComboCtrl m_ctrlReplaceText;
   CButton m_ctrlMatchWholeWord;
   CButton m_ctrlMatchCase;
   CButton m_ctrlRegExp;
   CButton m_ctrlWrap;
   CButton m_ctrlInSelection;
   CButton m_ctrlWholeFile;

   CReplaceDlg(IDevEnv* pDevEnv, HWND hWndScintilla, FINDREPLACEA& fr);

   BEGIN_MSG_MAP(CReplaceDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      COMMAND_CODE_HANDLER(CBN_EDITUPDATE, OnChange)
      COMMAND_CODE_HANDLER(CBN_CLOSEUP, OnChange)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDC_REPLACE, OnOK)
      COMMAND_ID_HANDLER(IDC_REPLACEALL, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   void _UpdateButtons();
};


#endif // !defined(AFX_REPLACEDLG_H__20030315_B179_BB41_9076_0080AD509054__INCLUDED_)
