
#include "StdAfx.h"
#include "resource.h"

#include "DebugManager.h"

#include "Project.h"
#include "MiInfo.h"

#include "GdbAdaptor.h"
#include "DbxAdaptor.h"

#pragma code_seg( "MISC" )


////////////////////////////////////////////////////////
//

void CDebugStopThread::Init(CDebugManager* pManager)
{
   ATLASSERT(pManager);
   m_pManager = pManager;
}

DWORD CDebugStopThread::Run()
{
   // This is a thread designed to wait a short while for the debugged
   // process to exit cleanly. If the debugged process refuses we will
   // need to force a termination of the entire shell session.
   const int TERMINATE_AFTER_SECONDS = 3;
   for( int i = 0; !ShouldStop() && i < TERMINATE_AFTER_SECONDS * 1000; i += 200 ) {
      if( !m_pManager->IsBusy() ) break;
      ::Sleep(200UL);
   }
   // If its still alive after waiting this long, we'll
   // need to force a termination.
   if( m_pManager->IsBusy() ) m_pManager->SignalStop();
   return 0;
}


////////////////////////////////////////////////////////
//

static CComAutoCriticalSection m_csBreakpoint;

struct CLockBreakpointData
{
   CLockBreakpointData() { m_csBreakpoint.Lock(); };
   ~CLockBreakpointData() { m_csBreakpoint.Unlock(); };
};


////////////////////////////////////////////////////////
//

CDebugManager::CDebugManager() :
   m_pProject(NULL),
   m_pAdaptor(NULL),
   m_bBreaked(false),
   m_bRunning(false),
   m_bSeenExit(false),
   m_bDebugging(false),
   m_bCommandMode(false),
   m_bDebugEvents(false),
   m_dblDebuggerVersion(0.0),
   m_nIgnoreErrors(0),
   m_nIgnoreBreaks(0)
{
   Clear();
   m_eventAck.Create(NULL, TRUE, FALSE);
}

CDebugManager::~CDebugManager()
{
   Stop();
}

void CDebugManager::Init(CRemoteProject* pProject)
{
   ATLASSERT(pProject);
   m_pProject = pProject;
   m_StopThread.Init(this);
   m_ShellManager.Init(pProject);
   m_aBreakpoints.RemoveAll();
}

void CDebugManager::Clear()
{
   m_ShellManager.Clear();

   m_sDebuggerType = _T("gdb");
   m_sCommandCD = _T("cd $PATH$");
   m_sAppExecutable = _T("./$PROJECTNAME$");
   m_sAppArgs = _T("");
   m_sDebuggerExecutable = _T("gdb");
   m_sAttachExe = _T("-i=mi ./$PROJECTNAME$");
   m_sAttachPid = _T("-i=mi -pid $PID$");
   m_sAttachCore = _T("-i=mi $PROCESS$ $COREFILE$");
   m_sDebugMain = _T("main");
   m_lStartTimeout = 4;
   m_bMaintainSession = FALSE;
}

bool CDebugManager::Load(ISerializable* pArc)
{
   Clear();
   if( !pArc->ReadGroupBegin(_T("Debugger")) ) return true;

   if( !m_ShellManager.Load(pArc) ) return false;

   if( !pArc->ReadItem(_T("Commands")) ) return false;
   pArc->Read(_T("changeDir"), m_sCommandCD.GetBufferSetLength(200), 200);
   m_sCommandCD.ReleaseBuffer();
   pArc->Read(_T("app"), m_sAppExecutable.GetBufferSetLength(200), 200);
   m_sAppExecutable.ReleaseBuffer();
   pArc->Read(_T("args"), m_sAppArgs.GetBufferSetLength(300), 300);
   m_sAppArgs.ReleaseBuffer();

   if( !pArc->ReadItem(_T("DbgConfig")) ) 
      if( !pArc->ReadItem(_T("GDB")) ) return false;
   pArc->Read(_T("type"), m_sDebuggerType.GetBufferSetLength(32), 32);
   m_sDebuggerType.ReleaseBuffer();
   pArc->Read(_T("app"), m_sDebuggerExecutable.GetBufferSetLength(200), 200);
   m_sDebuggerExecutable.ReleaseBuffer();
   pArc->Read(_T("args"), m_sAttachExe.GetBufferSetLength(300), 300);
   m_sAttachExe.ReleaseBuffer();
   pArc->Read(_T("core"), m_sAttachCore.GetBufferSetLength(300), 300);
   m_sAttachCore.ReleaseBuffer();
   pArc->Read(_T("pid"), m_sAttachPid.GetBufferSetLength(300), 300);
   m_sAttachPid.ReleaseBuffer();
   pArc->Read(_T("main"), m_sDebugMain.GetBufferSetLength(64), 64);
   m_sDebugMain.ReleaseBuffer();
   pArc->Read(_T("searchPath"), m_sSearchPath.GetBufferSetLength(200), 200);
   m_sSearchPath.ReleaseBuffer();
   pArc->Read(_T("startTimeout"), m_lStartTimeout);
   pArc->Read(_T("maintainSession"), m_bMaintainSession);

   if( m_lStartTimeout <= 0 ) m_lStartTimeout = 4;
   if( m_sDebuggerType.IsEmpty() ) m_sDebuggerType = _T("gdb");
   if( m_sAttachPid.IsEmpty() ) m_sAttachPid = _T("-i=mi -pid $PID$");
   if( m_sAttachCore.IsEmpty() ) m_sAttachCore = _T("-i=mi $PROCESS$ $COREFILE$");

   if( !pArc->ReadGroupEnd() ) return false;
   return true;
}

bool CDebugManager::Save(ISerializable* pArc)
{
   if( !pArc->WriteGroupBegin(_T("Debugger")) ) return true;

   if( !m_ShellManager.Save(pArc) ) return false;
   
   if( !pArc->WriteItem(_T("Commands")) ) return false;
   pArc->Write(_T("changeDir"), m_sCommandCD);
   pArc->Write(_T("app"), m_sAppExecutable);
   pArc->Write(_T("args"), m_sAppArgs);

   if( !pArc->WriteItem(_T("DbgConfig")) ) return false;
   pArc->Write(_T("type"), m_sDebuggerType);
   pArc->Write(_T("app"), m_sDebuggerExecutable);
   pArc->Write(_T("args"), m_sAttachExe);
   pArc->Write(_T("core"), m_sAttachCore);
   pArc->Write(_T("pid"), m_sAttachPid);
   pArc->Write(_T("main"), m_sDebugMain);
   pArc->Write(_T("searchPath"), m_sSearchPath);
   pArc->Write(_T("startTimeout"), m_lStartTimeout);
   pArc->Write(_T("maintainSession"), m_bMaintainSession);

   if( !pArc->WriteGroupEnd() ) return false;
   return true;
}

