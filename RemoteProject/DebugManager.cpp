
#include "StdAfx.h"
#include "resource.h"

#include "DebugManager.h"

#include "Project.h"
#include "MiInfo.h"

#pragma code_seg( "MISC" )


////////////////////////////////////////////////////////
//

CDebugManager::CDebugManager() :
   m_pProject(NULL),
   m_bBreaked(false),
   m_bDebugging(false),
   m_bCommandMode(false),
   m_nIgnoreErrors(0)
{
   Clear();
}

CDebugManager::~CDebugManager()
{
   Stop();
}

void CDebugManager::Init(CRemoteProject* pProject)
{
   ATLASSERT(pProject);
   m_pProject = pProject;
   m_ShellManager.Init(pProject);
   m_aBreakpoints.RemoveAll();
}

void CDebugManager::Clear()
{
   m_ShellManager.Clear();

   m_sCommandCD = _T("cd $PATH$");
   m_sAppExecutable = _T("./$PROJECTNAME$");
   m_sAppArgs = _T("");
   m_sDebuggerExecutable = _T("gdb");
   m_sDebuggerArgs = _T("-i=mi ./$PROJECTNAME$");
   m_sDebugMain = _T("main");
   m_lStartTimeout = 4;
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

   if( !pArc->ReadItem(_T("GDB")) ) return false;
   pArc->Read(_T("app"), m_sDebuggerExecutable.GetBufferSetLength(200), 200);
   m_sDebuggerExecutable.ReleaseBuffer();
   pArc->Read(_T("args"), m_sDebuggerArgs.GetBufferSetLength(300), 300);
   m_sDebuggerArgs.ReleaseBuffer();
   pArc->Read(_T("main"), m_sDebugMain.GetBufferSetLength(64), 64);
   m_sDebugMain.ReleaseBuffer();
   pArc->Read(_T("startTimeout"), m_lStartTimeout);

   if( m_lStartTimeout <= 0 ) m_lStartTimeout = 4;

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

   if( !pArc->WriteItem(_T("GDB")) ) return false;
   pArc->Write(_T("app"), m_sDebuggerExecutable);
   pArc->Write(_T("args"), m_sDebuggerArgs);
   pArc->Write(_T("main"), m_sDebugMain);
   pArc->Write(_T("startTimeout"), m_lStartTimeout);

   if( !pArc->WriteGroupEnd() ) return false;
   return true;
}

void CDebugManager::SignalStop()
{
   // Ok, this is a hopeless semaphore, but we really
   // don't like to be entering this as reentrant.
   static bool s_bStopping = false;
   if( s_bStopping ) return;
   s_bStopping = true;

   DoDebugCommand(_T("-gdb-exit"));
   DoDebugCommand(_T("exit"));
   m_ShellManager.SignalStop();

   m_pProject->DelayedDebugEvent(LAZY_DEBUG_KILL_EVENT);
   m_pProject->DelayedStatusBar(CString(MAKEINTRESOURCE(IDS_STATUS_DEBUG_STOPPED)));
   m_pProject->DelayedViewMessage(DEBUG_CMD_SET_CURLINE);

   m_bDebugging = false;
   m_bBreaked = false;

   s_bStopping = false;
}

bool CDebugManager::Stop()
{
   // NOTE: Don't call SignalStop() here because we've probably
   //       already sent the DelayedXXX messages once...

   m_ShellManager.RemoveLineListener(this);
   m_ShellManager.Stop();

   m_bCommandMode = false;
   m_bDebugging = false;
   m_bBreaked = false;
   m_nIgnoreErrors = 0;

   return true;
}

bool CDebugManager::IsBusy() const
{
   return m_ShellManager.IsBusy();
}

bool CDebugManager::IsBreaked() const
{
   return IsDebugging() && m_bBreaked;
}

