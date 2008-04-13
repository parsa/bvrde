
#include "StdAfx.h"
#include "resource.h"

#include "Files.h"

#pragma code_seg( "MISC" )


///////////////////////////////////////////////////////7
// CViewImpl 

CViewImpl::CViewImpl(IProject* pProject, IElement* pParent, LPCTSTR pstrFilename) : 
   m_pProject(pProject),
   m_pParent(pParent),
   m_sFilename(pstrFilename)
{
   TCHAR szName[MAX_PATH];
   _tcscpy(szName, m_sFilename);
   ::PathStripPath(szName);
   m_sName = szName;
}

BOOL CViewImpl::GetName(LPTSTR pstrName, UINT cchMax) const
{
   ATLASSERT(pstrName);
   _tcsncpy(pstrName, m_sName, cchMax);
   return TRUE;
}

IDispatch* CViewImpl::GetDispatch()
{
   return _pDevEnv->CreateStdDispatch(_T("View"), this);
}

BOOL CViewImpl::GetText(BSTR* pbstrText)
{
   if( m_wndFrame.IsWindow() ) {
      CComBSTR bstr = m_wndClient.GetViewText();
      *pbstrText = bstr.Detach();
   }
   else {
      CFile f;
      if( !f.Open(m_sFilename) ) return FALSE;
      DWORD dwSize = f.GetSize();
      LPBYTE pData = (LPBYTE) malloc(dwSize + 1);
      if( pData == NULL ) {
         ::SetLastError(ERROR_OUTOFMEMORY);
         return FALSE;
      }
      if( !f.Read(pData, dwSize) ) {
         free(pData);
         ::SetLastError(ERROR_LOCK_VIOLATION);
         return FALSE;
      }
      f.Close();
      pData[dwSize] = _T('\0');
      CComBSTR bstr = (LPSTR) pData;
      *pbstrText = bstr.Detach();
      free(pData);
   }
   return TRUE;
}

BOOL CViewImpl::GetFileName(LPTSTR pstrName, UINT cchMax) const
{
   ATLASSERT(pstrName);
   _tcscpy(pstrName, m_sFilename);
   return TRUE;
}

IElement* CViewImpl::GetParent() const
{
   return m_pParent;
}

BOOL CViewImpl::SetName(LPCTSTR pstrName)
{
   return FALSE;
}

BOOL CViewImpl::Load(ISerializable* pArc)
{
   pArc->Read(_T("name"), m_sName.GetBufferSetLength(128), 128);
   m_sName.ReleaseBuffer();
   pArc->Read(_T("filename"), m_sFilename.GetBufferSetLength(MAX_PATH), MAX_PATH);
   m_sFilename.ReleaseBuffer();
   return TRUE;
}

