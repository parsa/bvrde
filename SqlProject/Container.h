#if !defined(AFX_CONTAINER_H__20030608_F1CD_9419_8871_0080AD509054__INCLUDED_)
#define AFX_CONTAINER_H__20030608_F1CD_9419_8871_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CSqlProject;
class CView;

#include "ResultView.h"
#include "SchemaView.h"
#include "ScintillaView.h"


class CContainerWindow : 
   public CWindowImpl< CContainerWindow, CTabCtrl, CWinTraitsOR< TCS_SINGLELINE | TCS_FOCUSNEVER | TCS_FORCEICONLEFT | TCS_FORCELABELLEFT > >
{
public:
   DECLARE_WND_SUPERCLASS(_T("BVRDE_SqlTab"), CTabCtrl::GetWndClassName())

   CContainerWindow();

   CContainedWindow m_wndParent;
   CScintillaView m_wndSource;
   CResultView m_wndResult;
   CSchemaView m_wndSchema;
   CImageListCtrl m_Images;
   CWindow m_hWndClient;
   //
   CSqlProject* m_pProject;
   CView* m_pView;

   void Init(CSqlProject* pProject, CView* pView);
   void SetLanguage(CString& sLanguage);

   void OnIdle(IUpdateUI* pUIBase);

   BOOL PreTranslateMessage(MSG* pMsg);
   void OnFinalMessage(HWND hWnd);

   // Message map and handlers

   BEGIN_MSG_MAP(CContainerWindow)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
      MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSendMsgToAll)
      MESSAGE_HANDLER(WM_QUERYENDSESSION, OnSendMsgToClient)
      COMMAND_ID_HANDLER(ID_QUERY_RUN, OnQueryRun)
      COMMAND_ID_HANDLER(ID_QUERY_RUNSELECTED, OnQueryRunSelected)
      COMMAND_ID_HANDLER(ID_QUERY_STOP, OnQueryStop)
      COMMAND_ID_HANDLER(ID_VIEW_TAB1, OnViewTab1)
      COMMAND_ID_HANDLER(ID_VIEW_TAB2, OnViewTab2)
      COMMAND_ID_HANDLER(ID_VIEW_TAB3, OnViewTab3)
      COMMAND_ID_HANDLER(ID_VIEW_SETLINE, OnViewSetLine)
      COMMAND_ID_HANDLER(ID_HISTORY_LEFT, OnViewTab1)
      COMMAND_ID_HANDLER(ID_HISTORY_RIGHT, OnViewTab1)
      CHAIN_CLIENT_COMMANDS()
      REFLECT_NOTIFICATIONS()
   ALT_MSG_MAP(1) // Parent
      if( uMsg == WM_NOTIFY && ((LPNMHDR)lParam)->hwndFrom == m_hWnd ) 
      {
         NOTIFY_CODE_HANDLER(TCN_SELCHANGE, OnTabChange)
      }
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);   
   LRESULT OnSendMsgToAll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);   
   LRESULT OnSendMsgToClient(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);   
   LRESULT OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnQueryRun(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnQueryRunSelected(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnQueryStop(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewTab1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewTab2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewTab3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewSetLine(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnTabChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
};


#endif // !defined(AFX_CONTAINER_H__20030608_F1CD_9419_8871_0080AD509054__INCLUDED_)

