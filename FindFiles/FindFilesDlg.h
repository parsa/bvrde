#if !defined(AFX_FINDFILESDLG_H__20031225_6EB7_455C_FCAA_0080AD509054__INCLUDED_)
#define AFX_FINDFILESDLG_H__20031225_6EB7_455C_FCAA_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MruCombo.h"


class CFindFilesDlg : public CDialogImpl<CFindFilesDlg>
{
public:
   enum { IDD = IDD_FINDFILES };

   CMruComboCtrl m_ctrlFindText;
   CMruComboCtrl m_ctrlFolderText;
   CButton m_ctrlMatchWholeWord;
   CButton m_ctrlMatchCase;
   CButton m_ctrlRegExp;
   CButton m_ctrlSubFolders;
   TCHAR m_szPattern[128];
   TCHAR m_szFolder[MAX_PATH];
   static UINT m_iFlags;

   // Operations

   LPCTSTR GetPattern() const
   {
      return m_szPattern;
   }
   LPCTSTR GetFolder() const
   {
      return m_szFolder;
   }
   UINT GetFlags() const
   {
      return m_iFlags;
   }

   // Message map and handlers

   BEGIN_MSG_MAP(CFindFilesDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_CODE_HANDLER(CBN_EDITCHANGE, OnChange)
      COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlFindText.SubclassWindow(GetDlgItem(IDC_FINDTEXT));
      m_ctrlFindText.LimitText(100);
      m_ctrlFindText.ReadFromRegistry(REG_BVRDE _T("\\Mru"), _T("Find"));
      m_ctrlFolderText.SubclassWindow(GetDlgItem(IDC_FOLDER));
      m_ctrlFolderText.LimitText(200);
      m_ctrlFolderText.ReadFromRegistry(REG_BVRDE _T("\\Mru"), _T("FindFolder"));

      m_ctrlRegExp = GetDlgItem(IDC_REGEXP);
      m_ctrlMatchCase = GetDlgItem(IDC_MATCHCASE);
      m_ctrlSubFolders = GetDlgItem(IDC_SUBFOLDERS);
      m_ctrlMatchWholeWord = GetDlgItem(IDC_WHOLEWORD);

      if( (m_iFlags & FR_REGEXP) != 0 ) m_ctrlRegExp.SetCheck(BST_CHECKED);
      if( (m_iFlags & FR_MATCHCASE) != 0 ) m_ctrlMatchCase.SetCheck(BST_CHECKED);
      if( (m_iFlags & FR_SUBFOLDERS) != 0 ) m_ctrlSubFolders.SetCheck(BST_CHECKED);
      if( (m_iFlags & FR_WHOLEWORD) != 0 ) m_ctrlMatchWholeWord.SetCheck(BST_CHECKED);

      // Extract start path from project settings
      // It's an optional attribute on the project's scripting (IDispatch)
      // implementation.
      ISolution* pSolution = _pDevEnv->GetSolution();
      if( pSolution ) {
         IProject* pProject = pSolution->GetActiveProject();
         if( pProject ) {
            IDispatch* pDisp = pProject->GetDispatch();
            CComDispatchDriver dd = pDisp;
            CComVariant v;
            dd.GetPropertyByName(OLESTR("CurDir"), &v);
            if( v.vt == VT_BSTR ) m_ctrlFolderText.SetWindowText(v.bstrVal);
         }
      }

      // Start with previous "Find" item selected
      m_ctrlFindText.SetCurSel(0);
      _UpdateButtons();

      return 0;
   }
   LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      m_ctrlFindText.WriteToRegistry(REG_BVRDE _T("\\Mru"), _T("Find"));
      m_ctrlFolderText.WriteToRegistry(REG_BVRDE _T("\\Mru"), _T("FindFolder"));
      bHandled = FALSE;
      return 0;
   }
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      _tcscpy(m_szPattern, CWindowText(m_ctrlFindText));
      _tcscpy(m_szFolder, CWindowText(m_ctrlFolderText));

      m_iFlags = FR_FINDNEXT | FR_DOWN;
      if( m_ctrlMatchWholeWord.GetCheck() == BST_CHECKED ) m_iFlags |= FR_WHOLEWORD;
      if( m_ctrlMatchCase.GetCheck() == BST_CHECKED ) m_iFlags |= FR_MATCHCASE;
      if( m_ctrlRegExp.GetCheck() == BST_CHECKED ) m_iFlags |= FR_REGEXP;
      if( m_ctrlSubFolders.GetCheck() == BST_CHECKED ) m_iFlags |= FR_SUBFOLDERS;

      m_ctrlFindText.AddToList();
      m_ctrlFolderText.AddToList();

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
      _UpdateButtons();
      return 0;
   }

   // Implementation

   void _UpdateButtons()
   {
      CWindow(GetDlgItem(IDOK)).EnableWindow(m_ctrlFindText.GetWindowTextLength() > 0 && m_ctrlFolderText.GetWindowTextLength() > 0);
   }
};

UINT __declspec(selectany) CFindFilesDlg::m_iFlags = 0;


#endif // !defined(AFX_FINDFILESDLG_H__20031225_6EB7_455C_FCAA_0080AD509054__INCLUDED_)

