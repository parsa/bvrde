
#include "StdAfx.h"
#include "resource.h"

#include "MainFrm.h"

#include "RegSerializer.h"


// Implementation

void CMainFrame::UIReset()
{   
   IProject* pCurProject = g_pSolution->GetActiveProject();

   // Set all menus and toolbars back to default
   UIClear();
   UISetCheck(ID_VIEW_TOOLBAR, m_DefaultToolBar.IsWindowVisible());
   UISetCheck(ID_VIEW_STATUS_BAR, m_StatusBar.IsWindowVisible());
   UIEnable(ID_NEW_SOLUTION, m_bInitialized, TRUE);
   UIEnable(ID_FILE_OPENSOLUTION, m_bInitialized, TRUE);
   UIEnable(ID_FILE_CLOSESOLUTION, pCurProject != NULL, TRUE);
   UIEnable(ID_FILE_SAVE_ALL, pCurProject != NULL, TRUE);
   UIEnable(ID_FILE_SAVE, FALSE, TRUE);
   UIEnable(ID_FILE_PRINT, FALSE, TRUE);
   UIEnable(ID_VIEW_OPEN, FALSE, m_bInitialized);
   UIEnable(ID_VIEW_OUTPUT, TRUE, TRUE);
   UIEnable(ID_VIEW_COMMAND, TRUE, TRUE);
   UIEnable(ID_EDIT_UNDO, FALSE, TRUE);
   UIEnable(ID_EDIT_REDO, FALSE, TRUE);
   UIEnable(ID_EDIT_COPY, FALSE, TRUE);
   UIEnable(ID_EDIT_CUT, FALSE, TRUE);
   UIEnable(ID_EDIT_PASTE, FALSE, TRUE);
   UIEnable(ID_EDIT_CLEAR, FALSE, TRUE);
   UIEnable(ID_EDIT_CLEAR_ALL, FALSE, TRUE);
   UIEnable(ID_EDIT_GOTO, FALSE, TRUE);
   UIEnable(ID_EDIT_FIND, FALSE, TRUE);
   UIEnable(ID_EDIT_REPLACE, FALSE, TRUE);
   UIEnable(ID_EDIT_SELECT_ALL, FALSE, TRUE);
}

void CMainFrame::UISetMenu(HMENU hMenu)
{
   // Attach to Command Bar
   CMenuHandle menu = hMenu;
   
   // Append the MRU project list to menu
   CMenuHandle menuFile = menu.GetSubMenu(0);
   CMenuHandle menuMru = menuFile.GetSubMenu(menuFile.GetMenuItemCount() - 3);
   m_mru.SetMenuHandle(menuMru);
   m_mru.UpdateMenu();

   // Set up User Tools in menu
   CMenuHandle menuTools = menu.GetSubMenu(3);
   CRegSerializer arc;
   if( arc.Open(REG_BVRDE _T("\\Tools")) ) {
      int iMenuPos = 3;
      int i = 0;
      while( true ) {
         TCHAR szBuffer[300] = { 0 };
         ::wsprintf(szBuffer, _T("Tool%ld"), i + 1);
         if( !arc.ReadGroupBegin(szBuffer) ) break;
         arc.Read(_T("title"), szBuffer, 299);
         menuTools.InsertMenu(iMenuPos++, MF_BYPOSITION, ID_TOOLS_TOOL1 + i, szBuffer);
         arc.ReadGroupEnd();
         i++;
      }
      arc.Close();
   }

   // Allow plugins to customize menus
   for( int i = 0; i < g_aPlugins.GetSize(); i++ ) g_aPlugins[i].SetMenu(menu);

   // Realize menu
   m_CmdBar.AttachMenu(menu);
}

UINT CMainFrame::_ShowMessageBox(HWND hWnd, UINT nMessage, UINT nCaption, DWORD dwFlags)
{
   return ShowMessageBox(hWnd, CString(MAKEINTRESOURCE(nMessage)), CString(MAKEINTRESOURCE(nCaption)), dwFlags);
}

