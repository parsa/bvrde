#if !defined(AFX_FINDFILESVIEW_H__20031225_8C88_2614_5BFD_0080AD509054__INCLUDED_)
#define AFX_FINDFILESVIEW_H__20031225_8C88_2614_5BFD_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Show Wait cursor.
// Stripped copy of WTL sources.
class CWaitCursor
{
public:
   HCURSOR m_hOldCursor;
   CWaitCursor()
   {
      m_hOldCursor = ::SetCursor( ::LoadCursor(NULL, IDC_WAIT) );
   }
   ~CWaitCursor()
   {
      ::SetCursor(m_hOldCursor);
   }
};


class CFindThread : 
   public CThreadImpl<CFindThread>, 
   public ILineCallback
{
public:
   CRichEditCtrl m_ctrlEdit;
   TCHAR m_szPattern[128];
   TCHAR m_szFolder[MAX_PATH];
   TCHAR m_szLastFile[MAX_PATH];
   LONG m_nMatches;
   UINT m_iFlags;

   DWORD Run()
   {
      ATLASSERT(m_ctrlEdit.IsWindow());
      ::SetThreadLocale(_pDevEnv->GetLCID());
      CCoInitialize cominit;
      m_nMatches = 0;
      // Title
      TCHAR szText[100] = { 0 };
      ::LoadString(_Module.GetResourceInstance(), IDS_SEARCHING, szText, 99);
      TCHAR szTitle[250] = { 0 };
      ::wsprintf(szTitle, szText, m_szPattern);
      _AppendRtfText(szTitle, CFM_COLOR, 0, ::GetSysColor(COLOR_HIGHLIGHT));
      // Need to postfix with pattern-match for folder
      _tcscat(m_szFolder, _T("/*"));
      _tcscpy(m_szLastFile, _T(""));
      // Build 'grep' prompt and execute comment through
      // project's scripting mode.
      ISolution* pSolution = _pDevEnv->GetSolution();
      if( pSolution == NULL ) return 0;
      IProject* pProject = pSolution->GetActiveProject();
      if( pProject == NULL ) return 0;
      IDispatch* pDisp = pProject->GetDispatch();
      CComDispatchDriver dd = pDisp;
      TCHAR szCommand[400];
      ::wsprintf(szCommand, _T("grep -nH%s%s%s%s '%s' %s"),
         (m_iFlags & FR_REGEXP) != 0     ? _T("G") : _T(""),
         (m_iFlags & FR_SUBFOLDERS) != 0 ? _T("R") : _T(""),
         (m_iFlags & FR_MATCHCASE) == 0  ? _T("i") : _T(""),
         (m_iFlags & FR_WHOLEWORD) != 0  ? _T("w") : _T(""),
         m_szPattern,
         m_szFolder);
      CComVariant aParams[3];
      aParams[2] = szCommand;
      aParams[1] = static_cast<IUnknown*>(this);
      aParams[0] = 0L;
      dd.InvokeN(OLESTR("ExecCommand"), aParams, 3);
      if( m_nMatches == 0 ) {
         TCHAR szNoMatches[100] = { 0 };
         ::LoadString(_Module.GetResourceInstance(), IDS_NOMATCHES, szNoMatches, 99);
         _AppendRtfText(szNoMatches, CFM_ITALIC, CFM_ITALIC);
      }
      return 0;
   }

   // ILineCallback

   virtual HRESULT STDMETHODCALLTYPE OnIncomingLine(BSTR bstr)
   {
      if( ShouldStop() ) return E_ABORT;
      USES_CONVERSION;
      // Format of grep output is:
      //   filename:linenum:textsnippet
      // We wish to group all matches from same file in 
      // same batch so we split the lines.
      if( bstr == NULL ) return S_OK;
      if( ocslen(bstr) < 4 ) return S_OK;
      LPTSTR p1 = _tcschr(bstr + 3, ':');
      if( p1 == NULL ) return S_OK;
      LPTSTR p2 = _tcschr(p1 + 1, ':');
      if( p2 == NULL ) return S_OK;
      TCHAR szLine[500] = { ' ', ' ', ' ', 0 };
      LPTSTR p = p2 + 1;
      while( _istspace(*p) ) p++;
      _tcsncpy(szLine + 3, p, 495);
      *p1 = '\0';
      *p2 = '\0';
      if( wcscmp(bstr, T2CW(m_szLastFile)) != 0 ) {
         wcscpy(m_szLastFile, bstr);
         _AppendRtfText(m_szLastFile, CFM_BOLD, CFE_BOLD);
         _AppendRtfText(_T("\r\n"));
      }
      _AppendRtfText(p1 + 1, CFM_COLOR, 0, ::GetSysColor(COLOR_GRAYTEXT));
      _AppendRtfText(szLine, CFM_COLOR, 0, ::GetSysColor(COLOR_WINDOWTEXT));
      _AppendRtfText(_T("\r\n"));
      m_nMatches++;
      return S_OK;
   }

   // IUnknown

   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
   {
      if( riid == __uuidof(ILineCallback) || riid == IID_IUnknown ) {
         *ppvObject = (ILineCallback*) this;
         return S_OK;
      }
      return E_NOINTERFACE;
   }
   ULONG STDMETHODCALLTYPE AddRef(void)
   {
      return 1;
   }
   ULONG STDMETHODCALLTYPE Release(void)
   {
      return 1;
   }

   // Implementation

   void _AppendRtfText(LPCTSTR pstrText, DWORD dwMask = 0, DWORD dwEffects = 0, COLORREF clrText = 0)
   {
      ATLASSERT(m_ctrlEdit.IsWindow());
      ATLASSERT(!::IsBadStringPtr(pstrText,-1));
      m_ctrlEdit.HideSelection(TRUE);
      // Remove top lines if we've filled out the buffer
      GETTEXTLENGTHEX gtlx = { GTL_DEFAULT | GTL_CLOSE, 1200 };
      while( m_ctrlEdit.GetTextLengthEx(&gtlx) > m_ctrlEdit.GetLimitText() - 3000 ) {     
         LONG iStartPos = m_ctrlEdit.LineIndex(0);
         LONG iEndPos = m_ctrlEdit.LineIndex(1);
         m_ctrlEdit.SetSel(iStartPos, iEndPos);
         m_ctrlEdit.ReplaceSel(_T(""));
      }
      // Append line
      LONG iStartPos = 0;
      LONG iEndPos = 0;
      LONG iDummy = 0;
      CHARFORMAT cf;
      cf.cbSize = sizeof(CHARFORMAT);
      cf.dwMask = dwMask;;
      cf.dwEffects = dwEffects;
      cf.crTextColor = clrText;
      m_ctrlEdit.SetSel(-1, -1);
      m_ctrlEdit.GetSel(iStartPos, iDummy);
      m_ctrlEdit.ReplaceSel(pstrText);
      m_ctrlEdit.GetSel(iDummy, iEndPos);
      m_ctrlEdit.SetSel(iStartPos, iEndPos);
      m_ctrlEdit.SetSelectionCharFormat(cf);
      m_ctrlEdit.HideSelection(FALSE);
      m_ctrlEdit.SetSel(-1, -1);
   }
};


