
#include "stdafx.h"
#include "resource.h"

#include "CompileManager.h"

#include "TelnetProtocol.h"
#include "Project.h"

#pragma code_seg( "MISC" )


////////////////////////////////////////////////////////
//

DWORD CCompileThread::Run()
{
   ATLASSERT(m_pProject);
   ATLASSERT(m_pManager);

   while( !ShouldStop() ) 
   {
      m_pManager->m_event.WaitForEvent();
      if( ShouldStop() ) break;

      // First send window resize
      if( m_szWindow.cx > 0 ) m_pManager->m_ShellManager.WriteScreenSize(m_szWindow.cx, m_szWindow.cy);

      // Get a local copy of the commands
      m_cs.Lock();
      CSimpleValArray<CString> aActions;
      while( m_aCommands.GetSize() > 0 ) {
         aActions.Add(m_aCommands[0]);
         m_aCommands.RemoveAt(0);
      }
      m_pManager->m_bCommandMode = m_bCommandMode;
      m_pManager->m_bIgnoreOutput = m_bIgnoreOutput;
      m_cs.Unlock();

      // Execute commands
      for( int i = 0; i < aActions.GetSize(); i++ ) {
         CString sCommand = aActions[i];
         if( sCommand.IsEmpty() ) continue;
         if( !m_pManager->m_ShellManager.WriteData(sCommand) ) {
            // A send error occured.
            // Need to broadcast termination marker to allow
            // gracefull closure of views
            m_pManager->m_ShellManager.BroadcastLine(VT100_HIDDEN, TERM_MARKER);
            // We need to stop now
            m_pManager->SignalStop();
            // Let's just remove the remaining commands (if any was added
            // since), because they are not going to be sent anyway...
            m_cs.Lock();
            m_aCommands.RemoveAll();
            m_cs.Unlock();
            // Update statusbar
            m_pProject->DelayedStatusBar(CString(MAKEINTRESOURCE(IDS_STATUS_NETWORKWRITE)));
            return 0;
         }
         ::Sleep(200L);
         if( ShouldStop() ) break;
      }

      // Idle wait for completion before accepting
      // new prompt commands...
      while( m_pManager->IsBusy() ) ::Sleep(200L);
   }

   return 0;
}


////////////////////////////////////////////////////////
//

// This flag is a 'static' because we want all projects
// in a Solution to be aware of this state.
bool volatile CCompileManager::s_bBusy = false;


CCompileManager::CCompileManager() :
   m_pProject(NULL),
   m_bReleaseMode(true),
   m_bCommandMode(false),
   m_bIgnoreOutput(false)
{
   m_event.Create();
   Clear();
}

void CCompileManager::Init(CRemoteProject* pProject)
{
   ATLASSERT(pProject);
   m_pProject = pProject;

   m_ShellManager.Init(pProject);

   m_thread.m_pProject = pProject;
   m_thread.m_pManager = this;
}

void CCompileManager::Clear()
{
   m_ShellManager.Clear();
   m_sCommandCD = _T("cd $PATH$");
   m_sCommandBuild = _T("make all");
   m_sCommandRebuild = _T("make all");
   m_sCommandCompile = _T("make $NAME$");
   m_sCommandClean = _T("make clean");
   m_sCommandDebug = _T("export DEBUG_OPTIONS=\"-g -D_DEBUG\"");
   m_sCommandRelease = _T("export DEBUG_OPTIONS=");
   m_sPromptPrefix = _T("$~[/");
   m_bReleaseMode = false;
   m_bCommandMode = false;
   m_bIgnoreOutput = false;
}

