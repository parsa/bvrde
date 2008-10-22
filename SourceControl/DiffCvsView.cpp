
#include "StdAfx.h"
#include "resource.h"

#include "DiffCvsView.h"

#pragma code_seg( "MISC" )


////////////////////////////////////////////////////////
// Operations

BOOL CDiffCvsView::GeneratePage(IElement* pElement, CSimpleArray<CString>& aLines)
{
   ATLASSERT(::IsWindow(m_hWnd));
   ATLASSERT(m_wndBrowser.IsWindow());
   ATLASSERT(m_spBrowser);
   ATLASSERT(pElement);

   CWaitCursor cursor;
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, CString(MAKEINTRESOURCE(IDS_STATUS_DIFFVIEW)));

   CComDispatchDriver dd = pElement->GetDispatch();  
   CComVariant vText;
   if( FAILED( dd.GetPropertyByName(L"Text", &vText) ) ) return FALSE;
   if( vText.vt != VT_BSTR || vText.bstrVal == NULL ) return FALSE;
   CString sText = vText.bstrVal;
   int cchText = sText.GetLength();

   // Detect which type of diff output this is:
   //   Original diff
   //   Context diff
   //   Unidiff
   enum { DIFF_ORIGINAL, DIFF_CONTEXT, DIFF_UNIDIFF } DiffType = DIFF_ORIGINAL;
   const int NUM_LINES_DETECT = 20;
   int nDiffLines = aLines.GetSize();
   for( int i = 0; i < min(nDiffLines, NUM_LINES_DETECT); i++ ) {
      CString& sLine = aLines[i];
      if( _tcsncmp(sLine, _T("*** "), 4) == 0 ) DiffType = DIFF_CONTEXT;
      if( _tcsncmp(sLine, _T("+++ "), 4) == 0 ) DiffType = DIFF_UNIDIFF;
      if( _tcsncmp(sLine, _T("@@ "), 3) == 0 ) DiffType = DIFF_UNIDIFF;
   }

   // Split source file into lines
   CSimpleArray<CString> aFile;
   CString sLine;
   while( !sText.IsEmpty() ) {
      sLine = sText.SpanExcluding(_T("\r\n"));
      aFile.Add(sLine);
      int iPos = sLine.GetLength();
      int nLen = sText.GetLength();
      if( iPos < nLen && sText[iPos] == '\r' ) iPos++;
      if( iPos < nLen && sText[iPos] == '\n' ) iPos++;
      sText = sText.Mid(iPos);
   }

   TCHAR szFaceName[80] = { 0 };
   TCHAR szFontSize[80] = { 0 };
   _pDevEnv->GetProperty(_T("sourcecontrol.diffview.font"), szFaceName, 79);
   _pDevEnv->GetProperty(_T("sourcecontrol.diffview.fontsize"), szFontSize, 79);
   TCHAR szWordWrap[16] = { 0 };
   TCHAR szListUnchanged[16] = { 0 };
   _pDevEnv->GetProperty(_T("sourcecontrol.diffview.wordwrap"), szWordWrap, 15);
   _pDevEnv->GetProperty(_T("sourcecontrol.diffview.listUnchanged"), szListUnchanged, 15);

   // OPTI: Because of the slow CString/memory allocation in C++, we should try
   //       to minimize the number of allocations. Let's pre-allocate some memory.
   DIFFINFO Info;
   int nAlloc = (cchText * 5) + (aFile.GetSize() * 80) + (aLines.GetSize() * 240);
   Info.sHTML.GetBuffer(nAlloc);
   Info.sHTML.ReleaseBuffer(0);
   Info.iFirstChange = -1;
   Info.bWordWrap = (_tcscmp(szWordWrap, _T("true")) == 0);
   Info.bListUnchanged = (_tcscmp(szListUnchanged, _T("true")) == 0);

   // Process diff output...
   switch( DiffType ) {
   case DIFF_ORIGINAL:  _ParseDiffOriginal(aFile, aLines, Info); break;
   case DIFF_CONTEXT:   _ParseDiffContext(aFile, aLines, Info); break;
   case DIFF_UNIDIFF:   _ParseDiffUnidiff(aFile, aLines, Info); break;
   }
   if( Info.sHTML.IsEmpty() ) return FALSE;

   SetWindowText(CString(MAKEINTRESOURCE(IDS_CAPTION_DIFF)));

   CString sFileInfo;
   _GenerateInfoHeader(sFileInfo, IDS_DIFF_LEFTFILE, Info.sLeftFileInfo);
   _GenerateInfoHeader(sFileInfo, IDS_DIFF_RIGHTFILE, Info.sRightFileInfo);
   _GenerateInfoHeader(sFileInfo, IDS_DIFF_GENERALFILE, Info.sGeneralFileInfo);
   // When the first change occours a bit down on the page and we have
   // the 'List Unchanged lines' option enabled, we'll print a notice at the
   // top so the user knowns he must scroll down to see the changes.
   if( Info.iFirstChange > 80 ) {
      CString sFirstChange;
      sFirstChange.Format(IDS_DIFF_FIRSTCHANGE, Info.iFirstChange);
      _GenerateInfoHeader(sFileInfo, sFirstChange);
   }

   CString sPage = AtlLoadHtmlResource(IDR_DIFF);
   sPage.Replace(_T("$TITLE$"), CString(MAKEINTRESOURCE(IDS_CAPTION_DIFF)));
   sPage.Replace(_T("$FONT$"), szFaceName);
   sPage.Replace(_T("$FONT-SIZE$"), szFontSize);
   sPage.Replace(_T("$WRAP$"), Info.bWordWrap ? _T("normal") : _T("nowrap"));
   sPage.Replace(_T("$OVERFLOW$"), Info.bWordWrap ? _T("auto") : _T("hidden"));   
   sPage.Replace(_T("$INFO$"), sFileInfo);
   sPage.Replace(_T("$TABLE$"), Info.sHTML);
   _LoadHtml(m_spBrowser, sPage);

   int cxChar = 0;
   CClientDC dc = m_hWnd;
   dc.GetCharWidth('X', 'X', &cxChar);

   // Resize window to 4/5 of screen width, but not too large...
   int cxScreen = ::GetSystemMetrics(SM_CXSCREEN) * 80 / 100;
   if( cxScreen > cxChar * 170 ) cxScreen = cxChar * 170;
   ResizeClient(cxScreen, -1);
   CenterWindow();

   ShowWindow(SW_NORMAL);
   return TRUE;
}

