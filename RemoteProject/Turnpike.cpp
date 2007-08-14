
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
   bool bUpdatedAlready = false;        // Make sure to trigger debug update only once
   CString sMessage;                    // For displaying a message-box asynchroniously
   CString sCaption;                    // -"-
   UINT iFlags;                         // -"-

   for( int i = 0; i < m_aLazyData.GetSize(); i++ ) {
      LAZYDATA& data = m_aLazyData[i];
      switch( data.Action ) {
      case LAZY_OPEN_VIEW:
         {
            OpenView(data.szFilename, data.iLineNum);
         }
         break;
      case LAZY_GUI_ACTION:
         {
            switch( data.wParam ) {
            case GUI_ACTION_CLEARVIEW:
               {
                  CRichEditCtrl ctrlEdit = data.hWnd;
                  ctrlEdit.SetWindowText(_T(""));
               }
               break;
            case GUI_ACTION_ACTIVATEVIEW:
               {
                  CRichEditCtrl ctrlEdit = data.hWnd;
                  _pDevEnv->ActivateAutoHideView(ctrlEdit);
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
            // BUG: Multiple popup-messages will not be displayed
            //      because we overwrite the text variables here.
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
            // Broadcast message to all known views in solution
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
            // Broadcast message to all known views in project
            CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
            wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_DEBUG_EDIT_LINK, data.wParam), (LPARAM) &data);
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

            // Open up all debugger view requested
            if( m_DockManager.IsAutoShown(m_viewWatch, _T("showWatch")) ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_WATCH, 0));
            if( m_DockManager.IsAutoShown(m_viewStack, _T("showStack")) ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_CALLSTACK, 0));
            if( m_DockManager.IsAutoShown(m_viewThread, _T("showThread")) ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_THREADS, 0));
            if( m_DockManager.IsAutoShown(m_viewRegister, _T("showRegister")) ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_REGISTERS, 0));
            if( m_DockManager.IsAutoShown(m_viewMemory, _T("showMemory")) ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_MEMORY, 0));
            if( m_DockManager.IsAutoShown(m_viewDisassembly, _T("showDisassembly")) ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_DISASM, 0));
            if( m_DockManager.IsAutoShown(m_viewVariable, _T("showVariable")) ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_VARIABLES, 0));
            if( m_DockManager.IsAutoShown(m_viewBreakpoint, _T("showBreakpoint")) ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_BREAKPOINTS, 0));
            if( m_DockManager.IsAutoShown(m_viewOutput, _T("showOutput")) ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_DEBUGOUTPUT, 0));
         }
         break;
      case LAZY_DEBUG_KILL_EVENT:
         {
            // Notify all views
            DelayedGlobalViewMessage(DEBUG_CMD_DEBUG_STOP);

            // If we're closing the debug session, then dispose
            // all debug views as well...

            m_DockManager.SetInfo(m_viewWatch, _T("showWatch"));
            m_DockManager.SetInfo(m_viewStack, _T("showStack"));
            m_DockManager.SetInfo(m_viewThread, _T("showThread"));
            m_DockManager.SetInfo(m_viewRegister, _T("showRegister"));
            m_DockManager.SetInfo(m_viewMemory, _T("showMemory"));
            m_DockManager.SetInfo(m_viewDisassembly, _T("showDisassembly"));
            m_DockManager.SetInfo(m_viewVariable, _T("showVariable"));
            m_DockManager.SetInfo(m_viewBreakpoint, _T("showBreakpoint"));
            m_DockManager.SetInfo(m_viewOutput, _T("showOutput"));

            if( m_viewWatch.IsWindow() && m_viewWatch.IsWindowVisible() ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_WATCH, 0));
            if( m_viewStack.IsWindow() && m_viewStack.IsWindowVisible() ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_CALLSTACK, 0));
            if( m_viewThread.IsWindow() && m_viewThread.IsWindowVisible() ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_THREADS, 0));
            if( m_viewRegister.IsWindow() && m_viewRegister.IsWindowVisible() ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_REGISTERS, 0));
            if( m_viewMemory.IsWindow() && m_viewMemory.IsWindowVisible() ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_MEMORY, 0));
            if( m_viewDisassembly.IsWindow() && m_viewDisassembly.IsWindowVisible() ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_DISASM, 0));
            if( m_viewVariable.IsWindow() && m_viewVariable.IsWindowVisible() ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_VARIABLES, 0));
            if( m_viewBreakpoint.IsWindow() && m_viewBreakpoint.IsWindowVisible() ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_BREAKPOINTS, 0));
            if( m_viewOutput.IsWindow() && m_viewOutput.IsWindowVisible() ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_DEBUGOUTPUT, 0));
         }
         break;
      case LAZY_DEBUG_BREAK_EVENT:
         {
            // Debugger stopped at some point (breakpoint, user break, etc).
            // Let's update the debug views with fresh values.

            // NOTE: This is a rather expensive operation so we'll at least try not to repeat
            //       it in this batch.
            if( bUpdatedAlready ) break;
            bUpdatedAlready = true;

            if( m_viewBreakpoint.WantsData() ) {
               aDbgCmd.Add(CString(_T("-break-list")));
            }
            if( m_viewWatch.WantsData() ) {
               m_viewWatch.EvaluateValues(aDbgCmd);
            }
            if( m_viewThread.WantsData() || m_viewStack.WantsData() ) {
               aDbgCmd.Add(CString(_T("-thread-list-ids")));
            }
            if( m_viewStack.WantsData() ) {
               aDbgCmd.Add(CString(_T("-stack-list-frames")));
            }
            if( m_viewDisassembly.WantsData() ) {
               m_viewDisassembly.PopulateView(aDbgCmd);
            }
            if( m_viewVariable.WantsData() ) {
               switch( m_viewVariable.GetCurSel() ) {
               case 0:
                  aDbgCmd.Add(CString(_T("-stack-list-locals 1")));
                  break;
               case 1:
                  aDbgCmd.Add(CString(_T("-stack-list-arguments 1 0 0")));
                  break;                   
               }
            }
            if( m_viewRegister.WantsData() ) {
               if( m_viewRegister.GetNameCount() == 0 ) {
                  aDbgCmd.Add(CString(_T("-data-list-register-names")));
               }
               aDbgCmd.Add(CString(_T("-data-list-register-values N")));
            }
         }
         break;
      case LAZY_DEBUG_INFO:
         {
            // New debug information has arrived. Spread it around to the
            // debug views since they are the likely candidates for this data.
            // Most of these messages originates from the LAZY_DEBUG_BREAK_EVENT handling
            // above anyway.

            if( m_viewStack.WantsData() ) m_viewStack.SetInfo(data.szMessage, data.MiInfo);
            if( m_viewWatch.WantsData() ) m_viewWatch.SetInfo(data.szMessage, data.MiInfo);               
            if( m_viewThread.WantsData() ) m_viewThread.SetInfo(data.szMessage, data.MiInfo);
            if( m_viewRegister.WantsData() ) m_viewRegister.SetInfo(data.szMessage, data.MiInfo);
            if( m_viewMemory.WantsData() ) m_viewMemory.SetInfo(data.szMessage, data.MiInfo);
            if( m_viewDisassembly.WantsData() ) m_viewDisassembly.SetInfo(data.szMessage, data.MiInfo);
            if( m_viewVariable.WantsData() ) m_viewVariable.SetInfo(data.szMessage, data.MiInfo);
            if( m_viewBreakpoint.WantsData() ) m_viewBreakpoint.SetInfo(data.szMessage, data.MiInfo);
            if( m_pQuickWatchDlg && m_pQuickWatchDlg->IsWindow() && m_pQuickWatchDlg->IsWindowVisible() ) m_pQuickWatchDlg->SetInfo(data.szMessage, data.MiInfo);

            if( _tcscmp(data.szMessage, _T("value")) == 0 ) {
               // Pass information to active editor, since it might be mouse hover information...
               // NOTE: Must use SendMessage() rather than delayed message because
               //       of scope of 'data' structure.
               data.Action = LAZY_SEND_GLOBAL_VIEW_MESSAGE;
               data.wParam = DEBUG_CMD_HOVERINFO;
               m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_DEBUG_EDIT_LINK, data.wParam), (LPARAM) &data);
               break;
            }
         }
         break;
      }
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
   data.Action = LAZY_SHOW_MESSAGE;
   _tcscpy(data.szCaption, pstrCaption);
   _tcscpy(data.szMessage, pstrMessage);
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

void CRemoteProject::DelayedGuiAction(UINT iAction, HWND hWnd)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   data.Action = LAZY_GUI_ACTION;
   data.wParam = iAction;
   data.hWnd = hWnd;
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedStatusBar(LPCTSTR pstrText)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   data.Action = LAZY_SET_STATUSBARTEXT;
   _tcscpy(data.szMessage, pstrText);
   m_aLazyData.Add(data);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

void CRemoteProject::DelayedDebugCommand(LPCTSTR pstrCommand)
{
   CLockDelayedDataInit lock;
   LAZYDATA data;
   ATLASSERT(_tcslen(pstrCommand)<(sizeof(data.szMessage)/sizeof(TCHAR))-1);
   data.Action = LAZY_DEBUGCOMMAND;
   _tcscpy(data.szMessage, pstrCommand);
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
   ATLASSERT(_tcslen(pstrCommand)<(sizeof(data.szMessage)/sizeof(TCHAR))-1);
   data.Action = LAZY_DEBUG_INFO;
   _tcscpy(data.szMessage, pstrCommand);
   data.MiInfo.Copy(info);
   m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_PROCESS, 0));
}