bool CCompileManager::Load(ISerializable* pArc)
{
   Clear();
   if( !pArc->ReadGroupBegin(_T("Compiler")) ) return true;

   if( !m_ShellManager.Load(pArc) ) return false;

   if( !pArc->ReadItem(_T("Prompt")) ) return false;
   pArc->Read(_T("prefixes"), m_sPromptPrefix.GetBufferSetLength(32), 32);
   m_sPromptPrefix.ReleaseBuffer();

   if( !pArc->ReadItem(_T("Commands")) ) return false;
   pArc->Read(_T("changeDir"), m_sCommandCD.GetBufferSetLength(200), 200);
   m_sCommandCD.ReleaseBuffer();
   pArc->Read(_T("build"), m_sCommandBuild.GetBufferSetLength(200), 200);
   m_sCommandBuild.ReleaseBuffer();
   pArc->Read(_T("rebuild"), m_sCommandRebuild.GetBufferSetLength(200), 200);
   m_sCommandRebuild.ReleaseBuffer();
   pArc->Read(_T("compile"), m_sCommandCompile.GetBufferSetLength(200), 200);
   m_sCommandCompile.ReleaseBuffer();
   pArc->Read(_T("clean"), m_sCommandClean.GetBufferSetLength(200), 200);
   m_sCommandClean.ReleaseBuffer();
   pArc->Read(_T("debugExport"), m_sCommandDebug.GetBufferSetLength(200), 200);
   m_sCommandDebug.ReleaseBuffer();
   pArc->Read(_T("releaseExport"), m_sCommandRelease.GetBufferSetLength(200), 200);
   m_sCommandRelease.ReleaseBuffer();

   if( !pArc->ReadGroupEnd() ) return false;
   return true;
}

bool CCompileManager::Save(ISerializable* pArc)
{
   if( !pArc->WriteGroupBegin(_T("Compiler")) ) return true;

   if( !m_ShellManager.Save(pArc) ) return false;

   if( !pArc->WriteItem(_T("Prompt")) ) return false;
   pArc->Write(_T("prefixes"), m_sPromptPrefix);

   if( !pArc->WriteItem(_T("Commands")) ) return false;
   pArc->Write(_T("changeDir"), m_sCommandCD);
   pArc->Write(_T("build"), m_sCommandBuild);
   pArc->Write(_T("rebuild"), m_sCommandRebuild);
   pArc->Write(_T("compile"), m_sCommandCompile);
   pArc->Write(_T("clean"), m_sCommandClean);
   pArc->Write(_T("debugExport"), m_sCommandDebug);
   pArc->Write(_T("releaseExport"), m_sCommandRelease);

   if( !pArc->WriteGroupEnd() ) return false;
   return true;
}

bool CCompileManager::Start()
{
   Stop();
   m_ShellManager.Start();
   m_thread.Start();
   return true;
}

bool CCompileManager::Stop()
{
   SignalStop();
   m_ShellManager.RemoveLineListener(this);
   m_ShellManager.Stop();
   m_thread.Stop();
   m_bCommandMode = false;
   m_bIgnoreOutput = false;
   s_bBusy = false;
   return true;
}

void CCompileManager::SignalStop()
{
   m_pProject->DelayedGuiAction(GUI_ACTION_STOP_ANIMATION);
   m_ShellManager.SignalStop();
   m_thread.SignalStop();
   m_event.SetEvent();
   s_bBusy = false;
}

bool CCompileManager::IsBusy() const
{
   return s_bBusy && IsConnected();
}

bool CCompileManager::IsConnected() const
{
   return m_thread.IsRunning() && m_ShellManager.IsConnected();
}

