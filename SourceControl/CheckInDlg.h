#if !defined(AFX_CHECKINDLG_H__20031225_6EB7_455C_FCAA_0080AD509054__INCLUDED_)
#define AFX_CHECKINDLG_H__20031225_6EB7_455C_FCAA_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Commands.h"


class CCheckInDlg : public CDialogImpl<CCheckInDlg>
{
public:
   enum { IDD = IDD_CHECKIN };

   // Data Members

   CEdit m_ctrlComment;
   CEdit m_ctrlFiles;
   CButton m_ctrlKeepCheckedOut;
   CButton m_ctrlResetSticky;
   //
   CScCommands* m_pOwner;
   CSimpleArray<CString>* m_pFiles;
   //
   static TCHAR m_szComment[200];
   static bool m_bKeepCheckedOut;
   static bool m_bResetSticky;
   //
   CString m_sOptions;

   // Operations

   // Message map and handlers

   BEGIN_MSG_MAP(CCheckInDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CenterWindow();

      m_ctrlFiles = GetDlgItem(IDC_FILES);
      m_ctrlComment = GetDlgItem(IDC_COMMENT);
      m_ctrlKeepCheckedOut = GetDlgItem(IDC_KEEPOUT);
      m_ctrlResetSticky = GetDlgItem(IDC_RESETSTICKS);

      CString sFiles;
      for( int i = 0; i < m_pFiles->GetSize(); i++ ) {
         sFiles += _T(" ");
         sFiles += (*m_pFiles)[i];
      }
      m_ctrlFiles.SetWindowText(sFiles);
      m_ctrlComment.SetWindowText(m_szComment);
      m_ctrlKeepCheckedOut.SetCheck(m_bKeepCheckedOut ? BST_CHECKED : BST_UNCHECKED);
      m_ctrlResetSticky.SetCheck(m_bResetSticky ? BST_CHECKED : BST_UNCHECKED);

      CWindow(GetDlgItem(IDOK)).EnableWindow(m_ctrlComment.GetWindowTextLength() > 0);
      return 0;
   }

   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CString sComment = CWindowText(m_ctrlComment);
      sComment.Replace('\"', '\'');
      // NOTE: Newlines aren't exactly supported by CVS command line but left to
      //       the shell (whatever kind that may be installed) to handle!

      CString sTemp;
      m_sOptions.Empty();
      sTemp.Format(_T(" %s \"%s\""), m_pOwner->sOptMessage, sComment);
      m_sOptions += sTemp;

      ::ZeroMemory(m_szComment, sizeof(m_szComment));
      _tcsncpy(m_szComment, sComment, (sizeof(m_szComment) / sizeof(TCHAR)) - 1);

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

__declspec(selectany) TCHAR CCheckInDlg::m_szComment[200] = { 0 };
__declspec(selectany) bool CCheckInDlg::m_bKeepCheckedOut = false;
__declspec(selectany) bool CCheckInDlg::m_bResetSticky = false;


#endif // !defined(AFX_CHECKINDLG_H__20031225_6EB7_455C_FCAA_0080AD509054__INCLUDED_)

