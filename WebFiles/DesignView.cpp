
#include "StdAfx.h"
#include "resource.h"

#include "DesignView.h"

// Platform SDK header for MSHTML OLECMD commands
#include <MSHTMCID.H>

#pragma code_seg( "MISC" )


// Operations

HWND CDesignView::Create(HWND hWndParent, RECT& rcPos /*= CWindow::rcDefault*/)
{
   DWORD dwFlags = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL;
   return CWindowImpl<CDesignView, CAxWindow>::Create(hWndParent, rcPos, _T("about:<html><head></head><body></body><html>"), dwFlags);
}

BOOL CDesignView::PreTranslateMessage(MSG* pMsg)
{
   if( (pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) &&
      (pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST) )
   {
      return FALSE;
   }

   if( (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP ) && pMsg->wParam == VK_DELETE ) 
      return (BOOL) SendMessage(WM_FORWARDMSG, 0, (LPARAM) pMsg);

   // Give OCX a chance to translate this message
   //return (BOOL) SendMessage(WM_FORWARDMSG, 0, (LPARAM) pMsg);
   return FALSE;
}

void CDesignView::SetLanguage(CString& sLanguage)
{
   m_sLanguage = sLanguage;
}

CString CDesignView::GetViewText()
{
   ATLASSERT(m_spBrowser);
   CComPtr<IDispatch> spDoc;
   m_spBrowser->get_Document(&spDoc);
   if( spDoc == NULL ) return _T("");
   CComPtr<IStream> spStream;
   HRESULT Hr = ::CreateStreamOnHGlobal(NULL, FALSE, &spStream);
   if( FAILED(Hr) ) return _T("");
   CComQIPtr<IPersistStreamInit> spPersist = spDoc;
   if( spPersist == NULL ) return _T("");
   Hr = spPersist->Save(spStream, TRUE);
   if( FAILED(Hr) ) return _T("");
   LARGE_INTEGER ulMove;
   ULARGE_INTEGER ulPos;
   ulPos.QuadPart = 0;
   ulMove.QuadPart = 0;
   spStream->Seek(ulMove, STREAM_SEEK_END, &ulPos);
   spStream->Seek(ulMove, STREAM_SEEK_SET, NULL);
   DWORD dwSize = ulPos.LowPart;
   LPSTR pstrData = (LPSTR) malloc(dwSize + 1);
   if( pstrData == NULL ) return _T("");
   spStream->Read(pstrData, dwSize, NULL);
   pstrData[dwSize] = '\0';
   CString sText = pstrData;
   free(pstrData);
   return sText;
}

