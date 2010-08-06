
#include "StdAfx.h"
#include "resource.h"

#include "CompileManager.h"

#include "TelnetProtocol.h"
#include "Project.h"

#pragma code_seg( "MISC" )


////////////////////////////////////////////////////////
//

DWORD CRebuildThread::Run()
{
   _pDevEnv->SetThreadLanguage();

   CCoInitialize cominit;

   // Build all projects...
   ISolution* pSolution = _pDevEnv->GetSolution();
   INT nProjects = pSolution->GetItemCount();
   for( INT i = 0; i < nProjects; i++ ) {
      // Useless status message
      CString sStatus;
      sStatus.Format(IDS_STATUS_STARTING, CString(MAKEINTRESOURCE(IDS_REBUILD)));
      m_pProject->DelayedStatusBar(sStatus);
      // Get the next project and see if it supports a
      // "Rebuild" and "IsBusy" command...
      IProject* pProject = pSolution->GetItem(i);
      if( pProject == NULL ) break;
      CComDispatchDriver dd = pProject->GetDispatch();
      DISPID dispid = 0;
      if( dd.GetIDOfName(L"IsBusy", &dispid) != S_OK ) continue;
      if( dd.GetIDOfName(L"Rebuild", &dispid) != S_OK ) continue;
      // Start the build process
      dd.Invoke0(L"Rebuild");
      // Allow the build action to get started for 3 seconds
      for( int x = 0; x < 3000 && !ShouldStop(); x += 500 ) {
         // See if it started by itself
         CComVariant vRet;
         dd.GetPropertyByName(L"IsBusy", &vRet);
         if( vRet.boolVal != VARIANT_FALSE ) break;
         ::Sleep(500L);
      }
      // ...then monitor it until it stops
      while( !ShouldStop() ) {
         // Just poll to see if it's done
         CComVariant vRet;
         dd.GetPropertyByName(L"IsBusy", &vRet);
         if( vRet.boolVal == VARIANT_FALSE ) break;
         ::Sleep(500L);
      }
      if( ShouldStop() ) break;
   }

   CString sStatus;
   sStatus.Format(IDS_STATUS_FINISHED, CString(MAKEINTRESOURCE(IDS_REBUILD)));
   m_pProject->DelayedStatusBar(sStatus);

   return 0;
}


////////////////////////////////////////////////////////
//

CCompileThread::CCompileThread()
{
   m_szWindow.cx = 0;
   m_szWindow.cy = 0;
}

DWORD CCompileThread::Run()
{
   ATLASSERT(m_pProject);
   ATLASSERT(m_pManager);

   _pDevEnv->SetThreadLanguage();

   CCoInitialize cominit;

   while( !ShouldStop() ) 
   {
      m_pManager->m_event.WaitForEvent();
      if( ShouldStop() ) break;

      // First send window resize
      if( m_szWindow.cx > 0 ) m_pManager->m_ShellManager.WriteScreenSize(m_szWindow.cx, m_szWindow.cy);

      // Get a local copy of the currently queued commands
      m_cs.Lock();
      CSimpleValArray<CString> aActions;
      while( m_aCommands.GetSize() > 0 ) {
         aActions.Add(m_aCommands[0]);
         m_aCommands.RemoveAt(0);
      }
      // Assign flags to active session
      m_pManager->m_Flags = 0;
      while( m_aFlags.GetSize() > 0 ) {
         m_pManager->m_Flags |= m_aFlags[0];
         m_aFlags.RemoveAt(0);
      }
      m_cs.Unlock();

      // Execute commands...
      for( int i = 0; i < aActions.GetSize(); i++ ) {
         CString sCommand = aActions[i];
         if( sCommand.IsEmpty() ) continue;
         if( !m_pManager->m_ShellManager.WriteData(sCommand) ) {
            // A send error occured.
            // Need to broadcast termination marker to allow
            // gracefull closure of views
            m_pManager->m_ShellManager.BroadcastLine(VT100_HIDDEN, TERM_MARKER_LINE);
            m_pProject->DelayedStatusBar(CString(MAKEINTRESOURCE(IDS_STATUS_NETWORKWRITE)));
            // We should stop now
            m_pManager->SignalStop();
            // Let's just remove the remaining commands (if any was added
            // since), because they are not going to be sent anyway...
            m_cs.Lock();
            m_aCommands.RemoveAll();
            m_cs.Unlock();
            return 0;
         }
         ::Sleep(200L);  // FIX: Don't rush things; overlapping stdin/stdout
         if( ShouldStop() ) break;
      }

      // Request for re-loading of active file?
      if( (m_pManager->m_Flags & COMPFLAG_RELOADFILE) != 0 ) {
         m_pProject->DelayedGuiAction(LAZY_GUI_FILE_RELOAD);
      }

      // Idle wait for completion before accepting new prompt commands...
      while( m_pManager->IsBusy() && IsQueueEmpty() ) ::Sleep(200L);
   }

   return 0;
}

