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
   bool m_bVisible;

   CMDIContainer() :
      m_wndMDIClient(this, 1),
      m_bVisible(true)
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
      m_bVisible = bVisible == TRUE;
      UpdateLayout();
   }

   // Message map and handler

   BEGIN_MSG_MAP(CMDIContainer)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      NOTIFY_HANDLER(IDC_COOLTAB, TCN_SELCHANGE, OnSelChange)
      NOTIFY_HANDLER(IDC_COOLTAB, NM_RCLICK, OnRightClick)
      REFLECT_NOTIFICATIONS()
   ALT_MSG_MAP(1)      // MDI client messages
      MESSAGE_HANDLER(WM_PARENTNOTIFY, OnParentNotify)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      LRESULT lRes = DefWindowProc();
      SetFont( AtlGetDefaultGuiFont() );
      // Create the tab control
      m_ctrlTab.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, IDC_COOLTAB);
      m_ctrlTab.SetExtendedStyle(0, TCS_EX_SELHIGHLIGHT | TCS_EX_FLATSEPARATORS);
      return lRes;
   };
   LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      UpdateLayout();
      return 0;
   };
   LRESULT OnSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      // Someone clicked on a tab...
      int iIndex = m_ctrlTab.GetCurSel();
      if( iIndex < 0 ) return 0;
      TCITEM tci = { 0 };
      tci.mask = TCIF_PARAM;
      m_ctrlTab.GetItem(iIndex, &tci);
      m_wndMDIClient.SendMessage(WM_MDIACTIVATE, (WPARAM) tci.lParam);
      m_wndMDIClient.SetFocus();
      return 0;
   }
   LRESULT OnRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      // Someone right-clicked on a tab...
      int iIndex = m_ctrlTab.GetCurSel();
      if( iIndex < 0 ) return 0;
      POINT pt;
      ::GetCursorPos(&pt);
      CMenu menu;
      menu.LoadMenu(IDM_COOLTAB);
      CMenuHandle submenu;
      submenu = menu.GetSubMenu(0);
      CDummyElement Element(_T("Popup"), _T("TabMenu"));
      UINT nCmd = g_pDevEnv->ShowPopupMenu(&Element, submenu, pt);
      if( nCmd == 0 ) return 0;
      switch( nCmd ) {
      case ID_TAB_ACTIVATE:
         {
            TCITEM tci = { 0 };
            tci.mask = TCIF_PARAM;
            m_ctrlTab.GetItem(iIndex, &tci);
            ::SetFocus((HWND)tci.lParam);
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
            HWND hWnd = (HWND) lParam;
            int nLen = ::GetWindowTextLength(hWnd) + 1;
            LPTSTR pstrText = (LPTSTR) _alloca(nLen * sizeof(TCHAR));
            ::GetWindowText(hWnd, pstrText, nLen);
            CString sName = pstrText;
            CString sFileName = pstrText;
            _LookupViewNames(hWnd, sName, sFileName);
            // Create a new tab
            COOLTCITEM tci;
            tci.mask = TCIF_TEXT | TCIF_TOOLTIP | TCIF_PARAM;
            tci.pszText = (LPTSTR) (LPCTSTR) sName;
            tci.pszTipText = (LPTSTR) (LPCTSTR) sFileName;
            tci.lParam = lParam;
            m_ctrlTab.InsertItem(m_ctrlTab.GetItemCount(), &tci);
            UpdateLayout();
            // Maximize view
            if( m_bVisible ) m_wndMDIClient.SendMessage(WM_MDIMAXIMIZE, (WPARAM) hWnd, 0);
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
            // The call will eventually calls OnUserViewChange() which 
            // clears the view menus & tools.
            UpdateButtons();
            UpdateLayout();
         }
         break;
      }
      bHandled = FALSE; // MDI stuff relies on this, so...
      return 0;
   }

   // Operations

   void OnIdle()
   {
      BOOL bMaximized = FALSE;
      HWND hWndActive = (HWND) m_wndMDIClient.SendMessage(WM_MDIGETACTIVE, 0, (LPARAM) &bMaximized);
      static HWND s_hWndLastMDIClient = NULL;
      if( s_hWndLastMDIClient != hWndActive ) {
         s_hWndLastMDIClient = hWndActive;
         UpdateButtons();
      }
   }

   void UpdateLayout()
   {
      if( !IsWindowVisible() ) return;
      RECT rc;
      GetClientRect(&rc);
      // Place tab control in frame if populated
      int cy = ::GetSystemMetrics(SM_CYMENUSIZE) + 4;
      if( m_ctrlTab.GetItemCount() > 0 && m_bVisible ) {
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

   void _LookupViewNames(HWND hWnd, CString& sName, CString& sFilename)
   {
      CWinProp prop = hWnd;
      IElement* pElement = NULL;
      prop.GetProperty(_T("View"), (LPVOID&) pElement);
      if( pElement == NULL ) return;      
      pElement->GetName(sName.GetBufferSetLength(128), 128);
      sName.ReleaseBuffer();
      CComDispatchDriver dd = pElement->GetDispatch();
      if( dd == NULL ) return;
      CComVariant v;
      dd.GetPropertyByName(OLESTR("Filename"), &v);
      if( v.vt != VT_BSTR ) return;
      sFilename = v.bstrVal;
   }
};


#endif // !defined(AFX_MDICONTAINER_H__20020426_0903_1B62_2BDF_0080AD509054__INCLUDED_)