bool CDebugManager::IsDebugging() const
{
   return IsBusy() && m_bDebugging;
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
   CLockStaticDataInit lock;
   if( IsDebugging() ) {
      if( m_aBreakpoints.GetSize() > 0 ) {
         bool bRestartApp = _PauseDebugger();
         // We're going to delete the breakpoints in GDB itself first
         CString sCommand = _T("-break-delete");
         for( long i = 0; i < m_aBreakpoints.GetSize(); i++ ) {
            sCommand += _T(" ");
            sCommand.Append(m_aBreakpoints.GetValueAt(i));
         }
         DoDebugCommand(sCommand);
         if( bRestartApp ) _ResumeDebugger();
      }
   }
   // We can now safely remove our internal breakpoint list
   // and signal to the view to do the same...
   m_aBreakpoints.RemoveAll();
   m_pProject->DelayedViewMessage(DEBUG_CMD_CLEAR_BREAKPOINTS);
   return true;
}

bool CDebugManager::AddBreakpoint(LPCTSTR pstrText)
{
   // Add it to internal list
   CLockStaticDataInit lock;
   CString sText = pstrText;
   if( sText.IsEmpty() ) return false;
   // We add a dummy entry now. We don't know the internal GDB breakpoint-nr
   // for this breakpoint, but we'll soon learn when GDB answers our request.
   m_aBreakpoints.Add(sText, 0);
   // If we're debugging, we need to update GDB as well...
   if( IsDebugging() ) 
   {     
      // Attempt to halt app if currently running
      bool bRestartApp = _PauseDebugger();
      // Send GDB commands
      CString sCommand;
      sCommand.Format(_T("-break-insert %s"), pstrText);
      if( !DoDebugCommand(sCommand) ) return false;
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

bool CDebugManager::RemoveBreakpoint(LPCTSTR pstrText)
{
   CLockStaticDataInit lock;
   if( IsDebugging() ) {
      // Find the internal GDB breakpoint-nr for this breakpoint
      CString sText = pstrText;
      long lNumber = m_aBreakpoints.Lookup(sText);
      if( lNumber > 0 ) {
         bool bRestartApp = _PauseDebugger();
         // We have it; let's ask GDB to delete the breakpoint
         CString sCommand;
         sCommand.Format(_T("-break-delete %ld"), lNumber);
         if( !DoDebugCommand(sCommand) ) return false;
         if( !DoDebugCommand(_T("-break-list")) ) return false;
         if( bRestartApp ) _ResumeDebugger();
      }
   }
   // Remove it from our own internal list too 
   CString sText = pstrText;
   return m_aBreakpoints.Remove(sText) == TRUE;
}

bool CDebugManager::GetBreakpoints(LPCTSTR pstrFilename, CSimpleArray<long>& aLines) const
{
   CLockStaticDataInit lock;
   size_t cchName = _tcslen(pstrFilename);
   for( int i = 0; i < m_aBreakpoints.GetSize(); i++ ) {
      if( _tcsncmp(m_aBreakpoints.GetKeyAt(i), pstrFilename, cchName) == 0 ) {
         LPCTSTR pstr = _tcschr(m_aBreakpoints.GetKeyAt(i), ':');
         if( pstr ) {
            long lLineNo = _ttol(pstr + 1);
            aLines.Add(lLineNo);
         }
      }
   }
   return true;
}

bool CDebugManager::SetBreakpoints(LPCTSTR pstrFilename, CSimpleArray<CString>& aBreakpoints)
{
   ATLASSERT(IsDebugging());
   // This method is called from the views upon a DEBUG_CMD_REQUEST_BREAKPOINTS request
   // where we ask the views to identify all active breakpoints. We're supposed to
   // clear our internal breakpoint-list for that file and add the active items.
   // First, let's remove all existing breakpoints from this file.
   CLockStaticDataInit lock;
   size_t cchName = _tcslen(pstrFilename);
   for( int i = 0; i < m_aBreakpoints.GetSize(); i++ ) {
      if( _tcsncmp(m_aBreakpoints.GetKeyAt(i), pstrFilename, cchName) == 0 ) {
         m_aBreakpoints.Remove(m_aBreakpoints.GetKeyAt(i));
         i = -1;
         continue;
      }
   }
   // Add all the new breakpoints
   for( i = 0; i < aBreakpoints.GetSize(); i++ ) m_aBreakpoints.Add(aBreakpoints[i], 0);
   return true;
}

bool CDebugManager::RunTo(LPCTSTR pstrFilename, long lLineNum)
{
   ATLASSERT(IsDebugging());
   CString sCommand;
   sCommand.Format(_T("-exec-until %s:%ld"), pstrFilename, lLineNum);
   return DoDebugCommand(sCommand);
}

bool CDebugManager::SetNextStatement(LPCTSTR pstrFilename, long lLineNum)
{
   ATLASSERT(IsDebugging());
   CString sCommand;
   sCommand.Format(_T("-break-insert -t %s:%ld"), pstrFilename, lLineNum);
   DoDebugCommand(sCommand);
   // BUG: Argh, GDB MI doesn't come with a proper "SetNextStatement"
   //      command. The best we can do is to try to jump in the local file!
   sCommand.Format(_T("-interpreter-exec console \"jump %ld\""), lLineNum);
   DoDebugCommand(sCommand);
   return true;
}

bool CDebugManager::RunNormal()
{  
   // Launch the remote process and supervise it with
   // a telnet prompt and display it's output.

   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);

   // We may need to kill the old session
   if( !_CheckStatus() ) return false;

   CWaitCursor cursor;

   Stop();

   CTelnetView* pView = m_pProject->GetDebugView();

   CString sStatus;
   sStatus.Format(IDS_STATUS_CONNECT_WAIT);
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus, FALSE);
   _pDevEnv->PlayAnimation(TRUE, ANIM_TRANSFER);

   pView->Clear();
   pView->Init(&m_ShellManager, TNV_TERMINATEONCLOSE);

   RECT rcWin = { 120, 120, 800 + 120, 600 + 120 };   // TODO: Memorize this position
   _pDevEnv->AddDockView(pView->m_hWnd, IDE_DOCK_HIDE, rcWin);
   _pDevEnv->AddDockView(pView->m_hWnd, IDE_DOCK_FLOAT, rcWin);
   pView->CenterWindow();
   pView->UpdateWindow();

   m_ShellManager.Start();
   if( !m_ShellManager.WaitForConnection() ) {      
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_ERROR));
      CString sMsg(MAKEINTRESOURCE(IDS_ERR_HOSTCONNECT));
      _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_ICONEXCLAMATION | MB_MODELESS);
      _pDevEnv->PlayAnimation(FALSE, 0);
      Stop();
      return false;
   }

   sStatus.Format(IDS_STATUS_RUNNING);
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus, TRUE);
   _pDevEnv->PlayAnimation(FALSE, 0);

   CString sCommand = _TranslateCommand(m_sCommandCD);
   if( !m_ShellManager.WriteData(sCommand) ) {
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

bool CDebugManager::RunDebug()
{
   // Launch the remote process in debug mode and switch
   // editor state to debugging.

   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);

   if( !_CheckStatus() ) return false;

   CWaitCursor cursor;

   Stop();

   CTelnetView* pView = m_pProject->GetDebugView();
   
   CString sStatus;
   sStatus.LoadString(IDS_STATUS_CONNECT_WAIT);
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus, FALSE);
   _pDevEnv->PlayAnimation(TRUE, ANIM_TRANSFER);

   pView->Clear();
   pView->Init(&m_ShellManager, 0);

   // Display the output window
   RECT rcWin = { 120, 120, 800 + 120, 600 + 120 };   
   _pDevEnv->AddDockView(pView->m_hWnd, IDE_DOCK_HIDE, rcWin);

   // Prepare session
   m_ShellManager.AddLineListener(this);
   m_nIgnoreErrors = 0;
   m_bDebugging = true;
   m_bBreaked = false;
   m_nDebugAck = 0;
   m_nLastAck = 0;

   // Wait for the remote connect to happen
   m_ShellManager.Start();
   if( !m_ShellManager.WaitForConnection() ) {      
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_ERROR));
      CString sMsg(MAKEINTRESOURCE(IDS_ERR_HOSTCONNECT));
      _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_ICONEXCLAMATION | MB_MODELESS);
      _pDevEnv->PlayAnimation(FALSE, 0);
      Stop();
      return false;
   }

   sStatus.LoadString(IDS_STATUS_DEBUGGING);
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus, FALSE);
   _pDevEnv->PlayAnimation(FALSE, 0);

   m_pProject->DelayedDebugEvent(LAZY_DEBUG_START_EVENT);

   // Run debugger on remote server
   CString sCommand;
   sCommand = _TranslateCommand(m_sCommandCD);
   if( !m_ShellManager.WriteData(sCommand) ) {
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_ERROR));
      CString sMsg(MAKEINTRESOURCE(IDS_ERR_DATAWRITE));
      _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_ICONEXCLAMATION | MB_MODELESS);
      Stop();
      return false;
   }
   sCommand.Format(_T("%s %s"), m_sDebuggerExecutable, m_sDebuggerArgs);
   m_ShellManager.WriteData(_TranslateCommand(sCommand)); 

   // Wait for the debug prompt
   if( !_WaitForDebuggerStart() ) {
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

   // If there are no breakpoints defined, then let's
   // set a breakpoint in the main function
   if( m_aBreakpoints.GetSize() == 0 ) {
      CString sCommand;
      sCommand.Format(_T("-break-insert -t %s"), m_sDebugMain);
      DoDebugCommand(sCommand);
   }

   // Set known breakpoints.
   // This includes newly updated breakpoints from active views and
   // old breakpoints from the list.
   for( int i = 0; i < m_aBreakpoints.GetSize(); i++ ) {
      CString sCommand;
      sCommand.Format(_T("-break-insert %s"), m_aBreakpoints.GetKeyAt(i));
      DoDebugCommand(sCommand);
   }

   // Start debugging session
   DoDebugCommand(_T("-exec-run"));

   return true;
}

