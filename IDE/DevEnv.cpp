
#include "StdAfx.h"
#include "resource.h"

#include "MainFrm.h"
#include "ChildFrm.h"

#include "MsgBoxDlg.h"
#include "OptionsDlg.h"


// IDevEnv

DWORD CMainFrame::GetVersion() const
{
   return MAKELPARAM(1, 0);
}

LCID CMainFrame::SetThreadLanguage()
{
   // On Windows Vista we have to use the SetThreadUILanguage() for this to work.
   // The MSDN docs suggests that this is also needed for ealier version, but
   // it appears to work.
   OSVERSIONINFO ver = { sizeof(ver) };
   ::GetVersionEx(&ver);
   if( ver.dwMajorVersion >= 6 ) {
      LANGID lang = LANGIDFROMLCID(m_Locale);
      typedef LANGID (WINAPI *PFNSETTHREADUILANGUAGE)(LCID);
      PFNSETTHREADUILANGUAGE pfnSetThreadUILanguage = 
         (PFNSETTHREADUILANGUAGE) ::GetProcAddress( ::GetModuleHandle(_T("kernel32.dll")), "SetThreadUILanguage");
      if( pfnSetThreadUILanguage ) pfnSetThreadUILanguage(lang);
   }
   else {
      ::SetThreadLocale(m_Locale);
   }
   return m_Locale;
}

DWORD CMainFrame::GetGuiThreadId()
{
   return m_dwGuiThreadId;
}

IDispatch* CMainFrame::GetDispatch()
{
   CMainFrame* pThis = const_cast<CMainFrame*>(this);
   return &pThis->m_Dispatch;
}

ISolution* CMainFrame::GetSolution() const
{
   return g_pSolution;
}

IView* CMainFrame::GetActiveView() const
{
   // const HWND hWnd = MDIGetActive();
   HWND hWnd = (HWND) ::SendMessage(m_hWndMDIClient, WM_MDIGETACTIVE, 0, 0L);
   IView* pView = NULL;
   if( ::IsWindow(hWnd) ) {
      CWinProp prop = hWnd;
      prop.GetProperty(_T("View"), pView);
   }
   return pView;
}

IView* CMainFrame::CreateView(LPCTSTR pstrFilename, IProject* pProject /*= NULL*/, IElement* pElement /*= NULL*/)
{
   ATLASSERT(!::IsBadStringPtr(pstrFilename,-1));
   ATLASSERT(g_aPlugins.GetSize()>0);
   // Find the plugin that thinks its the best match for this
   // filename.
   CPlugin* pBestPlugin = NULL;
   UINT iBestScore = 0;
   for( int i = 0; i < g_aPlugins.GetSize(); i++ ) {
      CPlugin& plugin = g_aPlugins[i];
      if( !plugin.IsLoaded() ) continue;
      if( (plugin.GetType() & PLUGIN_FILETYPE) == 0 ) continue;
      UINT iScore = plugin.QueryAcceptFile(pstrFilename);
      if( iScore <= iBestScore ) continue;
      iBestScore = iScore;
      pBestPlugin = &plugin;
   }
   if( pBestPlugin == NULL ) {
      ::MessageBeep((UINT)-1);
      return NULL;
   }
   // Ok, let the plugin create a new view...
   return pBestPlugin->CreateView(pstrFilename, pProject, pElement);
}

IDispatch* CMainFrame::CreateStdDispatch(LPCTSTR pstrType, IElement* pElement)
{
   ATLASSERT(pstrType);
   ATLASSERT(pElement);
   if( pstrType == NULL ) return NULL;
   if( pElement == NULL ) return NULL;
   if( _tcscmp(pstrType, _T("Project")) == 0 ) return new CProjectOM( (IProject*) pElement );
   if( _tcscmp(pstrType, _T("View")) == 0 ) return new CFileOM( (IView*) pElement );
   ATLASSERT(false);
   return NULL;
}

BOOL CMainFrame::AddExplorerView(HWND hWnd, LPCTSTR pstrTitle, int iImage)
{
   ATLASSERT(::IsWindow(hWnd));
   ATLASSERT(!::IsBadStringPtr(pstrTitle,-1));
   CLockStaticDataInit lock;
   return m_viewExplorer.AddView(hWnd, pstrTitle, iImage);
}