bool CCompileThread::IsQueueEmpty()
{
   return m_aCommands.GetSize() == 0;
}


////////////////////////////////////////////////////////
//

// This flag is a 'static' because we want all projects
// in a Solution to be aware of this state.
// It signals that some project is currently compiling.
volatile bool CCompileManager::s_bBusy = false;


CCompileManager::CCompileManager() :
   m_Flags(0),
   m_pProject(NULL),
   m_bCompiling(false)
{
   m_event.Create();
   Clear();
}

void CCompileManager::Init(CRemoteProject* pProject)
{
   ATLASSERT(pProject);
   m_pProject = pProject;

   m_ShellManager.Init(pProject);

   m_CompileThread.m_pProject = pProject;
   m_CompileThread.m_pManager = this;
}

void CCompileManager::Clear()
{
   m_ShellManager.Clear();
   m_sCommandCD = _T("cd $PATH$");
   m_sCommandBuild = _T("make all");
   m_sCommandRebuild = _T("make all");
   m_sCommandCompile = _T("make $FILENAME$");
   m_sCommandCheckSyntax = _T("g++ -gnatc $FILEPATH$");
   m_sCommandClean = _T("make clean");
   m_sCommandBuildTags = _T("ctags *");
   m_sCommandDebug = _T("export DEBUG_OPTIONS=\"-g -D_DEBUG\"");
   m_sCommandRelease = _T("export DEBUG_OPTIONS=");
   m_sPromptPrefix = _T("$~[/");
   m_sCommandPreStep = _T("");
   m_sCommandPostStep = _T("");
   m_sCommandProcessList = _T("ps");
   m_sCompileFlags = _T("");
   m_sLinkFlags = _T("");
   m_sBuildMode = _T("debug");
   m_bWarningPlayed = false;
   m_Flags = 0;
}

bool CCompileManager::Load(ISerializable* pArc)
{
   Clear();
   if( !pArc->ReadGroupBegin(_T("Compiler")) ) return true;

   if( !m_ShellManager.Load(pArc) ) return false;

   if( pArc->ReadItem(_T("Prompt")) ) {
      pArc->Read(_T("prefixes"), m_sPromptPrefix.GetBufferSetLength(32), 32);
      m_sPromptPrefix.ReleaseBuffer();
      pArc->Read(_T("compileFlags"), m_sCompileFlags.GetBufferSetLength(100), 100);
      m_sCompileFlags.ReleaseBuffer();
      pArc->Read(_T("linkFlags"), m_sLinkFlags.GetBufferSetLength(100), 100);
      m_sLinkFlags.ReleaseBuffer();
   }

   if( pArc->ReadItem(_T("Commands")) ) {
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
      pArc->Read(_T("ps"), m_sCommandProcessList.GetBufferSetLength(200), 200);
      m_sCommandProcessList.ReleaseBuffer();
      pArc->Read(_T("checkSyntax"), m_sCommandCheckSyntax.GetBufferSetLength(200), 200);
      m_sCommandCheckSyntax.ReleaseBuffer();
      pArc->Read(_T("buildTags"), m_sCommandBuildTags.GetBufferSetLength(200), 200);
      m_sCommandBuildTags.ReleaseBuffer();
      pArc->Read(_T("debugExport"), m_sCommandDebug.GetBufferSetLength(200), 200);
      m_sCommandDebug.ReleaseBuffer();
      pArc->Read(_T("releaseExport"), m_sCommandRelease.GetBufferSetLength(200), 200);
      m_sCommandRelease.ReleaseBuffer();
   }

   if( pArc->ReadItem(_T("Steps")) ) {
      pArc->Read(_T("pre"), m_sCommandPreStep.GetBufferSetLength(300), 300);
      m_sCommandPreStep.ReleaseBuffer();
      pArc->Read(_T("post"), m_sCommandPostStep.GetBufferSetLength(300), 300);
      m_sCommandPostStep.ReleaseBuffer();
      ConvertToCrLf(m_sCommandPreStep);
      ConvertToCrLf(m_sCommandPostStep);
   }

   if( m_sCommandProcessList.IsEmpty() ) m_sCommandProcessList = _T("ps");

   if( !pArc->ReadGroupEnd() ) return false;
   return true;
}