void CMainFrame::_ArrangeToolBars()
{
   // Sort toolbars according to position
   int nCount = m_aToolBars.GetSize();
   for( int x = 0; x < nCount; x++ ) {
      for( int y = x + 1; y < nCount; y++ ) {
         if( m_aToolBars[x].iPosition > m_aToolBars[y].iPosition ) {
            TOOLBAR tb = m_aToolBars[x];
            m_aToolBars[x] = m_aToolBars[y];
            m_aToolBars[y] = tb;
         }
      }
   }
   // Add toolbars to the ReBar control now.
   // NOTE: Upon startup we will only display the "default" toolbar. All project
   //       toolbars are kept hidden until a project is loaded and it forces them
   //       to display.
   for( int i = 0; i < nCount; i++ ) {
      TOOLBAR& tb = m_aToolBars[i];
      if( i == 0 ) tb.bNewRow = TRUE;
      if( !m_CmdBar.AddToolbar(tb.hWnd) ) continue;
      if( !AddSimpleReBarBand(tb.hWnd, NULL, tb.bNewRow, 0, TRUE) ) continue;
      tb.nBand = m_Rebar.GetBandCount() - 1;
      m_Rebar.ShowBand(tb.nBand, _tcscmp(tb.szID, _T("Default")) == 0);
   }
   // Adjust sizes (Experimental; Windows XP only! Crashes on Windows 98/2000)
   DWORD dwMajor = 0;
   DWORD dwMinor = 0;
   AtlGetCommCtrlVersion(&dwMajor, &dwMinor);
   if( dwMajor >= 6 ) {
      for( int j = 0; j < nCount; j++ ) {
         TOOLBAR& tb = m_aToolBars[j];
         if( tb.bNewRow && !tb.bShowDefault && j > 2 ) {
            m_Rebar.MinimizeBand(tb.nBand);
            m_Rebar.MaximizeBand(tb.nBand, FALSE);
         }
      }
   }
   m_CmdBar.Prepare();
}

void CMainFrame::_AddDropDownButton(CToolBarCtrl tb, UINT nID)
{
   tb.SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS); 

   TBBUTTONINFO tbi = { 0 };
   tbi.cbSize  = sizeof(TBBUTTONINFO);
   tbi.dwMask  = TBIF_STYLE;
   tb.GetButtonInfo(nID, &tbi);
   tbi.fsStyle |= BTNS_WHOLEDROPDOWN;
   tb.SetButtonInfo(nID, &tbi);
}

void CMainFrame::_AddTextButton(CToolBarCtrl tb, UINT nID, UINT nRes)
{  
   tb.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);

   TBBUTTONINFO tbi = { 0 };
   tbi.cbSize = sizeof(TBBUTTONINFO),
   tbi.dwMask  = TBIF_STYLE;
   tb.GetButtonInfo(nID, &tbi);
   tbi.dwMask = TBIF_STYLE | TBIF_TEXT;
   tbi.fsStyle |= TBSTYLE_AUTOSIZE | BTNS_SHOWTEXT;
   CString s(MAKEINTRESOURCE(nRes));
   tbi.pszText = (LPTSTR) (LPCTSTR) s;
   tb.SetButtonInfo(nID, &tbi);
}

// CMessageMap

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
   if( CMDIFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg) ) return TRUE;

   __try
   {
      for( int i = m_aAppListeners.GetSize() - 1; i >= 0; --i ) if( m_aAppListeners[i]->PreTranslateMessage(pMsg) ) return TRUE;
   }
   __except(1)
   {
      // Ignore...
      ATLASSERT(false);
   }

   if( !m_UserAccel.IsNull() && m_UserAccel.TranslateAccelerator(m_hWnd, pMsg) ) return TRUE;
   if( !m_MacroAccel.IsNull() && m_MacroAccel.TranslateAccelerator(m_hWnd, pMsg) ) return TRUE;

   HWND hWnd = MDIGetActive();
   if( hWnd != NULL ) return (BOOL) ::SendMessage(hWnd, WM_FORWARDMSG, 0, (LPARAM) pMsg);

   return FALSE;
}

