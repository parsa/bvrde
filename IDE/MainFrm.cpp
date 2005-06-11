// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainFrm.h"

#include "ChooseSolutionDlg.h"
#include "ArgumentPromptDlg.h"

#include "RegSerializer.h"
#include "XmlSerializer.h"
#include "DummyElement.h"
#include "atlcmdline.h"
#include "Macro.h"

#include <htmlhelp.h>


CMainFrame::CMainFrame() :
   m_Dispatch(this),
   m_bFullScreen(FALSE),
   m_bInitialized(FALSE),
   m_bRecordingMacro(FALSE)
{
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   g_pDevEnv = this;

   ShowWindow(SW_HIDE);

   // Load settings properties
   _LoadSettings();

   // Create command bar (menu) window
   m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
   m_CmdBar.AttachMenu(GetMenu());
   m_CmdBar.LoadImages(IDR_MAINFRAME);
   m_CmdBar.LoadImages(IDR_FULLSCREEN);
   m_CmdBar.LoadImages(IDR_TOOLIMAGES);
   SetMenu(NULL);

   // Create default toolbar
   m_DefaultToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE | TBSTYLE_LIST & ~TBSTYLE_TOOLTIPS);
   _AddDropDownButton(m_DefaultToolBar, ID_POPUP_NEW);
   _AddDropDownButton(m_DefaultToolBar, ID_POPUP_ADD);
   _AddTextButton(m_DefaultToolBar, ID_POPUP_NEW, IDS_NEW);
   _AddTextButton(m_DefaultToolBar, ID_POPUP_ADD, IDS_ADD);
   m_DefaultToolBar.AutoSize();

   // Add the FullScreen toolbar too
   m_FullScreenToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_FULLSCREEN, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE | TBSTYLE_LIST & ~TBSTYLE_TOOLTIPS);
   _AddTextButton(m_FullScreenToolBar, ID_VIEW_FULLSCREEN, IDS_FULLSCREEN);

   // Create Rebar and put the menu and toolbar in it
   CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE | RBS_DBLCLKTOGGLE);
   m_Rebar = m_hWndToolBar;

   AddSimpleReBarBand(m_CmdBar);
   m_CmdBar.AddToolbar(m_CmdBar);

   AddToolBar(m_DefaultToolBar, CString(MAKEINTRESOURCE(IDS_MAIN_TOOLBAR)));
   AddToolBar(m_FullScreenToolBar, CString(MAKEINTRESOURCE(IDS_FULLSCREEN_TOOLBAR)));

   // Create MDI client
   CreateMDIClient();
   m_CmdBar.SetMDIClient(m_hWndMDIClient);

   // Create StatusBar
   m_hWndStatusBar = m_StatusBar.Create(m_hWnd);
   int aPanes[] = { ID_DEFAULT_PANE, ID_SB_PANE1, ID_SB_PANE2, ID_SB_PANE3 };
   m_StatusBar.SetPanes(aPanes, sizeof(aPanes)/sizeof(int));
   m_StatusBar.SetPaneWidth(ID_SB_PANE1, 20);
   m_StatusBar.SetText(1, NULL, SBT_OWNERDRAW);

   // Docking Windows
   m_Dock.Create(m_hWnd, rcDefault);
   m_Dock.SetExtendedDockStyle(DCK_EX_REMEMBERSIZE);

   // Load UI settngs and change pane sizes
   _LoadUIState();

   // Create docked views
   CString sExplorerName(MAKEINTRESOURCE(IDS_SOULTIONEXPLORER));
   m_viewExplorer.Create(m_Dock, rcDefault, sExplorerName, ATL_SIMPLE_DOCKVIEW_STYLE);
   m_Dock.AddWindow(m_viewExplorer);
   TCHAR szBuffer[64] = { 0 };
   GetProperty(_T("window.explorer.cy"), szBuffer, 63);
   m_Dock.DockWindow(m_viewExplorer, DOCK_RIGHT, _ttoi(szBuffer));

   CString sPropertiesName(MAKEINTRESOURCE(IDS_PROPERTIES));
   m_viewProperties.Create(m_Dock, rcDefault, sPropertiesName, ATL_SIMPLE_DOCKVIEW_STYLE);
   m_Dock.AddWindow(m_viewProperties);
   GetProperty(_T("window.properties.cy"), szBuffer, 63);
   m_Dock.DockWindow(m_viewProperties, DOCK_RIGHT, _ttoi(szBuffer));

   // Attach MDI Container (manages the MDI Client and tabbed frame)
   DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
   m_MDIContainer.Create(m_Dock, rcDefault, NULL, dwStyle);
   m_MDIContainer.SetMDIClient(m_hWndMDIClient);

   // Create docked views
   m_Dock.SetClient(m_MDIContainer);

   GetProperty(_T("window.pane.left"), szBuffer, 63);
   m_Dock.SetPaneSize(DOCK_LEFT, _ttoi(szBuffer));
   GetProperty(_T("window.pane.right"), szBuffer, 63);
   m_Dock.SetPaneSize(DOCK_RIGHT, _ttoi(szBuffer));
   GetProperty(_T("window.pane.top"), szBuffer, 63);
   m_Dock.SetPaneSize(DOCK_TOP, _ttoi(szBuffer));
   GetProperty(_T("window.pane.bottom"), szBuffer, 63);
   m_Dock.SetPaneSize(DOCK_BOTTOM, _ttoi(szBuffer));

   // The AutoHide control
   m_Images.Create(IDB_EXPLORER, 16, 1, RGB(255,0,255));
   m_AutoHide.Create(m_hWnd, rcDefault);
   m_AutoHide.SetImageList(m_Images);
   m_AutoHide.SetClient(m_Dock);
   m_hWndClient = m_AutoHide;

   // AutoHide views
   CString s;
   s.LoadString(IDS_CAPTION_OUTPUT);
   dwStyle = ES_MULTILINE | ES_NOHIDESEL | ES_SAVESEL | ES_READONLY | ES_DISABLENOSCROLL | 
             WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VSCROLL;
   m_viewOutput.Create(m_AutoHide, CWindow::rcDefault, s, dwStyle, WS_EX_CLIENTEDGE);
   ATLASSERT(m_viewOutput.IsWindow());

   s.LoadString(IDS_CAPTION_COMMAND);
   dwStyle = ES_MULTILINE | ES_WANTRETURN | ES_NOHIDESEL | ES_SAVESEL | ES_DISABLENOSCROLL | 
             WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VSCROLL;
   m_viewCommand.m_pMainFrame = this;
   m_viewCommand.Create(m_AutoHide, CWindow::rcDefault, s, dwStyle, WS_EX_CLIENTEDGE);
   ATLASSERT(m_viewCommand.IsWindow());

   AddAutoHideView(m_viewOutput, IDE_DOCK_BOTTOM, 1);
   AddAutoHideView(m_viewCommand, IDE_DOCK_BOTTOM, 6);

   m_viewOutput.Clear();
   m_viewCommand.Clear();

   // Set up MRU stuff
   CMenuHandle menu = m_CmdBar.GetMenu();
   CMenuHandle menuFile = menu.GetSubMenu(0);
   CMenuHandle menuMru = menuFile.GetSubMenu(menuFile.GetMenuItemCount() - 3);
   m_mru.SetMenuHandle(menuMru);
   m_mru.SetMaxEntries(4);
   m_mru.ReadFromRegistry(REG_BVRDE _T("\\Mru"));

   // We can drop files on the MDI window
   ::DragAcceptFiles(m_hWnd, TRUE);

   // Prepare UI elements
   UIAddStatusBar(m_StatusBar);
   UISetMenu(m_CmdBar.GetMenu());
   UISetBlockAccelerators(true);

   // Register object for message filtering and idle updates
   CMessageLoop* pLoop = static_cast<CMessageLoopEx*>(_Module.GetMessageLoop());
   ATLASSERT(pLoop);
   pLoop->AddMessageFilter(this);
   pLoop->AddIdleHandler(this);

   g_pSolution->Init(this);

   AddWizardListener(this);

   SendMessage(WM_SETTINGCHANGE);

   return 0;
}

