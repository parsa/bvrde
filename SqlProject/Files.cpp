
#include "StdAfx.h"
#include "resource.h"

#include "Files.h"
#include "Project.h"

#pragma code_seg( "MISC" )


///////////////////////////////////////////////////////7
//

CView::CView(CSqlProject* pProject, 
             IElement* pParent, 
             LPCTSTR pstrName /*= NULL*/, 
             LPCTSTR pstrType /*= NULL*/, 
             LPCTSTR pstrConnectionString /*= NULL*/) :
   m_thread(this),
   m_pProject(pProject),
   m_pParent(pParent),
   m_sName(pstrName),
   m_sConnectString(pstrConnectionString == NULL ? _T("") : pstrConnectionString),
   m_sType(pstrType),
   m_bDirty(FALSE)
{
}

CView::~CView()
{
   CloseView();
}

BOOL CView::GetName(LPTSTR pstrName, UINT cchMax) const
{
   return _tcsncpy(pstrName, m_sName, cchMax) > 0;
}

BOOL CView::SetName(LPCTSTR pstrName)
{
   m_sName = pstrName;
   m_bDirty = true;
   return TRUE;
}

BOOL CView::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Data Link"), cchMax);
   return TRUE;
}

IDispatch* CView::GetDispatch()
{
   return _pDevEnv->CreateStdDispatch(_T("View"), this);
}

BOOL CView::IsDirty() const
{
   // The view is not considered dirty if the SQL text has changed
   // so we don't prompt the user unnessecary.
   return m_bDirty;
}

BOOL CView::Load(ISerializable* pArc)
{
   pArc->Read(_T("name"), m_sName.GetBufferSetLength(MAX_PATH), MAX_PATH);
   m_sName.ReleaseBuffer();
   pArc->Read(_T("type"), m_sType.GetBufferSetLength(64), 64);
   m_sType.ReleaseBuffer();
   pArc->Read(_T("provider"), m_sProvider.GetBufferSetLength(128), 128);
   m_sProvider.ReleaseBuffer();
   pArc->Read(_T("server"), m_sServer.GetBufferSetLength(80), 80);
   m_sServer.ReleaseBuffer();
   pArc->ReadGroupBegin(_T("Info"));
   pArc->Read(_T("str"), m_sConnectString.GetBufferSetLength(400), 400);
   m_sConnectString.ReleaseBuffer();
   pArc->ReadGroupEnd();
   return TRUE;
}

BOOL CView::Save(ISerializable* pArc)
{
   if( m_Db.IsOpen() ) {
      if( m_sProvider.IsEmpty() ) m_sProvider = GetPropertyStr(DBPROPSET_DATASOURCEINFO, DBPROP_DBMSNAME);
      if( m_sServer.IsEmpty() ) m_sServer = GetPropertyStr(DBPROPSET_DATASOURCEINFO, DBPROP_SERVERNAME);
   }

   pArc->Write(_T("name"), m_sName);
   pArc->Write(_T("type"), _T("Data Link"));
   pArc->Write(_T("provider"), m_sProvider);
   pArc->Write(_T("server"), m_sServer);

   if( pArc->WriteGroupBegin(_T("Info")) ) 
   {
      pArc->Write(_T("str"), m_sConnectString);
      pArc->WriteGroupEnd();
   }

   return TRUE;
}

BOOL CView::OpenView(long /*lPosition*/)
{
   if( !ConnectDatabase(&m_Db, m_sConnectString) ) return FALSE;

   if( m_wndFrame.IsWindow() ) 
   {
      // View already open. Just set focus to the window.
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
      m_view.Init(m_pProject, this);
      m_view.Create(m_wndFrame, CWindow::rcDefault, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);
      ATLASSERT(m_view.IsWindow());
      if( !m_view.IsWindow() ) return FALSE;
      pFrame->SetClient(m_view);

      if( !m_thread.IsRunning() ) {
         m_thread.Start();
         m_thread.Connect(m_sConnectString);
      }

      m_thread.SetNotify(m_view.m_wndResult);

      m_view.ShowWindow(SW_NORMAL);
   }

   // Delayed activation of view
   m_wndFrame.PostMessage(WM_SETFOCUS);
   return TRUE;
}