BOOL CViewImpl::Save(ISerializable* pArc)
{
   CString sType;
   GetType(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();

   pArc->Write(_T("name"), m_sName);
   pArc->Write(_T("type"), sType);
   pArc->Write(_T("filename"), m_sFilename);
   return TRUE;
}

BOOL CViewImpl::Save()
{
   if( !m_wndClient.IsWindow() ) return FALSE;

   CString sText = m_wndClient.GetViewText();

   int nLen = sText.GetLength();
   LPSTR pstrData = (LPSTR) malloc(nLen + 1);
   if( pstrData == NULL ) {
      ::SetLastError(ERROR_OUTOFMEMORY);
      return FALSE;
   }
   AtlW2AHelper(pstrData, sText, nLen);
   pstrData[nLen] = '\0';

   CFile f;
   if( !f.Create(m_sFilename) ) return FALSE;
   if( !f.Write(pstrData, nLen) ) return FALSE;
   f.Close();
   free(pstrData);

   m_wndClient.m_wndSource.SetSavePoint();
   
   // Save Clears Undo stack?
   TCHAR szBuffer[32] = { 0 };
   _pDevEnv->GetProperty(_T("gui.document.clearUndo"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("true")) == 0 ) m_wndClient.m_wndSource.EmptyUndoBuffer();

   return TRUE;
}

BOOL CViewImpl::Reload()
{
   return FALSE;
}

BOOL CViewImpl::IsDirty() const
{
   return m_wndClient.IsDirty();
}

BOOL CViewImpl::OpenView(long lLineNum)
{
   if( m_wndFrame.IsWindow() ) 
   {
      m_wndFrame.SetFocus();
   }
   else 
   {
      m_wndClient.SetFilename(m_sFilename);
      m_wndClient.SetLanguage(m_sLanguage);

      // Load the file (local file or from remote server)
      // Allow a file operation to fail if line number is 0, so a non-existing
      // file can actually open...
      CComBSTR bstrText;
      if( !GetText(&bstrText) && lLineNum > 0 ) return FALSE;

      CString sName;
      GetName(sName.GetBufferSetLength(128), 128);
      sName.ReleaseBuffer();

      IViewFrame* pFrame = _pDevEnv->CreateClient(sName, m_pProject, this);
      ATLASSERT(pFrame);
      if( pFrame == NULL ) {
         ::SetLastError(ERROR_OUTOFMEMORY);
         return FALSE;
      }
      m_wndFrame = pFrame->GetHwnd();
      
      m_wndClient.Init(m_pProject, this);
      m_wndClient.Create(m_wndFrame, CWindow::rcDefault, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);
      ATLASSERT(m_wndClient.IsWindow());
      pFrame->SetClient(m_wndClient);

      m_wndClient.SetViewText(bstrText);
      m_wndClient.ShowWindow(SW_SHOWNOACTIVATE);
   }

   // Delayed activation of view
   m_wndFrame.PostMessage(WM_SETFOCUS);
   return TRUE;
}

void CViewImpl::CloseView()
{
   if( m_wndFrame.IsWindow() ) _pDevEnv->DestroyClient(m_wndFrame);
   delete this;
}

void CViewImpl::ActivateUI()
{
   CMenu menu;
   menu.LoadMenu(IDR_MAIN);
   ATLASSERT(menu.IsMenu());

   CMenuHandle menuMain = _pDevEnv->GetMenuHandle(IDE_HWND_MAIN);
   CMenuHandle menuEdit = menuMain.GetSubMenu(MENUPOS_EDIT_FB);
   MergeMenu(menuEdit, menu.GetSubMenu(0), menuEdit.GetMenuItemCount());

   _pDevEnv->AddIdleListener(this);
   _pDevEnv->AddAppListener(this);
}

void CViewImpl::DeactivateUI()
{
   _pDevEnv->RemoveAppListener(this);
   _pDevEnv->RemoveIdleListener(this);
}

void CViewImpl::EnableModeless(BOOL bEnable)
{
   if( m_wndFrame.IsWindow() ) m_wndFrame.EnableWindow(bEnable);
}

LRESULT CViewImpl::PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   return 0;
}

LRESULT CViewImpl::SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   return 0;
}

// IIdleListener

void CViewImpl::OnIdle(IUpdateUI* pUIBase)
{
   if( m_wndClient.IsWindow() ) m_wndClient.OnIdle(pUIBase);
}

void CViewImpl::OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax)
{
   AtlLoadString(wID, pstrText, cchMax);
}

// IAppMessageListener

LRESULT CViewImpl::OnAppMessage(HWND /*hWnd*/, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   bHandled = FALSE;
   return 0;
}

BOOL CViewImpl::PreTranslateMessage(MSG* pMsg)
{
   if( m_wndClient.IsWindow() ) m_wndClient.PreTranslateMessage(pMsg);
   return 0;
}


///////////////////////////////////////////////////////7
//

CHtmlView::CHtmlView(IProject* pProject, IElement* pParent, LPCTSTR pstrFilename) : 
   CViewImpl(pProject, pParent, pstrFilename)
{
   m_sLanguage = _T("html");
}

BOOL CHtmlView::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("HTML"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CPhpView::CPhpView(IProject* pProject, IElement* pParent, LPCTSTR pstrFilename) : 
   CViewImpl(pProject, pParent, pstrFilename)
{
   m_sLanguage = _T("php");
}

BOOL CPhpView::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("PHP"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CAspView::CAspView(IProject* pProject, IElement* pParent, LPCTSTR pstrFilename) : 
   CViewImpl(pProject, pParent, pstrFilename)
{
   m_sLanguage = _T("asp");
}

BOOL CAspView::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("ASP"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CXmlView::CXmlView(IProject* pProject, IElement* pParent, LPCTSTR pstrFilename) : 
   CViewImpl(pProject, pParent, pstrFilename)
{
   m_sLanguage = _T("xml");
}

BOOL CXmlView::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("XML"), cchMax);
   return TRUE;
}