LRESULT CMainFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   g_pDevEnv->ShowStatusText(ID_DEFAULT_PANE, CString(MAKEINTRESOURCE(IDS_STATUS_CLOSING)), TRUE);

   // Not accepting file drops anymore
   ::DragAcceptFiles(m_hWnd, FALSE);

   // Force all windows to close (Using SendMessage() because we mean it)
   HWND hWnd  = ::GetWindow(m_hWndMDIClient, GW_CHILD);
   while( hWnd != NULL ) {
      HWND hWndClose = hWnd;
      hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
      ::SendMessage(hWndClose, WM_CLOSE, 0, 0L);
   }
   
   // Close solution nicely
   SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_CLOSESOLUTION, 0));

   CLockWindowUpdate lock = m_hWnd;

   // Don't leave as fullscreen
   if( m_bFullScreen ) SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_FULLSCREEN, 0));

   // FIX: Removing the additional ReBars help keep the
   //      original workspace size when saving/restoring settings.
   while( m_Rebar.DeleteBand(2) ) /* */;

   _SaveUIState();   
   _SaveSettings();

   m_AnimateImages.Destroy();

   bHandled = FALSE;
   return 0;
}

LRESULT CMainFrame::OnMenuSelect(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   // NOTE: This method overrides the CMDIFrameWindowImpl::OnMenuSelect
   //       because of the special statusbar text-update handling.

   if( m_hWndStatusBar == NULL ) return 1;

   WORD wFlags = HIWORD(wParam);
   if( wFlags == 0xFFFF && lParam == NULL ) {
      // Menu closing
      ::SendMessage(m_hWndStatusBar, SB_SIMPLE, FALSE, 0L);
   }
   else {
      TCHAR szBuff[256] = { ' ', 0 };
      if( (wFlags & MF_POPUP) == 0 ) {
         UINT wID = LOWORD(wParam);
         // check for special cases
         if( wID >= 0xF000 && wID < 0xF1F0 )                            // system menu IDs
            wID = (WORD)(((wID - 0xF000) >> 4) + ATL_IDS_SCFIRST);
         else if( wID >= ID_FILE_MRU_FIRST && wID <= ID_FILE_MRU_LAST ) // MRU items
            wID = ATL_IDS_MRU_FILE;
         else if( wID >= ID_TOOLS_TOOL1 && wID <= ID_TOOLS_TOOL8 )
            wID = IDS_EXTERNALTOOL;
         else if( wID >= ATL_IDM_FIRST_MDICHILD )                       // MDI child windows
            wID = ATL_IDS_MDICHILD;
         // We translate it first if possible
         AtlLoadString(wID, szBuff + 1, 254);
         // Let other plugins translate it
         for( int i = 0; i < m_aIdleListeners.GetSize(); i++ ) {
            TCHAR szText[256] = { 0 };
            m_aIdleListeners[i]->OnGetMenuText(wID, szText, 254);
            if( szText[0] != '\0' ) _tcsncpy(szBuff + 1, szText, 254);
         }
         // Text may be formatted as:
         //    MenuDescription\nToolbarText
         // We'll extract the last part if possible
         LPTSTR p = _tcschr(szBuff, '\n');
         if( p != NULL ) *p = '\0';
         // Update statusbar
         ::SendMessage(m_hWndStatusBar, SB_SIMPLE, TRUE, 0L);
         ::SendMessage(m_hWndStatusBar, SB_SETTEXT, (255 | SBT_NOBORDERS), (LPARAM) szBuff);
      }
   }

   return 1;
}

LRESULT CMainFrame::OnToolTipText(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   // NOTE: This method overrides the CMDIFrameWindowImpl::OnToolTipText
   //       because of the special toolbar tooltip-update handling.

   LPNMTTDISPINFO pDispInfo = (LPNMTTDISPINFO) pnmh;
   pDispInfo->szText[0] = 0;
   if( (idCtrl != 0) && !(pDispInfo->uFlags & TTF_IDISHWND) ) {
      // We translate it first if possible
      int cchBuff = sizeof(pDispInfo->szText) / sizeof(pDispInfo->szText[0]);
      AtlLoadString(idCtrl, pDispInfo->szText, cchBuff);
      // Let other plugins translate it
      for( int i = 0; i < m_aIdleListeners.GetSize(); i++ ) {
         TCHAR szText[256] = { 0 };
         m_aIdleListeners[i]->OnGetMenuText(idCtrl, szText, 255);
         if( szText[0] != '\0' ) _tcsncpy(pDispInfo->szText, szText, cchBuff);
      }
      // Text may be formatted as:
      //    MenuDescription\nToolbarText
      // We'll extract the last part if possible
      LPTSTR p = _tcschr(pDispInfo->szText, '\n');
      if( p != NULL ) _tcscpy(pDispInfo->szText, p + 1);
   }
   return 0;
}