BOOL CMainFrame::RemoveExplorerView(HWND hWnd)
{
   CLockStaticDataInit lock;
   return m_viewExplorer.RemoveView(hWnd);
}

BOOL CMainFrame::AddToolBar(HWND hWnd, LPCTSTR pstrID, LPCTSTR pstrTitle)
{
   ATLASSERT(::IsWindow(hWnd));
   ATLASSERT(!::IsBadStringPtr(pstrID,-1));
   ATLASSERT(!::IsBadStringPtr(pstrTitle,-1));
   if( !::IsWindow(hWnd) ) return FALSE;
   CLockStaticDataInit lock;
   // Get stored properties
   CString sKey;
   sKey.Format(_T("window.toolbar.%s."), pstrID);
   TCHAR szBuffer[32] = { 0 };
   GetProperty(sKey + _T("show"), szBuffer, (sizeof(szBuffer)/sizeof(TCHAR))-1);
   BOOL bShowDefault = (_tcscmp(szBuffer, _T("true")) == 0);
   GetProperty(sKey + _T("position"), szBuffer, (sizeof(szBuffer)/sizeof(TCHAR))-1);
   int iPosition = _ttoi(szBuffer);
   GetProperty(sKey + _T("newRow"), szBuffer, (sizeof(szBuffer)/sizeof(TCHAR))-1);
   BOOL bNewRow = (_tcscmp(szBuffer, _T("true")) == 0);
   // If no longer in initialization phase, we should show it now.
   // Otherwise we add the info, but will show it later.
   if( m_bInitialized ) {
      BOOL bNewRow = ((m_Rebar.GetBandCount() & 1) == 0);
      if( !AddSimpleReBarBand(hWnd, NULL, bNewRow, 0, TRUE) ) return FALSE;
      if( !m_CmdBar.AddToolbar(hWnd) ) return FALSE;
      // We might need to hide it
      m_Rebar.ShowBand(m_Rebar.GetBandCount() - 1, bShowDefault);
      //if( bShowDefault ) m_Rebar.MaximizeBand(m_Rebar.GetBandCount() - 1);
      // Prepare band in Command Bar
      m_CmdBar.Prepare();
   }
   // We can UI update this guy
   UIAddToolBar(hWnd);
   // Add it to internal list
   TOOLBAR tb = { 0 };
   _tcsncpy(tb.szID, pstrID, (sizeof(tb.szID) / sizeof(TCHAR)) - 1);
   _tcsncpy(tb.szName, pstrTitle, (sizeof(tb.szName) / sizeof(TCHAR)) - 1);
   tb.hWnd = hWnd;
   tb.bShowDefault = bShowDefault;
   tb.bNewRow = bNewRow;
   tb.iPosition = iPosition;
   m_aToolBars.Add(tb);
   return TRUE;
}

BOOL CMainFrame::RemoveToolBar(HWND hWnd)
{
   ATLASSERT(::IsWindow(hWnd));
   if( !::IsWindow(hWnd) ) return FALSE;
   CLockStaticDataInit lock;
   // Remove it from the UI map
   int i;
   for( i = 0; i < m_UIElements.GetSize(); i++ ) {
      const _AtlUpdateUIElement& e = m_UIElements[i];
      if( e.m_hWnd == hWnd ) {
         m_UIElements.RemoveAt(i);
         break;
      }
   }
   // Find it in collection and remove
   for( i = 0; i < m_aToolBars.GetSize(); i++ ) {
      if( m_aToolBars[i].hWnd == hWnd ) {
         m_aToolBars.RemoveAt(i);
         break;
      }
   }
   // Find the Rebar band and delete it
   for( UINT j = 0; j < m_Rebar.GetBandCount(); j++ ) {
      REBARBANDINFO rbi = { 0 };
      rbi.cbSize = sizeof(REBARBANDINFO);
      rbi.fMask = RBBIM_CHILD;
      m_Rebar.GetBandInfo(j, &rbi);
      if( rbi.hwndChild == hWnd ) {
         m_Rebar.DeleteBand(j);
         return TRUE;
      }
   }
   return FALSE;
}