bool CCompileManager::Save(ISerializable* pArc)
{
   if( !pArc->WriteGroupBegin(_T("Compiler")) ) return true;

   if( !m_ShellManager.Save(pArc) ) return false;

   if( !pArc->WriteItem(_T("Prompt")) ) return false;
   pArc->Write(_T("prefixes"), m_sPromptPrefix);
   pArc->Write(_T("compileFlags"), m_sCompileFlags);
   pArc->Write(_T("linkFlags"), m_sLinkFlags);

   if( !pArc->WriteItem(_T("Commands")) ) return false;
   pArc->Write(_T("changeDir"), m_sCommandCD);
   pArc->Write(_T("build"), m_sCommandBuild);
   pArc->Write(_T("rebuild"), m_sCommandRebuild);
   pArc->Write(_T("compile"), m_sCommandCompile);
   pArc->Write(_T("clean"), m_sCommandClean);
   pArc->Write(_T("checkSyntax"), m_sCommandCheckSyntax);
   pArc->Write(_T("ps"), m_sCommandProcessList);
   pArc->Write(_T("buildTags"), m_sCommandBuildTags);
   pArc->Write(_T("debugExport"), m_sCommandDebug);
   pArc->Write(_T("releaseExport"), m_sCommandRelease);

   if( !pArc->WriteItem(_T("Steps")) ) return false;
   pArc->Write(_T("pre"), ConvertFromCrLf(m_sCommandPreStep));
   pArc->Write(_T("post"), ConvertFromCrLf(m_sCommandPostStep));

   if( !pArc->WriteGroupEnd() ) return false;
   return true;
}

bool CCompileManager::Start()
{
   Stop();
   m_ShellManager.Start();
   m_CompileThread.Start();
   return true;
}

bool CCompileManager::Stop()
{
   SignalStop();
   m_ShellManager.RemoveLineListener(this);
   m_ShellManager.Stop();
   m_CompileThread.Stop();
   // Reset variables
   m_bCompiling = false;
   s_bBusy = false;
   m_Flags = 0;
   return true;
}

void CCompileManager::SignalStop()
{
   m_pProject->DelayedGuiAction(LAZY_GUI_STOP_ANIMATION);
   m_pProject->DelayedGlobalViewMessage(DEBUG_CMD_COMPILE_STOP);
   m_ShellManager.SignalStop();
   m_CompileThread.SignalStop();
   m_event.SetEvent();
   // Reset variables
   m_bCompiling = false;
   s_bBusy = false;
   m_Flags = 0;
}

bool CCompileManager::IsBusy()
{
   return s_bBusy;
}

bool CCompileManager::IsConnected()
{
   return m_CompileThread.IsRunning() && m_ShellManager.IsConnected();
}

bool CCompileManager::IsCompiling()
{
   return m_bCompiling || m_RebuildThread.IsRunning() == TRUE;
}

bool CCompileManager::DoRebuild()
{
   if( m_RebuildThread.IsRunning() ) return false;
   // Clear the compile output window
   m_pProject->DelayedGuiAction(LAZY_GUI_CLEARVIEW, IDE_HWND_OUTPUTVIEW);
   m_pProject->DelayedGuiAction(LAZY_GUI_ACTIVATEVIEW, IDE_HWND_OUTPUTVIEW);
   // Start thread
   m_RebuildThread.Stop();
   m_RebuildThread.m_pProject = m_pProject;
   m_RebuildThread.m_pManager = this;
   m_RebuildThread.Start();
   return true;
}