void CDebugManager::ProgramStop()
{
   // Get out of debugger prompt
   if( m_bDebugging ) {
      _PauseDebugger();
      DoDebugCommand(_T("-gdb-exit"));
   }
   // If we maintain the session we don't actually terminate the connection
   // but retain it until the next time. Otherwise we exit the terminal to disconnect
   // the session completely.
   if( !m_bMaintainSession ) {
      DoDebugCommand(_T("exit"));
   }
   // Start the "StopThread" that monitors that we're actually stopping
   // the session eventually. The StopThread relies on the IsBusy() state.
   if( !m_StopThread.IsRunning() ) {
      m_StopThread.Stop();
      m_StopThread.Start();
   }
}

void CDebugManager::SignalStop()
{
   m_ShellManager.SignalStop();
   m_StopThread.SignalStop();
   _ClearLink();
}

bool CDebugManager::Stop()
{
   // NOTE: Don't call SignalStop() here because we've probably
   //       already sent the DelayedXXX messages once...
   m_ShellManager.Stop();
   m_StopThread.Stop();
   _ClearLink();
   return true;
}

bool CDebugManager::IsBusy() const
{
   return m_bRunning && m_ShellManager.IsConnected();
}

bool CDebugManager::IsBreaked() const
{
   return IsDebugging() && m_bBreaked;
}

bool CDebugManager::IsDebugging() const
{
   return m_bDebugging;
}

bool CDebugManager::Break()
{
   // NOTE: To break the debugger we're sending a CTRL+C control sequence or
   //       similar (depends on underlying protocol). The effect is that GDB
   //       catches a SIGINT and stops with a break error/warning. We'll ignore
   //       the error and resume our deeds.
   m_nIgnoreErrors++;
   return DoSignal(TERMINAL_BREAK);
}

bool CDebugManager::ClearBreakpoints()
{
   CLockBreakpointData lock;
   if( IsDebugging() ) {
      if( m_aBreakpoints.GetSize() > 0 ) {
         bool bRestartApp = _PauseDebugger();
         // We're going to delete the breakpoints in GDB itself first
         CString sArgs;
         for( long i = 0; i < m_aBreakpoints.GetSize(); i++ ) {
            if( i > 0 ) sArgs += _T(" ");
            sArgs.Append(m_aBreakpoints.GetValueAt(i));
         }
         DoDebugCommandV(_T("-break-delete %s"), sArgs);
         DoDebugCommand(_T("-break-list"));
         if( bRestartApp ) _ResumeDebugger();
      }
   }
   // We can now safely remove our internal breakpoint list
   // and signal to the view to do the same...
   m_aBreakpoints.RemoveAll();
   m_pProject->DelayedGlobalViewMessage(DEBUG_CMD_CLEAR_BREAKPOINTS);
   return true;
}

bool CDebugManager::AddBreakpoint(LPCTSTR pstrFilename, int iLineNum)
{
   CLockBreakpointData lock;
   // Add it to internal list
   pstrFilename = ::PathFindFileName(pstrFilename);
   CString sText;
   sText.Format(_T("%s:%d"), pstrFilename, iLineNum + 1);
   // We add a dummy entry now. We don't know the internal GDB breakpoint-no
   // for this breakpoint, but we'll soon learn when GDB answers our request.
   m_aBreakpoints.Add(sText, 0);
   // If we're debugging, we need to update GDB as well...
   if( IsDebugging() ) 
   {     
      // Attempt to halt app if currently running
      bool bRestartApp = _PauseDebugger();
      // Send GDB commands
      if( !DoDebugCommandV(_T("-break-insert %s"), sText) ) return false;
      if( !DoDebugCommand(_T("-break-list")) ) return false;
      // Warn about GDB's inability to add ASYNC breakpoints
      if( !IsBreaked() ) {
         CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_WARNING));
         CString sMsg(MAKEINTRESOURCE(IDS_ERR_BREAKPOINTASYNC));
         m_pProject->DelayedMessage(sMsg, sCaption, MB_ICONINFORMATION | MB_MODELESS | MB_SHOWONCE);
      }
      // Restart app if we halted it
      if( bRestartApp ) _ResumeDebugger();
   }
   return true;
}

bool CDebugManager::RemoveBreakpoint(LPCTSTR pstrFilename, int iLineNum)
{
   CLockBreakpointData lock;
   pstrFilename = ::PathFindFileName(pstrFilename);
   CString sText;
   sText.Format(_T("%s:%d"), pstrFilename, iLineNum + 1);
   if( IsDebugging() ) {
      // Find the internal GDB breakpoint-nr for this breakpoint
      int iIndex = m_aBreakpoints.FindKey(sText);;
      if( iIndex >= 0 ) {
         long lNumber = m_aBreakpoints.Lookup(sText);
         bool bRestartApp = _PauseDebugger();
         // We have it; let's ask GDB to delete the breakpoint
         if( !DoDebugCommandV(_T("-break-delete %ld"), lNumber) ) return false;
         if( !DoDebugCommand(_T("-break-list")) ) return false;
         if( bRestartApp ) _ResumeDebugger();
      }
   }
   // Remove it from our own internal list too 
   return m_aBreakpoints.Remove(sText) == TRUE;
}

bool CDebugManager::GetBreakpoints(LPCTSTR pstrFilename, CSimpleArray<int>& aLines) const
{
   CLockBreakpointData lock;
   pstrFilename = ::PathFindFileName(pstrFilename);
   size_t cchName = _tcslen(pstrFilename);
   for( int i = 0; i < m_aBreakpoints.GetSize(); i++ ) {
      if( _tcsncmp(m_aBreakpoints.GetKeyAt(i), pstrFilename, cchName) == 0 ) {
         LPCTSTR pstr = _tcschr(m_aBreakpoints.GetKeyAt(i), ':');
         if( pstr != NULL ) {
            int iLine = _ttoi(pstr + 1) - 1;
            aLines.Add(iLine);
         }
      }
   }
   return true;
}

bool CDebugManager::SetBreakpoints(LPCTSTR pstrFilename, CSimpleArray<int>& aLines)
{
   ATLASSERT(IsDebugging());
   // This method is called from the views upon a DEBUG_CMD_REQUEST_BREAKPOINTS request
   // where we ask the views to identify all active breakpoints. We're supposed to
   // clear our internal breakpoint-list for that file and add the active items.
   // It must be run *before* the debugger is started!
   // First, let's remove all existing breakpoints from this file.
   CLockBreakpointData lock;
   size_t cchName = _tcslen(pstrFilename);
   int i;
   for( i = 0; i < m_aBreakpoints.GetSize(); i++ ) {
      if( _tcsncmp(m_aBreakpoints.GetKeyAt(i), pstrFilename, cchName) == 0 ) {
         m_aBreakpoints.Remove(m_aBreakpoints.GetKeyAt(i));
         i = -1;
         continue;
      }
   }
   // Add all the new breakpoints...
   CString sText;
   for( i = 0; i < aLines.GetSize(); i++ ) {
      sText.Format(_T("%s:%d"), pstrFilename, aLines[i] + 1);
      m_aBreakpoints.Add(sText, 0L);
   }
   return true;
}

