
#include "StdAfx.h"

#pragma code_seg( "MISC" )

#include "View.h"


///////////////////////////////////////////////////////7
// CStartPageView

BOOL CStartPageView::PreTranslateMessage(MSG* pMsg)
{
   if( ::GetFocus() != m_hWnd ) return FALSE;

   if( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F1 ) return FALSE;

   if( (pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) 
       && (pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST) ) 
   {
      return FALSE;
   }

   // Give OCX a chance to translate this message
   return (BOOL) SendMessage(WM_FORWARDMSG, 0, (LPARAM) pMsg);
}


///////////////////////////////////////////////////////7
// IView

BOOL CStartPageView::GetName(LPTSTR pstrName, UINT cchMax) const
{
   _tcsncpy(pstrName, _T("Start Page"), cchMax);
   return TRUE;
}

BOOL CStartPageView::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Start Page"), cchMax);
   return TRUE;
}

IDispatch* CStartPageView::GetDispatch()
{
   return _pDevEnv->CreateStdDispatch(_T("View"), this);
}

BOOL CStartPageView::IsDirty() const
{
   return FALSE;
}

BOOL CStartPageView::Load(ISerializable* pArc)
{
   return TRUE;
}

BOOL CStartPageView::Save(ISerializable* pArc)
{
   TCHAR szName[64] = { 0 };
   GetName(szName, 63);
   pArc->Write(_T("name"), szName);
   return TRUE;
}

void CStartPageView::ActivateUI()
{
}

void CStartPageView::DeactivateUI()
{
}

void CStartPageView::EnableModeless(BOOL bEnable)
{
}

IElement* CStartPageView::GetParent() const
{
   return NULL;
}

BOOL CStartPageView::SetName(LPCTSTR pstrName)
{
   return FALSE;
}

BOOL CStartPageView::Save()
{
   ATLASSERT(false);
   return TRUE;
}

BOOL CStartPageView::OpenView(long lLineNum)
{
   if( IsWindow() ) {
      ShowWindow(SW_SHOW);
   }
   else {
      TCHAR szName[64] = { 0 };
      GetName(szName, 63);

      IViewFrame* pFrame = _pDevEnv->CreateClient(szName, NULL, this);
      ATLASSERT(pFrame);
      if( pFrame == NULL ) {
         ::SetLastError(ERROR_OUTOFMEMORY);
         return FALSE;
      }
      m_wndFrame = pFrame->GetHwnd();

      // Create CLSID_WebBrowser control
      Create(m_wndFrame, CWindow::rcDefault, _T("{8856F961-340A-11D0-A96B-00C04FD705A2}"), WS_CHILD | WS_VISIBLE, WS_EX_CLIENTEDGE);
      ATLASSERT(IsWindow());
      if( !IsWindow() ) return FALSE;
      pFrame->SetClient(m_hWnd);
   }
   m_wndFrame.PostMessage(WM_SETFOCUS);
   return TRUE;
}

void CStartPageView::CloseView()
{
   if( m_wndFrame.IsWindow() ) _pDevEnv->DestroyClient(m_hWnd);
}

BOOL CStartPageView::GetText(BSTR* /*pbstrText*/)
{
   return FALSE;
}

BOOL CStartPageView::GetFileName(LPTSTR /*pstrName*/, UINT /*cchMax*/) const
{
   return FALSE;
}

LRESULT CStartPageView::PostMessage(UINT /*uMsg*/, WPARAM /*wParam = 0*/, LPARAM /*lParam = 0*/)
{
   return 0;
}

LRESULT CStartPageView::SendMessage(UINT /*uMsg*/, WPARAM /*wParam = 0*/, LPARAM /*lParam = 0*/)
{
   return 0;
}


///////////////////////////////////////////////////////7
// Message handlers