bool CDebugManager::RunContinue()
{
   ATLASSERT(IsDebugging());
   // If we're already debugging (but have stopped in a breakpoint)
   // we might just need to continue debugging...
   return DoDebugCommand(_T("-exec-continue"));
}

CString CDebugManager::GetTagInfo(LPCTSTR pstrValue)
{
   ATLASSERT(!::IsBadStringPtr(pstrValue,-1));
   // We must be debugging to talk to the debugger
   if( !IsDebugging() ) return _T("");
   // Send query to GDB debugger about the data value.
   CString sValue = pstrValue;
   sValue.Replace(_T("\\"), _T("\\\\"));
   sValue.Replace(_T("\""), _T("\\\""));
   // NOTE: Somewhere down in the _TranslateCommand() method
   //       we'll make sure to ignore any errors returned.
   CString sCommand;
   sCommand.Format(_T("-data-evaluate-expression \"%s\""), sValue);
   DoDebugCommand(sCommand);
   return _T("");  // We don't have an answer right now!
}

bool CDebugManager::DoDebugCommand(LPCTSTR pstrText)
{
   ATLASSERT(!::IsBadStringPtr(pstrText,-1));
   // We must be debugging to execute anything
   if( !IsDebugging() ) return false;
   // HACK: We're having trouble with echoing
   //       out own strings, so we'll check for a brief
   //       moment if a new GDB prompt arrives.
   //       However, we quickly give up and rely on
   //       GDB being asyncroniously. This does have an
   //       inpact on UI responsiveness though.
   //       The problem is, however, that there's no 
   //       flow-control in the GDB-MI std-out handler, 
   //       so we'll risk to send the command in the middle 
   //       of regular output. This is a big mess!
   const int WAIT_COMMAND_COMPLETION_RETRIES = 3;
   for( int i = 0; i < WAIT_COMMAND_COMPLETION_RETRIES; i++ ) {
      if( m_nLastAck != m_nDebugAck ) break;
      ::Sleep(50L);
   }
   m_nLastAck = m_nDebugAck;
   // Translate command and send it
   return m_ShellManager.WriteData(_TranslateCommand(pstrText));
}