bool CDebugManager::RunTo(LPCTSTR pstrFilename, int iLineNum)
{
   ATLASSERT(IsDebugging());
   return DoDebugCommandV(_T("-exec-until %s:%d"), pstrFilename, iLineNum);
}

bool CDebugManager::SetNextStatement(LPCTSTR pstrFilename, int iLineNum)
{
   ATLASSERT(IsDebugging());
   DoDebugCommandV(_T("-break-insert -t %s:%d"), pstrFilename, iLineNum);
   DoDebugCommandV(_T("-exec-jump %s:%d"), pstrFilename, iLineNum);
   return true;
}

/**
 * Starts the compiled executable on the remote server.
 * This method runs the executable and displays its terminal output.
 */
bool CDebugManager::RunNormal()
{  
   // Launch the remote process and supervise it with
   // a telnet prompt and display it's output.

   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);

   // We may need to kill the old session
   if( !_CheckStatus() ) return false;

   CWaitCursor cursor;

   Stop();

   CString sStatus;
   sStatus.LoadString(IDS_STATUS_CONNECT_WAIT);
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus, FALSE);
   _pDevEnv->PlayAnimation(TRUE, ANIM_TRANSFER);

   // Initialize and show the console output view.
   // The Output view is always displayed. When it is closed we also want
   // the session to terminate.
   CTelnetView* pView = m_pProject->GetDebugView();
   pView->Clear();
   pView->Init(&m_ShellManager);
   pView->ModifyFlags(0, TELNETVIEW_TERMINATEONCLOSE);
   RECT rcLogWin = { 120, 120, 800 + 120, 600 + 120 };   // TODO: Memorize this position
   _pDevEnv->AddDockView(pView->m_hWnd, IDE_DOCK_HIDE, rcLogWin);
   _pDevEnv->AddDockView(pView->m_hWnd, IDE_DOCK_FLOAT, rcLogWin);
   pView->CenterWindow();
   pView->UpdateWindow();

   // Restart the connection to the remote machine
   m_ShellManager.Start();
   if( !m_ShellManager.WaitForConnection() ) {      
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_ERROR));
      CString sMsg(MAKEINTRESOURCE(IDS_ERR_HOSTCONNECT));
      _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_ICONEXCLAMATION | MB_MODELESS);
      _pDevEnv->PlayAnimation(FALSE, 0);
      Stop();
      return false;
   }
   m_bRunning = true;
   m_bSeenExit = false;

   sStatus.LoadString(IDS_STATUS_RUNNING);
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus, TRUE);
   _pDevEnv->PlayAnimation(FALSE, 0);

   CString sCommand = _TranslateCommand(m_sCommandCD);
   if( !m_ShellManager.WriteData(sCommand) ) {
      SignalStop();
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_ERROR));
      CString sMsg(MAKEINTRESOURCE(IDS_ERR_DATAWRITE));
      _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_ICONEXCLAMATION | MB_MODELESS);
      Stop();
      return false;
   }
   sCommand.Format(_T("%s %s"), m_sAppExecutable, m_sAppArgs);
   sCommand = _TranslateCommand(sCommand);
   m_ShellManager.WriteData(sCommand);
   m_ShellManager.WriteData(_T(""));

   return true;
}

/**
 * Starts the debugger.
 * This method starts the debugger and begins debugging the compiled executable.
 */
bool CDebugManager::RunDebug()
{
   // Launch the remote process in debug-mode and switch
   // editor state to debugging.
   CSimpleArray<CString> aList;
   CString sCommand = m_sCommandCD;
   aList.Add(sCommand);   
   sCommand.Format(_T("%s %s"), m_sDebuggerExecutable, m_sAttachExe);
   aList.Add(sCommand);   
   if( !_AttachDebugger(aList, false) ) return false;
   // Start debugging session
   DoDebugCommandV(_T("-exec-arguments %s"), m_sAppArgs);
   return DoDebugCommand(_T("-exec-run"));
}

/**
 * Debugs a specific Process ID (PID) on the remote server.
 * Starts the debugger and attaches to a specific PID.
 */
bool CDebugManager::AttachProcess(long lPID)
{  
   // Launch the remote process from a PID (Process ID)
   CSimpleArray<CString> aList;
   CString sCommand = m_sCommandCD;
   aList.Add(sCommand);   
   sCommand.Format(_T("%s %s"), m_sDebuggerExecutable, m_sAttachPid);
   sCommand.Replace(_T("$PID$"), ToString(lPID));
   aList.Add(sCommand);   
   if( !_AttachDebugger(aList, true) ) return false;
   // The sessions starts as halted; update views
   m_bBreaked = true;
   m_pProject->DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   return true;
}

/**
 * Debugs a specific core file on the remote server.
 * Starts the debugger and attaches to a specific core file.
 */
bool CDebugManager::AttachCoreFile(LPCTSTR pstrProcess, LPCTSTR pstrCoreFilename)
{  
   // Remember these for the next time...
   m_sCoreProcess = pstrProcess;
   m_sCoreFile = pstrCoreFilename;
   // Launch the remote process from a PID (Process ID)
   CSimpleArray<CString> aList;
   CString sCommand = m_sCommandCD;
   aList.Add(sCommand);   
   sCommand.Format(_T("%s %s"), m_sDebuggerExecutable, m_sAttachCore);
   sCommand.Replace(_T("$PROCESS$"), pstrProcess);
   sCommand.Replace(_T("$COREFILE$"), pstrCoreFilename);
   aList.Add(sCommand);   
   if( !_AttachDebugger(aList, true) ) return false;
   // The sessions starts as halted; update views
   m_bBreaked = true;
   m_pProject->DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   return true;
}

bool CDebugManager::RunContinue()
{
   ATLASSERT(IsDebugging());
   // If we're already debugging (but have stopped in a breakpoint)
   // we might just need to continue debugging...
   return DoDebugCommand(_T("-exec-continue"));
}

bool CDebugManager::EvaluateExpression(LPCTSTR pstrValue)
{
   ATLASSERT(!::IsBadStringPtr(pstrValue,-1));
   // We must be debugging to talk to the debugger
   if( !IsDebugging() ) return false;
   // Send query to GDB debugger about the data value.
   // NOTE: Somewhere down in the _TranslateCommand() method
   //       we'll make sure to ignore any errors returned since we cannot
   //       really control what the mouse-hover intercepts.
   CString sValue = pstrValue;
   sValue.Replace(_T("\\"), _T("\\\\"));
   sValue.Replace(_T("\""), _T("\\\""));
   DoDebugCommandV(_T("-data-evaluate-expression \"%s\""), sValue);
   return true;  // We don't know if there will be data available!! Just be happy about it.
}

