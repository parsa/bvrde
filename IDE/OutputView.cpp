
#include "stdafx.h"
#include "resource.h"

#include "OutputView.h"


/////////////////////////////////////////////////////////////////////////
// Constructor

COutputView::COutputView()
{
}

void COutputView::Clear()
{
   SetWindowText(_T(""));
}

/////////////////////////////////////////////////////////////////////////
// Message handlers

LRESULT COutputView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   LRESULT lRes = DefWindowProc();
   SetFont(AtlGetDefaultGuiFont());
   SetBackgroundColor(::GetSysColor(COLOR_WINDOW));
   LimitText(20000);
   SetSel(0, 0);
   return lRes;
}

LRESULT COutputView::OnPrintClient(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   // NOTE: We need to handle WM_PRINTCLIENT because we're using this
   //       window with the AnimateWindow() API and there's a bug in
   //       the RichEdit control which doesn't seem to handle this
   //       message correctly.
   CDCHandle dc = (HDC) wParam;
   RECT rcClient;
   GetClientRect(&rcClient);

   dc.FillSolidRect(&rcClient, ::GetSysColor(COLOR_WINDOW));

   RECT rcPage = rcClient;
   rcPage.right = ::MulDiv(rcPage.right, 1440, dc.GetDeviceCaps(LOGPIXELSX));
   rcPage.bottom = ::MulDiv(rcPage.bottom, 1440, dc.GetDeviceCaps(LOGPIXELSY));

   RECT rcMargin = { 8, 8, 8, 8 };
   RECT rcOutput = rcPage;
   // Convert from 1/1000" to twips
   rcOutput.left += ::MulDiv(rcMargin.left, 1440, 1000);
   rcOutput.right -= ::MulDiv(rcMargin.right, 1440, 1000);
   rcOutput.top += ::MulDiv(rcMargin.top, 1440, 1000);
   //rcOutput.bottom -= ::MulDiv(rcMargin.bottom, 1440, 1000);

   FORMATRANGE fr;
   fr.hdc = dc;
   fr.hdcTarget = dc;
   fr.rc = rcOutput;
   fr.rcPage = rcPage;
   fr.chrg.cpMin = LineIndex(GetFirstVisibleLine());
   fr.chrg.cpMax = -1;

   // We have to adjust the origin because 0,0 is not at the corner of the paper
   // but is at the corner of the printable region
   int nOffsetX = dc.GetDeviceCaps(PHYSICALOFFSETX);
   int nOffsetY = dc.GetDeviceCaps(PHYSICALOFFSETY);
   dc.SetViewportOrg(-nOffsetX, -nOffsetY, NULL);
   FormatRange(&fr, TRUE);
   DisplayBand(&rcOutput);

   // Cleanup cache in RichEdit
   FormatRange(NULL, FALSE);

   return 0;
}

LRESULT COutputView::OnLButtonDblClk(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   LONG iStart, iEnd;
   GetSel(iStart, iEnd);
   int iLine = LineFromChar(iStart);
   int iLen = LineLength(iStart);
   // Ok, the RTF GetLine() is kind of weird and needs the
   // first WORD to be the length...
   WORD wSize = (iLen + 1) * sizeof(TCHAR);
   LPTSTR pstr = (LPTSTR) _alloca(wSize  + sizeof(WORD));
   * (WORD*) pstr = iLen;
   GetLine(iLine, pstr);
   pstr[iLen] = _T('\0');

   CString s = pstr;
   for( int iPos = s.Find(_T('.')); iPos >= 0; iPos = s.Find(_T('.'), iPos + 1) ) {
      // Extract filename
      int iStart = iPos - 1;
      if( iStart < 0 ) continue;
      while( iStart >= 0 && _tcschr(_T(" \t:;(){}/\\\"'"), s[iStart]) == NULL ) iStart--;
      iStart++;
      int iEnd = iPos + 1;
      while( iEnd < iLen && _tcschr(_T(" \t:;(){}/\\\"'."), s[iEnd]) == NULL ) iEnd++;
      CString sFilename = s.Mid(iStart, iEnd - iStart);

      // Extract line number
      // Variations supported:
      //    filename:x
      //    filename line x
      //    filename, line x
      long lLineNum = 0;
      while( pstr[iEnd] == _T(',') ) iEnd++;
      while( pstr[iEnd] == _T(' ') ) iEnd++;
      if( pstr[iEnd] == _T(':') ) iEnd++; 
      if( _tcsncmp(pstr + iEnd, _T("line"), 4) == 0 ) iEnd += 4; 
      while( pstr[iEnd] == _T(' ') ) iEnd++;
      if( _istdigit(pstr[iEnd]) ) lLineNum = _ttol(pstr + iEnd);

      // Locate file in project
      ISolution* pSolution = g_pDevEnv->GetSolution();
      if( _OpenView(pSolution->GetActiveProject(), sFilename, lLineNum) ) return 0;
      for( int i = 0; i < pSolution->GetItemCount(); i++ ) {
         if( _OpenView(pSolution->GetItem(i), sFilename, lLineNum) ) return 0;
      }
   }
   return 0;
}

// Implementation

bool COutputView::_OpenView(IProject* pProject, LPCTSTR pstrFilename, long lLineNum)
{
   ATLASSERT(pProject);
   ATLASSERT(!::IsBadStringPtr(pstrFilename,-1));
   if( pProject == NULL ) return false;
   if( _tcslen(pstrFilename) < 2 ) return false;
   // Munge filename
   TCHAR szSearchFile[MAX_PATH];
   _tcscpy(szSearchFile, pstrFilename);
   ::PathStripPath(szSearchFile);
   // Locate file in project's views
   for( INT i = 0; i < pProject->GetItemCount(); i++ ) {
      IView* pView = pProject->GetItem(i);
      TCHAR szFilename[MAX_PATH + 1] = { 0 };
      pView->GetFileName(szFilename, MAX_PATH);
      ::PathStripPath(szFilename);
      if( _tcsicmp(szSearchFile, szFilename) == 0 ) return pView->OpenView(lLineNum) == TRUE;
   }
   // TODO: Consider launching files by filename even if they don't
   //       exist in one of the projects.
   return false;
}