void CDiffCvsView::OnFinalMessage(HWND /*hWnd*/)
{
   delete this;
}

LRESULT CDiffCvsView::OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   if( uMsg != WM_COMMAND || !IsWindowVisible() || !::IsChild(m_hWnd, ::GetFocus()) ) {
      bHandled = FALSE;
      return 0;
   }
   LRESULT lResult = 0;
   bHandled = ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult);
   return lResult;
}

BOOL CDiffCvsView::PreTranslateMessage(MSG* pMsg)
{
   if( ::GetFocus() != m_hWnd ) return FALSE;

   if((pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) &&
      (pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST))
   {
      return FALSE;
   }

   // Give OCX a chance to translate this message
   return (BOOL) m_wndBrowser.SendMessage(WM_FORWARDMSG, 0, (LPARAM) pMsg);
}


////////////////////////////////////////////////////////
// Implementation

BOOL CDiffCvsView::_LoadHtml(IUnknown* pUnk, LPCWSTR pstrHTML)
{
   CComQIPtr<IWebBrowser> spBrowser = pUnk;
   if( spBrowser != NULL ) {
      CComPtr<IDispatch> spDisp;
      spBrowser->get_Document(&spDisp);
      if( spDisp == NULL ) {
         // IE DOM Document not yet ready; instead of listening for the
         // ready event, we just repeatedly retry the operation.
         SetTimer(TIMERID_LOADHTML, 500);
         return FALSE;
      }
      pUnk = spDisp;
   }
   HANDLE hHTMLText = ::GlobalAlloc(GPTR, (wcslen(pstrHTML) + 1) * sizeof(WCHAR));
   if( hHTMLText == NULL ) return FALSE;
   CComPtr<IStream> spStream;
   wcscpy( (LPWSTR) hHTMLText, pstrHTML );
   HRESULT Hr = ::CreateStreamOnHGlobal(hHTMLText, TRUE, &spStream);
   if( SUCCEEDED(Hr) ) {
      CComPtr<IPersistStreamInit> spPersistStreamInit;
      Hr = pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID*) &spPersistStreamInit);
      if( SUCCEEDED(Hr) ) {
           Hr = spPersistStreamInit->InitNew();
           if( SUCCEEDED(Hr) ) {
              Hr = spPersistStreamInit->Load(spStream);
           }
      }
   }
   return SUCCEEDED(Hr);
}

CString CDiffCvsView::_Htmlize(CString s) const
{
   s.Replace(_T("&"), _T("&amp;"));
   s.Replace(_T("<"), _T("&lt;"));
   s.Replace(_T(">"), _T("&gt;"));
   s.Replace(_T("\""), _T("&quot;"));
   s.Replace(_T(" "), _T("&nbsp;"));
   s.Replace(_T("\t"), _T("&nbsp;&nbsp;&nbsp;"));
   if( s.IsEmpty() ) s = _T("&nbsp;");
   return s;
}

