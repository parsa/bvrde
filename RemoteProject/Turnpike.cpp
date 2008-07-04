
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"


////////////////////////////////////////////////////////
//

static CComAutoCriticalSection g_csDelayedData;

struct CLockDelayedDataInit
{
   CLockDelayedDataInit() { g_csDelayedData.Lock(); };
   ~CLockDelayedDataInit() { g_csDelayedData.Unlock(); };
};


////////////////////////////////////////////////////////
// Delayed UI processing

LRESULT CRemoteProject::OnProcess(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   // All interactions during compiling/debugging that could stir up
   // the user-interface needs to go through this method to avoid GUI/thread dead-locks.
   // This method is invoked solely through a PostMessage() to the main message-queue 
   // and executes a series of commands in a queue/list. This makes sure that all
   // GUI changes are called from the main thread only.

   bHandled = FALSE;
   if( m_aLazyData.GetSize() == 0 ) return 0;

   // Try to obtain the semaphore
   if( !::TryEnterCriticalSection(&g_csDelayedData.m_sec) ) {
      // No need to block the thread; let's just re-post the request...
      // TODO: C'mon... we could at least wait a little for the semaphore to be
      //       released. This is too slow.
      m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
      return 0;
   }

   CSimpleArray<CString> aDbgCmd;       // Collect all new debug commands in a batch
   bool bUpdateEvent = false;           // Make sure to trigger debug view update only once
   CString sMessage;                    // For displaying a message-box asynchroniously
   CString sCaption;                    // -"-
   UINT iFlags = 0;                     // -"-

   for( int i = 0; i < m_aLazyData.GetSize(); i++ ) {
      LAZYDATA& data = m_aLazyData[i];
      switch( data.Action ) {
      case LAZY_OPEN_VIEW:
         {
            OpenView(data.szFilename, data.iLineNum, FINDVIEW_ALL, false);
         }
         break;
      case LAZY_GUI_ACTION:
         {
            switch( data.wParam ) {
            case GUI_ACTION_CLEARVIEW:
               {
                  CRichEditCtrl ctrlEdit = _pDevEnv->GetHwnd(data.WindowType);
                  if( ctrlEdit.IsWindow() ) ctrlEdit.SetWindowText(_T(""));
               }
               break;
            case GUI_ACTION_APPENDVIEW:
               {
                  CRichEditCtrl ctrlEdit = _pDevEnv->GetHwnd(data.WindowType);
                  if( ctrlEdit.IsWindow() ) AppendRtfText(ctrlEdit, data.szMessage);
               }
               break;
            case GUI_ACTION_ACTIVATEVIEW:
               {
                  CRichEditCtrl ctrlEdit = _pDevEnv->GetHwnd(data.WindowType);
                  if( ctrlEdit.IsWindow() ) _pDevEnv->ActivateAutoHideView(ctrlEdit);
               }
               break;
            case GUI_ACTION_PLAY_ANIMATION:
               {
                  _pDevEnv->PlayAnimation(TRUE, data.iLineNum);
               }
               break;
            case GUI_ACTION_STOP_ANIMATION:
               {
                  _pDevEnv->PlayAnimation(FALSE, 0);
               }
               break;
            case GUI_ACTION_FILE_RELOAD:
               {
                  IView* pView = _pDevEnv->GetActiveView();
                  if( pView != NULL ) pView->Reload();
               }
               break;
            case GUI_ACTION_COMPILESTART:
               {
                  m_bNeedsRecompile = false;
               }
               break;
            }
         }
         break;
      case LAZY_SHOW_MESSAGE:
         {
            // NOTE: Multiple popup-messages will not be displayed because 
            //       we overwrite the text variables here. Don't bother user
            //       with multiple nonsense.
            sMessage = data.szMessage;
            sCaption = data.szCaption;
            iFlags = data.iFlags;
         }
         break;
      case LAZY_SET_STATUSBARTEXT:
         {
            _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, data.szMessage, TRUE);
         }
         break;
      case LAZY_SEND_GLOBAL_VIEW_MESSAGE:
         {
            // Broadcast message to all shown views in solution
            CWindow wndMdiClient = _pDevEnv->GetHwnd(IDE_HWND_MDICLIENT);
            wndMdiClient.SendMessageToDescendants(WM_COMMAND, MAKEWPARAM(ID_DEBUG_EDIT_LINK, data.wParam), (LPARAM) &data);
         }
         break;
      case LAZY_SEND_PROJECT_VIEW_MESSAGE:
         {
            // Broadcast message to all known views in project
            for( int i = 0; i < m_aFiles.GetSize(); i++ ) m_aFiles[i]->SendMessage(WM_COMMAND, MAKEWPARAM(ID_DEBUG_EDIT_LINK, data.wParam), (LPARAM) &data);
         }
         break;
      case LAZY_SEND_ACTIVE_VIEW_MESSAGE:
         {
            // Send message to active view in editor
            m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_DEBUG_EDIT_LINK, data.wParam), (LPARAM) &data);
         }
         break;
      case LAZY_CLASSTREE_INFO:
         {
            m_TagManager.m_LexInfo.MergeIntoTree(data.szFilename, data.pLexFile);
         }
         break;
      case LAZY_DEBUGCOMMAND:
         {
            aDbgCmd.Add(CString(data.szMessage));
         }
         break;
      case LAZY_DEBUG_START_EVENT:
         {
            // Notify all views
            DelayedGlobalViewMessage(DEBUG_CMD_DEBUG_START);

            // Open up all debugger views needed
            m_DockManager.OpenView(m_wndMain, m_viewWatch,       _T("showWatch"),       ID_VIEW_WATCH);
            m_DockManager.OpenView(m_wndMain, m_viewStack,       _T("showStack"),       ID_VIEW_CALLSTACK);
            m_DockManager.OpenView(m_wndMain, m_viewThread,      _T("showThread"),      ID_VIEW_THREADS);
            m_DockManager.OpenView(m_wndMain, m_viewRegister,    _T("showRegister"),    ID_VIEW_REGISTERS);
            m_DockManager.OpenView(m_wndMain, m_viewMemory,      _T("showMemory"),      ID_VIEW_MEMORY);
            m_DockManager.OpenView(m_wndMain, m_viewDisassembly, _T("showDisassembly"), ID_VIEW_DISASM);
            m_DockManager.OpenView(m_wndMain, m_viewVariable,    _T("showVariable"),    ID_VIEW_VARIABLES);
            m_DockManager.OpenView(m_wndMain, m_viewBreakpoint,  _T("showBreakpoint"),  ID_VIEW_BREAKPOINTS);
            m_DockManager.OpenView(m_wndMain, m_viewOutput,      _T("showOutput"),      ID_VIEW_DEBUGOUTPUT);

            // Ensure the cursor is still visible after opening debug views
            DelayedLocalViewMessage(DEBUG_CMD_CURSORVISIBLE);
         }
         break;
      case LAZY_DEBUG_KILL_EVENT:
         {
            // Notify all views
            DelayedGlobalViewMessage(DEBUG_CMD_DEBUG_STOP);

            // If we're closing the debug session, then dispose
            // all debug views as well...
            m_DockManager.CloseView(m_wndMain, m_viewWatch,       _T("showWatch"),       ID_VIEW_WATCH);
            m_DockManager.CloseView(m_wndMain, m_viewStack,       _T("showStack"),       ID_VIEW_CALLSTACK);
            m_DockManager.CloseView(m_wndMain, m_viewThread,      _T("showThread"),      ID_VIEW_THREADS);
            m_DockManager.CloseView(m_wndMain, m_viewRegister,    _T("showRegister"),    ID_VIEW_REGISTERS);
            m_DockManager.CloseView(m_wndMain, m_viewMemory,      _T("showMemory"),      ID_VIEW_MEMORY);
            m_DockManager.CloseView(m_wndMain, m_viewDisassembly, _T("showDisassembly"), ID_VIEW_DISASM);
            m_DockManager.CloseView(m_wndMain, m_viewVariable,    _T("showVariable"),    ID_VIEW_VARIABLES);
            m_DockManager.CloseView(m_wndMain, m_viewBreakpoint,  _T("showBreakpoint"),  ID_VIEW_BREAKPOINTS);
            m_DockManager.CloseView(m_wndMain, m_viewOutput,      _T("showOutput"),      ID_VIEW_DEBUGOUTPUT);
         }
         break;
      case LAZY_DEBUG_BREAK_EVENT:
         {
            // Debugger stopped at some point (breakpoint, user break, etc).
            // Let's update the debug views with fresh values.
            // NOTE: This is a rather expensive operation so we'll at least try not to repeat
            //       it in this batch. The actual update is started outside this loop...
            bUpdateEvent = true;
         }
         break;
      case LAZY_DEBUG_INFO:
         {
            // New debug information has arrived. Spread it around to the
            // debug views since they are the likely candidates for this data.
            // Most of these messages originates from the LAZY_DEBUG_BREAK_EVENT handling
            // above anyway.

            if( m_viewStack.WantsData() )       m_viewStack.SetInfo(data.szMessage, data.MiInfo);
            if( m_viewWatch.WantsData() )       m_viewWatch.SetInfo(data.szMessage, data.MiInfo);               
            if( m_viewThread.WantsData() )      m_viewThread.SetInfo(data.szMessage, data.MiInfo);
            if( m_viewRegister.WantsData() )    m_viewRegister.SetInfo(data.szMessage, data.MiInfo);
            if( m_viewMemory.WantsData() )      m_viewMemory.SetInfo(data.szMessage, data.MiInfo);
            if( m_viewDisassembly.WantsData() ) m_viewDisassembly.SetInfo(data.szMessage, data.MiInfo);
            if( m_viewVariable.WantsData() )    m_viewVariable.SetInfo(data.szMessage, data.MiInfo);
            if( m_viewBreakpoint.WantsData() )  m_viewBreakpoint.SetInfo(data.szMessage, data.MiInfo);
            if( m_pQuickWatchDlg && m_pQuickWatchDlg->IsWindow() && m_pQuickWatchDlg->IsWindowVisible() ) m_pQuickWatchDlg->SetInfo(data.szMessage, data.MiInfo);

            if( _tcscmp(data.szMessage, _T("value")) == 0 ) {
               // Pass information to active editor, since it might be mouse hover information...
               // NOTE: Must use SendMessage() rather than delayed message because
               //       of scope of 'data' structure.
               data.Action = LAZY_SEND_ACTIVE_VIEW_MESSAGE;
               data.wParam = DEBUG_CMD_HOVERINFO;
               m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_DEBUG_EDIT_LINK, data.wParam), (LPARAM) &data);
               break;
            }
         }
         break;
      }
   }

   // Need to refresh debug views?
   if( bUpdateEvent ) 
   {
      ATLTRACE("DEBUG VIEW REFRESH\n");
      if( m_viewBreakpoint.WantsData() )  m_viewBreakpoint.EvaluateView(aDbgCmd);
      if( m_viewWatch.WantsData() )       m_viewWatch.EvaluateView(aDbgCmd);
      if( m_viewThread.WantsData() || m_viewStack.WantsData() ) m_viewThread.EvaluateView(aDbgCmd);
      if( m_viewStack.WantsData() )       m_viewStack.EvaluateView(aDbgCmd);
      if( m_viewDisassembly.WantsData() ) m_viewDisassembly.EvaluateView(aDbgCmd);
      if( m_viewVariable.WantsData() )    m_viewVariable.EvaluateView(aDbgCmd);
      if( m_viewRegister.WantsData() )    m_viewRegister.EvaluateView(aDbgCmd);
   }

   // Empty queue
   m_aLazyData.RemoveAll();

   // Release semaphore
   ::LeaveCriticalSection(&g_csDelayedData.m_sec);

   // Now that we're not blocking, send debug commands
   for( int a = 0; a < aDbgCmd.GetSize(); a++ ) m_DebugManager.DoDebugCommand(aDbgCmd[a]);

   // Display message to user if needed
   if( !sMessage.IsEmpty() ) {
      HWND hWnd = ::GetActiveWindow();
      _pDevEnv->ShowMessageBox(hWnd, sMessage, sCaption, iFlags | MB_MODELESS);
   }

   return 0;   
}

