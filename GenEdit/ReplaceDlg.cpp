
#include "StdAfx.h"
#include "resource.h"

#include "ReplaceDlg.h"

#pragma code_seg( "DIALOGS" )


CReplaceDlg::CReplaceDlg(IDevEnv* pDevEnv, HWND hWndScintilla, FINDREPLACEA& fr) :
   m_pDevEnv(pDevEnv), 
   m_ctrlScintilla(hWndScintilla),
   m_fr(fr)
{
}

LRESULT CReplaceDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlFindText.SubclassWindow(GetDlgItem(IDC_FINDTEXT));
   m_ctrlReplaceText.SubclassWindow(GetDlgItem(IDC_REPLACETEXT));
   m_ctrlMatchWholeWord = GetDlgItem(IDC_WHOLEWORD);
   m_ctrlMatchCase = GetDlgItem(IDC_MATCHCASE);
   m_ctrlRegExp = GetDlgItem(IDC_REGEXP);
   m_ctrlWrap = GetDlgItem(IDC_WRAP);
   m_ctrlInSelection = GetDlgItem(IDC_SELECTION);
   m_ctrlWholeFile = GetDlgItem(IDC_WHOLEFILE);

   m_ctrlFindText.ReadFromRegistry(REG_BVRDE _T("\\Mru"), _T("Find"));
   m_ctrlReplaceText.ReadFromRegistry(REG_BVRDE _T("\\Mru"), _T("Replace"));

   TCHAR szBuffer[32] = { 0 };
   m_pDevEnv->GetProperty(_T("gui.document.findText"), szBuffer, 31);

   // Copy selection into text if available
   CHAR szText[128] = { 0 };
   TextRange tr;
   tr.chrg = m_ctrlScintilla.GetSelection();
   tr.chrg.cpMax = min(tr.chrg.cpMax, tr.chrg.cpMin + 127);
   tr.lpstrText = szText;
   m_ctrlScintilla.GetTextRange(&tr);
   if( _tcscmp(szBuffer, _T("false")) != 0 
       && strlen(szText) > 0 
       && strchr(szText, '\n') == NULL 
       && strchr(szText, '\r') == NULL)
   {
      ::SetWindowTextA(m_ctrlFindText, tr.lpstrText);
   }
   else {
      ::SetWindowTextA(m_ctrlFindText, m_fr.lpstrFindWhat);
   }
   m_ctrlFindText.LimitText(m_fr.wReplaceWithLen);

   ::SetWindowTextA(m_ctrlReplaceText, m_fr.lpstrReplaceWith);
   m_ctrlReplaceText.LimitText(m_fr.wReplaceWithLen);

   if( m_fr.Flags & FR_WHOLEWORD ) m_ctrlMatchWholeWord.SetCheck(BST_CHECKED);
   if( m_fr.Flags & FR_MATCHCASE ) m_ctrlMatchCase.SetCheck(BST_CHECKED);
   if( m_fr.Flags & SCFIND_REGEXP ) m_ctrlRegExp.SetCheck(BST_CHECKED);
   if( m_fr.Flags & FR_WRAP ) m_ctrlWrap.SetCheck(BST_CHECKED);

   if( tr.chrg.cpMin == tr.chrg.cpMax ) {
      m_ctrlWholeFile.SetCheck(BST_CHECKED);
   }
   else {
      if( m_fr.Flags & FR_INSEL) {
         m_ctrlInSelection.SetCheck(BST_CHECKED); 
      }
      else {
         m_ctrlWholeFile.SetCheck(BST_CHECKED);
      }
   }

   _UpdateButtons();
   return TRUE;
}

LRESULT CReplaceDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   m_ctrlFindText.WriteToRegistry(REG_BVRDE _T("\\Mru"), _T("Find"));
   m_ctrlReplaceText.WriteToRegistry(REG_BVRDE _T("\\Mru"), _T("Replace"));
   bHandled = FALSE;
   return 0;
}

LRESULT CReplaceDlg::OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   _UpdateButtons();
   return 0;
}

LRESULT CReplaceDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{   
   USES_CONVERSION;
   strcpy(m_fr.lpstrFindWhat, T2A(CWindowText(m_ctrlFindText)));
   strcpy(m_fr.lpstrReplaceWith, T2A(CWindowText(m_ctrlReplaceText)));
   m_fr.Flags = FR_REPLACE | FR_DOWN;
   if( wID == IDC_REPLACEALL ) m_fr.Flags = FR_REPLACEALL | FR_DOWN;
   if( m_ctrlMatchWholeWord.GetCheck() == BST_CHECKED ) m_fr.Flags |= FR_WHOLEWORD;
   if( m_ctrlMatchCase.GetCheck() == BST_CHECKED ) m_fr.Flags |= FR_MATCHCASE;
   if( m_ctrlRegExp.GetCheck() == BST_CHECKED ) m_fr.Flags |= SCFIND_REGEXP;
   if( m_ctrlWrap.GetCheck() == BST_CHECKED ) m_fr.Flags |= FR_WRAP;
   if( m_ctrlInSelection.GetCheck() == BST_CHECKED ) m_fr.Flags |= FR_INSEL;

   m_ctrlFindText.AddToList();
   m_ctrlReplaceText.AddToList();

   EndDialog(wID);
   return 0;
}

LRESULT CReplaceDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   EndDialog(wID);
   return 0;
}

void CReplaceDlg::_UpdateButtons()
{
   BOOL bOK = TRUE;
   if( m_ctrlFindText.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( m_ctrlFindText.GetCurSel() >= 0 ) bOK = FALSE;
   CWindow(GetDlgItem(IDOK)).EnableWindow(bOK);
   CWindow(GetDlgItem(IDC_REPLACE)).EnableWindow(bOK);
   CWindow(GetDlgItem(IDC_REPLACEALL)).EnableWindow(bOK);
   CharacterRange cr = m_ctrlScintilla.GetSelection();
   m_ctrlInSelection.EnableWindow(bOK && cr.cpMin != cr.cpMax);
}
