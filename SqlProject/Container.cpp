
#include "StdAfx.h"
#include "resource.h"

#include "Container.h"
#include "Files.h"

#pragma code_seg( "MISC" )

#include "../GenEdit/GenEdit.h"


CContainerWindow::CContainerWindow() :
   m_wndParent(this, 1),   
   m_pProject(NULL),
   m_pView(NULL)
{
}

void CContainerWindow::Init(CSqlProject* pProject, CView* pView)
{
   m_pProject = pProject;
   m_pView = pView;
   m_wndSchema.m_pDb = pView;
}

BOOL CContainerWindow::PreTranslateMessage(MSG* pMsg)
{
   int iCurSel = GetCurSel();
   switch( iCurSel ) {
   case 0:
      break;
   case 1:
      if( m_wndResult.PreTranslateMessage(pMsg) ) return TRUE;
      break;
   case 2:
      if( m_wndSchema.PreTranslateMessage(pMsg) ) return TRUE;
      break;
   }
   // Eat this command.
   // HACK: WTL doesn't eat the WM_COMMANDs from CHAIN_COMMAND_MSG() macros
   //       so we need to prevent the default handling...
   if( pMsg->message == WM_COMMAND && LOWORD(pMsg->wParam) == ID_FILE_OPEN ) {
      SetCurFocus(0);
      m_wndSource.SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_OPEN, 0));
      pMsg->message = WM_NULL;
      return TRUE;
   }
   return FALSE;
}

void CContainerWindow::OnIdle(IUpdateUI* pUIBase)
{
   BOOL bIsRunning = m_pView->IsQueryRunning();
   int nLen = m_wndSource.GetWindowTextLength();

   pUIBase->UIEnable(ID_QUERY_RUN, nLen > 0 && !bIsRunning);
   pUIBase->UIEnable(ID_QUERY_RUNSELECTED, nLen > 0 && !bIsRunning);
   pUIBase->UIEnable(ID_HISTORY_NEW, TRUE);
   pUIBase->UIEnable(ID_QUERY_STOP, bIsRunning);

   //int iHistoryPos = m_pView->GetHistoryPos();
   //int nHistoryCount = m_pView->GetHistoryCount() > 0;
   //pUIBase->UIEnable(ID_HISTORY_DELETE, nHistoryCount > 0);
   //pUIBase->UIEnable(ID_HISTORY_LEFT, iHistoryPos > 0);
   //pUIBase->UIEnable(ID_HISTORY_RIGHT, iHistoryPos < nHistoryCount - 1);

   int iCurSel = GetCurSel();

   pUIBase->UIEnable(ID_VIEW_TAB1, TRUE);
   pUIBase->UIEnable(ID_VIEW_TAB2, TRUE);
   pUIBase->UIEnable(ID_VIEW_TAB3, TRUE);
   pUIBase->UISetCheck(ID_VIEW_TAB1, iCurSel == 0);
   pUIBase->UISetCheck(ID_VIEW_TAB2, iCurSel == 1);
   pUIBase->UISetCheck(ID_VIEW_TAB3, iCurSel == 2);

   switch( iCurSel ) {
   case 0:
      m_wndSource.OnIdle(pUIBase);
      break;
   case 1:
      m_wndResult.OnIdle(pUIBase);
      break;
   case 2:
      m_wndSchema.OnIdle(pUIBase);
      break;
   }
}

void CContainerWindow::OnFinalMessage(HWND /*hWnd*/)
{
   if( m_pProject == NULL ) delete m_pView;
}