void CRemoteProject::DelayedMessage(LPCTSTR pstrMessage, LPCTSTR pstrCaption, UINT iFlags)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   ATLASSERT(_tcslen(pstrCaption)<(sizeof(data.szCaption)/sizeof(TCHAR)));
   ATLASSERT(_tcslen(pstrMessage)<(sizeof(data.szMessage)/sizeof(TCHAR)));
   ::ZeroMemory(data.szCaption, sizeof(data.szCaption));
   ::ZeroMemory(data.szMessage, sizeof(data.szMessage));
   data.Action = LAZY_SHOW_MESSAGE;
   _tcsncpy(data.szCaption, pstrCaption, (sizeof(data.szCaption) / sizeof(TCHAR)) - 1);
   _tcsncpy(data.szMessage, pstrMessage, (sizeof(data.szMessage) / sizeof(TCHAR)) - 1);
   data.iFlags = iFlags;
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedOpenView(LPCTSTR pstrFilename, int iLineNum)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   data.Action = LAZY_OPEN_VIEW;
   _tcscpy(data.szFilename, pstrFilename);
   data.iLineNum = iLineNum;
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedGuiAction(UINT iAction, LPCTSTR pstrFilename, int iLineNum)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   data.Action = LAZY_GUI_ACTION;
   data.wParam = iAction;
   _tcscpy(data.szFilename, pstrFilename == NULL ? _T("") : pstrFilename);
   data.iLineNum = iLineNum;
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedGuiAction(UINT iAction, IDE_HWND_TYPE WindowType, LPCTSTR pstrMessage)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   ::ZeroMemory(data.szMessage, sizeof(data.szMessage));
   data.Action = LAZY_GUI_ACTION;
   data.wParam = iAction;
   data.WindowType = WindowType;
   _tcsncpy(data.szMessage, pstrMessage == NULL ? _T("") : pstrMessage, (sizeof(data.szMessage) / sizeof(TCHAR)) - 1);
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedStatusBar(LPCTSTR pstrText)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   ATLASSERT(_tcslen(pstrText)<(sizeof(data.szMessage)/sizeof(TCHAR)));
   data.Action = LAZY_SET_STATUSBARTEXT;
   _tcscpy(data.szMessage, pstrText);
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedDebugCommand(LPCTSTR pstrCommand)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   ATLASSERT(_tcslen(pstrCommand)<(sizeof(data.szMessage)/sizeof(TCHAR)));
   ::ZeroMemory(data.szMessage, sizeof(data.szMessage));
   data.Action = LAZY_DEBUGCOMMAND;
   _tcsncpy(data.szMessage, pstrCommand, (sizeof(data.szMessage) / sizeof(TCHAR)) - 1);
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedGlobalViewMessage(WPARAM wCmd, LPCTSTR pstrFilename /*= NULL*/, int iLineNum /*= -1*/, UINT iFlags /*= 0*/)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   data.Action = LAZY_SEND_GLOBAL_VIEW_MESSAGE;
   data.wParam = wCmd;
   _tcscpy(data.szFilename, pstrFilename == NULL ? _T("") : pstrFilename);
   _tcscpy(data.szMessage, _T(""));
   _tcscpy(data.szCaption, _T(""));
   data.iLineNum = iLineNum;
   data.iFlags = iFlags;
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedLocalViewMessage(WPARAM wCmd, LPCTSTR pstrFilename /*= NULL*/, int iLineNum /*= -1*/, UINT iFlags /*= 0*/)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   data.Action = LAZY_SEND_ACTIVE_VIEW_MESSAGE;
   data.wParam = wCmd;
   _tcscpy(data.szFilename, pstrFilename == NULL ? _T("") : pstrFilename);
   _tcscpy(data.szMessage, _T(""));
   _tcscpy(data.szCaption, _T(""));
   data.iLineNum = iLineNum;
   data.iFlags = iFlags;
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedDebugBreakpoint(LPCTSTR pstrFilename, int iLineNum)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   data.Action = LAZY_SET_DEBUG_BREAKPOINT;
   _tcscpy(data.szFilename, pstrFilename);
   data.iLineNum = iLineNum;
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedDebugEvent(LAZYACTION event /*= LAZY_DEBUG_BREAK_EVENT*/)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   data.Action = event;
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedClassTreeInfo(LPCTSTR pstrFilename, LEXFILE* pLexFile)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   data.Action = LAZY_CLASSTREE_INFO;
   data.pLexFile = pLexFile;
   _tcscpy(data.szFilename, pstrFilename);
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedDebugInfo(LPCTSTR pstrCommand, CMiInfo& info)
{
   CLockDelayedDataInit lock;
   LAZYDATA dummy;
   m_aLazyData.Add(dummy);
   LAZYDATA& data = m_aLazyData[m_aLazyData.GetSize() - 1];
   ATLASSERT(_tcslen(pstrCommand)<(sizeof(data.szMessage)/sizeof(TCHAR)));
   ::ZeroMemory(data.szMessage, sizeof(data.szMessage));
   data.Action = LAZY_DEBUG_INFO;
   _tcsncpy(data.szMessage, pstrCommand, (sizeof(data.szMessage) / sizeof(TCHAR)) - 1);
   data.MiInfo.Copy(info);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

