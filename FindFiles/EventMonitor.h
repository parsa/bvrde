#if !defined(AFX_EVENTMONITOR_H__20031225_E956_CA66_2ED3_0080AD509054__INCLUDED_)
#define AFX_EVENTMONITOR_H__20031225_E956_CA66_2ED3_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FindFilesDlg.h"
#include "FindFilesView.h"


class CEventMonitor : public IAppListener, public IIdleListener
{
public:
   CFindFilesView m_viewFind;

   LRESULT OnAppMessage(HWND /*hWnd*/, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( uMsg == WM_COMMAND && LOWORD(wParam) == ID_EDIT_FINDFILES ) 
      {
         // Create auto-hide view if not already added
         if( !m_viewFind.IsWindow() ) {
            CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
            TCHAR szTitle[128] = { 0 };
            ::LoadString(_Module.GetResourceInstance(), IDS_TITLE, szTitle, 127);
            DWORD dwStyle = ES_MULTILINE | ES_NOHIDESEL | ES_SAVESEL | ES_READONLY | ES_DISABLENOSCROLL | 
                            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VSCROLL;
            m_viewFind.Create(wndMain, CWindow::rcDefault, szTitle, dwStyle, WS_EX_CLIENTEDGE);
            _pDevEnv->AddAutoHideView(m_viewFind, IDE_DOCK_BOTTOM, 5);
            m_viewFind.SetWindowText(_T(""));
         }
         // Display Find dialog
         CFindFilesDlg dlg;
         if( dlg.DoModal() == IDOK ) {
            // Commence search
            m_viewFind.DoSearch(dlg.GetPattern(), dlg.GetFolder(), dlg.GetFlags());
            _pDevEnv->ActivateAutoHideView(m_viewFind);
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
      // Currently we only enable the menu-item for C++ Projects...
      ISolution* pSolution = _pDevEnv->GetSolution();
      IProject* pProject = NULL;
      TCHAR szType[128] = { 0 };
      if( pSolution ) pProject = pSolution->GetActiveProject();
      if( pProject ) pProject->GetClass(szType, 127);
      pUIBase->UIEnable(ID_EDIT_FINDFILES, _tcscmp(szType, _T("Remote C++")) == 0);
   }
};


#endif // !defined(AFX_EVENTMONITOR_H__20031225_E956_CA66_2ED3_0080AD509054__INCLUDED_)
