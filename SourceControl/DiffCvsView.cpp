
#include "StdAfx.h"
#include "resource.h"

#include "DiffCvsView.h"


////////////////////////////////////////////////////////
// Operations

BOOL CDiffCvsView::GeneratePage(IElement* pElement, CSimpleArray<CString>& aLines)
{
   ATLASSERT(::IsWindow(m_hWnd));
   ATLASSERT(m_wndBrowser.IsWindow());
   ATLASSERT(m_spBrowser);
   ATLASSERT(pElement);

   CWaitCursor cursor;

   CComDispatchDriver dd = pElement->GetDispatch();  
   CComVariant vText;
   if( FAILED( dd.GetPropertyByName(L"Text", &vText) ) ) return FALSE;
   if( vText.vt != VT_BSTR || vText.bstrVal == NULL ) return FALSE;
   CString sFile = vText.bstrVal;

   // Detect which type of diff output this is:
   //   Original diff
   //   Context diff
   //   Unidiff
   enum { DIFF_ORIGINAL, DIFF_CONTEXT, DIFF_UNIDIFF } DiffType = DIFF_ORIGINAL;
   const int NUM_LINES_DETECT = 20;
   int nLines = aLines.GetSize();
   for( int i = 0; i < min(nLines, NUM_LINES_DETECT); i++ ) {
      if( aLines[i].Left(4) == _T("*** ") ) DiffType = DIFF_CONTEXT;
      if( aLines[i].Left(4) == _T("+++ ") ) DiffType = DIFF_UNIDIFF;
      if( aLines[i].Left(3) == _T("@@ ") ) DiffType = DIFF_UNIDIFF;
   }

   // Split source file into lines
   CSimpleArray<CString> aFile;
   CString sLine;
   while( !sFile.IsEmpty() ) {
      sLine = sFile.SpanExcluding(_T("\r\n"));
      aFile.Add(sLine);
      int iPos = sLine.GetLength();
      int nLen = sFile.GetLength();
      while( iPos < nLen && (sFile[iPos] == '\r' || sFile[iPos] == '\n') ) iPos++;
      sFile = sFile.Mid(iPos);
   }

   CString sHTML;
   if( DiffType == DIFF_ORIGINAL ) _ParseDiffOriginal(aFile, aLines, sHTML);
   if( DiffType == DIFF_CONTEXT ) _ParseDiffContext(aFile, aLines, sHTML);
   if( DiffType == DIFF_UNIDIFF ) _ParseDiffUnidiff(aFile, aLines, sHTML);
   if( sHTML.IsEmpty() ) return FALSE;

   CString sPage;

   ShowWindow(SW_NORMAL);
   return TRUE;
}

BOOL CDiffCvsView::_ParseDiffOriginal(CSimpleArray<CString>& aFile, CSimpleArray<CString>& aLines, CString& sHTML)
{
   return FALSE;
}

BOOL CDiffCvsView::_ParseDiffContext(CSimpleArray<CString>& aFile, CSimpleArray<CString>& aLines, CString& sHTML)
{
   return FALSE;
}

BOOL CDiffCvsView::_ParseDiffUnidiff(CSimpleArray<CString>& aFile, CSimpleArray<CString>& aLines, CString& sHTML)
{
   return FALSE;
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
   bHandled = FALSE;
   if( wParam != TIMERID_LOADHTML ) return 0;
   KillTimer(TIMERID_LOADHTML);
   _LoadHtml(m_spBrowser, m_bstrHTML);
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