bool CDebugManager::DoDebugCommand(LPCTSTR pstrCommand)
{
   ATLASSERT(!::IsBadStringPtr(pstrCommand,-1));
   // We must be debugging to execute anything!
   if( !IsDebugging() ) return false;
   // HACK: We're having trouble with echoing
   //       our own strings, so we'll check for a brief
   //       moment if a new GDB prompt arrives.
   //       However, we quickly give up and rely on
   //       GDB being somewhat asynchronously. This does have an
   //       inpact on UI responsiveness though.
   //       The problem is, however, that there's no 
   //       flow-control in the GDB-MI std-out handler, 
   //       so we'll risk to send the command in the middle 
   //       of regular output. This is a big mess!
   const DWORD COMMAND_SEQUENCE_TIMEOUT_MS = 150UL;
   m_eventAck.WaitForEvent(COMMAND_SEQUENCE_TIMEOUT_MS);
   m_eventAck.ResetEvent();
   // Translate command and send it
   return m_ShellManager.WriteData(_TranslateCommand(pstrCommand));
}

bool CDebugManager::DoDebugCommandV(LPCTSTR pstrCommand, ...)
{
   va_list args;
   va_start(args, pstrCommand);
   CString sCommand;
   sCommand.FormatV(pstrCommand, args);
   va_end(args);
   return DoDebugCommand(sCommand);
}

bool CDebugManager::DoSignal(BYTE bCmd)
{
   if( !IsDebugging() ) return false;
   return m_ShellManager.WriteSignal(bCmd);
}

CString CDebugManager::GetParam(LPCTSTR pstrName) const
{
   CString sName = pstrName;
   if( sName == _T("DebuggerType") ) return m_sDebuggerType;
   if( sName == _T("ChangeDir") ) return m_sCommandCD;
   if( sName == _T("App") ) return m_sAppExecutable;
   if( sName == _T("AppArgs") ) return m_sAppArgs;
   if( sName == _T("Debugger") ) return m_sDebuggerExecutable;
   if( sName == _T("AttachExe") ) return m_sAttachExe;
   if( sName == _T("AttachCore") ) return m_sAttachCore;
   if( sName == _T("AttachPid") ) return m_sAttachPid;
   if( sName == _T("DebugMain") ) return m_sDebugMain;
   if( sName == _T("SearchPath") ) return m_sSearchPath;
   if( sName == _T("StartTimeout") ) return ToString(m_lStartTimeout);
   if( sName == _T("MaintainSession") ) return m_bMaintainSession ? _T("true") : _T("false");
   if( sName == _T("CoreProcess") ) return m_sCoreProcess;
   if( sName == _T("CoreFile") ) return m_sCoreFile;
   if( sName == _T("InCommand") ) return m_bCommandMode ? _T("true") : _T("false");
   return m_ShellManager.GetParam(pstrName);
}

void CDebugManager::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   if( sName == _T("DebuggerType") ) m_sDebuggerType = pstrValue;
   if( sName == _T("ChangeDir") ) m_sCommandCD = pstrValue;
   if( sName == _T("App") ) m_sAppExecutable = pstrValue;
   if( sName == _T("AppArgs") ) m_sAppArgs = pstrValue;
   if( sName == _T("Debugger") ) m_sDebuggerExecutable = pstrValue;
   if( sName == _T("AttachExe") ) m_sAttachExe = pstrValue;
   if( sName == _T("AttachCore") ) m_sAttachCore = pstrValue;
   if( sName == _T("AttachPid") ) m_sAttachPid = pstrValue;
   if( sName == _T("DebugMain") ) m_sDebugMain = pstrValue;
   if( sName == _T("SearchPath") ) m_sSearchPath = pstrValue;
   if( sName == _T("StartTimeout") ) m_lStartTimeout = _ttol(pstrValue);
   if( sName == _T("MaintainSession") ) m_bMaintainSession = _tcscmp(pstrValue, _T("true")) == 0;
   if( sName == _T("CoreProcess") ) m_sCoreProcess = pstrValue;
   if( sName == _T("CoreFile") ) m_sCoreFile = pstrValue;
   if( sName == _T("InCommand") ) m_bCommandMode = _tcscmp(pstrValue, _T("true")) == 0;
   m_ShellManager.SetParam(pstrName, pstrValue);  
   if( m_lStartTimeout <= 0 ) m_lStartTimeout = 4;
}

// Implementation

/**
 * Runs the debugger.
 * This method runs the debugger on the remote server and hooks up
 * output display, environment settings and command interaction.
 */