bool CCompileManager::DoAction(LPCTSTR pstrName, LPCTSTR pstrParams /*= NULL*/)
{
   // Translate named actions into real prompt commands.
   // We usually start off by changing to the project folder
   // to make sure everything run from the correct path.
   // Then we submit the actual command (user configurable).
   CString sName = pstrName;
   CSimpleArray<CString> aCommands;
   CString sCommand;
   if( sName == "Clean" ) {
      if( !_PrepareProcess(pstrName) ) return false;
      CString sTitle(MAKEINTRESOURCE(IDS_CLEAN));
      sCommand = _TranslateCommand(m_sCommandCD);
      aCommands.Add(sCommand);
      sCommand = _TranslateCommand(m_sCommandClean);
      aCommands.Add(sCommand);
      return _StartProcess(sTitle, aCommands);
   }
   if( sName == "Build" ) {
      if( !_PrepareProcess(pstrName) ) return false;
      CString sTitle(MAKEINTRESOURCE(IDS_BUILD));
      sCommand = _TranslateCommand(m_sCommandCD);
      aCommands.Add(sCommand);
      sCommand = _TranslateCommand(m_bReleaseMode ? m_sCommandRelease : m_sCommandDebug);
      aCommands.Add(sCommand);
      sCommand = _TranslateCommand(m_sCommandBuild);
      aCommands.Add(sCommand);
      return _StartProcess(sTitle, aCommands);
   }
   if( sName == "Rebuild" ) {
      if( !_PrepareProcess(pstrName) ) return false;
      CString sTitle(MAKEINTRESOURCE(IDS_REBUILD));
      sCommand = _TranslateCommand(m_sCommandCD);
      aCommands.Add(sCommand);
      sCommand = _TranslateCommand(m_sCommandClean);
      aCommands.Add(sCommand);
      sCommand = _TranslateCommand(m_bReleaseMode ? m_sCommandRelease : m_sCommandDebug);
      aCommands.Add(sCommand);
      sCommand = _TranslateCommand(m_sCommandRebuild);
      aCommands.Add(sCommand);
      return _StartProcess(sTitle, aCommands);
   }
   if( sName == "Compile" ) {
      if( !_PrepareProcess(pstrName) ) return false;
      sCommand = _TranslateCommand(m_sCommandCD);
      aCommands.Add(sCommand);
      sCommand = _TranslateCommand(m_bReleaseMode ? m_sCommandRelease : m_sCommandDebug);
      aCommands.Add(sCommand);
      sCommand = _TranslateCommand(m_sCommandCompile, pstrParams);
      aCommands.Add(sCommand);
      return _StartProcess(CString(MAKEINTRESOURCE(IDS_COMPILE)), aCommands);
   }
   if( sName == "Stop" ) {
      SignalStop();
      // Update statusbar now
      m_pProject->DelayedStatusBar(CString(MAKEINTRESOURCE(IDS_STATUS_STOPPED)));
      return true;
   }
   if( sName.Find(_T("cc ")) == 0 ) {
      CString sCommand = sName.Mid(3);
      aCommands.Add(sCommand);
      return _StartProcess(CString(MAKEINTRESOURCE(IDS_EXECUTING)), aCommands, true, false);
   }
   if( sName.Find(_T("cz ")) == 0 ) {
      CString sCommand = _TranslateCommand(sName.Mid(3));
      aCommands.Add(sCommand);
      return _StartProcess(CString(MAKEINTRESOURCE(IDS_EXECUTING)), aCommands, true, true);
   }
   ATLASSERT(false);
   return false;
}

CString CCompileManager::GetParam(LPCTSTR pstrName) const
{
   CString sName = pstrName;
   if( sName == "ChangeDir" ) return m_sCommandCD;
   if( sName == "Build" ) return m_sCommandBuild;
   if( sName == "Rebuild" ) return m_sCommandRebuild;
   if( sName == "Compile" ) return m_sCommandCompile;
   if( sName == "Clean" ) return m_sCommandClean;
   if( sName == "DebugExport" ) return m_sCommandDebug;
   if( sName == "ReleaseExport" ) return m_sCommandRelease;
   if( sName == "Mode" ) return m_bReleaseMode ? _T("Release") : _T("Debug");
   if( sName == "InCommand" ) return m_bCommandMode ? _T("true") : _T("false");
   return m_ShellManager.GetParam(pstrName);
}