void CDiffCvsView::_GenerateInfoHeader(CString& sHTML, CString sValue) const
{
   if( sValue.IsEmpty() ) return;
   CString sTemp;
   sTemp.Format(_T("<tr><td>%s</td></tr>"), sValue);
   sHTML += sTemp;
}

void CDiffCvsView::_GenerateInfoHeader(CString& sHTML, UINT uLabel, CString sValue) const
{
   if( sValue.IsEmpty() ) return;
   CString sTemp;
   sTemp.Format(_T("<b>%s</b> %s"), CString(MAKEINTRESOURCE(uLabel)), _Htmlize(sValue));
   _GenerateInfoHeader(sHTML, sTemp);
}

void CDiffCvsView::_GenerateSectionHeader(CString& sHTML, int iLeftStart, int iRightStart) const
{
   CString sLeft, sRight;
   sLeft.Format(IDS_LEFTLINE, iLeftStart);
   sRight.Format(IDS_RIGHTLINE, iRightStart);
   CString sTemp;
   _GenerateRow(sHTML, sTemp, 0, _T("hdr"), sLeft, sRight);
}

void CDiffCvsView::_GenerateRow(CString& sHTML, CString& sTemp, int iLineNo, LPCTSTR pstrType, CString& sLeft, CString& sRight) const
{
   TCHAR szLineNo[12] = { 0 };
   if( iLineNo != 0 ) ::wsprintf(szLineNo, _T("%d"), iLineNo);
   // OPTI: We need to HTMLize the strings, but for most of the display, both sides are the same; so we
   //       need only convert one of them in this case.
   // OPTI: To avoid the alloc/free of a temporary string every time we need
   //       to concatenate more data, we pass a reusable temp. string to use for this purpose.
   CString sLine1 = _Htmlize(sLeft);
   CString sLine2 = sLeft == sRight ? sLine1 : _Htmlize(sRight);
   // Produce the HTML table cell snippet
   sTemp.Format(_T("<tr><td class=\"lin\">%s</td><td class=\"%s\">%s</td><td class=\"%s\">%s</td></tr>\n"), szLineNo, pstrType, sLine1, pstrType, sLine2);
   sHTML += sTemp;
}


////////////////////////////////////////////////////////
// Message map

LRESULT CDiffCvsView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
   LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);

   _pDevEnv->AddAppListener(this);

   // Create child windows
   DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_VSCROLL;
   m_wndBrowser.Create(m_hWnd, rcDefault, _T("about:<html><head></head><body></body><html>"), dwStyle);

   // Prepare web-browser
   HRESULT Hr;
   m_spBrowser.Release();
   Hr = m_wndBrowser.QueryControl(IID_IWebBrowser2, (LPVOID*) &m_spBrowser);
   if( FAILED(Hr) ) return 1;

   m_spBrowser->put_RegisterAsBrowser(VARIANT_FALSE);
   m_spBrowser->put_RegisterAsDropTarget(VARIANT_FALSE);

   // Listen for IE events
   DispEventAdvise(m_spBrowser, &DIID_DWebBrowserEvents2);

   ModifyStyleEx(0, WS_EX_CLIENTEDGE);

   // Default size now; resize later
   ResizeClient(800, 600);
   CenterWindow(); 

   return lRes;
}

LRESULT CDiffCvsView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( m_spBrowser ) {
      DispEventUnadvise(m_spBrowser, &DIID_DWebBrowserEvents2);
      m_spBrowser.Release();
   }
   _pDevEnv->RemoveAppListener(this);
   bHandled = FALSE;
   return 0;
}

LRESULT CDiffCvsView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   RECT rcClient = { 0 };
   GetClientRect(&rcClient);
   m_wndBrowser.MoveWindow(&rcClient);
   return 0;
}

LRESULT CDiffCvsView::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_wndBrowser.SetFocus();
   return 0;
}

LRESULT CDiffCvsView::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == TIMERID_LOADHTML )
   {
      KillTimer(TIMERID_LOADHTML);
      _LoadHtml(m_spBrowser, m_bstrHTML);
   }
   bHandled = FALSE;
   return 0;
}


////////////////////////////////////////////////////////
// Dispatch handlers

void __stdcall CDiffCvsView::__DocumentComplete(
   /*[in]*/ IDispatch* /*pDisp*/, 
   /*[in]*/ BSTR /*bstrText*/)
{
   m_wndBrowser.SetFocus();
}