BOOL CMainFrame::ShowToolBar(HWND hWnd, BOOL bShow /*= TRUE*/, BOOL bUseDefault /*= TRUE*/)
{
   // Find it in collection and override
   for( int i = 0; i < m_aToolBars.GetSize(); i++ ) {
      if( m_aToolBars[i].hWnd == hWnd ) {
         if( bUseDefault ) bShow = m_aToolBars[i].bShowDefault;
         break;
      }
   }
   // Find the Rebar band and change visibility
   for( UINT j = 0; j < m_Rebar.GetBandCount(); j++ ) {
      REBARBANDINFO rbi = { 0 };
      rbi.cbSize = sizeof(REBARBANDINFO);
      rbi.fMask = RBBIM_CHILD;
      m_Rebar.GetBandInfo(j, &rbi);
      if( rbi.hwndChild != hWnd ) continue;
      if( ::IsWindowVisible(hWnd) == bShow ) return TRUE;
      UIClear();
      m_Rebar.ShowBand(j, bShow);
      // HACK: The Rebar is a really crappy control, and Microsoft
      //       seems to have lost control on how it resizes the bands,
      //       so we need to trick it into compacting the bands.
      // FIX: Calling MaximizeBand before ShowBand crashes Windows.
      if( bShow ) {
         m_Rebar.MaximizeBand(j);
         m_Rebar.SendMessage(RB_MAXIMIZEBAND, j, 1L);
      }
      return TRUE;
   }
   return FALSE;
}

BOOL CMainFrame::AddDockView(HWND hWnd, IDE_DOCK_TYPE Direction, RECT rcWin)
{
   ATLASSERT(::IsWindow(hWnd));
   if( !::IsWindow(hWnd) ) return FALSE;
   CLockStaticDataInit lock;
   CWindow wnd = hWnd;
   // Not docked before? Then prepare it for docking...
   if( !m_Dock.IsDockPane(wnd) ) {
      wnd.SetParent(m_Dock);
      m_Dock.AddWindow(wnd);
   }
   // Dock window...
   BOOL bRes = FALSE;
   switch( Direction ) {
   case IDE_DOCK_HIDE:
      bRes = m_Dock.HideWindow(hWnd);
      break;
   case IDE_DOCK_FLOAT:
      bRes = m_Dock.FloatWindow(hWnd, rcWin);
      break;
   case IDE_DOCK_LEFT:
   case IDE_DOCK_RIGHT:
      bRes = m_Dock.DockWindow(hWnd, (short) Direction, rcWin.bottom - rcWin.top);
      break;
   case IDE_DOCK_TOP:
   case IDE_DOCK_BOTTOM:
      bRes = m_Dock.DockWindow(hWnd, (short) Direction, rcWin.right - rcWin.left);
      break;
   default:
      bRes = m_Dock.DockWindow(hWnd, (short) Direction);
      break;
   }
   return bRes;
}

BOOL CMainFrame::RemoveDockView(HWND hWnd)
{
   ATLASSERT(::IsWindow(hWnd));
   if( !::IsWindow(hWnd) ) return FALSE;
   CLockStaticDataInit lock;
   return m_Dock.RemoveWindow(hWnd);
}

BOOL CMainFrame::GetDockState(HWND hWnd, int& iState, RECT& rcWin)
{
   ATLASSERT(::IsWindow(hWnd));
   if( !::IsWindow(hWnd) ) return FALSE;
   CLockStaticDataInit lock;
   if( !m_Dock.IsDockPane(hWnd) ) return FALSE;
   m_Dock.GetWindowState(hWnd, iState, rcWin);
   return TRUE;
}

BOOL CMainFrame::AddAutoHideView(HWND hWnd, IDE_DOCK_TYPE Direction, int iImage)
{
   ATLASSERT(::IsWindow(hWnd));
   if( !::IsWindow(hWnd) ) return FALSE;
   CLockStaticDataInit lock;
   m_AutoHide.RemoveView(hWnd);
   BOOL bRes = FALSE;
   switch( Direction ) {
   case IDE_DOCK_LEFT:
      bRes = m_AutoHide.AddView(hWnd, AUTOHIDE_LEFT, iImage);
      break;
   case IDE_DOCK_BOTTOM:
      bRes = m_AutoHide.AddView(hWnd, AUTOHIDE_BOTTOM, iImage);
      break;
   default:
      ATLASSERT(FALSE);
   }
   return bRes;
}

BOOL CMainFrame::RemoveAutoHideView(HWND hWnd)
{
   ATLASSERT(m_AutoHide.IsWindow());
   if( !::IsWindow(hWnd) ) return FALSE;
   CLockStaticDataInit lock;
   return m_AutoHide.RemoveView(hWnd);
}