LRESULT CMainFrame::OnDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   // Support dropping files on the MDI window
   CWaitCursor cursor;
   HDROP hDrop = (HDROP) wParam;
   TCHAR szFilename[MAX_PATH + 1] = { 0 };
   int nCount = (int) ::DragQueryFile(hDrop, (UINT) -1, szFilename, MAX_PATH);
   for( int i = 0; i < nCount; i++ ) {
      ::DragQueryFile(hDrop, (int) i, szFilename, MAX_PATH);
      if( _tcslen(szFilename) == 0 ) continue;
      IView* pView = CreateView(szFilename);
      if( pView ) ATLTRY( pView->OpenView(0) );
   }
   return 0;
}

LRESULT CMainFrame::OnCopyData(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
   // Handle WM_COPYDATA for the sake of only starting a single instance
   // of the application. We'll use this IPC method for communicating the
   // command-line parameters.
   COPYDATASTRUCT* pCDS = (COPYDATASTRUCT*) lParam;
   ATLASSERT(pCDS);
   if( ::IsBadReadPtr(pCDS, sizeof(COPYDATASTRUCT)) ) return 0;
   switch( pCDS->dwData ) {
   case 1:
      {
         // Parse command line
         CCommandLine cmd;
         SendMessage(WM_APP_COMMANDLINE, 0, (LPARAM) pCDS->lpData);
         if( IsIconic() ) ShowWindow(SW_RESTORE);
      }
      break;
   default:
      ::MessageBeep((UINT)-1);
      break;
   }
   return 0;
}

LRESULT CMainFrame::OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   // Need to show MDI container/CoolTabs?
   TCHAR szBuffer[64] = { 0 };
   GetProperty(_T("gui.main.client"), szBuffer, 63);
   m_MDIContainer.SetVisible(_tcscmp(szBuffer, _T("mdi")) != 0);
   // Update view layout
   UpdateLayout();
   // Relay to all children
   SendMessageToDescendants(WM_SETTINGCHANGE);
   return 0;
}

LRESULT CMainFrame::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   bHandled = FALSE;
   if( wParam == ANIMATE_TIMERID ) 
   {
      if( m_iAnimatePos < 0 ) return 0;
      // Draw the little statusbar animation
      // TODO: How to prevent that nasty animation flickering?
      m_iAnimatePos = (m_iAnimatePos + 1) % m_AnimateImages.GetImageCount();
      RECT rcItem = { 0 };
      m_StatusBar.GetRect(1, &rcItem);
      m_StatusBar.InvalidateRect(&rcItem, FALSE);
   }
   else if( wParam == DELAY_TIMERID ) 
   {
      // Delay load some of the external libraries...
      KillTimer(DELAY_TIMERID);
      ::LoadLibrary(_T("GenEdit.dll"));
      ::LoadLibrary(_T("SciLexer.dll"));
      ::LoadLibrary(_T("CppLexer.dll"));
   }
   return 0;
}

LRESULT CMainFrame::OnHelp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   SendMessage(WM_COMMAND, MAKEWPARAM(ID_HELP_CONTENTS, 0));
   return 0;
}

LRESULT CMainFrame::OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
   bHandled = FALSE;
   LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT) lParam;
   if( lpDIS->hwndItem != m_StatusBar ) return 0;
   if( m_iAnimatePos < 0 ) return 0;
   // Draw the statusbar pane and animation
   ATLASSERT(!m_AnimateImages.IsNull());
   RECT rcItem = { 0 };
   m_StatusBar.GetRect(1, &rcItem);
   CClientDC dc = m_StatusBar;
   m_AnimateImages.Draw(dc, m_iAnimatePos, rcItem.left, rcItem.top + 1, ILD_TRANSPARENT);
   bHandled = TRUE;
   return 0;
}

LRESULT CMainFrame::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   // All messages are forwarded to the clients (listeners).
   // Most clients hook this to intercept WM_COMMAND messages.
   // This is blunt overkill, and we should really consider a 
   // pure WM_COMMAND message listener interface instead.
   __try
   {
      bHandled = FALSE;
      for( int i = m_aAppListeners.GetSize() - 1; i >= 0; --i ) {
         LRESULT lRes = m_aAppListeners[i]->OnAppMessage(m_hWnd, uMsg, wParam, lParam, bHandled);
         if( bHandled ) return lRes;
      }
   }
   __except(1)
   {
      // Ignore; even if this is serious!
      // You need to fix your plugin code!
      ATLASSERT(false);
   }
   return 0;
}

LRESULT CMainFrame::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   // NOTE: This is a copy of the WTL CUpdateUIBase::OnCommand message handler.
   //       I just wanted a MessageBeep() in there somewhere.
   bHandled = FALSE;
   if( m_bBlockAccelerators && HIWORD(wParam) == 1 )   // accelerators only
   {
      int nID = LOWORD(wParam);
      if( (UIGetState(nID) & UPDUI_DISABLED) == UPDUI_DISABLED ) {
         ATLTRACE2(atlTraceUI, 0, _T("CUpdateUIBase::OnCommand - blocked disabled command 0x%4.4X\n"), nID);
         ::MessageBeep((UINT)-1);
         bHandled = TRUE;   // Eat the command; UI item is disabled
      }
   }
   // Recording a macro? Cool, we record all WM_COMMAND messages and
   // replay them in the script.
   if( m_bRecordingMacro ) {
      // There's a number of commands we're not very interested in
      // recording...
      switch( LOWORD(wParam) ) {
      case ID_MACRO_RECORD:
      case ID_MACRO_CANCEL:
      case ID_MACRO_PLAY:
      case ID_MACRO_SAVE:
         return 0;
      }
      CString sMacro;
      sMacro.Format(_T("Call App.SendRawMessage(%ld,%ld,%ld)"), 
         (long) uMsg, 
         (long) wParam, 
         (long) lParam);
      RecordMacro(sMacro);
   }
   return 0;
}

