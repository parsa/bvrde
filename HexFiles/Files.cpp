
#include "StdAfx.h"
#include "resource.h"

#include "Files.h"

#pragma code_seg( "MISC" )


///////////////////////////////////////////////////////7
// CView

CView::CView(IProject* pProject, IElement* pParent, LPCTSTR pstrFilename) : 
   m_pProject(pProject),
   m_pParent(pParent),
   m_sFilename(pstrFilename)
{
   TCHAR szName[MAX_PATH];
   _tcscpy(szName, m_sFilename);
   ::PathStripPath(szName);
   m_sName = szName;
}

BOOL CView::GetName(LPTSTR pstrName, UINT cchMax) const
{
   ATLASSERT(pstrName);
   _tcsncpy(pstrName, m_sName, cchMax);
   return TRUE;
}

BOOL CView::GetType(LPTSTR pstrType, UINT cchMax) const
{
   return _tcsncpy(pstrType, _T("Binary Files"), cchMax) >= 0;
}

IDispatch* CView::GetDispatch()
{
   return _pDevEnv->CreateStdDispatch(_T("View"), this);
}

BOOL CView::GetText(BSTR* pbstrText)
{
   CComBSTR bstr = L"";
   *pbstrText = bstr.Detach();
   return TRUE;
}

BOOL CView::GetFileName(LPTSTR pstrName, UINT cchMax) const
{
   ATLASSERT(pstrName);
   _tcscpy(pstrName, m_sFilename);
   return TRUE;
}

IElement* CView::GetParent() const
{
   return m_pParent;
}

BOOL CView::SetName(LPCTSTR pstrName)
{
   return FALSE;
}

BOOL CView::Load(ISerializable* pArc)
{
   pArc->Read(_T("name"), m_sName.GetBufferSetLength(128), 128);
   m_sName.ReleaseBuffer();
   pArc->Read(_T("filename"), m_sFilename.GetBufferSetLength(MAX_PATH), MAX_PATH);
   m_sFilename.ReleaseBuffer();
   return TRUE;
}

BOOL CView::Save(ISerializable* pArc)
{
   CString sType;
   GetType(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();

   pArc->Write(_T("name"), m_sName);
   pArc->Write(_T("type"), sType);
   pArc->Write(_T("filename"), m_sFilename);
   return TRUE;
}

BOOL CView::Save()
{
   if( !m_wndClient.IsWindow() ) return FALSE;

   // Save clears Undo stack?
   TCHAR szBuffer[32] = { 0 };
   _pDevEnv->GetProperty(_T("gui.document.clearUndo"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("true")) == 0 ) m_wndClient.EmptyUndoBuffer();

   return TRUE;
}

BOOL CView::IsDirty() const
{
   if( !::IsWindow(m_wndClient) ) return FALSE;
   return m_wndClient.GetModify();
}

BOOL CView::OpenView(long lLineNum)
{
   if( m_wndFrame.IsWindow() ) 
   {
      m_wndFrame.SetFocus();
   }
   else 
   {
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
      m_wndClient.Create(m_wndFrame, CWindow::rcDefault, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
      ATLASSERT(m_wndClient.IsWindow());
      pFrame->SetClient(m_wndClient);
      m_wndClient.SetFilename(m_sFilename);
      m_wndClient.ShowWindow(SW_SHOW);
   }

   // Delayed activation of view
   m_wndFrame.PostMessage(WM_SETFOCUS);
   return TRUE;
}

void CView::CloseView()
{
   if( m_wndFrame.IsWindow() ) _pDevEnv->DestroyClient(m_wndFrame);
   delete this;
}

void CView::ActivateUI()
{
   CMenu menu;
   menu.LoadMenu(IDR_MAIN);
   ATLASSERT(menu.IsMenu());

   CMenuHandle menuMain = _pDevEnv->GetMenuHandle(IDE_HWND_MAIN);
   CMenuHandle menuView = menuMain.GetSubMenu(2);
   MergeMenu(menuView , menu.GetSubMenu(0), 1);

   _pDevEnv->AddIdleListener(this);
   _pDevEnv->AddAppListener(this);
}

void CView::DeactivateUI()
{
   _pDevEnv->RemoveAppListener(this);
   _pDevEnv->RemoveIdleListener(this);
}

void CView::EnableModeless(BOOL bEnable)
{
   if( m_wndFrame.IsWindow() ) m_wndFrame.EnableWindow(bEnable);
}

LRESULT CView::PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   return 0;
}

LRESULT CView::SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   return 0;
}

// IIdleListener

void CView::OnIdle(IUpdateUI* pUIBase)
{
   if( m_wndClient.IsWindow() ) m_wndClient.OnIdle(pUIBase);
}

// IAppListener

LRESULT CView::OnAppMessage(HWND /*hWnd*/, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   bHandled = FALSE;
   return 0;
}

BOOL CView::PreTranslateMessage(MSG* pMsg)
{
   if( m_wndClient.IsWindow() ) m_wndClient.PreTranslateMessage(pMsg);
   return 0;
}

