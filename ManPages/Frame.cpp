
#include "StdAfx.h"
#include "resource.h"

#include "Frame.h"
#include "Generator.h"

#include <initguid.h>
DEFINE_GUID(CGID_IWebBrowserPriv,0xED016940L,0xBD5B,0x11cf,0xBA,0x4E,0x00,0xC0,0x4F,0xD7, 0x08,0x16);


////////////////////////////////////////////////////////
// Operations

BOOL CFrameWindow::SetPage(LPCTSTR pstrKeyword, LPCTSTR pstrLanguage, long lPos)
{
   ATLASSERT(::IsWindow(m_hWnd));
   ATLASSERT(m_wndBrowser.IsWindow());
   ATLASSERT(m_spBrowser);

   CWaitCursor cursor;

   CComBSTR bstr;
   long lCount = 0;
   CManPageGenerator page;
   UINT nRes = page.Generate(m_hWnd, pstrKeyword, pstrLanguage, lPos, lCount, bstr);
   if( nRes != 0 ) {
      ShowWindow(SW_HIDE);
      if( nRes == IDS_ERR_NOTFOUND ) {
         // Show a regular message box here.
         // (Don't know why; it just feels better?)
         AtlMessageBox(m_hWnd, nRes, IDS_CAPTION_MESSAGE, MB_ICONINFORMATION);
      }
      else {
         // Display error message
         _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(nRes)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
      }
      PostMessage(WM_CLOSE, 0, 0L);
      return FALSE;
   }
   _LoadHtml(m_spBrowser, bstr);

   m_lMatchPos = lPos;
   m_nMatchCount = lCount;
   m_bstrKeyword = pstrKeyword;
   m_bstrLanguage = pstrLanguage;

   CString sTitle;
   sTitle.Format(IDS_CAPTION_KEYWORD, pstrKeyword);
   SetWindowText(sTitle);

   PAGEINFO info;
   info.bstrKeyword = m_bstrKeyword;
   info.bstrLanguage = m_bstrLanguage;
   info.lPos = lPos;
   m_aStack.Add(info);

   _UpdateButtons();

   return TRUE;
}

void CFrameWindow::OnFinalMessage(HWND /*hWnd*/)
{
   delete this;
}

LRESULT CFrameWindow::OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   if( uMsg != WM_COMMAND || !IsWindowVisible() || !::IsChild(m_hWnd, ::GetFocus()) ) {
      bHandled = FALSE;
      return 0;
   }
   LRESULT lResult = 0;
   bHandled = ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult);
   return lResult;
}

BOOL CFrameWindow::PreTranslateMessage(MSG* pMsg)
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