void CDesignView::SetViewText(LPCTSTR pstrText)
{
   ATLASSERT(m_spBrowser);
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

void CDesignView::WaitBusy() const
{
   ATLASSERT(m_spBrowser);
   // Wait until document has been read.
   //VARIANT_BOOL vbBusy;
   //while( (m_spBrowser->get_Busy(&vbBusy)==S_OK) && vbBusy==VARIANT_TRUE ) ;
}

void CDesignView::OnIdle(IUpdateUI* pUIBase)
{
   static OLECMD nOleCmds[] = 
   { 
      { OLECMDID_COPY, 0 },
      { OLECMDID_CUT, 0 },
      { OLECMDID_PASTE, 0 },
      { OLECMDID_UNDO, 0 },
      { OLECMDID_REDO, 0 },
      { OLECMDID_PRINT, 0 },
      { OLECMDID_DELETE, 0 },
      { OLECMDID_SELECTALL, 0 },
   };
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   if( spOleTarget == NULL ) return;
   spOleTarget->QueryStatus(NULL, sizeof(nOleCmds)/sizeof(OLECMD), nOleCmds, NULL);

   pUIBase->UIEnable(ID_FILE_SAVE, TRUE);
   pUIBase->UIEnable(ID_FILE_PRINT, nOleCmds[5].cmdf & OLECMDF_ENABLED);
   pUIBase->UIEnable(ID_EDIT_UNDO, nOleCmds[3].cmdf & OLECMDF_ENABLED);
   pUIBase->UIEnable(ID_EDIT_REDO, nOleCmds[4].cmdf & OLECMDF_ENABLED);
   pUIBase->UIEnable(ID_EDIT_COPY, nOleCmds[0].cmdf & OLECMDF_ENABLED);
   pUIBase->UIEnable(ID_EDIT_CUT, nOleCmds[1].cmdf & OLECMDF_ENABLED);
   pUIBase->UIEnable(ID_EDIT_PASTE, nOleCmds[2].cmdf & OLECMDF_ENABLED);
   pUIBase->UIEnable(ID_EDIT_DELETE, nOleCmds[6].cmdf & OLECMDF_ENABLED);
   pUIBase->UIEnable(ID_EDIT_SELECT_ALL, nOleCmds[7].cmdf & OLECMDF_ENABLED);
   pUIBase->UIEnable(ID_EDIT_GOTO, FALSE);
   pUIBase->UIEnable(ID_EDIT_FIND, FALSE);
   pUIBase->UIEnable(ID_EDIT_REPLACE, FALSE);
   pUIBase->UIEnable(ID_EDIT_CLEAR, FALSE);
   pUIBase->UIEnable(ID_EDIT_CLEAR_ALL, FALSE);

   static OLECMD nHtmlCmds[] = 
   { 
      { IDM_BOLD, 0 },
      { IDM_ITALIC, 0 },
      { IDM_UNDERLINE, 0 },
      { IDM_JUSTIFYLEFT, 0 },
      { IDM_JUSTIFYCENTER, 0 },
      { IDM_JUSTIFYRIGHT, 0 },
      { IDM_INDENT, 0 },
      { IDM_OUTDENT, 0 },
   };
   spOleTarget->QueryStatus(&CGID_MSHTML, sizeof(nHtmlCmds)/sizeof(OLECMD), nHtmlCmds, NULL);
   pUIBase->UIEnable(ID_EDIT_BOLD, nHtmlCmds[0].cmdf & OLECMDF_ENABLED);  
   pUIBase->UIEnable(ID_EDIT_ITALIC, nHtmlCmds[1].cmdf & OLECMDF_ENABLED);  
   pUIBase->UIEnable(ID_EDIT_UNDERLINE, nHtmlCmds[2].cmdf & OLECMDF_ENABLED);  
   pUIBase->UIEnable(ID_EDIT_ALIGN_LEFT, nHtmlCmds[3].cmdf & OLECMDF_ENABLED);  
   pUIBase->UIEnable(ID_EDIT_ALIGN_MIDDLE, nHtmlCmds[4].cmdf & OLECMDF_ENABLED);  
   pUIBase->UIEnable(ID_EDIT_ALIGN_RIGHT, nHtmlCmds[5].cmdf & OLECMDF_ENABLED);  
   pUIBase->UIEnable(ID_EDIT_INDENT, nHtmlCmds[6].cmdf & OLECMDF_ENABLED);  
   pUIBase->UIEnable(ID_EDIT_UNINDENT, nHtmlCmds[7].cmdf & OLECMDF_ENABLED);  
}

// Message map and handlers

LRESULT CDesignView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   // Create the control for me
   LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);

   HRESULT Hr;
   m_spBrowser.Release();
   Hr = QueryControl(IID_IWebBrowser2, (LPVOID*) &m_spBrowser);
   if( FAILED(Hr) ) return 1;

   CComPtr<IDispatch> spDisp;
   m_spBrowser->get_Document(&spDisp);
   if( spDisp == NULL ) return 0;
   CComQIPtr<IHTMLDocument2> spDoc = spDisp;
   if( spDoc == NULL ) return 0;
   
   // Enter design-mode for web-browser.
   spDoc->put_designMode(L"On");

   ModifyStyleEx(0, WS_EX_CLIENTEDGE);

   return lRes;
}

LRESULT CDesignView::OnFilePrint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   // &CGID_MSHTML
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(NULL, OLECMDID_PRINT, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditUndo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(NULL, OLECMDID_UNDO, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditRedo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(NULL, OLECMDID_REDO, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(NULL, OLECMDID_COPY, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditCut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(NULL, OLECMDID_CUT, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditPaste(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(NULL, OLECMDID_PASTE, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(NULL, OLECMDID_FIND, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(NULL, OLECMDID_SELECTALL, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditBold(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(&CGID_MSHTML, IDM_BOLD, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditItalic(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(&CGID_MSHTML, IDM_ITALIC, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditUnderline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(&CGID_MSHTML, IDM_UNDERLINE, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditAlignLeft(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(&CGID_MSHTML, IDM_JUSTIFYLEFT, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditAlignMiddle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(&CGID_MSHTML, IDM_JUSTIFYCENTER, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditAlignRight(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(&CGID_MSHTML, IDM_JUSTIFYRIGHT, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditIndent(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(&CGID_MSHTML, IDM_INDENT, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}

LRESULT CDesignView::OnEditUnindent(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CComQIPtr<IOleCommandTarget> spOleTarget = m_spBrowser;
   spOleTarget->Exec(&CGID_MSHTML, IDM_OUTDENT, MSOCMDEXECOPT_DODEFAULT, NULL, NULL);
   return 0;
}