LRESULT CMainFrame::OnFileOpen(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CString sFilter(MAKEINTRESOURCE(IDS_FILTER_ALL));
   for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == _T('|') ) sFilter.SetAt(i, _T('\0'));
   DWORD dwStyle = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ENABLESIZING;
   CFileDialog dlg(TRUE, NULL, NULL, dwStyle, sFilter, m_hWnd);
   dlg.m_ofn.Flags &= ~OFN_ENABLEHOOK;
   if( dlg.DoModal() != IDOK ) return 0;
   // Create the new view. It will not be attached to any project.
   // TODO: Consider attaching to current project?
   CWaitCursor cursor;
   IView* pView = CreateView(dlg.m_ofn.lpstrFile, NULL, NULL);
   if( pView ) ATLTRY( pView->OpenView(0) );
   return 0;
}

LRESULT CMainFrame::OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CWaitCursor cursor;
   PlayAnimation(TRUE, ANIM_SAVE);
   // TODO: Don't rely on messages; use IView::Save()
   CWindow wnd = MDIGetActive();
   wnd.SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE, 0));
   PlayAnimation(FALSE, 0);
   return 0;
}

LRESULT CMainFrame::OnFileSaveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CWaitCursor cursor;
   PlayAnimation(TRUE, ANIM_SAVE);
   // TODO: Don't rely on messages
   CWindow(m_hWndMDIClient).SendMessageToDescendants(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE, 0));
   if( g_pSolution->IsLoaded() ) g_pSolution->SaveSolution(NULL);
   m_MDIContainer.RefreshItems();
   PlayAnimation(FALSE, 0);
   return 0;
}

LRESULT CMainFrame::OnFileRecent(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   // Get file name from the MRU list
   TCHAR szFile[MAX_PATH + 1] = { 0 };
   if( !m_mru.GetFromList(wID, szFile) ) {
      ::MessageBeep(MB_ICONERROR);
      return 0;
   }
   m_mru.MoveToTop(wID);
   // Open solution file
   if( !SendMessage(WM_APP_LOADSOLUTION, 0, (LPARAM) szFile) ) {
      // Failed to load? Remove from MRU list
      m_mru.RemoveFromList(wID);
   }
   m_mru.WriteToRegistry(REG_BVRDE _T("\\Mru"));
   return 0;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   PostMessage(WM_CLOSE);
   return 0;
}

LRESULT CMainFrame::OnFileOpenSolution(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CChooseSolutionDlg dlg;
   dlg.Init(this);
   if( dlg.DoModal() != IDOK ) return 0;
   // Launch the solution
   switch( dlg.m_SelectType ) {
   case CChooseSolutionDlg::SOLUTION_BLANK:
      PostMessage(WM_APP_CLOSESTARTPAGE);
      PostMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_CLOSESOLUTION, 0));
      break;
   case CChooseSolutionDlg::SOLUTION_WIZARD:
      PostMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_STARTWIZARD, 0));
      break;
   case CChooseSolutionDlg::SOLUTION_FILE:
      SendMessage(WM_APP_LOADSOLUTION, 0, (LPARAM) (LPCTSTR) dlg.m_sFilename);
      break;
   }
   return 0;
}

LRESULT CMainFrame::OnFileCloseSolution(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   // Should we need to save the project first?
   if( g_pSolution->IsDirty() ) {
      if( IDYES == _ShowMessageBox(m_hWnd, IDS_SAVE_DIRTY, IDS_CAPTION_QUESTION, MB_ICONQUESTION | MB_YESNO) ) {
         SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE_ALL, 0));
      }
   }
   // Disable tool buttons etc
   CWaitCursor cursor;
   UIReset();   
   // Remove solution structures
   g_pSolution->Close();
   // Clear explorer tree as well
   m_viewProperties.Clear();
   m_viewExplorer.m_viewFile.Clear();
   return 0;
}

LRESULT CMainFrame::OnFileStartWizard(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   IProject* pProject = _CreateSolutionWizard();
   if( pProject == NULL ) return 0;
   SendMessage(WM_APP_CLOSESTARTPAGE);
   SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_CLOSESOLUTION, 0));
   g_pSolution->AddProject(pProject);
   g_pSolution->SetActiveProject(g_pSolution->GetItem(0));
   return 0;
}

LRESULT CMainFrame::OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   // Copy/Cut/Paste is handled before view gets a change. The
   // view must override the clipboard if possible!
   ::SendMessage(::GetFocus(), WM_COPY, 0, 0L);
   bHandled = FALSE;
   return 0;
}

LRESULT CMainFrame::OnNewSolution(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   return SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_OPENSOLUTION, 0));
}

LRESULT CMainFrame::OnNewProject(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   IProject* pProject = _CreateProjectWizard();
   if( pProject == NULL ) return 0;
   g_pSolution->AddProject(pProject);
   g_pSolution->SetActiveProject(g_pSolution->GetItem(g_pSolution->GetItemCount() - 1));
   return 0;
}

LRESULT CMainFrame::OnAddProject(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   // Browse for the project file (locally)
   CString sPath;
   sPath.Format(_T("%s%s"), CModulePath(), SOLUTIONDIR);
   CString sFilter(MAKEINTRESOURCE(IDS_FILTER_PROJECT));
   for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == _T('|') ) sFilter.SetAt(i, _T('\0'));
   DWORD dwStyle = OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ENABLESIZING;
   CFileDialog dlg(TRUE, _T("prj"), NULL, dwStyle, sFilter, m_hWnd);
   dlg.m_ofn.Flags &= ~OFN_ENABLEHOOK;
   dlg.m_ofn.lpstrInitialDir = sPath;
   if( dlg.DoModal() != IDOK ) return 0;
   // Add project to solution
   g_pSolution->AddProject(dlg.m_ofn.lpstrFile);
   return 0;
}