void CView::CloseView()
{
   m_thread.Disconnect();
   if( m_wndFrame.IsWindow() ) _pDevEnv->DestroyClient(m_wndFrame);
   m_thread.Stop();
}

void CView::ActivateUI()
{
   _pDevEnv->AddAppListener(this);
   _pDevEnv->AddIdleListener(this);

   CMenu menu;
   menu.LoadMenu(IDR_MAIN);
   ATLASSERT(menu.IsMenu());
   CMenuHandle menuMain = _pDevEnv->GetMenuHandle(IDE_HWND_MAIN);
   CMenuHandle menuEdit = menuMain.GetSubMenu(1);
   MergeMenu(menuEdit, menu.GetSubMenu(1), menuEdit.GetMenuItemCount());
}

void CView::DeactivateUI()
{
   _pDevEnv->RemoveAppListener(this);
   _pDevEnv->RemoveIdleListener(this);
}

void CView::EnableModeless(BOOL bEnable)
{
}

IElement* CView::GetParent() const
{
   return m_pParent;
}

BOOL CView::Save()
{
   m_bDirty = FALSE;
   return TRUE;
}

BOOL CView::GetText(BSTR* pbstrText)
{
   return FALSE;
}

BOOL CView::GetFileName(LPTSTR pstrName, UINT cchMax) const
{
   _tcscpy(pstrName, _T(""));
   return TRUE;
}

LRESULT CView::PostMessage(UINT uMsg, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/)
{
   return 0;
}

LRESULT CView::SendMessage(UINT uMsg, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/)
{
   return 0;
}

// IIdleListener

void CView::OnIdle(IUpdateUI* pUIBase)
{
   if( m_view.IsWindow() ) m_view.OnIdle(pUIBase);
}

// IAppListener

LRESULT CView::OnAppMessage(HWND /*hWnd*/, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   bHandled = FALSE;
   return 0;
}

BOOL CView::PreTranslateMessage(MSG* pMsg)
{
   if( m_view.IsWindow() ) m_view.PreTranslateMessage(pMsg);
   return 0;
}

// Operations

BOOL CView::Run(BOOL bSelectedTextOnly)
{
   // The database must be connected and the view
   // visible before we can execute anything!
   if( !OpenView(0) ) return FALSE;

   // Is already running? Shouldn't be happening!
   if( IsQueryRunning() ) return FALSE;

   // Grab text (only selection or entire text)
   CString sText;
   int iLineNo = 1;
   if( bSelectedTextOnly ) {
      int nLen = m_view.m_wndSource.GetTextLength();
      LPSTR pstrText = (LPSTR) malloc(nLen + 1);
      if( pstrText == NULL ) return FALSE;
      m_view.m_wndSource.GetSelText(pstrText);
      pstrText[nLen] = '\0';
      sText = pstrText;
      free(pstrText);   
      iLineNo = m_view.m_wndSource.LineFromPosition(m_view.m_wndSource.GetSelection().cpMin) + 1;
   }
   if( sText.IsEmpty() ) {
      int nLen = m_view.m_wndSource.GetTextLength();
      LPSTR pstrText = (LPSTR) malloc(nLen + 1);
      if( pstrText == NULL ) return FALSE;
      m_view.m_wndSource.GetText(nLen + 1, pstrText);
      pstrText[nLen] = '\0';
      sText = pstrText;
      free(pstrText);
      iLineNo = 1;
   }

   BOOL bDummy;
   m_view.SetCurSel(1);
   m_view.OnTabChange(0, NULL, bDummy);

   m_thread.ExecuteSql(sText, iLineNo);

   return TRUE;
}

BOOL CView::ChangeProperties(HWND hWnd)
{
   m_sConnectString = CDbOperations::ChangeProperties(hWnd, m_sConnectString);
   return TRUE;
}

BOOL CView::IsQueryRunning() const
{
   return m_thread.IsRunning() && m_thread.m_bExecuting;
}

BOOL CView::Abort()
{
   if( IsQueryRunning() ) m_thread.Abort();
   return TRUE;
}

