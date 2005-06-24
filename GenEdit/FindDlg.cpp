
#include "StdAfx.h"
#include "resource.h"

#pragma code_seg( "DIALOGS" )

#include "FindDlg.h"


CFindDlg::CFindDlg(IDevEnv* pDevEnv, HWND hWndScintilla, FINDREPLACEA& fr) :
   m_pDevEnv(pDevEnv),
   m_ctrlScintilla(hWndScintilla),
   m_fr(fr)
{
}

LRESULT CFindDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlFindText.SubclassWindow(GetDlgItem(IDC_FINDTEXT));
   m_ctrlUp = GetDlgItem(IDC_DIRECTIONUP);
   m_ctrlDown = GetDlgItem(IDC_DIRECTIONDOWN);
   m_ctrlMatchWholeWord = GetDlgItem(IDC_WHOLEWORD);
   m_ctrlMatchCase = GetDlgItem(IDC_MATCHCASE);
   m_ctrlRegExp = GetDlgItem(IDC_REGEXP);
   m_ctrlWrap = GetDlgItem(IDC_WRAP);

   m_ctrlFindText.ReadFromRegistry(REG_BVRDE _T("\\Mru"), _T("Find"));

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
      m_ctrlFindText.SetWindowText(CString(tr.lpstrText));
   }
   else 
   {
      m_ctrlFindText.SetWindowText(CString(m_fr.lpstrFindWhat));
   }
   m_ctrlFindText.LimitText(m_fr.wFindWhatLen);

   if( (m_fr.Flags & FR_DOWN) != 0 ) m_ctrlDown.SetCheck(BST_CHECKED); else m_ctrlUp.SetCheck(BST_CHECKED);
   if( (m_fr.Flags & FR_WHOLEWORD) != 0 ) m_ctrlMatchWholeWord.SetCheck(BST_CHECKED);
   if( (m_fr.Flags & FR_MATCHCASE) != 0 ) m_ctrlMatchCase.SetCheck(BST_CHECKED);
   if( (m_fr.Flags & SCFIND_REGEXP) != 0 ) m_ctrlRegExp.SetCheck(BST_CHECKED);
   if( (m_fr.Flags & FR_WRAP) != 0 ) m_ctrlWrap.SetCheck(BST_CHECKED);

   _UpdateButtons();
   return 0;
}

LRESULT CFindDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   m_ctrlFindText.WriteToRegistry(REG_BVRDE _T("\\Mru"), _T("Find"));
   bHandled = FALSE;
   return 0;
}

LRESULT CFindDlg::OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   _UpdateButtons();
   return 0;
}

LRESULT CFindDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{   
   USES_CONVERSION;
   strcpy(m_fr.lpstrFindWhat, T2CA(CWindowText(m_ctrlFindText)));
   m_fr.Flags = FR_FINDNEXT;
   if( m_ctrlDown.GetCheck() == BST_CHECKED ) m_fr.Flags |= FR_DOWN;
   if( m_ctrlMatchWholeWord.GetCheck() == BST_CHECKED ) m_fr.Flags |= FR_WHOLEWORD;
   if( m_ctrlMatchCase.GetCheck() == BST_CHECKED ) m_fr.Flags |= FR_MATCHCASE;
   if( m_ctrlRegExp.GetCheck() == BST_CHECKED ) m_fr.Flags |= SCFIND_REGEXP;
   if( m_ctrlWrap.GetCheck() == BST_CHECKED ) m_fr.Flags |= FR_WRAP;

   m_ctrlFindText.AddToList();

   EndDialog(wID);
   return 0;
}

LRESULT CFindDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   EndDialog(wID);
   return 0;
}

void CFindDlg::_UpdateButtons()
{
   CWindow(GetDlgItem(IDOK)).EnableWindow(m_ctrlFindText.GetWindowTextLength() > 0 || m_ctrlFindText.GetCurSel() >= 0);
}