LRESULT CMainFrame::OnToolsRun(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   TCHAR szBuffer[32] = { 0 };
   g_pDevEnv->GetProperty(_T("editors.general.saveBeforeTool"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("true")) == 0 ) SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE_ALL, 0));

   // Get information about this tool
   CRegSerializer arc;
   if( !arc.Open(REG_BVRDE _T("\\Tools")) ) return 0;
   CString sKey;
   sKey.Format(_T("Tool%ld"), wID - ID_TOOLS_TOOL1 + 1L);
   if( !arc.ReadGroupBegin(sKey) ) return 0;
   TCHAR szCommand[300] = { 0 };
   TCHAR szArguments[300] = { 0 };
   TCHAR szPath[300] = { 0 };
   TCHAR szType[100] = { 0 };
   long lFlags = 0;
   arc.Read(_T("command"), szCommand, 299);
   arc.Read(_T("arguments"), szArguments, 299);
   arc.Read(_T("path"), szPath, 299);
   arc.Read(_T("type"), szType, 99);
   arc.Read(_T("flags"), lFlags);
   arc.Close();

   CString sCommand = szCommand;
   CString sArguments = szArguments;
   CString sPath = szPath;

   // Do replacement of special meta-tokens
   //    $PROJECTNAME$
   //    $PROJECTPATH$
   //    $HOSTNAME$
   //    $NAME$
   //    $FILENAME$
   //    $SELECTION$
   //    $APPPATH$
   HWND hWnd = MDIGetActive();
   IView* pView = NULL;
   IProject* pProject = NULL;  
   if( ::IsWindow(hWnd) ) {
      CWinProp prop = hWnd;
      prop.GetProperty(_T("Project"), pProject);
      prop.GetProperty(_T("View"), pView);
   }
   if( pProject == NULL ) {
      pProject = GetSolution()->GetActiveProject();
   }
   if( pProject != NULL ) {
      // Extract information from IDispatch interface
      CComDispatchDriver dd = pProject->GetDispatch();
      CComVariant vRet;
      vRet.Clear();
      dd.GetPropertyByName(L"Name", &vRet);
      if( vRet.vt == VT_BSTR ) sPath.Replace(_T("$PROJECTNAME$"), CString(vRet.bstrVal));
      if( vRet.vt == VT_BSTR ) sArguments.Replace(_T("$PROJECTNAME$"), CString(vRet.bstrVal));
      vRet.Clear();
      dd.GetPropertyByName(L"Filename", &vRet);
      if( vRet.vt == VT_BSTR ) sArguments.Replace(_T("$FILENAME$"), CString(vRet.bstrVal));
      vRet.Clear();
      dd.GetPropertyByName(L"CurDir", &vRet);
      if( vRet.vt == VT_BSTR ) sPath.Replace(_T("$PROJECTPATH$"), CString(vRet.bstrVal));
      if( vRet.vt == VT_BSTR ) sArguments.Replace(_T("$PROJECTPATH$"), CString(vRet.bstrVal));
      vRet.Clear();
      dd.GetPropertyByName(L"Server", &vRet);
      if( vRet.vt == VT_BSTR ) sArguments.Replace(_T("$HOSTNAME$"), CString(vRet.bstrVal));
   }
   if( pView != NULL ) {
      // Extract information from IDispatch interface
      CComDispatchDriver dd = pView->GetDispatch();
      CComVariant vRet;
      vRet.Clear();
      dd.Invoke0(L"GetSelection", &vRet);
      if( vRet.vt == VT_BSTR ) sArguments.Replace(_T("$SELECTION$"), CString(vRet.bstrVal));
   }
   sArguments.Replace(_T("$APPPATH$"), CModulePath());

   // Do we need to prompt for changes in arguments
   if( (lFlags & TOOLFLAGS_PROMPTARGS) != 0 ) {
      CArgumentPromptDlg dlg;
      dlg.m_sCommand = sCommand;
      dlg.m_sArguments = sArguments;
      if( dlg.DoModal() != IDOK ) return 0;
      sCommand = dlg.m_sCommand;
      sArguments = dlg.m_sArguments;
   }

   // Let's see who handles this command...
   BOOL bHandled = FALSE;
   for( int i = m_aCommandListeners.GetSize() - 1; i >= 0; --i ) {
      m_aCommandListeners[i]->OnMenuCommand(szType, sCommand, sArguments, sPath, (int) lFlags, bHandled);
      if( bHandled ) break;
   }

   return 0;
}

LRESULT CMainFrame::OnMacroShortcut(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   TCHAR szBuffer[32] = { 0 };
   g_pDevEnv->GetProperty(_T("editors.general.saveBeforeTool"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("true")) == 0 ) SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE_ALL, 0));

   CString sFilename;
   sFilename.Format(_T("%sBVRDE.xml"), CModulePath());
   CXmlSerializer arc;
   if( !arc.Open(_T("Settings"), sFilename) ) return 0;
   if( arc.ReadGroupBegin(_T("MacroMappings")) ) {
      while( arc.ReadGroupBegin(_T("Macro")) ) {
         CString sFilename;
         CString sFunction;
         long lCmd;
         arc.Read(_T("cmd"), lCmd);
         arc.Read(_T("filename"), sFilename);
         arc.Read(_T("function"), sFunction);
         arc.ReadGroupEnd();
         // If it's a match, then execute macro...
         if( lCmd == wID ) {
            CString sFullFilename = sFilename;
            sFullFilename.Format(_T("%s%s.vbs"), CModulePath(), sFilename);
            CComObject<CMacro>* pScript;
            HRESULT Hr = CComObject<CMacro>::CreateInstance(&pScript);
            if( FAILED(Hr) ) return 0;
            pScript->AddRef();
            pScript->Init(this, &m_Dispatch);
            pScript->RunMacroFromFile(sFullFilename, sFunction);
            pScript->Release();
         }
      }
      arc.ReadGroupEnd();
   }
   arc.Close();

   return 0;
}

LRESULT CMainFrame::OnViewExplorer(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( m_viewExplorer.IsWindowVisible() ) m_Dock.HideWindow(m_viewExplorer); else m_Dock.DockWindow(m_viewExplorer, DOCK_LASTKNOWN);
   return 0;
}

LRESULT CMainFrame::OnViewPropertyBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( m_viewProperties.IsWindowVisible() ) m_Dock.HideWindow(m_viewProperties); else m_Dock.DockWindow(m_viewProperties, DOCK_LASTKNOWN);
   return 0;
}