BOOL CMainFrame::OnIdle()
{
   static IView* s_pOldView = NULL;
   static IElement* s_pOldElement = NULL;
   static BOOL s_bOldDirtyFlag = NULL;

   m_Dock.OnIdle();
   m_MDIContainer.OnIdle();

   IProject* pCurProject = g_pSolution->GetActiveProject();
   HWND hWnd = MDIGetActive();  

   UIEnable(ID_VIEW_OPEN, FALSE);
   UIEnable(ID_VIEW_PROPERTIES, s_pOldElement != NULL);

   UIEnable(ID_FILE_SAVE_ALL, pCurProject != NULL);
   UIEnable(ID_FILE_CLOSESOLUTION, pCurProject != NULL);
   UIEnable(ID_TOOLS_CUSTOMIZE, !m_bFullScreen);
   UIEnable(ID_MACRO_RECORD, !m_bRecordingMacro);
   UIEnable(ID_MACRO_CANCEL, m_bRecordingMacro);
   UIEnable(ID_MACRO_SAVE, !m_sMacro.IsEmpty());
   UIEnable(ID_MACRO_PLAY, !m_sMacro.IsEmpty());
   UISetCheck(ID_VIEW_EXPLORER, m_viewExplorer.IsWindowVisible());
   UISetCheck(ID_VIEW_PROPERTYBAR, m_viewProperties.IsWindowVisible());
   UIEnable(ID_WINDOW_CLOSE, hWnd != NULL);
   UIEnable(ID_WINDOW_CLOSE_ALL, hWnd != NULL);
   UIEnable(ID_WINDOW_CASCADE, hWnd != NULL);
   UIEnable(ID_WINDOW_TILE_HORZ, hWnd != NULL);
   UIEnable(ID_WINDOW_TILE_VERT, hWnd != NULL);
   UIEnable(ID_WINDOW_ARRANGE, hWnd != NULL);
   UIEnable(ID_WINDOW_PREVIOUS, hWnd != NULL);
   UIEnable(ID_WINDOW_NEXT, hWnd != NULL);

   for( int i = 0; i < m_aIdleListeners.GetSize(); i++ ) ATLTRY( m_aIdleListeners[i]->OnIdle(this) );

   // HACK: To allow the Copy menuitem to become active when
   //       focus is on a regular EDIT control.
   HWND hWndFocus = ::GetFocus();
   if( AtlIsEditControl(hWndFocus) ) {
      CEdit ctrlEdit = hWndFocus;
      int iStart = 0;
      int iEnd = 0;
      ctrlEdit.GetSel(iStart, iEnd);
      UIEnable(ID_EDIT_COPY, iEnd > iStart);
      UIEnable(ID_EDIT_CUT, iEnd > iStart);
      UIEnable(ID_EDIT_PASTE, ::IsClipboardFormatAvailable(CF_TEXT));
      UIEnable(ID_EDIT_UNDO, ctrlEdit.CanUndo());
   }

   // Here we determine if the view/tree-focus has changed and
   // update the Properties pane with the active view's properties.
   CTreeViewCtrl& ctrlTree = m_viewExplorer.m_viewFile.m_ctrlTree;
   HTREEITEM hItem = ctrlTree.GetSelectedItem();
   if( hItem ) {
      IElement* pElement = (IElement*) ctrlTree.GetItemData(hItem);
      if( pElement != s_pOldElement ) {
         s_pOldElement = pElement;
         ShowProperties(pElement, FALSE);
         UISetText(0, CString(MAKEINTRESOURCE(ATL_IDS_IDLEMESSAGE)));
      }
   }

   // Here we determine if the active view suddenly changed dirty
   // state and updates the MDI Tab folders.
   if( hWnd != NULL ) {
      IView* pView = NULL;
      CWinProp prop = hWnd;
      prop.GetProperty(_T("View"), pView);
      if( pView != NULL ) {
         BOOL bDirty = pView->IsDirty();
         if( pView != s_pOldView || bDirty != s_bOldDirtyFlag ) {
            m_MDIContainer.ChangeDirtyState(hWnd);
         }
         s_bOldDirtyFlag = bDirty;
      }
      s_pOldView = pView;
   }

   UIUpdateToolBar();
   UIUpdateStatusBar();

   return FALSE;
}

// IUpdateUI

BOOL CMainFrame::UIEnable(INT nID, BOOL bEnable, BOOL bForceUpdate /* = FALSE*/)
{
   return CUpdateDynamicUI<CMainFrame>::UIEnable(nID, bEnable, bForceUpdate);
}

BOOL CMainFrame::UISetCheck(INT nID, INT nCheck, BOOL bForceUpdate /* = FALSE*/)
{
   return CUpdateDynamicUI<CMainFrame>::UISetCheck(nID, nCheck, bForceUpdate);
}

BOOL CMainFrame::UISetRadio(INT nID, BOOL bRadio, BOOL bForceUpdate /* = FALSE*/)
{
   return CUpdateDynamicUI<CMainFrame>::UISetRadio(nID, bRadio, bForceUpdate);
}

BOOL CMainFrame::UISetText(INT nID, LPCTSTR lpstrText, BOOL bForceUpdate /* = FALSE*/)
{
   return CUpdateDynamicUI<CMainFrame>::UISetText(nID, lpstrText, bForceUpdate);
}