BOOL CFrameWindow::_LoadHtml(IUnknown* pUnk, LPCWSTR pstrHTML)
{
   m_bstrHTML = pstrHTML;
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

void CFrameWindow::_UpdateButtons()
{
   m_ctrlToolBar.SetState(ID_VIEW_BACK, m_aStack.GetSize() > 1 ? TBSTATE_ENABLED : 0);
   m_ctrlToolBar.SetState(ID_VIEW_PREVIOUS, m_lMatchPos > 0 ? TBSTATE_ENABLED : 0);
   m_ctrlToolBar.SetState(ID_VIEW_NEXT, m_lMatchPos < m_nMatchCount - 1 && m_nMatchCount > 1 ? TBSTATE_ENABLED : 0);
}


////////////////////////////////////////////////////////
// Message map

LRESULT CFrameWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
   LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);

   _pDevEnv->AddAppListener(this);

   // Create child windows
   DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | CCS_TOP | TBSTYLE_LIST;
   m_ctrlToolBar.Create(m_hWnd, rcDefault, NULL, dwStyle);
   dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_VSCROLL;
   m_wndBrowser.Create(m_hWnd, rcDefault, _T("about:<html><head></head><body></body><html>"), dwStyle);

   m_ctrlToolBar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);
   m_ctrlToolBar.SetButtonStructSize();

   CString sTextClose(MAKEINTRESOURCE(IDS_CLOSE));
   CString sTextPrint(MAKEINTRESOURCE(IDS_PRINT));
   CString sTextPrevious(MAKEINTRESOURCE(IDS_PREVIOUS));
   CString sTextNext(MAKEINTRESOURCE(IDS_NEXT));
   CString sTextBack(MAKEINTRESOURCE(IDS_BACK));

   // Toolbar buttons
   TBBUTTON buttons[5] = { 0, 0, 0, 0, 0 };
   buttons[0].iBitmap = -1;
   buttons[0].idCommand = ID_FILE_CLOSE;
   buttons[0].fsState = TBSTATE_ENABLED;
   buttons[0].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | BTNS_SHOWTEXT;
   buttons[0].dwData = 0;
   buttons[0].iString = (INT_PTR) (LPCTSTR) sTextClose;
   buttons[1].iBitmap = -1;
   buttons[1].idCommand = ID_FILE_PRINT;
   buttons[1].fsState = TBSTATE_ENABLED;
   buttons[1].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | BTNS_SHOWTEXT;
   buttons[1].dwData = 0;
   buttons[1].iString = (INT_PTR) (LPCTSTR) sTextPrint;
   buttons[2].iBitmap = -1;
   buttons[2].idCommand = ID_VIEW_BACK;
   buttons[2].fsState = TBSTATE_ENABLED;
   buttons[2].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | BTNS_SHOWTEXT;
   buttons[2].dwData = 0;
   buttons[2].iString = (INT_PTR) (LPCTSTR) sTextBack;
   buttons[3].iBitmap = -1;
   buttons[3].idCommand = ID_VIEW_PREVIOUS;
   buttons[3].fsState = TBSTATE_ENABLED;
   buttons[3].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | BTNS_SHOWTEXT;
   buttons[3].dwData = 0;
   buttons[3].iString = (INT_PTR) (LPCTSTR) sTextPrevious;
   buttons[4].iBitmap = -1;
   buttons[4].idCommand = ID_VIEW_NEXT;
   buttons[4].fsState = TBSTATE_ENABLED;
   buttons[4].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | BTNS_SHOWTEXT;
   buttons[4].dwData = 0;
   buttons[4].iString = (INT_PTR) (LPCTSTR) sTextNext;
   m_ctrlToolBar.AddButtons(5, buttons);

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

   int cxChar = (int) LOWORD(GetDialogBaseUnits());
   int cyChar = (int) HIWORD(GetDialogBaseUnits());

   RECT rcDesktop = { 0 };
   ::GetWindowRect(::GetDesktopWindow(), &rcDesktop);
   RECT rcWindow = { rcDesktop.right / 5, 40 };
   MoveWindow(&rcWindow);
   ResizeClient(cxChar * 62, cyChar * 32);

   return lRes;
}

LRESULT CFrameWindow::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( m_spBrowser ) {
      DispEventUnadvise(m_spBrowser, &DIID_DWebBrowserEvents2);
      m_spBrowser.Release();
   }
   _pDevEnv->RemoveAppListener(this);
   bHandled = FALSE;
   return 0;
}

LRESULT CFrameWindow::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   RECT rcClient = { 0 };
   GetClientRect(&rcClient);
   RECT rcToolBar = { 0, 0, rcClient.right, 0 };
   m_ctrlToolBar.MoveWindow(&rcToolBar);
   m_ctrlToolBar.GetWindowRect(&rcToolBar);
   RECT rcBrowser = { rcClient.left, rcToolBar.bottom - rcToolBar.top, rcClient.right, rcClient.bottom };
   m_wndBrowser.MoveWindow(&rcBrowser);
   return 0;
}

LRESULT CFrameWindow::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == TIMERID_LOADHTML ) 
   {
      KillTimer(TIMERID_LOADHTML);
      _LoadHtml(m_spBrowser, m_bstrHTML);
   }     
   bHandled = FALSE;
   return 0;
}

LRESULT CFrameWindow::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_wndBrowser.SetFocus();
   return 0;
}

LRESULT CFrameWindow::OnFilePrint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(NULL, OLECMDID_PRINT, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CFrameWindow::OnFileClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   PostMessage(WM_CLOSE);
   return 0;
}

LRESULT CFrameWindow::OnEditFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   const DWORD HTMLID_FIND = 1;
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(&CGID_IWebBrowserPriv, HTMLID_FIND, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CFrameWindow::OnViewPrevious(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   SetPage(m_bstrKeyword, m_bstrLanguage, m_lMatchPos - 1);
   return 0;
}

LRESULT CFrameWindow::OnViewNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   SetPage(m_bstrKeyword, m_bstrLanguage, m_lMatchPos + 1);
   return 0;
}

LRESULT CFrameWindow::OnViewBack(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( m_aStack.GetSize() <= 1 ) return 0;
   int iPos = m_aStack.GetSize() - 2;
   PAGEINFO info = m_aStack[iPos];
   m_aStack.RemoveAt(iPos);
   m_aStack.RemoveAt(iPos);
   SetPage(info.bstrKeyword, info.bstrLanguage, info.lPos);
   return 0;
}