LRESULT CMainFrame::OnViewOutput(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ActivateAutoHideView(m_viewOutput);
   return 0;
}

LRESULT CMainFrame::OnViewCommand(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ActivateAutoHideView(m_viewCommand);
   return 0;
}

LRESULT CMainFrame::OnViewProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ShowProperties(m_viewExplorer.m_viewFile.GetActiveView(), TRUE);
   return 0;
}

LRESULT CMainFrame::OnViewFullScreen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CLockWindowUpdate lock = m_hWnd;
   m_bFullScreen = !m_bFullScreen;
   // Toggle client-area between MDIClient and AutoHide control
   m_hWndClient = m_bFullScreen ? m_hWndMDIClient : m_AutoHide;
   // Remove window caption
   DWORD dwStyle = WS_CAPTION | WS_SYSMENU;
   ModifyStyle(m_bFullScreen ? dwStyle : 0, m_bFullScreen ? 0 : dwStyle, SWP_NOACTIVATE);
   // Hide the dock/autohide controls
   m_Dock.ShowWindow(m_bFullScreen ? SW_HIDE : SW_SHOWNOACTIVATE);
   m_AutoHide.ShowWindow(m_bFullScreen ? SW_HIDE : SW_SHOWNOACTIVATE);
   // Go fullscreen
   // NOTE: We only go as fullscreen as to the taskbar...
   ShowWindow(m_bFullScreen ? SW_MAXIMIZE : SW_RESTORE);
   // Hide statusbar
   m_StatusBar.ShowWindow(m_bFullScreen ? SW_HIDE : SW_SHOWNOACTIVATE);
   // Move FullScreen ToolBar to top
   UINT iPos;
   for( iPos = 0; iPos < m_Rebar.GetBandCount(); iPos++ ) {
      REBARBANDINFO rbbi = { 0 };
      rbbi.cbSize = sizeof(rbbi);
      rbbi.fMask = RBBIM_CHILD;
      m_Rebar.GetBandInfo(iPos, &rbbi);
      if( rbbi.hwndChild != m_FullScreenToolBar ) continue;
      m_Rebar.MoveBand(iPos, 1);
      m_Rebar.ShowBand(iPos, m_bFullScreen);
   }
   // Hide the remaining bands if fullscreen, otherwise restore state
   for( iPos = 2; iPos < m_Rebar.GetBandCount(); iPos++ ) {
      REBARBANDINFO rbbi = { 0 };
      rbbi.cbSize = sizeof(rbbi);
      rbbi.fMask = RBBIM_CHILD | RBBIM_LPARAM;
      m_Rebar.GetBandInfo(iPos, &rbbi);
      LPARAM lBeforeState = (LPARAM) ::IsWindowVisible(rbbi.hwndChild);
      m_Rebar.ShowBand(iPos, m_bFullScreen ? FALSE : (BOOL) rbbi.lParam);
      rbbi.lParam = lBeforeState;
      rbbi.fMask = RBBIM_LPARAM;
      m_Rebar.SetBandInfo(iPos, &rbbi);
   }
   // Ok, rearrange windows
   UISetCheck(ID_VIEW_FULLSCREEN, m_bFullScreen);
   SendMessage(WM_APP_VIEWCHANGE);
   UpdateLayout();
   return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CLockWindowUpdate lock = m_hWnd;
   BOOL bVisible = !m_DefaultToolBar.IsWindowVisible();
   for( UINT i = 1; i < m_Rebar.GetBandCount(); i++ ) m_Rebar.ShowBand(i, bVisible);
   UISetCheck(ID_VIEW_TOOLBAR, bVisible);
   UpdateLayout();
   return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   BOOL bVisible = !m_StatusBar.IsWindowVisible();
   m_StatusBar.ShowWindow(bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
   UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
   UpdateLayout();
   return 0;
}

LRESULT CMainFrame::OnMacroRecord(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   m_sMacro.Empty();
   m_bRecordingMacro = TRUE;
   RecordMacro(_T("' Recorded Macro"));
   RecordMacro(_T(""));
   RecordMacro(_T("' Macro sequence"));
   RecordMacro(_T("Sub Macro"));
   RecordMacro(_T("On Error Resume Next"));
   bHandled = FALSE;  // Other clients must see this!
   return 0;
}

LRESULT CMainFrame::OnMacroCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( !m_bRecordingMacro ) return 0;
   RecordMacro(_T("End Sub"));
   m_bRecordingMacro = FALSE;
   bHandled = FALSE;  // Other clients must see this!
   return 0;
}

LRESULT CMainFrame::OnMacroPlay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( m_bRecordingMacro ) {
      if( IDNO == _ShowMessageBox(m_hWnd, IDS_STOPRECORDING, IDS_CAPTION_QUESTION, MB_ICONQUESTION | MB_YESNO) ) {
         return 0;
      }
      SendMessage(WM_COMMAND, MAKEWPARAM(ID_MACRO_CANCEL, 0));
   }
   // TODO: Add some kind of message/notification to clients to allow
   //       editors to wrap this sequence in an Undo/Redo transaction.
   CComObjectGlobal<CMacro> macro;
   macro.Init(this, &m_Dispatch);
   macro.RunMacroFromScript(CComBSTR(m_sMacro), L"Macro");
   return 0;
}

LRESULT CMainFrame::OnMacroSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   SendMessage(WM_COMMAND, MAKEWPARAM(ID_MACRO_CANCEL, 0));
   CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_SAVEMACRO));
   CString sFilter(MAKEINTRESOURCE(IDS_FILTER_MACRO));
   CString sPath = CModulePath();
   for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == _T('|') ) sFilter.SetAt(i, _T('\0'));
   DWORD dwStyle = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ENABLESIZING;
   CFileDialog dlg(FALSE, _T("vbs"), NULL, dwStyle, sFilter, m_hWnd);
   dlg.m_ofn.lpstrTitle = sCaption;
   dlg.m_ofn.Flags &= ~OFN_ENABLEHOOK;
   dlg.m_ofn.lpstrInitialDir = sPath;
   if( dlg.DoModal(m_hWnd) != IDOK ) return 0;
   USES_CONVERSION;
   CFile f;
   if( !f.Create(dlg.m_ofn.lpstrFile) ) return 0;
   f.Write(T2CA(m_sMacro), m_sMacro.GetLength());
   f.Close();
   return 0;
}

