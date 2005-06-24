
#include "StdAfx.h"
#include "resource.h"

#include "GenEdit.h"
#include "ScintillaView.h"
#include "SciLexer.h"

#pragma code_seg( "EDITOR" )

#include "GotoDlg.h"
#include "FindDlg.h"
#include "ReplaceDlg.h"


LRESULT CScintillaView::OnGotoLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CGotoDlg dlg(m_hWnd);
   dlg.DoModal();
   return 0;
}

LRESULT CScintillaView::OnFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CFindDlg dlg(m_pDevEnv, m_hWnd, s_frFind);
   UINT nRet = dlg.DoModal();
   switch( nRet ) {
   case IDC_MARKALL:
      {
         int iCurrent = GetCurrentPos();
         int posFirstFound = _FindNext(s_frFind.Flags | FR_DOWN, s_frFind.lpstrFindWhat, false);
         int posFound = posFirstFound;
         while( posFound != -1 ) {
            int iLineNum = LineFromPosition(posFound);
            MarkerAdd(iLineNum, MARKER_BOOKMARK);
            posFound = _FindNext(s_frFind.Flags | FR_DOWN | FR_WRAP, s_frFind.lpstrFindWhat, false);
            if( posFound == posFirstFound ) break;
         }
         SetCurrentPos(iCurrent);
      }
      // FALL THROUGH
   case IDOK:
      _FindNext(s_frFind.Flags, s_frFind.lpstrFindWhat, true);
      break;
   }
   return 0;
}

LRESULT CScintillaView::OnReplace(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CReplaceDlg dlg(m_pDevEnv, m_hWnd, s_frFind);
   UINT nRes = dlg.DoModal();
   switch( nRes ) {
   case IDOK:
      _FindNext(s_frFind.Flags | FR_DOWN, s_frFind.lpstrFindWhat, true);
      break;
   case IDC_REPLACE:
      {
         // First try to find string inside current selection (if we already selected the
         // text to do replacement on), then try to find next matching entry...
         if( _FindNext(s_frFind.Flags | FR_DOWN | FR_INSEL, s_frFind.lpstrFindWhat, false) == -1 ) {
            _FindNext(s_frFind.Flags | FR_DOWN, s_frFind.lpstrFindWhat, true);
         }
         _ReplaceOnce();
      }
      break;
   case IDC_REPLACEALL:
      {
         BeginUndoAction();
         int iStartPos = 0;
         int nReplaced = 0;
         int cchSearched = (int) strlen(s_frFind.lpstrFindWhat);
         int cchReplaced = (int) strlen(s_frFind.lpstrReplaceWith);
         while( _FindNext(s_frFind.Flags | FR_WRAP | FR_DOWN, s_frFind.lpstrFindWhat, false) >= 0 ) {
            // Avoid endless loop
            CharacterRange crFound = GetSelection();
            if( nReplaced++ == 0 ) iStartPos = crFound.cpMin;
            else if( iStartPos >= crFound.cpMin && iStartPos <= crFound.cpMin + cchReplaced ) break;
            else if( crFound.cpMin < iStartPos ) iStartPos += cchReplaced - cchSearched;
            // Do replacement
            _ReplaceOnce();
         }
         EndUndoAction();
      }
      break;
   }
   return 0;
}

LRESULT CScintillaView::OnRepeat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   return _FindNext(s_frFind.Flags, s_frFind.lpstrFindWhat, true);
}

LRESULT CScintillaView::OnUpperCase(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   UpperCase();
   return 0;
}

LRESULT CScintillaView::OnLowerCase(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   LowerCase();
   return 0;
}

LRESULT CScintillaView::OnZoomIn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ZoomIn();
   return 0;
}

LRESULT CScintillaView::OnZoomOut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ZoomOut();
   return 0;
}

LRESULT CScintillaView::OnDeleteLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   LineCut();
   return 0;
}

LRESULT CScintillaView::OnRectSelection(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   SetSelectionMode(GetSelectionMode() == SC_SEL_RECTANGLE ? SC_SEL_STREAM : SC_SEL_RECTANGLE);
   return 0;
}

LRESULT CScintillaView::OnIndent(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iStart = GetSelectionStart();
   int iEnd = GetSelectionEnd();
   int selStartLine = LineFromPosition(iStart);
   int selEndLine = LineFromPosition(iEnd);
   int iDiff = wID == ID_EDIT_INDENT ? 1 : -1;
   BeginUndoAction();
   for( int i = selStartLine; i <= selEndLine; i++ ) {
      int indent = GetLineIndentation(i);
      indent += iDiff * GetIndent();
      _SetLineIndentation(i, indent);
   }
   EndUndoAction();
   return 0;
}

