#if !defined(AFX_EVENTMONITOR_H__20030621_B6BE_022E_A77B_0080AD509054__INCLUDED_)
#define AFX_EVENTMONITOR_H__20030621_B6BE_022E_A77B_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "View.h"
 

class CEventMonitor : public IAppListener
{
public:
   CStartPageView m_wndStartPage;

   LRESULT OnAppMessage(HWND /*hWnd*/, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( uMsg == WM_COMMAND && LOWORD(wParam) == ID_HELP_STARTPAGE ) m_wndStartPage.OpenView(0);
      bHandled = FALSE;
      return 0;
   }
   BOOL PreTranslateMessage(MSG* pMsg)
   {
      if( ::GetFocus() != m_wndStartPage ) return FALSE;
      return m_wndStartPage.PreTranslateMessage(pMsg);
   }
};


#endif // !defined(AFX_EVENTMONITOR_H__20030621_B6BE_022E_A77B_0080AD509054__INCLUDED_)