LRESULT CMainFrame::OnWindowClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   HWND hWnd = MDIGetActive();
   if( hWnd != NULL ) ::SendMessage(hWnd, WM_CLOSE, 0, 0L);
   return 0;
}

LRESULT CMainFrame::OnWindowCloseAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ATLASSERT(::IsWindow(m_hWndMDIClient));
   HWND hWnd  = ::GetWindow(m_hWndMDIClient, GW_CHILD);
   while( hWnd != NULL ) {
      ::PostMessage(hWnd, WM_CLOSE, 0, 0L);
      hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
   }
   return 0;
}

LRESULT CMainFrame::OnWindowNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   MDINext(MDIGetActive(), TRUE);
   return 0;
}

LRESULT CMainFrame::OnWindowPrevious(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   MDINext(MDIGetActive(), FALSE);
   return 0;
}

LRESULT CMainFrame::OnHelpContents(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CString sFilename;
   sFilename.Format(_T("%sBVRDE.chm"), CModulePath());
   ::HtmlHelp(m_hWnd, sFilename, HH_DISPLAY_TOC, 0);
   return 0;
}

LRESULT CMainFrame::OnHelpIndex(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CString sFilename;
   sFilename.Format(_T("%sBVRDE.chm"), CModulePath());
   ::HtmlHelp(m_hWnd, sFilename, HH_DISPLAY_INDEX, 0);
   return 0;
}

LRESULT CMainFrame::OnHelpSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CString sFilename;
   sFilename.Format(_T("%sBVRDE.chm"), CModulePath());
   HH_FTS_QUERY q = { 0 };
   q.cbStruct = sizeof(q);
   ::HtmlHelp(m_hWnd, sFilename, HH_DISPLAY_SEARCH, (DWORD) &q);
   return 0;
}

LRESULT CMainFrame::OnUserInit(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{  
   SendMessage(WM_APP_VIEWCHANGE);

   // Add and arrange toolbars
   _ArrangeToolBars();

   // Interpret commandline
   CCommandLine cmdline;
   cmdline.Parse(::GetCommandLine());
   if( cmdline.GetSize() > 1 ) {
      // There are commandline arguments! Let's parse them.
      // NOTE: We can use PostMessage() because GetCommandLine() is 
      //       static in the system!
      PostMessage(WM_APP_COMMANDLINE, 0, (LPARAM) ::GetCommandLine());
   }
   else {
      TCHAR szBuffer[64] = { 0 };
      GetProperty(_T("gui.main.start"), szBuffer, 63);
      if( _tcscmp(szBuffer, _T("blank")) == 0 ) /* nothing */;
      else if( _tcscmp(szBuffer, _T("openfile")) == 0 ) PostMessage(WM_COMMAND, MAKEWPARAM(ID_NEW_SOLUTION, 0));
      else if( _tcscmp(szBuffer, _T("startpage")) == 0 ) PostMessage(WM_COMMAND, MAKEWPARAM(ID_HELP_STARTPAGE, 0));
      else if( _tcscmp(szBuffer, _T("lastsolution")) == 0 ) PostMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_MRU_FIRST, 0));
      else PostMessage(WM_COMMAND, MAKEWPARAM(ID_HELP_STARTPAGE, 0));
   }

   // We're properly initialized now
   m_bInitialized = TRUE;

   // Update UI with refresh state information
   UIReset();
   OnIdle();

   // Display main window now
   if( !LoadWindowPos() ) ShowWindow(SW_SHOW);  
   m_MDIContainer.UpdateLayout();

   ::SetForegroundWindow(m_hWnd);

   return 0;
}

LRESULT CMainFrame::OnUserIdle(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{  
   static long s_once = 0;
   if( s_once++ == 0 ) SetTimer(DELAY_TIMERID, 500L);
   return 0;
}

LRESULT CMainFrame::OnUserLoadSolution(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
   LPCTSTR pstrFilename = (LPCTSTR) lParam;
   ATLASSERT(!::IsBadStringPtr(pstrFilename,-1));
   if( pstrFilename == NULL ) return 0;
   m_mru.AddToList(pstrFilename);
   m_mru.WriteToRegistry(REG_BVRDE _T("\\Mru"));
   SendMessage(WM_APP_CLOSESTARTPAGE);
   SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_CLOSESOLUTION, 0));
   return (LRESULT) g_pSolution->LoadSolution(pstrFilename);
}

LRESULT CMainFrame::OnUserTreeMessage(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
   // The treeview forwards its messages to this method and we're supposed
   // to forward everything to clients (listeners). This will allow each
   // plugin to handle its tree actions (rename, context-menu, etc).
   LPNMHDR pNMHDR = (LPNMHDR) lParam;
   bHandled = FALSE;
   for( int i = m_aTreeListeners.GetSize() - 1; i >= 0; --i ) {
      LRESULT lRes = m_aTreeListeners[i]->OnTreeMessage(pNMHDR, bHandled);
      if( bHandled ) return lRes;
   }
   return 0;
}

LRESULT CMainFrame::OnUserViewMessage(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   // The views forwards certain messages to this method and we're supposed
   // to forward everything to clients (listeners). This will allow plugins
   // to react on window focus changes.
   LPMSG pMsg = (LPMSG) lParam;
   bHandled = FALSE;
   for( int i = m_aViewListeners.GetSize() - 1; i >= 0; --i ) {
      LRESULT lRes = m_aViewListeners[i]->OnViewMessage((IView*)wParam, pMsg->message, pMsg->wParam, pMsg->lParam, bHandled);
      if( bHandled ) return lRes;
   }
   return 0;
}

LRESULT CMainFrame::OnUserBuildSolutionUI(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
   _BuildSolutionUI();
   return 0;
}

LRESULT CMainFrame::OnUserUpdateLayout(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   UpdateLayout(FALSE);
   return 0;
}