LRESULT CStartPageView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
   LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);
   
   // Get the web-browser control
   HRESULT Hr;
   CComPtr<IUnknown> spUnk;
   Hr = QueryControl(IID_IUnknown, (LPVOID*) &spUnk);
   ATLASSERT(SUCCEEDED(Hr));
   if( FAILED(Hr) ) return 0;

   CComQIPtr<IServiceProvider> spSP = spUnk;
   ATLASSERT(spSP);
   if( spSP == NULL ) return 0;

   // Get to the IE Web Browser
   CComPtr<IWebBrowserApp> spWebApp;
   Hr = spSP->QueryService(IID_IWebBrowserApp, &spWebApp);
   ATLASSERT(SUCCEEDED(Hr));
   if( FAILED(Hr) ) return 0;
   m_spBrowser = spWebApp;
   if( m_spBrowser == NULL ) return 0;

   m_spBrowser->put_RegisterAsBrowser(VARIANT_FALSE);
   m_spBrowser->put_RegisterAsDropTarget(VARIANT_FALSE);

   // Attach our scripting interface
   SetExternalDispatch(_pDevEnv->GetDispatch());

   // Turn off text selection and right-click menu
   CComPtr<IAxWinAmbientDispatch> spHost;
   Hr = QueryHost(IID_IAxWinAmbientDispatch, (LPVOID*) &spHost);
   if( SUCCEEDED(Hr) ) {
      spHost->put_AllowContextMenu(VARIANT_FALSE);
      spHost->put_DocHostFlags(DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_DISABLE_HELP_MENU);
   }

   // Listen for IE events
   DispEventAdvise(m_spBrowser, &DIID_DWebBrowserEvents2);

   // Display HTML page
   CComVariant vURL = L"res://startpage.pkg/index.html";
   CComVariant vEmpty;
   Hr = m_spBrowser->Navigate2(&vURL, &vEmpty, &vEmpty, &vEmpty, &vEmpty);

   return lRes;
}

LRESULT CStartPageView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( m_spBrowser ) {
      DispEventUnadvise(m_spBrowser, &DIID_DWebBrowserEvents2);
      m_spBrowser.Release();
   }
   bHandled = FALSE;
   return 0;
}


///////////////////////////////////////////////////////7
// Dispatch handlers

void __stdcall CStartPageView::__BeforeNavigate2(/*[in]*/ IDispatch* pDisp, 
   /*[in]*/ VARIANT* URL, 
   /*[in]*/ VARIANT* Flags, 
   /*[in]*/ VARIANT* TargetFrameName, 
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
   bool bCancel = false;

   if( wcsnicmp(bstrURL, L"bvrde://", 8) == 0 ) {
      CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
      if( wcsnicmp(bstrURL, L"bvrde://wizard", 14) == 0 ) {
         wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_STARTWIZARD, 0));
      }
      if( wcsnicmp(bstrURL, L"bvrde://empty", 13) == 0 ) {
         wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_CLOSESOLUTION, 0));
         m_wndFrame.PostMessage(WM_CLOSE);
      }
      if( wcsnicmp(bstrURL, L"bvrde://load", 12) == 0 ) {
         wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_OPENSOLUTION, 0));
      }
      bCancel = true;
   }
   if( wcsnicmp(bstrURL, L"project://", 10) == 0 ) {
      CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
      CString sFilename = bstrURL + 10;
      sFilename.TrimRight(_T("/"));
      // Send private message instead of calling LoadSolution() directly because
      // it doesn't update the MRU history...
      enum { WM_APP_LOADSOLUTION = WM_APP + 1000 };
      if( wndMain.SendMessage(WM_APP_LOADSOLUTION, 0, (LPARAM) (LPCTSTR) sFilename) ) {
         m_wndFrame.PostMessage(WM_CLOSE);
      }
      bCancel = true;
   }

   *Cancel = bCancel ? VARIANT_TRUE : VARIANT_FALSE;
}

void __stdcall CStartPageView::__DocumentComplete(/*[in]*/ IDispatch* pDisp, 
   /*[in]*/ BSTR bstrText)
{
   enum { WM_APP_IDLETIME = WM_APP + 1001 };
   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
   wndMain.PostMessage(WM_APP_IDLETIME);
}
