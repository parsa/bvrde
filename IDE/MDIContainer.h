#if !defined(AFX_MDICONTAINER_H__20020426_0903_1B62_2BDF_0080AD509054__INCLUDED_)
#define AFX_MDICONTAINER_H__20020426_0903_1B62_2BDF_0080AD509054__INCLUDED_

#pragma once

/////////////////////////////////////////////////////////////////////////////
// A container window for the MDI Client with tabbed child selection
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2002 Bjarke Viksoe.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//

class CMDIContainer : 
   public CWindowImpl<CMDIContainer>
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_MDIContainer"))

   enum { IDC_COOLTAB = 1234 };

   CContainedWindow m_wndMDIClient;
   CDotNetTabCtrl m_ctrlTab;
   bool m_bTabsVisible;

   CMDIContainer() :
      m_wndMDIClient(this, 1),
      m_bTabsVisible(true)
   {
   }
   ~CMDIContainer()
   {
      if( m_wndMDIClient.IsWindow() )
/*scary!*/   m_wndMDIClient.UnsubclassWindow();
   }

   // Operations
   
   void SetMDIClient(HWND hWnd)
   {
      m_wndMDIClient.SubclassWindow(hWnd);
   }

   void SetVisible(BOOL bVisible)
   {
      m_bTabsVisible = bVisible == TRUE;
      UpdateLayout();
   }

   void ChangeDirtyState(HWND hWnd)
   {
      ATLASSERT(::IsWindow(hWnd));
      TCITEM tci = { 0 };
      tci.mask = TCIF_PARAM;
      tci.lParam = (LPARAM) hWnd;
      int iIndex = m_ctrlTab.FindItem(&tci);
      if( iIndex < 0 ) return;
      CString sName;
      CString sFileName;
      _LookupViewNames(hWnd, sName, sFileName);
      tci.mask = TCIF_TEXT;
      tci.pszText = (LPTSTR) (LPCTSTR) sName;
      m_ctrlTab.SetItem(iIndex, &tci);
   }
   void RefreshItems()
   {
      int nCount = m_ctrlTab.GetItemCount();
      for( int i = 0; i < nCount; i++ ) {
         TCITEM tci = { 0 };
         tci.mask = TCIF_PARAM;
         m_ctrlTab.GetItem(i, &tci);
         CString sName;
         CString sFileName;
         _LookupViewNames((HWND) tci.lParam, sName, sFileName);
         tci.mask = TCIF_TEXT;
         tci.pszText = (LPTSTR) (LPCTSTR) sName;
         m_ctrlTab.SetItem(i, &tci);
      }
   }

   // Message map and handler

   BEGIN_MSG_MAP(CMDIContainer)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      NOTIFY_HANDLER(IDC_COOLTAB, TCN_SELCHANGE, OnSelChange)
      NOTIFY_HANDLER(IDC_COOLTAB, TCN_REQUESTREMOVE, OnRequestRemove)
      NOTIFY_HANDLER(IDC_COOLTAB, NM_RCLICK, OnRightClick)
      REFLECT_NOTIFICATIONS()
   ALT_MSG_MAP(1)      // MDI client messages
      MESSAGE_HANDLER(WM_PARENTNOTIFY, OnParentNotify)
      MESSAGE_HANDLER(WM_MDINEXT, OnMDINext)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      LRESULT lRes = DefWindowProc();
      SetFont( AtlGetDefaultGuiFont() );
      // Create the tab control
      m_ctrlTab.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, IDC_COOLTAB);
      m_ctrlTab.SetExtendedStyle(0, TCS_EX_SELHIGHLIGHT | TCS_EX_FLATSEPARATORS | TCS_EX_SCROLLBUTTONS);
      return lRes;
   }
   LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      UpdateLayout();
      return 0;
   }
   LRESULT OnSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      // Someone clicked on a tab...
      int iIndex = m_ctrlTab.GetCurSel();
      if( iIndex < 0 ) return 0;
      TCITEM tci = { 0 };
      tci.mask = TCIF_PARAM;
      m_ctrlTab.GetItem(iIndex, &tci);
      m_wndMDIClient.SendMessage(WM_MDIACTIVATE, (WPARAM) tci.lParam);
      return 0;
   }
   LRESULT OnRequestRemove(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      int iIndex = m_ctrlTab.GetCurSel();
      if( iIndex < 0 ) return 0;
      TCITEM tci = { 0 };
      tci.mask = TCIF_PARAM;
      m_ctrlTab.GetItem(iIndex, &tci);
      ::PostMessage((HWND)tci.lParam, WM_CLOSE, 0, 0L);
      return 0;
   }
   LRESULT OnRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      // Someone right-clicked on a tab...
      int iIndex = m_ctrlTab.GetCurSel();
      if( iIndex < 0 ) return 0;
      POINT pt = { 0 };
      ::GetCursorPos(&pt);
      CMenu menu;
      menu.LoadMenu(IDM_COOLTAB);
      CMenuHandle submenu;
      submenu = menu.GetSubMenu(0);
      CDummyElement Element(_T("Popup"), _T("TabMenu"));
      UINT nCmd = g_pDevEnv->ShowPopupMenu(&Element, submenu, pt, FALSE);
      switch( nCmd ) {
      case ID_TAB_ACTIVATE:
         {
            TCITEM tci = { 0 };
            tci.mask = TCIF_PARAM;
            m_ctrlTab.GetItem(iIndex, &tci);
            ::SetFocus((HWND)tci.lParam);
         }
         break;
      case ID_FILE_SAVE:
         {
            CWindow wnd = GetTopLevelWindow();
            wnd.SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE, 0));
         }
         break;
      case ID_TAB_CLOSE:
         {
            TCITEM tci = { 0 };
            tci.mask = TCIF_PARAM;
            m_ctrlTab.GetItem(iIndex, &tci);
            ::PostMessage((HWND)tci.lParam, WM_CLOSE, 0, 0L);
         }
         break;
      case ID_TAB_CLOSEALL:
         {
            for( int i = 0; i < m_ctrlTab.GetItemCount(); i++ ) {
               TCITEM tci = { 0 };
               tci.mask = TCIF_PARAM;
               m_ctrlTab.GetItem(i, &tci);
               ::PostMessage((HWND)tci.lParam, WM_CLOSE, 0, 0L);
            }
         }
         break;
      case ID_TAB_CLOSEALLEXCEPT:
         {
            for( int i = 0; i < m_ctrlTab.GetItemCount(); i++ ) {
               if( i != iIndex ) {
                  // NOTE: Because we PostMessage stuff we'll manage
                  //       to iterate the list before things actually
                  //       happens and affects the tab items.
                  TCITEM tci = { 0 };
                  tci.mask = TCIF_PARAM;
                  m_ctrlTab.GetItem(i, &tci);
                  ::PostMessage((HWND)tci.lParam, WM_CLOSE, 0, 0L);
               }
            }
         }
         break;
      }
      return 0;
   }

   // MDI client messages

   LRESULT OnParentNotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
   {
      switch( LOWORD(wParam) ) {
      case WM_CREATE:
         {
            // Get Tab text and tooltip text
            CWindow wnd = (HWND) lParam;
            CString sName;
            CString sFileName;
            _LookupViewNames(wnd, sName, sFileName);
            // Create a new tab
            COOLTCITEM tci;
            tci.mask = TCIF_TEXT | TCIF_TOOLTIP | TCIF_PARAM;
            tci.pszText = (LPTSTR) (LPCTSTR) sName;
            tci.pszTipText = (LPTSTR) (LPCTSTR) sFileName;
            tci.lParam = lParam;
            m_ctrlTab.InsertItem(m_ctrlTab.GetItemCount(), &tci);
            // Position view in container (maximize if needed)
            if( m_bTabsVisible ) m_wndMDIClient.SendMessage(WM_MDIMAXIMIZE, (WPARAM) (HWND) wnd);
            UpdateLayout();
         }
         break;
      case WM_DESTROY:
         {
            TCITEM tci = { 0 };
            tci.mask = TCIF_PARAM;
            tci.lParam = lParam;
            int iIndex = m_ctrlTab.FindItem(&tci);
            if( iIndex >= 0 ) m_ctrlTab.DeleteItem(iIndex);
            // The UpdateButtons() call is important because we need to
            // update the UI while the reference to the view is still intact.
            // The call will eventually reach CMainFrame::OnUserViewChange()
            // which clears the view menus & tools.
            UpdateButtons();
            UpdateLayout();
         }
         break;
      }
      bHandled = FALSE; // MDI stuff relies on this, so...
      return 0;
   }
   LRESULT OnMDINext(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      if( m_ctrlTab.GetItemCount() <= 1 ) return 0;
      int iIndex = m_ctrlTab.GetCurSel() + (lParam == 0 ? 1 : -1);
      if( iIndex >= m_ctrlTab.GetItemCount() ) iIndex = 0;
      if( iIndex < 0 ) iIndex = m_ctrlTab.GetItemCount() - 1;
      TCITEM tci = { 0 };
      tci.mask = TCIF_PARAM;
      m_ctrlTab.GetItem(iIndex, &tci);
      m_wndMDIClient.SendMessage(WM_MDIACTIVATE, (WPARAM) tci.lParam);
      return 0;
   }

   // Operations

   void OnIdle()
   {
      HWND hWndActive = (HWND) m_wndMDIClient.SendMessage(WM_MDIGETACTIVE, 0, 0L);
      static HWND s_hWndLastMDIClient = NULL;
      if( s_hWndLastMDIClient != hWndActive ) {
         s_hWndLastMDIClient = hWndActive;
         UpdateButtons();
      }
   }

   void UpdateLayout()
   {
      RECT rc = { 0 };
      GetClientRect(&rc);
      // Place tab control in frame if populated
      int cy = ::GetSystemMetrics(SM_CYMENUSIZE) + 4;
      if( m_ctrlTab.GetItemCount() > 0 && m_bTabsVisible ) {
         RECT rcTab = { rc.left, rc.top, rc.right, rc.top + cy };
         m_ctrlTab.SetWindowPos(NULL, &rcTab, SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE);
         rc.top += cy;
      }
      else {
         m_ctrlTab.ShowWindow(SW_HIDE);
      }
      // The MDI Client goes below
      if( m_wndMDIClient.IsWindow() ) {
         ::MapWindowPoints(m_hWnd, m_wndMDIClient.GetParent(), (LPPOINT) &rc, 2);
         m_wndMDIClient.SetWindowPos(NULL, &rc, SWP_NOZORDER | SWP_NOACTIVATE);
      }
   }

   void UpdateButtons()
   {
      // Select the active MDI child tab
      if( !m_wndMDIClient.IsWindow() ) return;
      BOOL bMaximized = FALSE;
      HWND hWndActive = (HWND) m_wndMDIClient.SendMessage(WM_MDIGETACTIVE, 0, (LPARAM) &bMaximized);
      TCITEM tci = { 0 };
      tci.mask = TCIF_PARAM;
      tci.lParam = (LPARAM) hWndActive;
      int iIndex = m_ctrlTab.FindItem(&tci);
      if( iIndex >= 0 ) m_ctrlTab.SetCurSel(iIndex);
      ::SendMessage(GetTopLevelWindow(), WM_APP_VIEWCHANGE, 0, 0L);
   }

   // Implementation

   void _LookupViewNames(HWND hWnd, CString& sName, CString& sFilename) const
   {
      // Try to get the IElement from the window so we can extract
      // the proper name. If the window is not a known element, we'll
      // just use the window-text.
      CWinProp prop = hWnd;
      IView* pView = NULL;
      prop.GetProperty(_T("View"), pView);
      if( pView != NULL ) {
         pView->GetName(sName.GetBufferSetLength(128), 128);
         sName.ReleaseBuffer();
         if( pView->IsDirty() ) sName += _T(" *");
         pView->GetFileName(sFilename.GetBufferSetLength(MAX_PATH), MAX_PATH);
         sFilename.ReleaseBuffer();
      }
      else {
         sName = sFilename = CWindowText(hWnd);
      }
   }
};


#endif // !defined(AFX_MDICONTAINER_H__20020426_0903_1B62_2BDF_0080AD509054__INCLUDED_)