BOOL CMainFrame::ActivateAutoHideView(HWND hWnd)
{
   ATLASSERT(m_AutoHide.IsWindow());
   ATLASSERT(::IsWindow(hWnd));
   if( !::IsWindow(hWnd) ) return FALSE;
   // BUG: Cannot thread-protect here because of dead-lock issues.
   BOOL bRes = m_AutoHide.ActivateView(hWnd);
   ::RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
   return bRes;
}

BOOL CMainFrame::ActivateExplorerView(HWND hWnd)
{
   ATLASSERT(m_viewExplorer.IsWindow());
   ATLASSERT(::IsWindow(hWnd));
   if( !::IsWindow(hWnd) ) return FALSE;
   // BUG: Cannot thread-protect here because of dead-lock issues.
   return m_viewExplorer.SelectView(hWnd);
}

BOOL CMainFrame::AddCommandBarImage(UINT nCmd, HICON hIcon)
{
   ATLASSERT(m_CmdBar.IsWindow());
   ATLASSERT(nCmd>=0 && nCmd<ID_CMD_FIRST);
   if( nCmd == 0 ) return TRUE;
   if( !m_CmdBar.ReplaceIcon(hIcon, nCmd) ) m_CmdBar.AddIcon(hIcon, nCmd);
   return TRUE;
}

BOOL CMainFrame::ShowWizard(IDE_WIZARD_TYPE Type, IProject* pProject)
{
   switch( Type ) {
   case IDE_WIZARD_SOLUTION:
      return _CreateSolutionWizard() != NULL;
   case IDE_WIZARD_PROJECT:
      return _CreateProjectWizard() != NULL;
   case IDE_WIZARD_FILE:
      {
         ATLASSERT(pProject);
         if( pProject == NULL ) return FALSE;
         BOOL bRes = _CreateFileWizard(pProject) != NULL;
         PostMessage(WM_APP_BUILDSOLUTIONUI);
         return bRes;
      }
   default:
      return FALSE;
   }
}

BOOL CMainFrame::ShowConfiguration(IElement* pElement)
{
   ATLASSERT(pElement);
   if( pElement == NULL ) return FALSE;
   CLockStaticDataInit lock;
   COptionsDlg dlg(this, NULL, pElement);
   UINT nRes = dlg.DoModal();
   return nRes == IDOK;
}

BOOL CMainFrame::ShowProperties(IElement* pElement, BOOL bForceVisible)
{
   ATLASSERT(pElement);
   ATLASSERT(m_viewProperties.IsWindow());
   if( pElement == NULL ) return FALSE;
   if( bForceVisible && !m_viewProperties.IsWindowVisible() ) SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_PROPERTIES, 0));
   m_viewProperties.SetActiveElement(pElement);
   return TRUE;
}

IViewFrame* CMainFrame::CreateClient(LPCTSTR pstrTitle, IProject* pProject, IView* pView)
{
   ATLASSERT(::IsWindow(m_hWnd));
   ATLASSERT(pView);
   CLockStaticDataInit lock;
   // Create a MDI Client window for the caller.
   CChildFrame* pChild = NULL;
   ATLTRY( pChild = new CChildFrame(this, this, pProject, pView) );
   ATLASSERT(pChild);
   if( pChild == NULL ) return NULL;
   HWND hWnd = pChild->CreateEx(m_hWndMDIClient, NULL, pstrTitle);
   ATLASSERT(::IsWindow(hWnd));
   if( !::IsWindow(hWnd) ) return NULL;
   return pChild;
}

BOOL CMainFrame::DestroyClient(HWND hWnd)
{
   ATLASSERT(::IsWindow(hWnd));
   if( !::IsWindow(hWnd) ) return FALSE;
   ::SendMessage(hWnd, WM_CLOSE, 0, 0L);
   return TRUE;
}

void CMainFrame::EnableModeless(BOOL bEnable)
{
   CLockStaticDataInit lock;
   EnableWindow(bEnable);
   for( int i = 0; i < GetSolution()->GetItemCount(); i++ ) {
      IProject* pProject = GetSolution()->GetItem(i);
      for( int j = 0; j < pProject->GetItemCount(); j++ ) {
         pProject->GetItem(j)->EnableModeless(bEnable);
      }
   }
   m_Dock.EnableModeless(bEnable);
}