LRESULT CScintillaView::OnTabify(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iStart = GetSelectionStart();
   int iEnd = GetSelectionEnd();   
   int nLen = iEnd - iStart;
   LPSTR pstrBuffer = (LPSTR) malloc(nLen + 1);
   if( pstrBuffer == NULL ) return 0;
   GetTextRange(iStart, iEnd, pstrBuffer);
   pstrBuffer[nLen] = '\0';
   CString s = pstrBuffer;
   free(pstrBuffer);
   int iIndent = GetUseTabs() ? GetTabWidth() : GetIndent();
   CString sSpaces(' ', iIndent);
   if( wID == ID_EDIT_TABIFY ) s.Replace(sSpaces, _T("\t")); else s.Replace(_T("\t"), sSpaces);
   USES_CONVERSION;
   ReplaceSel(T2CA(s));
   return 0;
}

LRESULT CScintillaView::OnComment(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CHAR szLine[2000] = { 0 };
   LPSTR pstrComment = "// ";
   LPSTR pstrLongComment = "// ";
   if( m_sLanguage == _T("sql") ) {
      pstrComment = "-- ";
      pstrLongComment = "-- ";
   }
   int comment_length = 3;
   int iStart = GetSelectionStart();
   int iEnd = GetSelectionEnd();
   int caretPosition = GetCurrentPos();
   // Checking if caret is located in _beginning_ of selected block
   bool bMoveCaret = caretPosition < iEnd;
   int selStartLine = LineFromPosition(iStart);
   int selEndLine = LineFromPosition(iEnd);
   int lines = selEndLine - selStartLine;
   int firstSelLineStart = PositionFromLine(selStartLine);
   // "caret return" is part of the last selected line
   if( lines > 0 && (iEnd == PositionFromLine(selEndLine)) ) selEndLine--;
   BeginUndoAction();
   for( int i = selStartLine; i <= selEndLine; i++ ) {
      int lineStart = PositionFromLine(i);
      int lineIndent = lineStart;
      int lineEnd = GetLineEndPosition(i);
      GetTextRange(lineIndent, lineEnd, szLine);
      // empty lines are not commented
      if( strlen(szLine) < 1 ) continue;
      if( memcmp(szLine, pstrComment, comment_length - 1) == 0)  {
         if( memcmp(szLine, pstrLongComment, comment_length) == 0) {
            // Removing comment with space after it
            SetSel(lineIndent, lineIndent + comment_length);
            ReplaceSel("");
            if (i == selStartLine) // is this the first selected line?
               iStart -= comment_length;
            iEnd -= comment_length; // every iteration
            continue;
         } 
         else {
            // removing comment _without_ space
            SetSel(lineIndent, lineIndent + comment_length - 1);
            ReplaceSel("");
            if (i == selStartLine) // is this the first selected line?
               iStart -= (comment_length - 1);
            iEnd -= (comment_length - 1); // every iteration
            continue;
         }
      }
      if( i == selStartLine ) {// is this the first selected line?
         iStart += comment_length;
      }
      iEnd += comment_length; // every iteration
      InsertText(lineIndent, pstrLongComment);
   }
   // After uncommenting selection may promote itself to the lines
   // before the first initially selected line;
   // another problem - if only comment symbol was selected;
   if( iStart < firstSelLineStart ) {
      if( iStart >= iEnd - (comment_length - 1) ) iEnd = firstSelLineStart;
      iStart = firstSelLineStart;
   }
   if( bMoveCaret ) {
      // Moving caret to the beginning of selected block
      GotoPos(iEnd);
      SetCurrentPos(iStart);
   } 
   else {
      SetSel(iStart, iEnd);
   }
   EndUndoAction();
   return 0;
}

