
#include "StdAfx.h"
#include "resource.h"

#pragma code_seg( "DIALOGS" )

#include "GotoDlg.h"


CGotoDlg::CGotoDlg(HWND hWndScintilla) :
   m_ctrlScintilla(hWndScintilla)
{
}

LRESULT CGotoDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlLine = GetDlgItem(IDC_GOLINE);
   SetDlgItemInt(IDC_CURRLINE, m_ctrlScintilla.GetCurrentLine() + 1);
   SetDlgItemInt(IDC_LASTLINE, m_ctrlScintilla.GetLineCount());
   _UpdateButtons();
   return 0;
}

LRESULT CGotoDlg::OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   _UpdateButtons();
   return 0;
}

LRESULT CGotoDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int nLine = _ttol(CWindowText(m_ctrlLine)) - 1;
   m_ctrlScintilla.GotoLine(nLine);
   m_ctrlScintilla.EnsureVisible(nLine);
   EndDialog(wID);
   return 0;
}

LRESULT CGotoDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   EndDialog(wID);
   return 0;
}

void CGotoDlg::_UpdateButtons()
{
   CWindow(GetDlgItem(IDOK)).EnableWindow(_ttol(CWindowText(m_ctrlLine)) > 0);
}