HWND CMainFrame::GetHwnd(IDE_HWND_TYPE WinType) const
{
   ATLASSERT(::IsWindow(m_hWnd));
   switch( WinType ) {
   case IDE_HWND_MAIN:
      return m_hWnd;
   case IDE_HWND_COMMANDBAR:
      return m_CmdBar;
   case IDE_HWND_EXPLORER_TREE:
      return m_viewExplorer.m_viewFile.m_ctrlTree;
   case IDE_HWND_TOOLBAR:
      return m_DefaultToolBar;
   case IDE_HWND_REBAR:
      return m_Rebar;
   case IDE_HWND_STATUSBAR:
      return m_StatusBar;
   case IDE_HWND_DOCK:
      return m_Dock;
   case IDE_HWND_AUTOHIDE:
      return m_AutoHide;
   case IDE_HWND_MDICLIENT:
      return m_hWndMDIClient;
   case IDE_HWND_TABS:
      return m_MDIContainer.m_ctrlTab;
   case IDE_HWND_OUTPUTVIEW:
      return m_viewOutput;
   case IDE_HWND_COMMANDVIEW:
      return m_viewCommand;
   default:
      ATLASSERT(false);
      return NULL;
   }
}

HMENU CMainFrame::GetMenuHandle(IDE_HWND_TYPE WinType) const
{
   ATLASSERT(::IsWindow(m_hWnd));
   switch( WinType ) {
   case IDE_HWND_MAIN:
   case IDE_HWND_COMMANDBAR:
      return m_CmdBar.GetMenu();
   default:
      ATLASSERT(false);
      return NULL;
   }
}

BOOL CMainFrame::ReserveUIRange(UINT iStart, UINT iEnd)
{
   return CUpdateDynamicUI<CMainFrame>::ReserveUIRange(iStart, iEnd);
}

BOOL CMainFrame::GetPrinterInfo(HGLOBAL& hDevMode, HGLOBAL& hDevNames, RECT& rcMargins) const
{
   hDevMode = m_hDevMode;
   hDevNames = m_hDevNames;
   rcMargins = m_rcPageMargins;
   return TRUE;
}

HIMAGELIST CMainFrame::GetImageList(IDE_HWND_TYPE WinType) const
{
   ATLASSERT(::IsWindow(m_hWnd));
   switch( WinType ) {
   case IDE_HWND_COMMANDBAR:
      return m_CmdBar.m_hImageList;
   case IDE_HWND_EXPLORER_TREE:
      return m_viewExplorer.m_viewFile.m_Images;
   case IDE_HWND_AUTOHIDE:
      return m_Images;
   default:
      ATLASSERT(false);
      return NULL;
   }
}

UINT CMainFrame::ShowPopupMenu(IElement* pElement, HMENU hMenu, POINT pt, BOOL bSendCommand /*= TRUE*/, IIdleListener* pListener /*= NULL*/)
{
   ATLASSERT(::IsWindow(m_hWnd));
   ATLASSERT(::IsMenu(hMenu));
   if( !::IsMenu(hMenu) ) return 0;
   // Allow plugins to customize menu
   for( int i = 0; i < g_aPlugins.GetSize(); i++ ) g_aPlugins[i].SetPopupMenu(pElement, hMenu);
   // Make sure menu-items are properly UI updated before
   // displaying any menu. The caller can optionally provide an
   // IdleListener to enable the specific popup items.
   OnIdle();
   if( pListener != NULL ) pListener->OnIdle(this);
   // Show popup-menu
   DWORD dwFlags = TPM_LEFTBUTTON | TPM_VERTICAL | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD;
   UINT nCmd = (UINT) m_CmdBar.TrackPopupMenu(hMenu, dwFlags, pt.x, pt.y, NULL);
   // We need the command returned now because TrackPopupMenu() would otherwise post the
   // result-message - rather than sending it. We need the value, because we might
   // loose the current right-click focus in the explorer tree!
   if( nCmd != 0 && bSendCommand ) SendMessage(WM_COMMAND, MAKEWPARAM(nCmd, 0));
   return nCmd;
}