LRESULT CScintillaView::OnBoxComment(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   LPSTR pstrStartComment = "/* ";
   LPSTR pstrEndComment = " */";
   if( m_sLanguage == _T("html") || m_sLanguage == _T("php") || m_sLanguage == _T("asp") ) {
      pstrStartComment = "<!-- ";
      pstrEndComment = " -->";
   }
   long iStart = GetSelectionStart();
   long iEnd = GetSelectionEnd();
   int caretPosition = GetCurrentPos();
   // Checking if caret is located in _beginning_ of selected block
   bool bMoveCaret = caretPosition < iEnd;
   // If there is no selection?
   if( iEnd - iStart <= 0 ) {
      CHAR szLine[500] = { 0 };
      if( GetLineLength(GetCurrentLine()) >= sizeof(szLine) ) return true;
      GetLine(GetCurrentLine(), szLine);
      int iCurrent = GetCaretInLine();
      // Checking if we are not inside a word
      if( !isalnum(szLine[iCurrent]) ) return true; // caret is located _between_ words
      int iStartWord = iCurrent;
      int iEndWord = iCurrent;
      int iStartCounter = 0;
      int iEndCounter = 0;
      while( iStartWord > 0 && isalnum(szLine[iStartWord - 1]) ) {
         iStartCounter++;
         iStartWord--;
      }
      // Checking beginning of the word
      if( iStartWord == iCurrent ) return true; // caret is located _before_ a word
      while( szLine[iEndWord + 1] != '\0' && isalnum(szLine[iEndWord + 1]) ) {
         iEndCounter++;
         iEndWord++;
      }
      iStart -= iStartCounter;
      iEnd += (iEndCounter + 1);
   }
   BeginUndoAction();
   InsertText(iStart, pstrStartComment);
   iEnd += strlen(pstrStartComment);
   iStart += strlen(pstrStartComment);
   InsertText(iEnd, pstrEndComment);
   if( bMoveCaret ) {
      // Moving caret to the beginning of selected block
      GotoPos(iEnd);
      SetCurrentPos(iStart);
   } 
   else {
      SetSel(iStart, iEnd);
   }
   EndUndoAction();
   return 0;
}

LRESULT CScintillaView::OnViewWhiteSpace(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iState = GetViewWS() == SCWS_INVISIBLE ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE;
   SetViewWS(iState);
   return 0;
}

LRESULT CScintillaView::OnViewEOL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   SetViewEOL(!GetViewEOL());
   return 0;
}

LRESULT CScintillaView::OnViewIndentGuides(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   SetIndentationGuides(!GetIndentationGuides());
   return 0;
}

LRESULT CScintillaView::OnViewWordWrap(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   SetWrapMode(GetWrapMode() == SC_WRAP_NONE ? SC_WRAP_WORD : SC_WRAP_NONE);
   return 0;
}

LRESULT CScintillaView::OnMarkerToggle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iLineNum = GetCurrentLine();
   int iValue = MarkerGet(iLineNum);
   int iMask = (1 << MARKER_BOOKMARK);
   if( iValue & iMask ) {
      MarkerDelete(iLineNum, MARKER_BOOKMARK);
   }
   else {
      MarkerAdd(iLineNum, MARKER_BOOKMARK);
   }
   return 0;
}

LRESULT CScintillaView::OnMarkerClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   MarkerDeleteAll(MARKER_BOOKMARK);
   return 0;
}

LRESULT CScintillaView::OnMarkerNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iLineNum = GetCurrentLine();
   iLineNum = MarkerNext(iLineNum + 1, 1 << MARKER_BOOKMARK);
   if( iLineNum == -1 ) {
      ::MessageBeep((UINT)-1);
   }
   else {
      GotoLine(iLineNum);
      EnsureVisible(iLineNum);
   }
   return 0;
}

LRESULT CScintillaView::OnMarkerGoto(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iLineNum = -1;
   for( int i = 0; i <= wID - ID_BOOKMARKS_GOTO1; i++ ) {
      iLineNum = MarkerNext(iLineNum + 1, 1 << MARKER_BOOKMARK);
   }
   if( iLineNum >= 0 ) {
      GotoLine(iLineNum);
      EnsureVisible(iLineNum);
   }
   return 0;
}

LRESULT CScintillaView::OnMarkerPrevious(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iLineNum = GetCurrentLine();
   iLineNum = MarkerPrevious(iLineNum - 1, 1 << MARKER_BOOKMARK);
   if( iLineNum == -1 ) {
      ::MessageBeep((UINT)-1);
   }
   else {
      GotoLine(iLineNum);
      EnsureVisible(iLineNum);
   }
   return 0;
}

LRESULT CScintillaView::OnMacroStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   StartRecord();
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnMacroCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   StopRecord();
   bHandled = FALSE;
   return 0;
}