////////////////////////////////////////////////////////
// Dispatch handlers

void __stdcall CFrameWindow::__BeforeNavigate2(
   /*[in]*/ IDispatch* /*pDisp*/, 
   /*[in]*/ VARIANT* URL, 
   /*[in]*/ VARIANT* /*Flags*/, 
   /*[in]*/ VARIANT* /*TargetFrameName*/, 
   /*[in]*/ VARIANT* PostData, 
   /*[in]*/ VARIANT* Headers, 
   /*[out]*/ VARIANT_BOOL* Cancel)
{
   ATLASSERT(V_VT(URL) == VT_BSTR);
   ATLASSERT(V_VT(TargetFrameName) == VT_BSTR);
   ATLASSERT(V_VT(PostData) == (VT_VARIANT | VT_BYREF)); PostData;
   ATLASSERT(V_VT(Headers) == VT_BSTR); Headers;
   ATLASSERT(Cancel != NULL);

   CComBSTR bstrURL = V_BSTR(URL);
   bool bCancel = true;

   if( wcsnicmp(bstrURL, L"ftp:", wcslen(L"ftp:")) == 0 ) bCancel = false;
   if( wcsnicmp(bstrURL, L"mailto:", wcslen(L"mailto:")) == 0 ) bCancel = false;
   if( wcsnicmp(bstrURL, L"http://www", wcslen(L"http://www")) == 0 ) bCancel = false;

   // This is a local link
   if( wcsnicmp(bstrURL, L"about:blank#", wcslen(L"about:blank#")) == 0 ) {
      // FIX: The IE WebBrowser fails to navigate to a named anchor because
      //      we never loaded a real webpage in the first place. Instead
      //      it claims that it should navigate to 'about:blank#anchorname'.
      //      We'll have to scroll to the anchor ourselves.
      CComPtr<IDispatch> spDisp;
      m_spBrowser->get_Document(&spDisp);
      if( spDisp != NULL ) {
         CComQIPtr<IHTMLDocument2> spDoc = spDisp;
         CComPtr<IHTMLElementCollection> spAnchors;
         spDoc->get_anchors(&spAnchors);
         if( spAnchors ) {
            spDisp.Release();
            CComVariant vName = bstrURL + wcslen(L"about:blank#");
            CComVariant vEmpty;
            vEmpty.vt = VT_ERROR;
            spAnchors->item(vName, vEmpty, &spDisp);
            if( spDisp != NULL ) {
               CComQIPtr<IHTMLElement> spElement = spDisp;
               long lScroll = 0;
               spElement->get_offsetTop(&lScroll);
               CComPtr<IHTMLWindow2> spWindow;
               spDoc->get_parentWindow(&spWindow);
               if( spWindow != NULL ) spWindow->scrollTo(0L, lScroll);
            }
         }
      }
   }

   // The Index closes the page
   if( wcsicmp(bstrURL, L"http://localhost/cgi-bin/man/man2html") == 0 ) {
      PostMessage(WM_CLOSE, 0, 0L);
   }

   // Trying to view a file; let's load it in the editor instead
   if( wcsnicmp(bstrURL, L"file:", wcslen(L"file:")) == 0 ) {
      _pDevEnv->CreateView(OLE2CT(bstrURL.m_str + wcslen(L"file:")));
   }

   // Loading a new man page?
   if( wcsnicmp(bstrURL, L"http://localhost", wcslen(L"http://localhost")) == 0 ) {
      while( wcschr(bstrURL, '+') != NULL ) *(wcschr(bstrURL, '+')) = ' ';
      if( wcschr(bstrURL, '?') ) SetPage(wcschr(bstrURL, '?') + 1, NULL, -1);
   }
   if( wcsnicmp(bstrURL, L"method://", wcslen(L"method://")) == 0 ) {
      while( wcschr(bstrURL, '+') != NULL ) *(wcschr(bstrURL, '+')) = ' ';
      SetPage(bstrURL + 10, NULL, -1);
   }

   *Cancel = bCancel ? VARIANT_TRUE : VARIANT_FALSE;
}

void __stdcall CFrameWindow::__DocumentComplete(
   /*[in]*/ IDispatch* /*pDisp*/, 
   /*[in]*/ BSTR /*bstrText*/)
{
   m_wndBrowser.SetFocus();
}