UINT CMainFrame::ShowMessageBox(HWND hWnd, LPCTSTR pstrMessage, LPCTSTR pstrCaption, DWORD dwFlags)
{
   ATLASSERT(!::IsBadStringPtr(pstrMessage,-1));
   ATLASSERT(!::IsBadStringPtr(pstrCaption,-1));
   if( hWnd == NULL ) hWnd = m_hWnd;
   // HACK: Can't have reentrant modal dialogs so we simply fake
   //       a positive return answer. We will break plugins if not
   //       instructed about this feature.
   static CMsgBoxDlg dlg(this);
   if( dlg.IsWindow() && dlg.IsWindowVisible() ) return (dwFlags & 0x0F) == MB_YESNO ? IDYES : IDOK;
   UINT nRes = dlg.DoModal(hWnd, pstrMessage, pstrCaption, dwFlags);
   return nRes;
}

BOOL CMainFrame::ShowStatusText(UINT nID, LPCTSTR pstrText, BOOL bPermanent /*= FALSE*/)
{
   ATLASSERT(nID==ID_DEFAULT_PANE || nID==ID_SB_PANE1 || nID==ID_SB_PANE2 || nID==ID_SB_PANE3);
   ATLASSERT(!::IsBadStringPtr(pstrText,-1));
   ATLASSERT(m_StatusBar.IsWindow());
   if( ::GetCurrentThreadId() != GetGuiThreadId() ) return FALSE;
   if( nID != ID_DEFAULT_PANE ) {
      ATLASSERT(bPermanent);
      // Change one of the secondary panes in the statusbar. We'll need to
      // calculate the text width to make it look nice.
      CWindowDC dc = m_StatusBar;
      SIZE size = { 0 };
      dc.GetTextExtent(pstrText, _tcslen(pstrText), &size);
      m_StatusBar.SetPaneWidth(nID, size.cx);
      m_StatusBar.SetPaneText(nID, pstrText);
   }
   else {
      ATLASSERT(ID_DEFAULT_PANE==0);
      // Change the statusbar text now. Windows will update the screen right away.
      // If we choose the non-permanent option, we'll use UISetText() to eventually
      // set the statusbar text back once the system is idle.
      m_StatusBar.SetPaneText(ID_DEFAULT_PANE, pstrText);
      UISetText(0, bPermanent ? pstrText : CString(MAKEINTRESOURCE(ATL_IDS_IDLEMESSAGE)), TRUE);
   }
   return 0;
}

BOOL CMainFrame::PlayAnimation(BOOL bStart, UINT uType)
{
   // No more timer events
   KillTimer(ANIMATE_TIMERID);
   m_iAnimatePos = -1;
   // Reset statusbar pane
   RECT rcItem = { 0 };
   m_StatusBar.GetRect(1, &rcItem);
   m_StatusBar.InvalidateRect(&rcItem, TRUE);
   if( !bStart ) return TRUE;
   // Get ready to load new animation
   UINT nRes = 0;
   UINT nSize = 16;
   switch( uType ) {
   case ANIM_BUILD:    nRes = IDB_BUILDANIM; break;
   case ANIM_TRANSFER: nRes = IDB_TRANSFERANIM; break;
   case ANIM_SAVE:     nRes = IDB_SAVEANIM; nSize = 15; break;
   case ANIM_PRINT:    nRes = IDB_PRINTANIM; nSize = 15; break;
   }
   ATLASSERT(nRes>0);
   if( nRes == 0 ) return FALSE;
   // Load animation image
   m_AnimateImages.Destroy();
   m_AnimateImages.Create(nRes, nSize, 0, RGB(255,0,255));
   ATLASSERT(!m_AnimateImages.IsNull());
   if( m_AnimateImages.IsNull() ) return FALSE;
   // Start the show
   m_iAnimatePos = 0;
   SetTimer(ANIMATE_TIMERID, 200L); // 200ms timer
   PostMessage(WM_TIMER, (WPARAM) ANIMATE_TIMERID);
   return TRUE;
}

BOOL CMainFrame::RecordMacro(LPCTSTR pstrText)
{
   if( !m_bRecordingMacro ) return FALSE;
   m_sMacro += pstrText;
   m_sMacro += _T("\r\n");
   return TRUE;
}

BOOL CMainFrame::GetProperty(LPCTSTR pstrKey, LPTSTR pstrValue, UINT cchMax)
{
   ATLASSERT(!::IsBadStringPtr(pstrKey,-1));
   ATLASSERT(!::IsBadWritePtr(pstrValue,cchMax));
   ATLASSERT(cchMax>0);
   CLockStaticDataInit lock;
   if( cchMax == 0 ) return FALSE;
   *pstrValue = '\0';
   CString sValue;
   CString sKey = pstrKey;
   if( !m_aProperties.Lookup(sKey, sValue) ) return FALSE;   
   _tcsncpy(pstrValue, sValue, cchMax);
   return TRUE;
}