void CCompileManager::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   if( sName == "ChangeDir" ) m_sCommandCD = pstrValue;
   if( sName == "Build" ) m_sCommandBuild = pstrValue;
   if( sName == "Rebuild" ) m_sCommandRebuild = pstrValue;
   if( sName == "Compile" ) m_sCommandCompile = pstrValue;
   if( sName == "Clean" ) m_sCommandClean = pstrValue;
   if( sName == "DebugExport" ) m_sCommandDebug = pstrValue;
   if( sName == "ReleaseExport" ) m_sCommandRelease = pstrValue;
   if( sName == "Mode" ) m_bReleaseMode = _tcscmp(pstrValue, _T("Release")) == 0;
   if( sName == "InCommand" ) m_bCommandMode = _tcscmp(pstrValue, _T("true")) == 0;
   m_ShellManager.SetParam(pstrName, pstrValue);
}

// Implementation

CString CCompileManager::_TranslateCommand(LPCTSTR pstrCommand, LPCTSTR pstrParam /*= NULL*/) const
{
   CString sName = pstrCommand;
   TCHAR szProjectName[128] = { 0 };
   m_pProject->GetName(szProjectName, 127);

   sName.Replace(_T("$PROJECTNAME$"), szProjectName);
   sName.Replace(_T("$PATH$"), m_ShellManager.GetParam(_T("Path")));
   sName.Replace(_T("\\n"), _T("\r\n"));
   if( pstrParam ) {
      TCHAR szName[MAX_PATH] = { 0 };
      _tcscpy(szName, pstrParam);
      sName.Replace(_T("$FILEPATH$"), szName);
      ::PathStripPath(szName);
      sName.Replace(_T("$FILENAME$"), szName);
      ::PathRemoveExtension(szName);
      sName.Replace(_T("$NAME$"), szName);
   }
   return sName;
}

bool CCompileManager::_PrepareProcess(LPCTSTR /*pstrName*/)
{
   // Detect if we're in "release" mode
   m_bReleaseMode = m_pProject->GetBuildMode() == _T("Release");

   // Does project need save before it can run?
   // Let's warn user about this!
   if( m_pProject->IsDirty() ) {
      bool bAutoSave = false;
      bool bSavePrompt = false;
      TCHAR szBuffer[32] = { 0 };
      _pDevEnv->GetProperty(_T("editors.general.savePrompt"), szBuffer, 31);
      if( _tcscmp(szBuffer, _T("true")) == 0 ) bSavePrompt = true;
      _pDevEnv->GetProperty(_T("editors.general.saveBeforeTool"), szBuffer, 31);
      if( _tcscmp(szBuffer, _T("true")) == 0 ) bAutoSave = true;

      UINT nRes = IDNO;
      if( bSavePrompt ) 
      {
         nRes = _pDevEnv->ShowMessageBox(NULL, 
                                         CString(MAKEINTRESOURCE(IDS_SAVECHANGES)), 
                                         CString(MAKEINTRESOURCE(IDS_CAPTION_QUESTION)), 
                                         MB_YESNO | MB_ICONQUESTION);
      }
      if( bAutoSave || nRes == IDYES ) m_pProject->m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE_ALL, 0));
   }
   return true;
}