bool CCompileManager::DoAction(LPCTSTR pstrName, LPCTSTR pstrParams /*= NULL*/, UINT Flags /*= 0*/)
{
   // Should we compile in Release or Debug mode?
   BOOL bReleaseMode = (m_sBuildMode == _T("release"));
   // Translate named actions into real prompt commands.
   // We usually start off by changing to the project folder
   // to make sure everything run from the correct path.
   // Then we submit the actual command (user configurable).
   CString sTitle;
   CString sName = pstrName;
   CSimpleArray<CString> aCommands;
   if( sName == _T("Clean") ) {
      if( !_PrepareProcess(pstrName) ) return false;
      sTitle.LoadString(IDS_CLEAN);
      aCommands.Add(m_sCommandCD);
      aCommands.Add(m_sCommandClean);
   }
   if( sName == _T("Build") ) {
      if( !_PrepareProcess(pstrName) ) return false;
      sTitle.LoadString(IDS_BUILD);
      aCommands.Add(m_sCommandCD);
      aCommands.Add(m_sCommandPreStep);
      aCommands.Add(bReleaseMode ? m_sCommandRelease : m_sCommandDebug);
      aCommands.Add(m_sCommandBuild);
      aCommands.Add(m_sCommandPostStep);
      Flags |= COMPFLAG_BUILDSESSION | COMPFLAG_COMPILENOTIFY;
   }
   if( sName == _T("Rebuild") ) {
      if( !_PrepareProcess(pstrName) ) return false;
      sTitle.LoadString(IDS_REBUILD);
      aCommands.Add(m_sCommandCD);
      aCommands.Add(m_sCommandPreStep);
      aCommands.Add(m_sCommandClean);
      aCommands.Add(bReleaseMode ? m_sCommandRelease : m_sCommandDebug);
      aCommands.Add(m_sCommandRebuild);
      aCommands.Add(m_sCommandPostStep);
      Flags |= COMPFLAG_BUILDSESSION | COMPFLAG_COMPILENOTIFY;
   }
   if( sName == _T("Compile") ) {
      if( !_PrepareProcess(pstrName) ) return false;
      sTitle.LoadString(IDS_COMPILE);
      aCommands.Add(m_sCommandCD);
      aCommands.Add(bReleaseMode ? m_sCommandRelease : m_sCommandDebug);
      aCommands.Add(m_sCommandCompile);
      Flags |= COMPFLAG_BUILDSESSION;
   }
   if( sName == _T("CheckSyntax") ) {
      if( !_PrepareProcess(pstrName) ) return false;
      sTitle.LoadString(IDS_CHECKSYNTAX);
      aCommands.Add(m_sCommandCD);
      aCommands.Add(bReleaseMode ? m_sCommandRelease : m_sCommandDebug);
      aCommands.Add(m_sCommandCheckSyntax);
   }
   if( sName == _T("BuildTags") ) {
      if( !_PrepareProcess(pstrName) ) return false;
      sTitle.LoadString(IDS_BUILD);
      aCommands.Add(m_sCommandCD);
      aCommands.Add(bReleaseMode ? m_sCommandRelease : m_sCommandDebug);
      aCommands.Add(m_sCommandBuildTags);
   }
   if( sName == _T("Stop") ) {
      SignalStop();
      m_RebuildThread.SignalStop();
      // Update state...
      m_pProject->DelayedCompilerBroadcast(VT100_RED, CString(MAKEINTRESOURCE(IDS_ERR_BUILDSTOPPED)));
      m_pProject->DelayedCompilerBroadcast(VT100_HIDDEN, TERM_MARKER_LINE);
      m_pProject->DelayedStatusBar(CString(MAKEINTRESOURCE(IDS_STATUS_STOPPED)));
      ::PlaySound(_T("BVRDE_BuildCancelled"), NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT);
      return true;
   }
   if( sName.Find(_T("cc ")) == 0 ) {
      sTitle.LoadString(IDS_EXECUTING);
      aCommands.Add(sName.Mid(3));
      Flags |= COMPFLAG_COMMANDMODE;
   }
   if( sName.Find(_T("cz ")) == 0 ) {
      sTitle.LoadString(IDS_EXECUTING);
      aCommands.Add(sName.Mid(3));
      Flags |= COMPFLAG_COMMANDMODE | COMPFLAG_IGNOREOUTPUT;
   }
   
   // Finally translate the commands (replace meta-tokens)...
   for( int i = 0; i < aCommands.GetSize(); i++ ) {
      aCommands[i] = _TranslateCommand(aCommands[i], pstrParams);
   }

   // Some additional flags
   // Referencing a meta-token in the command (such as $FILENAME$
   // will cause the file to be save and reloaded once the commands
   // are executed.
   if( sName.Find(_T("$FILE")) > 0 ) {
      IView* pView = _pDevEnv->GetActiveView();
      if( pView != NULL ) pView->Save();
      Flags |= COMPFLAG_RELOADFILE;
   }

   // Run the commands on the remote server...
   return _StartProcess(sTitle, aCommands, Flags);
}