bool CDebugManager::DoSignal(BYTE bCmd)
{
   if( !IsDebugging() ) return false;
   return m_ShellManager.WriteSignal(bCmd);
}

CString CDebugManager::GetParam(LPCTSTR pstrName) const
{
   CString sName = pstrName;
   if( sName == "ChangeDir" ) return m_sCommandCD;
   if( sName == "App" ) return m_sAppExecutable;
   if( sName == "AppArgs" ) return m_sAppArgs;
   if( sName == "Debugger" ) return m_sDebuggerExecutable;
   if( sName == "DebuggerArgs" ) return m_sDebuggerArgs;
   if( sName == "DebugMain" ) return m_sDebugMain;
   if( sName == "StartTimeout" ) return ToString(m_lStartTimeout);
   if( sName == "InCommand" ) return m_bCommandMode ? _T("true") : _T("false");
   return m_ShellManager.GetParam(pstrName);
}

void CDebugManager::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   if( sName == "ChangeDir" ) m_sCommandCD = pstrValue;
   if( sName == "App" ) m_sAppExecutable = pstrValue;
   if( sName == "AppArgs" ) m_sAppArgs = pstrValue;
   if( sName == "Debugger" ) m_sDebuggerExecutable = pstrValue;
   if( sName == "DebuggerArgs" ) m_sDebuggerArgs = pstrValue;
   if( sName == "DebugMain" ) m_sDebugMain = pstrValue;
   if( sName == "StartTimeout" ) m_lStartTimeout = _ttol(pstrValue);
   if( sName == "InCommand" ) m_bCommandMode = _tcscmp(pstrValue, _T("true")) == 0;
   m_ShellManager.SetParam(pstrName, pstrValue);
}

