#if !defined(AFX_UPDATEDLG_H__20040612_7B7A_A13E_127F_0080AD509054__INCLUDED_)
#define AFX_UPDATEDLG_H__20040612_7B7A_A13E_127F_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Commands.h"


class CUpdateDlg : public CDialogImpl<CUpdateDlg>
{
public:
   enum { IDD = IDD_UPDATE };

   // Data Members

   CEdit m_ctrlBranchText;
   CEdit m_ctrlFiles;
   CButton m_ctrlBranch;
   CButton m_ctrlUpdateDirs;
   CButton m_ctrlResetSticky;
   //
   CScCommands* m_pOwner;
   CSimpleArray<CString>* m_pFiles;
   //
   CString m_sOptions;

   // Operations

   // Message map and handlers

   BEGIN_MSG_MAP(CUpdateDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlFiles = GetDlgItem(IDC_FILES);
      m_ctrlUpdateDirs = GetDlgItem(IDC_DIRS);
      m_ctrlResetSticky = GetDlgItem(IDC_RESETSTICKS);
      m_ctrlBranch = GetDlgItem(IDC_BRANCH);
      m_ctrlBranchText = GetDlgItem(IDC_BRANCHTEXT);

      CString sFiles;
      for( int i = 0; i < m_pFiles->GetSize(); i++ ) {
         sFiles += _T(" ");
         sFiles += (*m_pFiles)[i];
      }
      m_ctrlFiles.SetWindowText(sFiles);

      BOOL bDummy;
      OnChange(0, 0, NULL, bDummy);

      return 0;
   }
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      bool bBranch = m_ctrlBranch.GetCheck() == BST_CHECKED;
      CString sBranchText = CWindowText(m_ctrlBranchText);
      sBranchText.Replace(_T("\""), _T("'"));
      bool bResetSticky = m_ctrlResetSticky.GetCheck() == BST_CHECKED;
      bool bUpdateDirs = m_ctrlUpdateDirs.GetCheck() == BST_CHECKED;

      CString sTemp;
      m_sOptions.Empty();
      sTemp.Format(_T(" %s \"%s\""), m_pOwner->sOptMessage, sBranchText);
      m_sOptions += sTemp;
      if( bResetSticky ) m_sOptions += m_pOwner->sOptStickyTag;
      if( bUpdateDirs ) m_sOptions += m_pOwner->sOptUpdateDirs;
      if( bBranch ) {
         sTemp.Format(_T("%s %s"), m_pOwner->sOptBranch, sBranchText);
         m_sOptions += sTemp;
      }

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
      m_ctrlBranchText.EnableWindow(m_ctrlBranch.GetCheck() == BST_CHECKED);
      return 0;
   }
};


#endif // !defined(AFX_UPDATEDLG_H__20040612_7B7A_A13E_127F_0080AD509054__INCLUDED_)