bool CDebugManager::_AttachDebugger(CSimpleArray<CString>& aCommands, bool bExternalProcess)
{
   // Launch the remote process in debug mode and switch
   // editor state to debugging.

   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);

   if( !_CheckStatus() ) return false;

   CWaitCursor cursor;

   // Stop the current debug session and initiate a new?
   if( !m_bMaintainSession || !m_bSeenExit ) Stop();
 
   CString sStatus;
   sStatus.LoadString(IDS_STATUS_CONNECT_WAIT);
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus, FALSE);
   _pDevEnv->PlayAnimation(TRUE, ANIM_TRANSFER);

   // Initialize and hide the debug-log output window
   CTelnetView* pView = m_pProject->GetDebugView();
   pView->Clear();
   pView->Init(&m_ShellManager);
   RECT rcLogWin = { 120, 120, 800 + 120, 600 + 120 };   
   _pDevEnv->AddDockView(pView->m_hWnd, IDE_DOCK_HIDE, rcLogWin);

   // Initialize and hide the output-log view.
   // The start event will show the view based on user's preferences.
   CTelnetView* pOutput = m_pProject->GetOutputView();
   pOutput->Clear();
   pOutput->Init(&m_ShellManager);
   pOutput->ModifyFlags(0, TELNETVIEW_FILTERDEBUG);
   if( pOutput->IsWindow() ) _pDevEnv->AddDockView(pOutput->m_hWnd, IDE_DOCK_HIDE, rcLogWin);

   // Create the debugger adaptor that will translate input/output
   // into the GDB MI which we support internally.
   if( m_pAdaptor != NULL ) delete m_pAdaptor;
   m_pAdaptor = NULL;
   if( m_sDebuggerType == _T("gdb") ) m_pAdaptor = new CGdbAdaptor();
   if( m_sDebuggerType == _T("dbx") ) m_pAdaptor = new CDbxAdaptor();
   if( m_pAdaptor == NULL ) return false;
   m_pAdaptor->Init(m_pProject);

   // Prepare session
   m_ShellManager.AddLineListener(this);
   m_nIgnoreErrors = 0;
   m_nIgnoreBreaks = 0;
   m_bDebugging = true;
   m_bBreaked = false;
   m_nDebugAck = 0;
   m_nLastAck = 0;
   m_eventAck.ResetEvent();

   // (Re)start connection?
   if( !m_ShellManager.IsConnected() ) 
   {
      m_ShellManager.Start();
      if( !m_ShellManager.WaitForConnection() ) {      
         SignalStop();
         CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_ERROR));
         CString sMsg(MAKEINTRESOURCE(IDS_ERR_HOSTCONNECT));
         _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_ICONEXCLAMATION | MB_MODELESS);
         _pDevEnv->PlayAnimation(FALSE, 0);
         Stop();
         return false;
      }
   }

   // We should detect a clean exit for this session...
   m_bRunning = true;
   m_bSeenExit = false;
   m_bDebugEvents = false;

   sStatus.LoadString(IDS_STATUS_DEBUGGING);
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus, FALSE);
   _pDevEnv->PlayAnimation(FALSE, 0);

   // Fire up the debug views
   m_pProject->DelayedDebugEvent(LAZY_DEBUG_START_EVENT);

   // Execute the commands
   for( int i = 0; i < aCommands.GetSize(); i++ ) {
      // Run debugger on remote server
      CString sCommand = _TranslateCommand(aCommands[i]);
      if( !m_ShellManager.WriteData(sCommand) ) {
         SignalStop();
         CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_ERROR));
         CString sMsg(MAKEINTRESOURCE(IDS_ERR_DATAWRITE));
         _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_ICONEXCLAMATION | MB_MODELESS);
         Stop();
         return false;
      }
   }

   // Wait for the debug prompt
   if( !_WaitForDebuggerStart() ) {
      SignalStop();
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_ERROR));
      CString sMsg(MAKEINTRESOURCE(IDS_ERR_DEBUGGERSTART));
      _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_ICONEXCLAMATION | MB_MODELESS);
      Stop();
      return false;
   }

   sStatus.LoadString(IDS_STATUS_DEBUGGING);
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus, TRUE);

   // Collect new breakpoint positions directly from open views.
   LAZYDATA data;
   m_pProject->SendViewMessage(DEBUG_CMD_REQUEST_BREAKPOINTS, &data);

   // Prevent "Press <return> to continue" interruption
   DoDebugCommand(_T("-gdb-set confirm off"));  
   DoDebugCommand(_T("-gdb-set width 0"));  
   DoDebugCommand(_T("-gdb-set height 0"));  

   // Initialize some GDB settings depending on connection type
   CString sShellType = m_ShellManager.GetParam(_T("Type"));
   if( sShellType == _T("comspec") ) {
      DoDebugCommand(_T("-gdb-set debugevents on"));  
      m_bDebugEvents = true;
   }

   // This is one of the first commands we send; views can interpret the reply (cwd) as 
   // a "startup" command.
   DoDebugCommand(_T("-environment-pwd"));

   // If there are no breakpoints defined, then let's
   // set a breakpoint in the main function
   if( !bExternalProcess && m_aBreakpoints.GetSize() == 0 ) {
      DoDebugCommandV(_T("-break-insert -t %s"), m_sDebugMain);
   }

   // Set known breakpoints.
   // This includes newly updated breakpoints from active views and
   // old breakpoints from the list.
   for( int b = 0; b < m_aBreakpoints.GetSize(); b++ ) {
      DoDebugCommandV(_T("-break-insert %s"), m_aBreakpoints.GetKeyAt(b));
   }

   // Set the GDB search paths. The user can here add additional search paths
   // once GDB is started.
   if( !m_sSearchPath.IsEmpty() ) {
      CString sPaths;
      CString s = m_sSearchPath;
      CString sToken = s.SpanExcluding(_T(";,"));
      while( !sToken.IsEmpty() ) {
         sPaths += _T(" \"");
         sPaths += sToken;
         sPaths += _T("\"");
         s = s.Mid(sToken.GetLength() + 1);
         sToken = s.SpanExcluding(_T(";,"));
      }
      DoDebugCommandV(_T("-environment-directory %s"), sPaths);
      DoDebugCommandV(_T("-environment-path %s"), sPaths);
   }

   return true;
}

/**
 * Idle wait until debugger is seen starting.
 * Waits for a debugger process to launch and acknowledge its existance
 * by sending its first prompt.
 */
bool CDebugManager::_WaitForDebuggerStart()
{
   // Wait for for the first debugger acknowledgement (GDB prompt)
   DWORD dwStartTick = ::GetTickCount();
   while( ::GetTickCount() - dwStartTick < m_lStartTimeout * 1000UL ) {
      PumpIdleMessages(200L);
      if( m_eventAck.WaitForEvent(0) ) return true;
   }
   return false;
}

CString CDebugManager::_TranslateCommand(LPCTSTR pstrCommand, LPCTSTR pstrParam /*= NULL*/)
{
   ATLASSERT(!::IsBadStringPtr(pstrCommand,-1));

   // Here is an important transformation. For ordinary GDB commands we format them
   // as "232-command" (prefixing with 232) so we can easily recognize the response.
   // We currently don't use an incrementing number-prefix to recognize individual command 
   // sequences, but that may change in the future. So far we just hardcode the (randomly 
   // picked) number: 232.
   CString sCommand = pstrCommand;
   if( pstrCommand[0] == '-' && m_pAdaptor != NULL ) {
      sCommand = m_pAdaptor->TransformInput(pstrCommand);
   }

   TCHAR szProjectName[128] = { 0 };
   m_pProject->GetName(szProjectName, 127);

   // Translate meta-tokens in command
   sCommand.Replace(_T("$PROJECTNAME$"), szProjectName);
   sCommand.Replace(_T("$PATH$"), m_ShellManager.GetParam(_T("Path")));
   sCommand.Replace(_T("$DRIVE$"), m_ShellManager.GetParam(_T("Path")).Left(2));
   sCommand.Replace(_T("\\n"), _T("\r\n"));
   if( pstrParam != NULL ) {
      TCHAR szName[MAX_PATH + 1] = { 0 };
      _tcscpy(szName, pstrParam);
      sCommand.Replace(_T("$FILEPATH$"), szName);
      ::PathStripPath(szName);
      sCommand.Replace(_T("$FILENAME$"), szName);
      ::PathRemoveExtension(szName);
      sCommand.Replace(_T("$NAME$"), szName);
   }

   // Do some manipulation depending on the debug command.
   // Some commands might spuriously fail and we don't want to see error
   // messages all the time, so we'll bluntly ignore these.
   if( sCommand.Find(_T("-delete")) >= 0 ) m_nIgnoreErrors++;
   if( sCommand.Find(_T("-evaluate")) >= 0 ) m_nIgnoreErrors++;
   // For some commands we really need to track the information that
   // was asked. We book-keep this in an internal state variable. This
   // is a problem because we can now only have one command lingering.
   int iPos;
   if( (iPos = sCommand.Find(_T("-var-info-expression"))) >= 0 ) m_sVarName = sCommand.Mid(iPos + 21);
   if( (iPos = sCommand.Find(_T("-var-evaluate-expression"))) >= 0 ) m_sVarName = sCommand.Mid(iPos + 25);
   if( (iPos = sCommand.Find(_T("-data-evaluate-expression"))) >= 0 ) m_sVarName = sCommand.Mid(iPos + 26);
   return sCommand;
}