bool CCompileManager::_StartProcess(LPCTSTR pstrName, 
                                    CSimpleArray<CString>& aCommands, 
                                    bool bCommandMode /*= false*/,
                                    bool bIgnoreOutput /*= false*/)
{
   ATLASSERT(m_pProject);
   ATLASSERT(!::IsBadStringPtr(pstrName,-1));
   ATLASSERT(aCommands.GetSize()>0);

   CWaitCursor cursor;
   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
   CRichEditCtrl ctrlEdit = _pDevEnv->GetHwnd(IDE_HWND_OUTPUTVIEW);

   m_sProcessName = pstrName;

   // If the remote session isn't running, then it's about time...
   if( !IsConnected() ) Start();

   // Are we busy doing another build action?
   if( IsBusy() ) {
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_WARNING));
      CString sMsg(MAKEINTRESOURCE(IDS_COMPILE_BUSY));
      _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_ICONEXCLAMATION | MB_MODELESS);
      return false;
   }

   // Busy debugging?
   if( m_pProject->m_DebugManager.IsBusy() ) {
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_QUESTION));
      CString sMsg(MAKEINTRESOURCE(IDS_DEBUGGER_BUSY));
      if( IDNO == _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_YESNO | MB_ICONQUESTION) ) return false;
      m_pProject->m_DebugManager.SignalStop();
      m_pProject->m_DebugManager.Stop();
   }

   // Has the remote host accepted connection?
   if( !m_ShellManager.IsConnected() ) {
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_QUESTION));
      CString sMsg(MAKEINTRESOURCE(IDS_WAITCONNECTION));
      if( IDNO == _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_YESNO | MB_ICONQUESTION) ) return false;
      // Not connected yet? Let's connect now!
      CWaitCursor cursor;
      CString sStatus;
      sStatus.Format(IDS_STATUS_CONNECT_WAIT, m_sProcessName);
      _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus, TRUE);
      _pDevEnv->PlayAnimation(TRUE, ANIM_TRANSFER);
      m_ShellManager.WaitForConnection();
   }

   // Change statusbar and idle animation
   CString sStatus;
   sStatus.Format(IDS_STATUS_STARTED, m_sProcessName);
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus, TRUE);  
   _pDevEnv->PlayAnimation(TRUE, ANIM_BUILD);

   m_ShellManager.AddLineListener(this);

   // Update execute thread
   // Add new commands to command list.
   // and add marker that will end the compile session
   m_thread.m_cs.Lock();
   m_thread.m_szWindow = _GetViewWindowSize();
   m_thread.m_bCommandMode = bCommandMode;
   m_thread.m_bIgnoreOutput = bIgnoreOutput;
   for( int i = 0; i < aCommands.GetSize(); i++ ) {
      CString sCommand = aCommands[i];
      m_thread.m_aCommands.Add(sCommand);
   }
   CString sCommand = TERM_MARKER;
   m_thread.m_aCommands.Add(sCommand);
   m_thread.m_cs.Unlock();
   
   // Finally, signal that we have new data
   s_bBusy = true;
   m_event.SetEvent();

   // If this is a regular compile command (not user-entered or scripting prompt)
   // let's pop up the compile window...
   if( !bCommandMode && !bIgnoreOutput ) {
      ctrlEdit.SetWindowText(_T(""));
      m_pProject->DelayedGuiAction(GUI_ACTION_ACTIVATEVIEW, ctrlEdit);
   }

   return false;
}

SIZE CCompileManager::_GetViewWindowSize() const
{
   // Helper function to determine current width of the compile-pane/window.
   // This will be relayed to the TELNET/SSH session as the window size.
   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_OUTPUTVIEW);
   RECT rc;
   wndMain.GetClientRect(&rc);
   if( rc.right - rc.left == 0 ) {
      SIZE szTemp = { 80, 25 };
      return szTemp;
   }
   CClientDC dc = wndMain;
   HFONT hOldFont = dc.SelectFont(wndMain.GetFont());
   SIZE szText = { 0 };
   dc.GetTextExtent(_T("X"), 1, &szText);
   dc.SelectFont(hOldFont);
   ATLASSERT(szText.cx!=0);
   SIZE size = { (rc.right - rc.left) / szText.cx, 25 };
   return size;
}


/////////////////////////////////////////////////////////////////////////
// IOutputLineListener

