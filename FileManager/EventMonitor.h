#if !defined(AFX_EVENTMONITOR_H__20030706_CAA7_AA73_02AC_0080AD509054__INCLUDED_)
#define AFX_EVENTMONITOR_H__20030706_CAA7_AA73_02AC_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BrowserView.h"


class CEventMonitor : public IAppListener, public IIdleListener
{
public:
   CBrowserView m_viewBrowser;

   LRESULT OnAppMessage(HWND /*hWnd*/, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( uMsg == WM_COMMAND && LOWORD(wParam) == ID_VIEW_FILEMANAGER ) 
      {
         if( !m_viewBrowser.IsWindow() ) {
            CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
            TCHAR szTitle[128] = { 0 };
            ::LoadString(_Module.GetResourceInstance(), IDS_TITLE, szTitle, 127);
            m_viewBrowser.Create(wndMain, CWindow::rcDefault, szTitle, WS_CHILD);
            _pDevEnv->AddAutoHideView(m_viewBrowser, IDE_DOCK_LEFT, 7);
            _pDevEnv->ActivateAutoHideView(m_viewBrowser);
         }
         else {
            _pDevEnv->RemoveAutoHideView(m_viewBrowser);
            m_viewBrowser.PostMessage(WM_CLOSE);
         }
      }
      bHandled = FALSE;
      return 0;
   }
   BOOL PreTranslateMessage(MSG* pMsg)
   {
      return FALSE;
   }
   void OnIdle(IUpdateUI* pUIBase)
   {
      pUIBase->UIEnable(ID_VIEW_FILEMANAGER, TRUE);
      pUIBase->UISetCheck(ID_VIEW_FILEMANAGER, m_viewBrowser.IsWindow());
   }
};


#endif // !defined(AFX_EVENTMONITOR_H__20030706_CAA7_AA73_02AC_0080AD509054__INCLUDED_)