CString CCompileManager::GetParam(LPCTSTR pstrName) const
{
   CString sName = pstrName;
   if( sName == _T("ChangeDir") ) return m_sCommandCD;
   if( sName == _T("Build") ) return m_sCommandBuild;
   if( sName == _T("Rebuild") ) return m_sCommandRebuild;
   if( sName == _T("Compile") ) return m_sCommandCompile;
   if( sName == _T("Clean") ) return m_sCommandClean;
   if( sName == _T("CheckSyntax") ) return m_sCommandCheckSyntax;
   if( sName == _T("BuildTags") ) return m_sCommandBuildTags;
   if( sName == _T("PreStep") ) return m_sCommandPreStep;
   if( sName == _T("PostStep") ) return m_sCommandPostStep;
   if( sName == _T("ProcessList") ) return m_sCommandProcessList;
   if( sName == _T("DebugExport") ) return m_sCommandDebug;
   if( sName == _T("ReleaseExport") ) return m_sCommandRelease;
   if( sName == _T("CompileFlags") ) return m_sCompileFlags;
   if( sName == _T("LinkFlags") ) return m_sLinkFlags;
   if( sName == _T("BuildMode") ) return m_sBuildMode;
   if( sName == _T("ActiveProcess") ) return m_sProcessName;
   if( sName == _T("InCommand") ) return (m_Flags & COMPFLAG_COMMANDMODE) != 0 ? _T("true") : _T("false");
   return m_ShellManager.GetParam(pstrName);
}

void CCompileManager::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   if( sName == _T("ChangeDir") ) m_sCommandCD = pstrValue;
   if( sName == _T("Build") ) m_sCommandBuild = pstrValue;
   if( sName == _T("Rebuild") ) m_sCommandRebuild = pstrValue;
   if( sName == _T("Compile") ) m_sCommandCompile = pstrValue;
   if( sName == _T("Clean") ) m_sCommandClean = pstrValue;
   if( sName == _T("CheckSyntax") ) m_sCommandCheckSyntax = pstrValue;
   if( sName == _T("BuildTags") ) m_sCommandBuildTags = pstrValue;
   if( sName == _T("PreStep") ) m_sCommandPreStep = pstrValue;
   if( sName == _T("PostStep") ) m_sCommandPostStep = pstrValue;
   if( sName == _T("ProcessList") ) m_sCommandProcessList = pstrValue;
   if( sName == _T("DebugExport") ) m_sCommandDebug = pstrValue;
   if( sName == _T("ReleaseExport") ) m_sCommandRelease = pstrValue;
   if( sName == _T("CompileFlags") ) m_sCompileFlags = pstrValue;
   if( sName == _T("LinkFlags") ) m_sLinkFlags = pstrValue;
   if( sName == _T("BuildMode") ) m_sBuildMode = pstrValue;
   if( sName == _T("InCommand") ) m_Flags |= _tcscmp(pstrValue, _T("true")) == 0 ? COMPFLAG_COMMANDMODE : 0;
   m_ShellManager.SetParam(pstrName, pstrValue);
}

// Implementation