void CCompileManager::OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText)
{
   ATLASSERT(!::IsBadStringPtr(pstrText,-1));
   
   CString sText;
   sText.Format(_T("%s\r\n"), pstrText);

   int iPos = sText.Find(TERM_MARKER);
   if( iPos >= 0 ) {
      if( iPos == 0 ) return; // HACK: Ignore if printed on first column!
                              //       Avoid halting on echo.
      m_ShellManager.RemoveLineListener(this);
      CString sStatus;
      sStatus.Format(IDS_STATUS_FINISHED, m_sProcessName);
      m_pProject->DelayedStatusBar(sStatus);
      m_pProject->DelayedGuiAction(GUI_ACTION_STOP_ANIMATION);
      m_bCommandMode = false;
      m_bIgnoreOutput = false;
      s_bBusy = false;
      return;
   }

   if( nColor == VT100_HIDDEN ) return;

   // Yield control?
   if( ::InSendMessage() ) ::ReplyMessage(TRUE);

   // Output text
   CRichEditCtrl ctrlEdit = _pDevEnv->GetHwnd(IDE_HWND_OUTPUTVIEW);
   if( m_bCommandMode ) ctrlEdit = _pDevEnv->GetHwnd(IDE_HWND_COMMANDVIEW);
   if( m_bIgnoreOutput ) return;

   ATLASSERT(ctrlEdit.IsWindow());
   SCROLLINFO sinfo = { 0 };
   sinfo.cbSize = sizeof(SCROLLINFO);
   sinfo.fMask = SIF_ALL;
   ctrlEdit.GetScrollInfo(SB_VERT, &sinfo);

   CHARRANGE cr;
   ctrlEdit.GetSel(cr);
   ctrlEdit.HideSelection(TRUE);

   // Remove lines if we've filled out the buffer
   GETTEXTLENGTHEX gtlx = { GTL_DEFAULT | GTL_CLOSE, 1200 };
   while( ctrlEdit.GetTextLengthEx(&gtlx) > ctrlEdit.GetLimitText() - 2000 ) {     
      LONG iStartPos = ctrlEdit.LineIndex(0);
      LONG iEndPos = ctrlEdit.LineIndex(1);
      ctrlEdit.SetSel(iStartPos, iEndPos);
      ctrlEdit.ReplaceSel(_T(""));
   }

   // Append text
   ctrlEdit.SetSel(-1, -1);
   LONG iStartPos = 0;
   LONG iDummy = 0;
   ctrlEdit.GetSel(iStartPos, iDummy);
   ctrlEdit.ReplaceSel(sText);
   LONG iEndPos = 0;
   ctrlEdit.GetSel(iDummy, iEndPos);

   // Style the text depending on certain search-strings, prompt-prefixes etc
   CHARFORMAT cf;
   cf.cbSize = sizeof(CHARFORMAT);
   cf.dwMask = CFM_COLOR | CFM_BOLD;
   cf.dwEffects = 0;
   cf.crTextColor = ::GetSysColor(COLOR_WINDOWTEXT);
   if( nColor == VT100_RED ) cf.crTextColor = RGB(200,0,0);
   if( _tcsstr(pstrText, _T("error:")) != NULL ) cf.crTextColor = RGB(150,70,0);
   if( _tcsstr(pstrText, _T("warning:")) != NULL ) cf.crTextColor = RGB(70,70,0);
   if( _tcschr(m_sPromptPrefix, *pstrText) != NULL ) cf.dwEffects |= CFE_BOLD;
   if( *pstrText == '/' && _tcschr(pstrText, ':') != NULL ) cf.dwEffects &= ~CFE_BOLD;

   ctrlEdit.SetSel(iStartPos, iEndPos);
   ctrlEdit.SetSelectionCharFormat(cf);

   // Count the number of lines in the text
   int nLines = 0;
   while( *pstrText && *pstrText++ == '\n' ) nLines++;

   if( iStartPos != cr.cpMin ) {
      ctrlEdit.SetSel(cr); 
   } 
   else {
      ctrlEdit.SetSel(-1, -1);
      // Support page scrolling only when the scroll-thumb
      // is at the very bottom of the page...
      if( (sinfo.nPage == 0U) || (sinfo.nPos + sinfo.nPage >= sinfo.nMax - 40U) ) {
         // Scroll a few additional lines because this way, we can "catch up" with
         // a scroll-thumb "close" to the bottom!
         int iExtra = sinfo.nPos + (int) sinfo.nPage >= sinfo.nMax ? 0U : 3U;
         ctrlEdit.LineScroll(nLines + iExtra);
      }
   }
   ctrlEdit.HideSelection(FALSE);
   ctrlEdit.UpdateWindow();
}
