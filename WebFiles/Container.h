#if !defined(AFX_CONTAINER_H__20030608_F1CD_9419_8871_0080AD509054__INCLUDED_)
#define AFX_CONTAINER_H__20030608_F1CD_9419_8871_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PreviewView.h"
#include "DesignView.h"

#include "../GenEdit/GenEdit.h"


class CContainerWindow : 
   public CWindowImpl<CContainerWindow>
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_WebTab"))

   CDotNetTabCtrl m_ctrlTab;
   CPreviewView m_wndPreview;
   CScintillaCtrl m_wndSource;
   CDesignView m_wndDesign;
   CWindow m_hWndClient;
   CString m_sLanguage;
   CString m_sText;
   //
   IView* m_pView;
   IProject* m_pProject;

   void Init(IView* pView, IProject* pProject);
   void SetFilename(CString& sFilename);
   void SetLanguage(CString& sLanguage);
   CString GetViewText();
   void SetViewText(LPCTSTR pstrText);
   void OnIdle(IUpdateUI* pUIBase);
   BOOL PreTranslateMessage(MSG* pMsg);
   void OnFinalMessage(HWND hWnd);

   // Message map and handlers

   BEGIN_MSG_MAP(CContainerWindow)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
      MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSendMsgToAll)
      MESSAGE_HANDLER(WM_QUERYENDSESSION, OnQueryEndSession)
      COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
      NOTIFY_CODE_HANDLER(TCN_SELCHANGING, OnTabChanging)
      NOTIFY_CODE_HANDLER(TCN_SELCHANGE, OnTabChange)
      // HACK: IE eats WM_COMMAND with hWnd handles (non-menu generated)
      if( uMsg == WM_COMMAND && m_hWndClient.IsWindow() ) {
         m_hWndClient.SendMessage(uMsg, LOWORD(wParam), 0);
      }
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnQueryEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);   
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);   
   LRESULT OnSendMsgToAll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);   
   LRESULT OnSendMsgToClient(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);   
   LRESULT OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnTabChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTabChanging(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
};


#endif // !defined(AFX_CONTAINER_H__20030608_F1CD_9419_8871_0080AD509054__INCLUDED_)