LRESULT CMainFrame::OnUserViewChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   HWND hWnd = MDIGetActive();
   IView* pView = NULL;
   IProject* pProject = NULL;
   
   // Update the Explorer with latest view selection
   if( ::IsWindow(hWnd) ) {
      CWinProp prop = hWnd;
      prop.GetProperty(_T("Project"), pProject);
      prop.GetProperty(_T("View"), pView);
      m_viewExplorer.m_viewFile.SetActiveView(pView);
   }

   // Deactivate old view if needed
   // NOTE: Both members below are 'static' and could potentially
   //       have been destroyed by now, so this is slightly unsafe code.
   //       Code inside MDIContainer.h has been added to make it mostly 
   //       safe.
   static IProject* s_pOldProject = NULL;
   static IView* s_pOldView = NULL;
   if( s_pOldView != pView ) {
      if( s_pOldView ) s_pOldView->DeactivateUI();
      if( s_pOldProject ) s_pOldProject->DeactivateUI();
      s_pOldView = pView;
      s_pOldProject = pProject;
   }

   // Need to recreate entire menu
   // NOTE: Windows MDI doesn't like its menu to be destroyed (doesn't
   //       seem to be equally well protected by reference-counting as other
   //       GDI objects) so we'll keep it alive in a static.
   static CMenu menu;
   if( menu.IsMenu() ) menu.DestroyMenu();
   menu.LoadMenu(IDR_MAINFRAME);
   ATLASSERT(menu.IsMenu());
   UIReset();
   UISetMenu(menu);
   // Let the project and view customize the menu
   if( pProject ) pProject->ActivateUI();
   else if( g_pSolution->GetActiveProject() ) g_pSolution->GetActiveProject()->ActivateUI();
   if( pView ) pView->ActivateUI();
   // Realize new menu
   m_CmdBar.AttachMenu(menu);

   // Update properties window as well
   if( pView ) ShowProperties(pView, FALSE);

   return 0;
}

LRESULT CMainFrame::OnUserProjectChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
   IProject* pProject = (IProject*) lParam;
   m_viewExplorer.m_viewFile.SetActiveProject(pProject);
   SendMessage(WM_APP_VIEWCHANGE);
   PlayAnimation(FALSE, 0);
   return 0;
}

LRESULT CMainFrame::OnUserCommandLine(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
   CCommandLine cmdline;
   cmdline.Parse( (LPCTSTR) lParam );
   for( int i = 1; i < cmdline.GetSize(); i++ ) {
      CString sFilename = cmdline.GetItem(i);
      if( ::PathMatchSpec(sFilename, _T("*.sln")) ) {
         SendMessage(WM_APP_LOADSOLUTION, 0, (LPARAM) (LPCTSTR) sFilename);
      }
      else {
         IView* pView = CreateView(sFilename);
         if( pView ) ATLTRY( pView->OpenView(1) );
      }
   }
   // Bring focus back to main app
   ::SetForegroundWindow(m_hWnd);
   return 0;
}

LRESULT CMainFrame::OnUserCloseStartPage(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
   HWND hWnd  = ::GetWindow(m_hWndMDIClient, GW_CHILD);
   while( hWnd != NULL ) {
      IView* pView = NULL;
      CWinProp prop = hWnd;
      prop.GetProperty(_T("View"), pView);
      if( pView != NULL ) {
         TCHAR szType[40] = { 0 };
         pView->GetType(szType, (sizeof(szType) / sizeof(TCHAR)) - 1);
         if( _tcscmp(szType, _T("Start Page")) == 0 ) {
            pView->CloseView();
            return 0;
         }
      }     
      hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
   }
   return 0;
}

LRESULT CMainFrame::OnToolBarDropDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   LPNMTOOLBAR lptb = (LPNMTOOLBAR) pnmh;
   if( lptb->iItem != ID_POPUP_NEW && lptb->iItem != ID_POPUP_ADD ) {
      bHandled = FALSE;
      return 0;
   }
   CToolBarCtrl tb = lptb->hdr.hwndFrom;
   RECT rcItem;
   tb.GetItemRect(tb.CommandToIndex(lptb->iItem), &rcItem);
   POINT pt = { rcItem.left, rcItem.bottom };
   tb.ClientToScreen(&pt);
   // Display the "File" menu
   CMenuHandle menu = m_CmdBar.m_hMenu;
   CMenuHandle submenu = menu.GetSubMenu(0);
   submenu = submenu.GetSubMenu(lptb->iItem == ID_POPUP_NEW ? 0 : 1);
   CDummyElement Element(_T("Popup"), lptb->iItem == ID_POPUP_NEW ? _T("FileNew") : _T("FileAdd"));
   g_pDevEnv->ShowPopupMenu(&Element, submenu, pt);
   return TBDDRET_DEFAULT;
}

LRESULT CMainFrame::OnRebarLayoutChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   _SaveToolBarState();
   return 0;
}

LRESULT CMainFrame::OnRebarRClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   bHandled = FALSE;
   DWORD dwPos = ::GetMessagePos();
   POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
   // Need an extensive test to check that it's really a click on the rebar.
   // Why? Because most popup menus go through the CommandBar control (which is
   // also a parent of the Rebar). Windows seems to generate an extra NM_RCLICK just
   // for the TrackPopupMenuEx() call, and we need to filter that out.
   POINT ptClient = pt;
   ::ScreenToClient(m_Rebar, &ptClient);
   if( ::ChildWindowFromPoint(m_Rebar, ptClient) == NULL ) return 0;
   // Ok, build popup menu
   CMenu menu;
   menu.CreatePopupMenu();
   for( int i = 0; i < m_aToolBars.GetSize(); i++ ) {
      menu.AppendMenu(MF_STRING | (::IsWindowVisible(m_aToolBars[i].hWnd) ? MF_CHECKED : 0), i + 0x1000, m_aToolBars[i].szName);
   }
   CDummyElement Element(_T("Popup"), _T("ToolBarMenu"));
   UINT nCmd = g_pDevEnv->ShowPopupMenu(&Element, menu, pt, FALSE);
   if( nCmd == 0 ) return 0;
   // Toolbar name was clicked; now show/hide it
   CLockWindowUpdate lock = m_hWnd;
   HWND hWnd = m_aToolBars[nCmd - 0x1000].hWnd;
   ShowToolBar(hWnd, !::IsWindowVisible(hWnd));
   _SaveToolBarState();
   return 0;
}