class CFindFilesView : public CWindowImpl<CFindFilesView, CRichEditCtrl>
{
public:
   CFindThread m_thread;

   BEGIN_MSG_MAP(CFindFilesView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_PRINTCLIENT, OnPrintClient)
      MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      LRESULT lRes = DefWindowProc();

      SetFont(AtlGetDefaultGuiFont());
      SetBackgroundColor(::GetSysColor(COLOR_WINDOW));
      SetSel(-1, -1);
      LimitText(60000);

      return lRes;
   }
   LRESULT OnLButtonDblClk(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CWaitCursor cursor;
      // Look up the line where user double-clicked. Try to locate
      // the line-number or filename, and then launch view.
      int iLineNum = -1;
      LONG iStart, iEnd;
      GetSel(iStart, iEnd);
      int iLine = LineFromChar(iStart);
      while( iLine >= 0 ) {
         int iPos = LineIndex(iLine);
         int iLen = LineLength(iPos);
         // Ok, the RTF GetLine() is kind of weird and needs the
         // first WORD to be the length...
         WORD wSize = (iLen + 1) * sizeof(TCHAR);
         LPTSTR pstr = (LPTSTR) _alloca(wSize  + sizeof(WORD));
         * (WORD*) pstr = iLen;
         GetLine(iLine, pstr);
         pstr[iLen] = '\0';
         if( _istdigit(*pstr) ) {
            // Found a linenumber. Let's remember the first
            // one found only.
            if( iLineNum < 0 ) iLineNum = _ttol(pstr);
         }
         else {
            // Found what looks like a filename. Open view.
            _OpenView(pstr, max(iLineNum, 0));
            return 0;
         }
         iLine--;
      }
      return 0;
   }
   LRESULT OnPrintClient(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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
      rcPage.bottom = ::MulDiv(rcPage.bottom + 20, 1440, dc.GetDeviceCaps(LOGPIXELSY));

      RECT rcMargin = { 8, 8, 8, 8 };
      RECT rcOutput = rcPage;
      // Convert from 1/1000" to twips
      rcOutput.left += ::MulDiv(rcMargin.left, 1440, 1000);
      rcOutput.right -= ::MulDiv(rcMargin.right, 1440, 1000);
      //rcOutput.top += ::MulDiv(rcMargin.top, 1440, 1000);
      //rcOutput.bottom -= ::MulDiv(rcMargin.bottom, 1440, 1000);
  
      FORMATRANGE fr;
      fr.hdc = dc;
      fr.hdcTarget = dc;
      fr.rc = rcOutput;
      fr.rcPage = rcPage;
      fr.chrg.cpMin = LineIndex(GetFirstVisibleLine());
      fr.chrg.cpMax = -1;

      POINT pt;
      pt = PosFromChar(fr.chrg.cpMin);
      dc.SetViewportOrg(pt.x - 1, pt.y - 1);

      FormatRange(&fr, TRUE);
      DisplayBand(&rcOutput);

      // Cleanup cache in richedit
      FormatRange(NULL, FALSE);
      return 0;
   }

   // Operations

   void DoSearch(LPCTSTR pstrPattern, LPCTSTR pstrFolder, UINT iFlags)
   {
      // First, clear display
      SetWindowText(_T(""));
      // Start a new thread to collect result
      m_thread.Stop();
      m_thread.m_ctrlEdit = m_hWnd;
      _tcsncpy(m_thread.m_szPattern, pstrPattern, (sizeof(m_thread.m_szPattern)/sizeof(TCHAR)) - 1);
      _tcsncpy(m_thread.m_szFolder, pstrFolder, (sizeof(m_thread.m_szFolder)/sizeof(TCHAR)) - 1);
      m_thread.m_iFlags = iFlags;
      m_thread.Start();
   }
   BOOL IsRunning() const
   {
      return m_thread.IsRunning();
   }

   // Implementation

   bool _OpenView(LPCTSTR pstrFilename, long lLineNum)
   {
      ATLASSERT(!::IsBadStringPtr(pstrFilename,-1));
      if( _tcslen(pstrFilename) < 2 ) return false;
      // Munge filename
      TCHAR szSearchFile[MAX_PATH];
      _tcscpy(szSearchFile, pstrFilename);
      ::PathStripPath(szSearchFile);
      // Locate file in project's views
      ISolution* pSolution = _pDevEnv->GetSolution();
      if( pSolution == NULL ) return false;
      for( INT i = 0; i < pSolution->GetItemCount(); i++ ) {
         IProject* pProject = pSolution->GetItem(i);
         for( INT j = 0; j < pProject->GetItemCount(); j++ ) {
            IView* pView = pProject->GetItem(j);
            TCHAR szFilename[MAX_PATH + 1] = { 0 };
            pView->GetFileName(szFilename, MAX_PATH);
            ::PathStripPath(szFilename);
            if( _tcsicmp(szSearchFile, szFilename) == 0 ) return pView->OpenView(lLineNum) == TRUE;
         }
      }
      return false;
   }
};


#endif // !defined(AFX_FINDFILESVIEW_H__20031225_8C88_2614_5BFD_0080AD509054__INCLUDED_)