/**
 * Clean up the debugger state.
 * Notifies the UI that debugging stopped. Clears links and state to signal
 * that we are no longer debugging the app.
 */
void CDebugManager::_ClearLink()
{
   // Send event that we have stopped
   if( m_bDebugging ) {
      m_pProject->DelayedDebugEvent(LAZY_DEBUG_KILL_EVENT);
      m_pProject->DelayedGlobalViewMessage(DEBUG_CMD_SET_CURLINE);
      m_pProject->DelayedStatusBar(CString(MAKEINTRESOURCE(IDS_STATUS_DEBUG_STOPPED)));
   }
   // Detach adaptor
   if( m_pAdaptor != NULL ) delete m_pAdaptor;
   m_pAdaptor = NULL;
   // Detach listener
   m_ShellManager.RemoveLineListener(this);
   // Clear state
   m_bCommandMode = false;
   m_bDebugging = false;
   m_bRunning = false;
   m_bBreaked = false;
   m_nIgnoreErrors = 99;  // No GDB errors/popups during exit!
   m_nIgnoreBreaks = 99;  // No GDB updates during exit!
}

/**
 * Check debug status before start.
 * We can only start debugging if we're not already debugging another app/module.
 * As a courtesy we also ask to save files before starting a debug session.
 */
bool CDebugManager::_CheckStatus()
{
   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);

   // Cannot do neither run nor debug when a session is
   // already running...
   if( IsBusy() ) {
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_WARNING));
      CString sMsg(MAKEINTRESOURCE(IDS_COMPILEBUSY));
      m_pProject->DelayedMessage(sMsg, sCaption, MB_ICONEXCLAMATION | MB_MODELESS);
      return false;
   }
 
   // If the project needs save/compile then ask if we
   // should do so now...
   if( m_pProject->IsDirty() || m_pProject->IsRecompileNeeded() ) {
      CString sMsg(MAKEINTRESOURCE(IDS_NEEDS_RECOMPILE));
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_QUESTION));
      if( IDYES == _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_ICONQUESTION | MB_YESNO) ) {
         wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE_ALL, 0));
         wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_BUILD_PROJECT, 0));
         return false;
      }
   }

   return true;
}

/**
 * Parse stack frame information from debugger.
 * Spread the word about debugger going to to "halted" state.
 */
void CDebugManager::_ParseNewFrame(CMiInfo& info)
{
   CString sFunction = info.GetItem(_T("func"));
   if( sFunction.IsEmpty() ) sFunction = info.GetItem(_T("at"));
   CString sFilename = info.GetItem(_T("file"), _T("frame"));
   if( !sFilename.IsEmpty() ) 
   {
      // Debugger stopped in source file.
      // We'll attempt to bring the source file into view and
      // place the "current line" marker at the breaked position.
      bool bKnownFile = m_pProject->FindView(sFilename, true) != NULL;
      int iLineNum = _ttoi(info.GetItem(_T("line"), _T("frame")));
      m_pProject->DelayedOpenView(sFilename, iLineNum);
      m_pProject->DelayedGlobalViewMessage(DEBUG_CMD_SET_CURLINE, sFilename, iLineNum);
      CString sText;
      if( bKnownFile ) sText.LoadString(IDS_STATUS_DEBUG_BREAKPOINT);
      else if( sFunction.IsEmpty() ) sText.LoadString(IDS_STATUS_DEBUG_NOFILE);
      else sText.Format(IDS_STATUS_DEBUG_FUNCTION, sFunction);
      m_pProject->DelayedStatusBar(sText);
   }
   else
   {
      // Debugger stopped at a location without debug information.
      m_pProject->DelayedGlobalViewMessage(DEBUG_CMD_SET_CURLINE);
      CString sText(MAKEINTRESOURCE(IDS_STATUS_DEBUG_NOSOURCE));
      if( !sFunction.IsEmpty() ) sText.Format(IDS_STATUS_DEBUG_NOSOURCE_FUNCTION, sFunction);
      m_pProject->DelayedStatusBar(sText);
   }
   // Cause the debug views to refresh
   m_pProject->DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   // Make sure the new state is known to our engine
   m_pProject->DelayedDebugInfo(_T("stopped"), info);
}

/**
 * Update internal breakpoint structures.
 * This method gets called when the GDB debugger delivers an updated
 * debug list and tells us which internal breakpoint-nr it assigned to 
 * our new breakpoint.
 */
void CDebugManager::_UpdateBreakpoint(CMiInfo& info)
{   
   CString sFile = info.GetItem(_T("file"));
   CString sLine = info.GetItem(_T("line"));
   if( sFile.IsEmpty() || sLine.IsEmpty() ) return;
   long lNumber = _ttol(info.GetItem(_T("number")));
   CString sLocation;
   sLocation.Format(_T("%s:%s"), ::PathFindFileName(sFile), sLine);
   if( !m_aBreakpoints.SetAt(sLocation, lNumber) ) {
      m_aBreakpoints.Add(sLocation, lNumber);
   }
}

void CDebugManager::_ParseOutOfBand(LPCTSTR pstrText)
{
   CString sLine = pstrText;
   if( sLine.IsEmpty() ) return;
   CString sCommand = sLine.SpanExcluding(_T(","));
   sLine = sLine.Mid(sCommand.GetLength());
   CString sToken;
   CString sValue;
   if( sCommand == _T("stopped") ) {
      // Debugger has stopped and is waiting for input.
      // We mark the session as "breaked". We then continue to
      // show possible errors/warnings and update the debug views.
      // NOTE: For temporary breaks (ie. where we stop the running debugger 
      //       to insert a breakpoint) we make sure not to try to update
      //       the user-interface, since this is an internal event.
      m_bBreaked = true;
      if( m_nIgnoreBreaks > 0 ) return;
      // Parse command...
      CMiInfo info = sLine;
      sValue = info.GetItem(_T("reason"));
      // Handles reason:
      //   'exited'
      //   'exited-normally'
      //   'exited-signalled'
      //   'exited-with-errorcode'
      if( sValue.Find(_T("exited")) == 0 ) {
         ProgramStop();
         return;
      }
      // Handles reason:
      //   'signal'
      //   'signal-received'
      if( sValue.Find(_T("signal")) == 0 && m_nIgnoreErrors == 0 ) {
         CString sMessage;
         sMessage.Format(IDS_SIGNAL, 
            info.GetItem(_T("signal-name")), 
            info.GetItem(_T("signal-meaning")),
            info.GetItem(_T("addr")),
            info.GetItem(_T("func")));
         m_pProject->DelayedMessage(sMessage, CString(MAKEINTRESOURCE(IDS_STOPPED)), MB_ICONINFORMATION);
      }
      // Handles reason:
      //    'breakpoint-hit'
      if( sValue.Find(_T("breakpoint")) == 0 ) {
         // Play the breakpoint sound
         ::PlaySound(_T("BVRDE_BreakpointHit"), NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT);
      }
      // Assume current position changed, so we'll need to notify views about this.
      // This call will inform all the views of the new active source-code position
      // and possible allow them to refresh their display.
      _ParseNewFrame(info);
      return;
   }
}

