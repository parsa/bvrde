
#include "StdAfx.h"
#include "resource.h"

#include "PreviewView.h"

#pragma code_seg( "MISC" )


// Operations

HWND CPreviewView::Create(HWND hWndParent, RECT& rcPos /*= CWindow::rcDefault*/)
{
   DWORD dwFlags = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
   return CWindowImpl<CPreviewView, CAxWindow>::Create(hWndParent, rcPos, _T("about:<html><head></head><body></body><html>"), dwFlags);
}

void CPreviewView::SetLanguage(CString& sLanguage)
{
   m_sLanguage = sLanguage;
}

BOOL CPreviewView::PreTranslateMessage(MSG* pMsg)
{
   if((pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) &&
      (pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST))
      return FALSE;

   // Give OCX a chance to translate this message
   return (BOOL) SendMessage(WM_FORWARDMSG, 0, (LPARAM) pMsg);
}

void CPreviewView::OnIdle(IUpdateUI* pUIBase)
{
   pUIBase->UIEnable(ID_FILE_SAVE, TRUE);
   pUIBase->UIEnable(ID_FILE_PRINT, TRUE);
   pUIBase->UIEnable(ID_EDIT_UNDO, FALSE);
   pUIBase->UIEnable(ID_EDIT_REDO, FALSE);
   pUIBase->UIEnable(ID_EDIT_COPY, FALSE);
   pUIBase->UIEnable(ID_EDIT_PASTE, FALSE);
   pUIBase->UIEnable(ID_EDIT_DELETE, FALSE);
   pUIBase->UIEnable(ID_EDIT_GOTO, FALSE);
   pUIBase->UIEnable(ID_EDIT_FIND, FALSE);
   pUIBase->UIEnable(ID_EDIT_REPLACE, FALSE);
   pUIBase->UIEnable(ID_EDIT_CLEAR, FALSE);
   pUIBase->UIEnable(ID_EDIT_CLEAR_ALL, FALSE);
   pUIBase->UIEnable(ID_EDIT_SELECT_ALL, FALSE);      

   pUIBase->UIEnable(ID_EDIT_BOLD, FALSE);  
   pUIBase->UIEnable(ID_EDIT_ITALIC, FALSE);  
   pUIBase->UIEnable(ID_EDIT_UNDERLINE, FALSE);  
   pUIBase->UIEnable(ID_EDIT_ALIGN_LEFT, FALSE);  
   pUIBase->UIEnable(ID_EDIT_ALIGN_MIDDLE, FALSE);  
   pUIBase->UIEnable(ID_EDIT_ALIGN_RIGHT, FALSE);  
   pUIBase->UIEnable(ID_EDIT_INDENT, FALSE);  
   pUIBase->UIEnable(ID_EDIT_UNINDENT, FALSE);  
}

CString CPreviewView::GetViewText()
{
   ATLASSERT(false);
   return _T("");
}

void CPreviewView::SetViewText(LPCTSTR pstrText)
{
   ATLASSERT(m_spBrowser);
   if( m_sLanguage == _T("html") )
   {
      CComPtr<IDispatch> spDoc;
      m_spBrowser->get_Document(&spDoc);
      if( spDoc == NULL ) return;
      int nLen = _tcslen(pstrText);
      LPSTR pstrData = (LPSTR) malloc(nLen + 1);
      if( pstrData == NULL ) return;
      AtlW2AHelper(pstrData, pstrText, nLen);
      pstrData[nLen] = '\0';
      AtlLoadHTML(spDoc, pstrData);
      free(pstrData);
   }
   else
   {
      CComBSTR bstr;

      CComPtr<IXMLDOMDocument> spXml;
      HRESULT Hr = spXml.CoCreateInstance(PROGID_XMLDOMDocument);
      if( FAILED(Hr) ) return;
      spXml->put_async(VARIANT_FALSE);
      VARIANT_BOOL vbSuccess;
      Hr = spXml->loadXML(CComBSTR(pstrText), &vbSuccess);
      if( vbSuccess == VARIANT_FALSE || FAILED(Hr) ) {
         CComPtr<IXMLDOMParseError> spErr;
         spXml->get_parseError(&spErr);
         CComBSTR bstrReason;
         CComBSTR bstrSource;
         long lLine;
         long lColumn;
         spErr->get_reason(&bstrReason);
         spErr->get_srcText(&bstrSource);
         spErr->get_line(&lLine);
         spErr->get_linepos(&lColumn);
         CString sReason = bstrReason;
         sReason.TrimRight(_T("\r\n\t "));
         CString s;
         s.Format(IDS_XML_ERR, sReason, bstrSource, lLine, lColumn);
         bstr = s;
      }
      else {
         CString sXslt = AtlLoadHTML(IDR_XML);

         CComPtr<IXMLDOMDocument> spXslt;
         Hr = spXslt.CoCreateInstance(PROGID_XMLDOMDocument);
         if( FAILED(Hr) ) return;
         spXslt->put_async(VARIANT_FALSE);
         Hr = spXslt->loadXML(CComBSTR(sXslt), &vbSuccess);
         ATLASSERT(vbSuccess!=VARIANT_FALSE);
         if( vbSuccess == VARIANT_FALSE ) return;
         if( FAILED(Hr) ) return;
         // Transform node 
         Hr = spXml->transformNode(spXslt, &bstr);
         if( FAILED(Hr) ) {
            CComPtr<IErrorInfo> spErr;
            ::GetErrorInfo(0, &spErr);
            CComBSTR bstrErr;
            spErr->GetDescription(&bstr);
         }
      }

      if( bstr.Length() == 0 ) return;

      CComPtr<IDispatch> spDoc;
      m_spBrowser->get_Document(&spDoc);
      if( spDoc == NULL ) return;

      int nLen = bstr.Length();
      LPSTR pstrData = (LPSTR) malloc(nLen + 1);
      if( pstrData == NULL ) return;
      AtlW2AHelper(pstrData, bstr, nLen);
      pstrData[nLen] = '\0';
      AtlLoadHTML(spDoc, pstrData);
      free(pstrData);
   }
   WaitBusy();
}

void CPreviewView::WaitBusy() const
{
   ATLASSERT(m_spBrowser);
   // Wait until document has been read.
   //VARIANT_BOOL vbBusy;
   //while( (m_spBrowser->get_Busy(&vbBusy)==S_OK) && vbBusy==VARIANT_TRUE ) ;
}

// Message map and handlers

LRESULT CPreviewView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   // Create the control for me
   LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);

   HRESULT Hr;
   m_spBrowser.Release();
   Hr = QueryControl(IID_IWebBrowser, (LPVOID *) &m_spBrowser);
   if( FAILED(Hr) ) return 1;

   // Turn off text selection and right-click menu
   CComPtr<IAxWinAmbientDispatch> spHost;
   Hr = QueryHost(IID_IAxWinAmbientDispatch, (LPVOID*) &spHost);
   if( SUCCEEDED(Hr) ) {
      spHost->put_AllowContextMenu(VARIANT_FALSE);
      spHost->put_DocHostFlags(DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_DISABLE_HELP_MENU);
   }

   ModifyStyleEx(0, WS_EX_CLIENTEDGE);
   
   return lRes;
}

LRESULT CPreviewView::OnFilePrint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
  spOleTarget->Exec(NULL, OLECMDID_PRINT, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
  return 0;
}

