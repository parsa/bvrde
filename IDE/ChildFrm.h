// ChildFrm.h : interface of the CChildFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHILDFRM_H__C321D17A_E7F5_40DA_944D_182C18705C63__INCLUDED_)
#define AFX_CHILDFRM_H__C321D17A_E7F5_40DA_944D_182C18705C63__INCLUDED_

#pragma once

class CMainFrame;  // Forward declare


class CChildFrame : 
   public CMDIChildWindowImpl<CChildFrame>,
   public IViewFrame
{
public:
   DECLARE_FRAME_WND_CLASS(_T("BVRDE_Frame"), IDR_MDICHILD)

   CMainFrame* m_pFrame;
   IDevEnv* m_pDevEnv;
   IProject* m_pProject;
   IView* m_pView;

   CChildFrame(CMainFrame* pFrame, IDevEnv* pDevEnv, IProject* pProject, IView* pView);

   virtual void OnFinalMessage(HWND /*hWnd*/);

   // IViewFrame

   HWND GetHwnd() const;
   HWND SetClient(HWND /*hWnd*/);

   // Message map and handlers

   BEGIN_MSG_MAP(CChildFrame)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_CLOSE, OnClose)
      MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
      MESSAGE_HANDLER(WM_SETFOCUS, OnViewMessage)
      MESSAGE_HANDLER(WM_KILLFOCUS, OnViewMessage)
      CHAIN_CLIENT_COMMANDS()
      CHAIN_MSG_MAP( CMDIChildWindowImpl<CChildFrame> )
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
   LRESULT OnViewMessage(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDFRM_H__C321D17A_E7F5_40DA_944D_182C18705C63__INCLUDED_)