void CDebugManager::_ParseResultRecord(LPCTSTR pstrText)
{
   CString sLine = pstrText;
   if( sLine.IsEmpty() ) return;
   CString sCommand = sLine.SpanExcluding(_T(","));
   sLine = sLine.Mid(sCommand.GetLength());
   // App is running again...
   if( sCommand == _T("running") ) {
      m_bBreaked = false;
      m_pProject->DelayedGlobalViewMessage(DEBUG_CMD_SET_RUNNING);
      m_pProject->DelayedStatusBar(CString(MAKEINTRESOURCE(IDS_STATUS_DEBUGGING)));
      return;
   }
   // Debugger stopped for good
   if( sCommand == _T("exit") ) {
      m_pProject->DelayedStatusBar(_T(""));
      // We should flag that we did a clean exit!
      _ClearLink();
      m_bSeenExit = true;
      return;
   }
   // Debugger returned an error message
   if( sCommand == _T("error") ) {
      // Ignore errors might have been requested, so fulfill thy wish...
      if( m_nIgnoreErrors > 0 ) return;
      // Extract error description and display it formatted nicely
      CMiInfo info = sLine;
      CString sMessage;
      sMessage.Format(IDS_ERR_DEBUG, info.GetItem(_T("msg")));
      int iPos = sMessage.Find(_T(". "));
      if( iPos > 0 ) sMessage.Format(_T("%s\n%s"), sMessage.Left(iPos + 1), sMessage.Mid(iPos + 2));
      m_pProject->DelayedMessage(sMessage, CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
      return;
   }
   // Debugger executed command
   if( sCommand == _T("done") ) 
   {
      sLine.TrimLeft(_T(","));
      // This is the list of GDB MI results that we wish to have a closer look
      // at. Each view will get a chance to interpret the data.
      static LPCTSTR pstrList[] = 
      {
         _T("BreakpointTable"),
         _T("thread-ids"),
         _T("new-thread-id"),
         _T("stack"),
         _T("locals"),
         _T("stack-args"),
         _T("register-names"),
         _T("register-values"),
         _T("name"),
         _T("value"),
         _T("addr"),
         _T("lang"),
         _T("cwd"),
         _T("changelist"),
         _T("ndeleted"),
         _T("asm_insns"),
         _T("numchild"),
         NULL
      };
      CString sCommand = sLine.SpanExcluding(_T("="));
      if( sCommand == _T("value") || sCommand == _T("lang") ) {
         // Unfortunately the GDB MI "value" result doesn't always
         // contain information about what was asked, so we
         // internally book-keep this information and append
         // it to the output (see _TranslateCommand()).
         // The consequence is that we can only have one GDB command
         // lingering at any time.
         // TODO: Make use of the GDB command-prefix number instead
         //       of just hard-coding it to 232 as we do today.
         CString sVarName= m_sVarName;
         sVarName.TrimLeft(_T("\" "));
         sVarName.TrimRight(_T("\"\t\r\n "));
         CString sTemp;
         sTemp.Format(_T(",name=\"%s\""), sVarName);
         sLine += sTemp;
      }
      // Check for internal commands or route it to client processing
      CMiInfo info = sLine;
      if( sCommand == _T("frame") ) _ParseNewFrame(info);
      if( sCommand == _T("bkpt") ) _UpdateBreakpoint(info);
      // Let all the debug views get a shot at this information.
      for( LPCTSTR* ppList = pstrList; *ppList; ppList++ ) {
         if( sCommand == *ppList ) {
            m_pProject->DelayedDebugInfo(sCommand, info);
            return;
         }
      }
   }
}

void CDebugManager::_ParseConsoleOutput(LPCTSTR pstrText)
{
   CString sLine = pstrText;
   if( sLine.IsEmpty() ) return;

   // Not seen first GDB prompt yet? GDB might be spitting out error messages
   // in raw format then.
   if( m_nDebugAck == 0 ) {
      // Verify correct version of debugger...
      int iPos = sLine.Find(_T("GNU gdb "));
      if( iPos == 0 || iPos == 1 ) {
         LPTSTR pEnd = NULL;
         m_dblDebuggerVersion = _tcstod(sLine.Mid(iPos + 8), &pEnd);
         if( m_dblDebuggerVersion >= 4.0 && m_dblDebuggerVersion < 5.3 ) {
            m_pProject->DelayedMessage(CString(MAKEINTRESOURCE(IDS_ERR_DEBUGVERSION)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
         }
      }
      // Some outdated versions of GDB didn't include these texts in the console-stream
      // tags, but merely printed them as text. We'll try to detect both (hence the skipping
      // of the first character in error messages).
      typedef struct tagDEBUGERR {
         LPCTSTR pstrText; UINT nCaption; UINT nMsg;
      } DEBUGERR;
      static DEBUGERR errs[] = 
      {
         { _T("No debugging symbols found"),  IDS_CAPTION_MESSAGE, IDS_ERR_NODEBUGINFO },
         { _T("Unable to attach to process"), IDS_CAPTION_MESSAGE, IDS_ERR_NOATTACH },
         { _T("No such process"),             IDS_CAPTION_MESSAGE, IDS_ERR_NOATTACH },
         { _T("No symbol table is loaded"),   IDS_CAPTION_MESSAGE, IDS_ERR_NODEBUGINFO },
         { _T("No such file"),                IDS_CAPTION_ERROR,   IDS_ERR_NODEBUGFILE },
         { _T("gdb: unrecognized option"),    IDS_CAPTION_ERROR,   IDS_ERR_DEBUGVERSION },
      };
      for( int i = 0; i < sizeof(errs) / sizeof(errs[0]); i++ ) {
         if( sLine.Find(errs[i].pstrText + 1) >= 0 ) {
            m_pProject->DelayedMessage(CString(MAKEINTRESOURCE(errs[i].nMsg)), CString(MAKEINTRESOURCE(errs[i].nCaption)), MB_ICONINFORMATION);
            break;
         }
      }
   }
   // GDB is known to be unstable at times...
   if( sLine.Find(_T("An internal GDB error was detected.")) >= 0 ) {
      m_pProject->DelayedMessage(CString(MAKEINTRESOURCE(IDS_ERR_DEBUGUNSTABLE)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
      ProgramStop();
   }
   // For some targets we really wish to know the PID of the running program.
   // Settings debugevent reporting on seems to be the only way to trigger this
   // early in GDB.
   if( m_bDebugEvents && sLine.Find(_T("do_initial_child_stuff")) >= 0 ) {
      m_ShellManager.SetParam(_T("ProcessID"), sLine.Mid(sLine.Find(_T("process")) + 7));
      DoDebugCommand(_T("-gdb-set debugevents off"));
      m_bDebugEvents = false;
   }
   if( m_bDebugEvents && sLine.Find(_T("kernel event for pid=")) >= 0 ) {
      m_ShellManager.SetParam(_T("ProcessID"), sLine.Mid(sLine.Find(_T("pid=")) + 4));
      DoDebugCommand(_T("-gdb-set debugevents off"));
      m_bDebugEvents = false;
   }
   // Outputting directly to the Command View if we're
   // in "command mode" (user entered custom command at prompt or scripting).
   if( m_bCommandMode ) {
      CString sText = sLine.Mid(1, sLine.GetLength() - 2);
      sText.Replace(_T("\\n"), _T("\r\n"));
      sText.Replace(_T("\\t"), _T("\t"));
      m_pProject->DelayedGuiAction(GUI_ACTION_APPENDVIEW, IDE_HWND_COMMANDVIEW, sText);
   }
}

void CDebugManager::_ParseTargetOutput(LPCTSTR pstrText)
{
   CString sLine = pstrText;
   if( sLine.IsEmpty() ) return;
   // Outputting directly to the Command View if we're
   // in "command mode" (user entered custom command at prompt or scripting).
   if( m_bCommandMode ) {
      CString sText = sLine.Mid(1, sLine.GetLength() - 2);
      sText.Replace(_T("\\n"), _T("\r\n"));
      sText.Replace(_T("\\t"), _T("\t"));
      m_pProject->DelayedGuiAction(GUI_ACTION_APPENDVIEW, IDE_HWND_COMMANDVIEW, sText);
   }
}

void CDebugManager::_ParseLogOutput(LPCTSTR pstrText)
{
   CString sLine = pstrText;
   if( sLine.IsEmpty() ) return;
   // GDB didn't find the executable?
   // Project name is not the executable name
   if( sLine.Find(_T("No executable file specified")) >= 0 
       || sLine.Find(_T("No executable specified, use")) >= 0 ) 
   {
      m_pProject->DelayedMessage(CString(MAKEINTRESOURCE(IDS_ERR_NOEXECUTABLE)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
      ProgramStop();
   }
}

void CDebugManager::_ParseKeyPrompt(LPCTSTR pstrText)
{
   CString sLine = pstrText;
   if( sLine.IsEmpty() ) return;
   // FIX: How "Machine Interface" is it really, when GDB from time to time
   //      prompts me to hit return to continue?!!
   if( sLine.Find(_T("---Type <return>")) >= 0 ) {
      DoDebugCommand(_T("\r\n"));
   }
   if( sLine.Find(_T("---Select one")) >= 0 ) {
      DoDebugCommand(_T("0\r\n"));
   }
}

/**
 * Halts the debugger so we can send more GDB commands.
 * GDB is not async and doesn't allow debug commands to be sent while it is actively
 * debugging the app.
 * We need to suspend it temporarily and send our commands, then we can resume debugging.
 * @return true If we halted the debugger and should restart it later.
 */
bool CDebugManager::_PauseDebugger()
{
   // Do we need to stop the debugger?
   if( IsBreaked() ) return false;
   // Yes, let's give it a break. Make sure to mark the
   // event so we don't update views as well. This is a temporary
   // state as we intent to immediately resume the session once
   // we have sent our commands.
   m_nIgnoreBreaks++;
   Break();
   // Wait for break sequence to complete
   const int TIMEOUT = 10;  // in 100ms
   for( int i = 0; !IsBreaked() && i < TIMEOUT; i++ ) ::Sleep(100L);
   // Did we succeed?
   return IsBreaked();
}

/**
 * Resumes debugging.
 * @see _PauseDebugger
 */
void CDebugManager::_ResumeDebugger()
{
   RunContinue();
}


/////////////////////////////////////////////////////////////////////////
// IOutputLineListener

void CDebugManager::OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText)
{
   ATLASSERT(!::IsBadStringPtr(pstrText,-1));
   ATLASSERT(m_pAdaptor!=NULL);
   // Only if debugging.
   if( !m_bDebugging ) return;
   // Transform to MI format
   if( m_pAdaptor == NULL ) return;
   CSimpleArray<CString> aOutput;
   m_pAdaptor->TransformOutput(pstrText, aOutput);
   for( int i = 0; i < aOutput.GetSize(); i++ ) {
      pstrText = aOutput[i];
      // Count number of debug prompts (=acknoledgements)
      if( _tcsncmp(pstrText, _T("(gdb"), 4) == 0 ) {
         m_bCommandMode = false;
         if( m_nIgnoreErrors > 0 ) m_nIgnoreErrors--;
         if( m_nIgnoreBreaks > 0 ) m_nIgnoreBreaks--;
         m_nDebugAck++;
         m_eventAck.SetEvent();
         continue;
      }
      // Look at raw text always
      if( *pstrText == '~' || m_nDebugAck == 0 ) _ParseConsoleOutput(pstrText + 1);
      if( *pstrText == '-' ) _ParseKeyPrompt(pstrText);
      // Not accepting lines before first acknoledge
      if( m_nDebugAck == 0 ) continue;
      // Parse output stream
      if( *pstrText == '@' ) _ParseTargetOutput(pstrText + 1);
      if( *pstrText == '&' ) _ParseLogOutput(pstrText + 1);
      // Unfortunately Out-of-band output may occur in the middle
      // of the stream! This is an annoying GDB feature, but we'll try
      // to handle the situation by adding a command-handshake (see 
      // DoDebugCommand()) and by looking for substrings.
      LPCTSTR pstr = _tcsstr(pstrText, _T("232^"));
      if( pstr != NULL ) _ParseResultRecord(pstr + 4);
      pstr = _tcsstr(pstrText, _T("232*"));
      if( pstr != NULL ) _ParseOutOfBand(pstr + 4);
      // ...and happily ignore everything else!
   }
}

