#if !defined(AFX_ADDFILEDLG_H__20040612_D1C3_051F_A2A8_0080AD509054__INCLUDED_)
#define AFX_ADDFILEDLG_H__20040612_D1C3_051F_A2A8_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "Commands.h"


class CAddFileDlg : public CDialogImpl<CAddFileDlg>
{
public:
   enum { IDD = IDD_ADDFILE };

   // Data Members

   CEdit m_ctrlComment;
   CEdit m_ctrlFiles;
   //
   CScCommands* m_pOwner;
   CSimpleArray<CString>* m_pFiles;
   //
   static TCHAR m_szComment[200];
   //
   CString m_sOptions;

   // Operations

   // Message map and handlers

   BEGIN_MSG_MAP(CAddFileDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlFiles = GetDlgItem(IDC_FILES);
      m_ctrlComment = GetDlgItem(IDC_COMMENT);

      CString sFiles;
      for( int i = 0; i < m_pFiles->GetSize(); i++ ) {
         sFiles += _T(" ");
         sFiles += (*m_pFiles)[i];
      }
      m_ctrlFiles.SetWindowText(sFiles);
      m_ctrlComment.SetWindowText(m_szComment);

      CWindow(GetDlgItem(IDOK)).EnableWindow(m_ctrlComment.GetWindowTextLength() > 0);
      return 0;
   }
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CString sComment = CWindowText(m_ctrlComment);
      sComment.Replace(_T("\""), _T("'"));

      CString sTemp;
      m_sOptions.Empty();
      sTemp.Format(_T(" %s \"%s\""), m_pOwner->sOptMessage, sComment);
      m_sOptions += sTemp;

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

__declspec(selectany) TCHAR CAddFileDlg::m_szComment[200] = { 0 };


#endif // !defined(AFX_ADDFILEDLG_H__20040612_D1C3_051F_A2A8_0080AD509054__INCLUDED_)