CString CCompileManager::_TranslateCommand(LPCTSTR pstrCommand, LPCTSTR pstrParam /*= NULL*/) const
{
   if( _tcschr(pstrCommand, '$') == NULL ) return pstrCommand;

   // Replace meta-tokens:
   //   $PROJECTNAME$  = name of project
   //   $PROJECTPATH$  = path where projectfile is located (local)
   //   $PATH$         = source path on remote server
   //   $FILEPATH$     = path of active view
   //   $FILENAME$     = filename of active view
   //   $NAME$         = namepart of filename of active view

   CString sName = pstrCommand;
   TCHAR szProjectName[128] = { 0 };
   m_pProject->GetName(szProjectName, 127);
   TCHAR szProjectPath[MAX_PATH + 1] = { 0 };
   m_pProject->GetPath(szProjectPath, MAX_PATH);

   sName.Replace(_T("$PROJECTNAME$"), szProjectName);
   sName.Replace(_T("$PROJECTPATH$"), szProjectPath);
   sName.Replace(_T("$PATH$"), m_ShellManager.GetParam(_T("Path")));
   sName.Replace(_T("$DRIVE$"), m_ShellManager.GetParam(_T("Path")).Left(2));
   sName.Replace(_T("\\n"), _T("\r\n"));

   TCHAR szName[MAX_PATH + 1] = { 0 };
   if( pstrParam == NULL ) {
      IView* pView = _pDevEnv->GetActiveView();
      if( pView != NULL ) pView->GetFileName(szName, MAX_PATH);
   }
   else {
      _tcscpy(szName, pstrParam);
   }

   sName.Replace(_T("$FILEPATH$"), szName);
   ::PathStripPath(szName);
   sName.Replace(_T("$FILENAME$"), szName);
   ::PathRemoveExtension(szName);
   sName.Replace(_T("$NAME$"), szName);

   return sName;
}

bool CCompileManager::_PrepareProcess(LPCTSTR /*pstrName*/)
{
   // Does project need save before it can run?
   // Let's warn user about this!
   if( m_pProject->IsDirty() || m_pProject->IsViewsDirty() ) 
   {
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

#pragma optimize( "", off )

bool CCompileManager::_StartProcess(LPCTSTR pstrName, CSimpleArray<CString>& aCommands, UINT Flags)
{
   ATLASSERT(m_pProject);
   ATLASSERT(!::IsBadStringPtr(pstrName,-1));
   ATLASSERT(aCommands.GetSize()>0);

   CWaitCursor cursor;
   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);

   m_sProcessName = pstrName;
   m_sUserName = GetParam(_T("Username")) + _T("@");
   if( m_sUserName.GetLength() < 4 ) m_sUserName = _T("user@");

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
   // We won't allow compiling and debugging at the same time!
   // Background tasks, however, are allowed to run since they are not likely
   // to interfere with the debug shell.
   if( m_pProject->m_DebugManager.IsBusy() && (Flags & COMPFLAG_COMMANDMODE) == 0 ) {
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_QUESTION));
      CString sMsg(MAKEINTRESOURCE(IDS_DEBUGGER_BUSY));
      if( ((Flags & COMPFLAG_IGNOREOUTPUT) != 0) || IDNO == _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_YESNO | MB_ICONQUESTION) ) return false;
      m_pProject->m_DebugManager.SignalStop();
      m_pProject->m_DebugManager.Stop();
   }

   // Has the remote host accepted connection?
   if( !m_ShellManager.IsConnected() ) {
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_QUESTION));
      CString sMsg(MAKEINTRESOURCE(IDS_WAITCONNECTION));
      if( ((Flags & COMPFLAG_IGNOREOUTPUT) != 0) || IDNO == _pDevEnv->ShowMessageBox(wndMain, sMsg, sCaption, MB_YESNO | MB_ICONQUESTION) ) return false;
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
   m_pProject->DelayedStatusBar(sStatus);
   long lAnim = (Flags & (COMPFLAG_IGNOREOUTPUT | COMPFLAG_COMMANDMODE)) == 0 ? ANIM_BUILD : ANIM_TRANSFER;
   m_pProject->DelayedGuiAction(LAZY_GUI_PLAY_ANIMATION, NULL, lAnim);

   m_ShellManager.AddLineListener(this);

   // Update execute thread
   // Add new commands to command list and add marker that will end the compile session.
   m_CompileThread.m_cs.Lock();
   m_CompileThread.m_szWindow = _GetViewWindowSize();
   m_CompileThread.m_aFlags.Add(Flags);
   for( int i = 0; i < aCommands.GetSize(); i++ ) {
      CString sCommand = aCommands[i];
      if( !sCommand.IsEmpty() ) m_CompileThread.m_aCommands.Add(sCommand);
   }
   CString sCommand = TERM_MARKER;
   m_CompileThread.m_aCommands.Add(sCommand);
   m_CompileThread.m_cs.Unlock();

   // If this is a regular compile command (not user-entered or scripting prompt)
   // let's pop up the compile window...
   if( (Flags & (COMPFLAG_IGNOREOUTPUT | COMPFLAG_COMMANDMODE)) == 0 ) {
      // We should clean the compile output panel and bring it up
      if( (Flags & COMPFLAG_SILENT) == 0 ) {
         m_pProject->DelayedGuiAction(LAZY_GUI_CLEARVIEW, IDE_HWND_OUTPUTVIEW);
         m_pProject->DelayedGuiAction(LAZY_GUI_ACTIVATEVIEW, IDE_HWND_OUTPUTVIEW);
      }
      // Notify views that an actual compiler session is starting.
      // A "compiler session" may be any makefile or direct gcc issued command.
      // The DEBUG_CMD_COMPILE_START event will cause this class to attach
      // itself as a listener for compiler output. It's delayed to the main
      // GUI thread to prevent horrendous dead-lock situations.
      m_pProject->DelayedGlobalViewMessage(DEBUG_CMD_COMPILE_START);
   }

   // Finally signal that we have new data
   s_bBusy = true;
   m_bCompiling = true;
   m_bWarningPlayed = false;

   // Trigger the compile thread so it will detect the
   // new commands and submit them to the remote server.
   m_event.SetEvent();

   // Notify views
   // The COMPFLAG_COMPILENOTIFY event just tells the project that
   // it can clear various internal states related to the project being
   // fully recompiled.
   if( (Flags & COMPFLAG_COMPILENOTIFY) != 0 ) m_pProject->DelayedGuiAction(LAZY_GUI_COMPILESTART);

   // If this is in Command Mode, we'll be polite and wait for the command to
   // actually be submitted to the remote server before we continue...
   if( (Flags & COMPFLAG_COMMANDMODE) != 0 ) {
      const DWORD COMMAND_MODE_TIMEOUT = 8;
      DWORD dwStartTick = ::GetTickCount();
      while( !m_CompileThread.IsQueueEmpty() ) {
         ::Sleep(50L);
         if( ::GetTickCount() - dwStartTick > COMMAND_MODE_TIMEOUT * 100UL ) break;         
      }
   }

   return true;
}