// Implementation

bool CDebugManager::_WaitForDebuggerStart()
{
   // Wait for for the first debugger acknowledgement (GDB prompt)
   DWORD dwStartTick = ::GetTickCount();
   while( ::GetTickCount() - dwStartTick < m_lStartTimeout * 1000UL ) {
      ::Sleep(200L);
      PumpIdleMessages();
      if( m_nDebugAck > 0 ) return true;
   }
   return false;
}

CString CDebugManager::_TranslateCommand(LPCTSTR pstrCommand, LPCTSTR pstrParam /*= NULL*/)
{
   ATLASSERT(!::IsBadStringPtr(pstrCommand,-1));

   CString sCommand = pstrCommand;

   TCHAR szProjectName[128] = { 0 };
   m_pProject->GetName(szProjectName, 127);

   // Translate meta-tokens in command
   sCommand.Replace(_T("$PROJECTNAME$"), szProjectName);
   sCommand.Replace(_T("$PATH$"), m_ShellManager.GetParam(_T("Path")));
   sCommand.Replace(_T("\\n"), _T("\r\n"));
   if( pstrParam ) {
      TCHAR szName[MAX_PATH];
      _tcscpy(szName, pstrParam);
      sCommand.Replace(_T("$FILEPATH$"), szName);
      ::PathStripPath(szName);
      sCommand.Replace(_T("$FILENAME$"), szName);
      ::PathRemoveExtension(szName);
      sCommand.Replace(_T("$NAME$"), szName);
   }

   // Do some manipulation depending on the debug command.
   // Some commands might spuriously fail and we don't want to see error
   // messages all the time, so we'll ignore these.
   if( sCommand.Find(_T("-delete")) >= 0 ) m_nIgnoreErrors++;
   if( sCommand.Find(_T("-evaluate")) >= 0 ) m_nIgnoreErrors++;
   if( sCommand.Find(_T("-var-evaluate-expression")) >= 0 ) m_sVarName = sCommand.Mid(25);
   if( sCommand.Find(_T("-data-evaluate-expression")) >= 0 ) m_sVarName = sCommand.Mid(26);
   return sCommand;
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
   if( m_ShellManager.IsBusy() ) {
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_WARNING));
      CString sMsg(MAKEINTRESOURCE(IDS_COMPILEBUSY));
      m_pProject->DelayedMessage(sMsg, sCaption, MB_ICONEXCLAMATION | MB_MODELESS);
      return false;
   }

   // If the project needs save/compile then ask if we
   // should do so now...
   if( m_pProject->IsDirty() ) {
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

void CDebugManager::_ParseNewFrame(CMiInfo& info)
{
   CString sFilename = info.GetItem(_T("file"), _T("frame"));
   if( !sFilename.IsEmpty() ) 
   {
      // Debugger stopped in source file.
      // Bring up trace window...
      bool bKnownFile = m_pProject->FindView(sFilename, true) != NULL;
      long lLineNum = _ttol(info.GetItem(_T("line"), _T("frame")));
      m_pProject->DelayedOpenView(sFilename, lLineNum);
      m_pProject->DelayedViewMessage(DEBUG_CMD_SET_CURLINE, sFilename, lLineNum);
      m_pProject->DelayedStatusBar(CString(MAKEINTRESOURCE(bKnownFile ? IDS_STATUS_DEBUG_BREAKPOINT : IDS_STATUS_DEBUG_NOFILE)));
   }
   else
   {
      // Debugger stopped at a point without debug information.
      m_pProject->DelayedViewMessage(DEBUG_CMD_SET_CURLINE);
      m_pProject->DelayedStatusBar(CString(MAKEINTRESOURCE(IDS_STATUS_DEBUG_NOSOURCE)));
   }
   // Cause the debug views to refresh
   m_pProject->DelayedDebugEvent(LAZY_DEBUG_STOP_EVENT);
   // Make sure the new state is known to our engine
   m_pProject->DelayedDebugInfo(_T("stopped"), info);
}

/**
 * Update internal breakpoint structures.
 * This method gets called when the GDB debugger deliver an updated
 * debug list and tells us which internal breakpoint-nr it assigned
 * out new breakpoint.
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
      // Debugger has stopped and is waiting for input
      m_bBreaked = true;
      // Parse command
      CMiInfo info = sLine;
      sValue = info.GetItem(_T("reason"));
      // Handles reason:
      //   'exited'
      //   'exited-normally'
      //   'exited-signalled'
      //   'exited-with-errorcode'
      if( sValue.Find(_T("exited")) == 0 ) {
         SignalStop();
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
      // Assume current position changed
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
      m_pProject->DelayedViewMessage(DEBUG_CMD_SET_RUNNING);
      m_pProject->DelayedStatusBar(CString(MAKEINTRESOURCE(IDS_STATUS_DEBUGGING)));
      return;
   }
   // Debugger stopped for good.
   if( sCommand == _T("exit") ) {
      m_pProject->DelayedStatusBar(_T(""));
      SignalStop();
      return;
   }
   // Debugger returned an error message
   if( sCommand == _T("error") ) 
   {
      // Ignore errors might have been requested, so fulfill thy wish...
      if( m_nIgnoreErrors > 0 ) return;
      CMiInfo info = sLine;
      CString sMessage;
      sMessage.Format(IDS_ERR_DEBUG, info.GetItem(_T("msg")));
      m_pProject->DelayedMessage(sMessage, CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
      return;
   }
   // Debugger executed command
   if( sCommand == _T("done") ) 
   {
      sLine.TrimLeft(_T(","));
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
         _T("asm_insns"),
         _T("numchild"),
         NULL
      };
      CString sCommand = sLine.SpanExcluding(_T("="));
      if( sCommand == _T("value") ) {
         // Unfortunately the GDB MI "value" result doesn't always
         // contain information about what was asked, so we
         // internally book-keep this information and append
         // it to the output (see _TranslateCommand()).
         // The consequence is that we can only have one GDB command
         // lingering at any time.
         CString sTemp;
         m_sVarName.Remove('\"');
         sTemp.Format(_T(",name=\"%s\""), m_sVarName);
         sLine += sTemp;
      }
      // Check command and route it to client processing
      CMiInfo info = sLine;
      if( sCommand == _T("frame") ) {
         _ParseNewFrame(info);
      }
      if( sCommand == _T("bkpt") ) {
         _UpdateBreakpoint(info);
      }
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
   // in raw format then...
   if( m_nDebugAck == 0 ) {
      if( sLine.Find(_T("no debugging symbols found")) >= 0 ) {
         m_pProject->DelayedMessage(CString(MAKEINTRESOURCE(IDS_ERR_NODEBUGINFO)), CString(MAKEINTRESOURCE(IDS_CAPTION_MESSAGE)), MB_ICONINFORMATION);
      }
      if( sLine.Find(_T("No symbol table is loaded")) >= 0 ) {
         m_pProject->DelayedMessage(CString(MAKEINTRESOURCE(IDS_ERR_NODEBUGINFO)), CString(MAKEINTRESOURCE(IDS_CAPTION_MESSAGE)), MB_ICONINFORMATION);
      }
      if( sLine.Find(_T("No such file")) >= 0 ) {
         m_pProject->DelayedMessage(CString(MAKEINTRESOURCE(IDS_ERR_NODEBUGFILE)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
      }
      if( sLine.Find(_T("gdb: unrecognized option")) >= 0 ) {
         m_pProject->DelayedMessage(CString(MAKEINTRESOURCE(IDS_ERR_DEBUGVERSION)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
      }
   }
   // GDB is known to be unstable at times
   if( sLine.Find(_T("An internal GDB error was detected.")) >= 0 ) {
      m_pProject->DelayedMessage(CString(MAKEINTRESOURCE(IDS_ERR_DEBUGUNSTABLE)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
      SignalStop();
   }
   // Outputting directly to the Command View if we're
   // in "command mode" (user entered custom command at prompt or scripting).
   if( m_bCommandMode ) {
      CString sText = sLine.Mid(1, sLine.GetLength() - 2);
      sText.Replace(_T("\\n"), _T("\r\n"));
      sText.Replace(_T("\\t"), _T("\t"));
      CRichEditCtrl ctrlEdit = _pDevEnv->GetHwnd(IDE_HWND_COMMANDVIEW);
      AppendRtfText(ctrlEdit, sText);
   }
}

void CDebugManager::_ParseTargetOutput(LPCTSTR pstrText)
{
   CString sLine = pstrText;
   if( sLine.IsEmpty() ) return;
}

void CDebugManager::_ParseLogOutput(LPCTSTR pstrText)
{
   CString sLine = pstrText;
   if( sLine.IsEmpty() ) return;
}

/**
 * Halts the debugger so we can send more GDB commands.
 * GDB is not async and doesn't allow debug commands to be sent while it is debugging the app.
 * We need to suspend it temporarily and send our commands, then we can resume debugging.
 * @return true If we halted the debugger and should restart it later.
 */
bool CDebugManager::_PauseDebugger()
{
   // Do we need to stop the debugger?
   if( IsBreaked() ) return false;
   // Yes, let's give it a break.
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
   // Only if debugging.
   if( !m_bDebugging ) return;
   // Count number of debug prompts (=acknoledgements)
   if( _tcsncmp(pstrText, _T("(gdb"), 4) == 0 ) {
      m_bCommandMode = false;
      if( m_nIgnoreErrors > 0 ) m_nIgnoreErrors--;
      m_nDebugAck++;
      return;
   }
   // Look at raw text always
   if( *pstrText == '~' ) _ParseConsoleOutput(pstrText + 1);
   // Not accepting lines before first acknoledge
   if( m_nDebugAck == 0 ) return;
   // Parse output stream
   if( *pstrText == '^' ) _ParseResultRecord(pstrText + 1);
   if( *pstrText == '@' ) _ParseTargetOutput(pstrText + 1);
   if( *pstrText == '&' ) _ParseLogOutput(pstrText + 1);
   // Unfortunately Out-of-band output may occur in the middle
   // of the stream! This is obvious a GDB bug, but we'll try to
   // handle the situation by adding a command-handshake (see 
   // DoDebugCommand()) and by looking for substrings.
   LPTSTR pstr = _tcschr(pstrText, '*');
   if( pstr ) _ParseOutOfBand(pstr + 1);
   // Happily ignore everything else...
}