BOOL CMainFrame::SetProperty(LPCTSTR pstrKey, LPCTSTR pstrValue)
{
   ATLASSERT(!::IsBadStringPtr(pstrKey,-1));
   ATLASSERT(!::IsBadStringPtr(pstrValue,-1));
   CLockStaticDataInit lock;
   CString sKey = pstrKey;
   CString sValue = pstrValue;
   return m_aProperties.Set(sKey, sValue) == true;
}

BOOL CMainFrame::EnumProperties(int& iStart, LPCTSTR pstrPattern, LPTSTR pstrKey, LPTSTR pstrValue)
{
   ATLASSERT(!::IsBadStringPtr(pstrPattern,-1));
   ATLASSERT(!::IsBadWritePtr(pstrKey,256));
   ATLASSERT(!::IsBadWritePtr(pstrValue,256));
   CLockStaticDataInit lock;
   CString sKey;
   CString sValue;
   int nCount = m_aProperties.GetSize();
   for( int i = iStart; i < nCount; i++ ) {
      if( m_aProperties.GetItemAt(i, sKey, sValue) ) {
         if( ::PathMatchSpec(sKey, pstrPattern) ) {
            // BUG: Possible buffer-overrun here
            _tcscpy(pstrKey, sKey);
            _tcscpy(pstrValue, sValue);
            iStart = i + 1;
            return TRUE;
         }
      }
   }
   return FALSE;
}

BOOL CMainFrame::AddAppListener(IAppMessageListener* pListener)
{
   CLockStaticDataInit lock;
   if( m_aAppListeners.Find(pListener) >= 0 ) return FALSE;
   return m_aAppListeners.Add(pListener);
}

BOOL CMainFrame::RemoveAppListener(IAppMessageListener* pListener)
{
   CLockStaticDataInit lock;
   return m_aAppListeners.Remove(pListener);
}

BOOL CMainFrame::AddIdleListener(IIdleListener* pListener)
{
   CLockStaticDataInit lock;
   if( m_aIdleListeners.Find(pListener) >= 0 ) return FALSE;
   return m_aIdleListeners.Add(pListener);
}

BOOL CMainFrame::RemoveIdleListener(IIdleListener* pListener)
{
   CLockStaticDataInit lock;
   return m_aIdleListeners.Remove(pListener);
}

BOOL CMainFrame::AddTreeListener(ITreeMessageListener* pListener)
{
   CLockStaticDataInit lock;
   if( m_aTreeListeners.Find(pListener) >= 0 ) return FALSE;
   return m_aTreeListeners.Add(pListener);
}

BOOL CMainFrame::RemoveTreeListener(ITreeMessageListener* pListener)
{
   CLockStaticDataInit lock;
   return m_aTreeListeners.Remove(pListener);
}

BOOL CMainFrame::AddViewListener(IViewMessageListener* pListener)
{
   CLockStaticDataInit lock;
   if( m_aViewListeners.Find(pListener) >= 0 ) return FALSE;
   return m_aViewListeners.Add(pListener);
}

BOOL CMainFrame::RemoveViewListener(IViewMessageListener* pListener)
{
   CLockStaticDataInit lock;
   return m_aViewListeners.Remove(pListener);
}

BOOL CMainFrame::AddCommandListener(ICustomCommandListener* pListener)
{
   CLockStaticDataInit lock;
   if( m_aCommandListeners.Find(pListener) >= 0 ) return FALSE;
   return m_aCommandListeners.Add(pListener);
}

BOOL CMainFrame::RemoveCommandListener(ICustomCommandListener* pListener)
{
   CLockStaticDataInit lock;
   return m_aCommandListeners.Remove(pListener);
}

BOOL CMainFrame::AddWizardListener(IWizardListener* pListener)
{
   CLockStaticDataInit lock;
   if( m_aWizardListeners.Find(pListener) >= 0 ) return FALSE;
   return m_aWizardListeners.Add(pListener);
}

BOOL CMainFrame::RemoveWizardListener(IWizardListener* pListener)
{
   CLockStaticDataInit lock;
   return m_aWizardListeners.Remove(pListener);
}