#pragma optimize( "", on )

SIZE CCompileManager::_GetViewWindowSize() const
{
   // Helper function to determine current width of the compile-pane/window.
   // This will be relayed to the TELNET/SSH session as the window size.
   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_OUTPUTVIEW);
   RECT rc = { 0 };
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

void CCompileManager::AppendOutputText(IDE_HWND_TYPE WindowType, LPCTSTR pstrText, VT100COLOR nColor)
{
   CRichEditCtrl ctrlEdit = _pDevEnv->GetHwnd(WindowType);
   ATLASSERT(ctrlEdit.IsWindow());
   if( !ctrlEdit.IsWindow() ) return;

   SCROLLINFO sinfo = { 0 };
   sinfo.cbSize = sizeof(SCROLLINFO);
   sinfo.fMask = SIF_ALL;
   ctrlEdit.GetScrollInfo(SB_VERT, &sinfo);

   CHARRANGE cr = { 0 };
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

   // Append text...
   ctrlEdit.SetSel(-1, -1);
   LONG iStartPos = 0;
   LONG iDummy = 0;
   ctrlEdit.GetSel(iStartPos, iDummy);
   ctrlEdit.ReplaceSel(pstrText);
   LONG iEndPos = 0;
   ctrlEdit.GetSel(iDummy, iEndPos);

   // Style the text depending on certain search-strings, prompt-prefixes etc
   // TODO: This algorithm is a little too hard-coded for my taste. These are
   //       the strings we know the gcc/g++ compilers are producing. Should be
   //       extended (dynamically configured) somehow!
   CHARFORMAT cf;
   cf.cbSize = sizeof(CHARFORMAT);
   cf.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC;
   cf.dwEffects = 0;
   cf.crTextColor = ::GetSysColor(COLOR_WINDOWTEXT);
   if( nColor == VT100_RED ) cf.crTextColor = RGB(200,0,0);
   if( _tcsstr(pstrText, _T("error:")) != NULL 
       || _tcsstr(pstrText, _T(": error ")) != NULL
       || _tcsstr(pstrText, _T("Error ")) != NULL ) 
   {
      cf.crTextColor = RGB(150,70,0);
      if( !m_bWarningPlayed ) {
         ::PlaySound(_T("BVRDE_OutputError"), NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT | SND_NOSTOP);
         m_bWarningPlayed = true;
      }
   }
   if( _tcsstr(pstrText, _T("warning:")) != NULL 
       || _tcsstr(pstrText, _T("Warning ")) != NULL 
       || _tcsstr(pstrText, _T("not found")) != NULL ) 
   {
      cf.crTextColor = RGB(70,70,0);
      if( !m_bWarningPlayed ) {
         ::PlaySound(_T("BVRDE_OutputWarning"), NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT | SND_NOSTOP);
         m_bWarningPlayed = true;
      }
   }
   if( _tcsncmp(pstrText, _T("make["), 5) == 0 ) cf.dwEffects |= CFE_ITALIC;

   // Mark prompts...
   // Some lines that look like prompts aren't really prompts.
   // TODO: I can't make heads and tails of this anymore, but it does colour most prompts
   //       correctly. Rewrite and make it dynamic. Make option to disable.
   if( _tcsstr(pstrText, m_sUserName) != NULL ) cf.dwEffects |= CFE_BOLD;
   if( _tcschr(m_sPromptPrefix, *pstrText) != NULL ) cf.dwEffects |= CFE_BOLD;
   if( *pstrText == '[' && _tcsstr(pstrText, _T("]$ ")) != NULL ) cf.dwEffects |= CFE_BOLD;
   if( _tcsstr(pstrText, _T(" $ ")) != NULL && _tcschr(pstrText, ':') != NULL ) cf.dwEffects |= CFE_BOLD;
   if( _tcsstr(pstrText, _T("-bash")) == pstrText && _tcschr(pstrText, '$') != NULL ) cf.dwEffects |= CFE_BOLD;
   if( *pstrText == '/' && _tcschr(pstrText, ':') != NULL ) cf.dwEffects &= ~CFE_BOLD;
   if( *pstrText == '/' && _tcsstr(pstrText, _T(" line ")) != NULL ) cf.dwEffects &= ~CFE_BOLD;
   if( nColor == VT100_PROMPT ) cf.dwEffects |= CFE_BOLD;

   ctrlEdit.SetSel(iStartPos, iEndPos);
   ctrlEdit.SetSelectionCharFormat(cf);

   // Count the number of lines in the text
   int nLines = 0;
   while( *pstrText != '\0' && *pstrText++ == '\n' ) nLines++;

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


/////////////////////////////////////////////////////////////////////////
// IOutputLineListener

void CCompileManager::OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText)
{
   ATLASSERT(!::IsBadStringPtr(pstrText,-1));

   CString sText;
   sText.Format(_T("%s\r\n"), pstrText);

   // Is this the termination marker? 
   // Let's finish the session.
   if( sText.Find(TERM_MARKER) > 0 ) 
   {
      m_ShellManager.RemoveLineListener(this);
      // Notify the rest of the environment
      if( (m_Flags & COMPFLAG_SILENT) == 0 ) {
         CString sStatus;
         sStatus.Format(IDS_STATUS_FINISHED, m_sProcessName);
         m_pProject->DelayedStatusBar(sStatus);
      }
      m_pProject->DelayedGuiAction(LAZY_GUI_STOP_ANIMATION);
      m_pProject->DelayedGlobalViewMessage(DEBUG_CMD_COMPILE_STOP);
      // Play annoying build sound
      if( (m_Flags & COMPFLAG_BUILDSESSION) != 0 ) ::PlaySound(_T("BVRDE_BuildSucceeded"), NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT);
      else ::PlaySound(_T("BVRDE_CommandComplete"), NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT);
      // Reset state
      m_Flags = 0;
      s_bBusy = false;
      m_bCompiling = false;
      return;
   }

   if( nColor == VT100_HIDDEN ) return;
   if( sText.Find(TERM_MARKER) == 0 ) return;

   // Output text.
   // To avoid a GUI dead-lock we'll have to make sure that access of the actual
   // output window is made in the GUI thread only - so we delay the text processing.
   IDE_HWND_TYPE WindowType = IDE_HWND_OUTPUTVIEW;
   if( (m_Flags & COMPFLAG_COMMANDMODE) != 0 ) WindowType = IDE_HWND_COMMANDVIEW;
   if( (m_Flags & COMPFLAG_IGNOREOUTPUT) != 0 ) return;
   m_pProject->DelayedGuiAction(LAZY_GUI_PRINTOUTPUT, WindowType, sText, nColor);
}