LRESULT CContainerWindow::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   LRESULT lRes = DefWindowProc();
   ATLASSERT(m_pView);

   m_wndParent.SubclassWindow(GetParent());

   SIZE szPadding = { 6, 6 };
   SetPadding(szPadding);

   TCHAR szFilename[MAX_PATH] = { 0 };
   m_pView->GetFileName(szFilename, MAX_PATH);

   m_wndSource.SubclassWindow( Bvrde_CreateScintillaView(m_hWnd, _pDevEnv, szFilename, _T("sql")) );
   m_wndSource.Init(m_pProject, m_pView);
   m_wndSource.ModifyStyle(0, WS_BORDER);
   m_wndSource.ModifyStyleEx(0, WS_EX_CLIENTEDGE);
   m_wndSource.SendMessage(WM_SETTINGCHANGE);
   m_wndResult.Create(m_hWnd, rcDefault);
   m_wndSchema.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

   CString s;
   TCITEM item = { 0 };
   item.mask = TCIF_TEXT;
   //
   s.LoadString(IDS_SQL);
   item.pszText = (LPTSTR) (LPCTSTR) s;
   InsertItem(0, &item);
   //
   s.LoadString(IDS_RESULT);
   item.pszText = (LPTSTR) (LPCTSTR) s;
   InsertItem(1, &item);
   //
   s.LoadString(IDS_SCHEMA);
   item.pszText = (LPTSTR) (LPCTSTR) s;
   InsertItem(2, &item);
   //
   SetCurSel(0);
   BOOL bDummy;
   OnTabChange(0, NULL, bDummy);

   return lRes;
}

LRESULT CContainerWindow::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   if( !m_hWndClient.IsWindow() ) return 0;
   m_hWndClient.SetFocus();
   return 0;
}

LRESULT CContainerWindow::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   bHandled = FALSE;
   if( !IsWindow() ) return 0;
   if( !m_hWndClient.IsWindow() ) return 0;
   RECT rc;
   GetClientRect(&rc);
   AdjustRect(FALSE, &rc);
   m_hWndClient.MoveWindow(&rc);
   return 0;
}

LRESULT CContainerWindow::OnSendMsgToAll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
   if( m_wndSource.IsWindow() ) m_wndSource.SendMessage(uMsg, wParam, lParam);
   if( m_wndResult.IsWindow() ) m_wndResult.SendMessage(uMsg, wParam, lParam);
   if( m_wndSchema.IsWindow() ) m_wndSchema.SendMessage(uMsg, wParam, lParam);
   return 1; // No need
}

LRESULT CContainerWindow::OnSendMsgToClient(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
   if( ::IsWindow(m_hWndClient) ) m_hWndClient.SendMessage(uMsg, wParam, lParam);
   return 1; // No need
}

LRESULT CContainerWindow::OnQueryRun(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pView);
   m_pView->Run(FALSE);
   return 0;
}

LRESULT CContainerWindow::OnQueryRunSelected(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pView);
   m_pView->Run(TRUE);
   return 0;
}

LRESULT CContainerWindow::OnQueryStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pView);
   if( m_pView->IsQueryRunning() ) m_pView->Abort(); else PostMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_TAB1, 0));
   return 0;
}

LRESULT CContainerWindow::OnViewTab1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   SetCurFocus(0);
   return 0;
}

LRESULT CContainerWindow::OnViewTab2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   SetCurFocus(1);
   return 0;
}

LRESULT CContainerWindow::OnViewTab3(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   SetCurFocus(2);
   return 0;
}

LRESULT CContainerWindow::OnViewSetLine(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
   SetCurFocus(0);
   int iLine = (int) wNotifyCode - 1;
   long lPos = m_wndSource.PositionFromLine(iLine);
   m_wndSource.SetSel(lPos, lPos);
   m_wndSource.EnsureVisible(lPos);
   return 0;
}

LRESULT CContainerWindow::OnTabChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   if( m_hWndClient.IsWindow() ) m_hWndClient.ShowWindow(SW_HIDE);
   int iCurSel = GetCurSel();
   switch( iCurSel ) {
   case -1:
      m_hWndClient = NULL;
      break;
   case 0:
      m_hWndClient = m_wndSource;
      break;
   case 1:
      m_hWndClient = m_wndResult;
      break;
   case 2:
      m_hWndClient = m_wndSchema;
      break;
   }
   SendMessage(WM_SIZE);
   m_hWndClient.ShowWindow(SW_SHOW);
   m_hWndClient.SetFocus();
   return 0;
}
