// ChildFrm.cpp : implementation of the CChildFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainFrm.h"
#include "ChildFrm.h"


// CChildFrame 

CChildFrame::CChildFrame(CMainFrame* pFrame, IDevEnv* pDevEnv, IProject* pProject, IView* pView) :
   m_pFrame(pFrame),
   m_pDevEnv(pDevEnv),
   m_pProject(pProject),
   m_pView(pView)
{
}

void CChildFrame::OnFinalMessage(HWND hWnd)
{
   // Remove our Window Properties
   // FIX: On ATL7 the window is no longer alive at this point.
   if( ::IsWindow(hWnd) ) {
      CWinProp prop = hWnd;
      prop.RemoveProperty(_T("DevEnv"));
      prop.RemoveProperty(_T("Project"));
      prop.RemoveProperty(_T("View"));
      prop.RemoveProperty(_T("Frame"));
   }

   // Commit suicide...
   delete this;
}

LRESULT CChildFrame::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   ATLASSERT(m_pDevEnv);
   ATLASSERT(m_pView);

   // Create identification-references to the project/view by adding Window
   // Properties to the window itself.
   // These pointers are primarily for the internal handling of views/focus
   // of menus/toolbar states. Clients may however also take advantage of these.
   CWinProp prop = m_hWnd;
   prop.SetProperty(_T("DevEnv"), m_pDevEnv);
   prop.SetProperty(_T("Project"), m_pProject);
   prop.SetProperty(_T("View"), m_pView);
   prop.SetProperty(_T("Frame"), this);

   BOOL bDummy = FALSE;
   OnViewMessage(uMsg, wParam, lParam, bDummy);

   bHandled = FALSE;
   return TRUE;
}

LRESULT CChildFrame::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   // Notify views about the forthcoming close event. We use
   // the WM_QUERYENDSESSION message for this.
   // TODO: Don't use WM_QUERYENDSESSION for this.
   if( ::SendMessage(m_hWndClient, WM_QUERYENDSESSION, 0, 0x80000000) == 0 ) return 0;

   BOOL bDummy = FALSE;
   OnViewMessage(uMsg, wParam, lParam, bDummy);

   bHandled = FALSE;
   return 0;
}

LRESULT CChildFrame::OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
   LPMSG pMsg = (LPMSG) lParam;
   if( CMDIChildWindowImpl<CChildFrame>::PreTranslateMessage(pMsg) ) return (LRESULT) TRUE;
   return (LRESULT) FALSE;
}

LRESULT CChildFrame::OnViewMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   ATLASSERT(m_pFrame);
   MSG msg = { 0 };
   msg.hwnd = m_hWnd;
   msg.message = uMsg;
   msg.wParam = wParam;
   msg.lParam = lParam;
   m_pFrame->SendMessage(WM_APP_VIEWMESSAGE, (WPARAM) m_pView, (LPARAM) &msg);
   bHandled = FALSE;
   return 0;
}

// IViewFrame

HWND CChildFrame::GetHwnd() const
{
   return m_hWnd;
}

HWND CChildFrame::SetClient(HWND hWnd)
{
   HWND hWndPrevious = m_hWndClient;
   m_hWndClient = hWnd;
   UpdateLayout();
   if( m_hWndClient != NULL ) ::SetFocus(m_hWndClient);
   return hWndPrevious;
}
