#if !defined(AFX_DEBUGMANAGER_H__20030506_F7F6_1BCA_D2DB_0080AD509054__INCLUDED_)
#define AFX_DEBUGMANAGER_H__20030506_F7F6_1BCA_D2DB_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ShellProxy.h"

////////////////////////////////////////////////////////
//

class CDebugManager;


class CDebugStopThread : public CThreadImpl<CDebugStopThread>
{
public:
   DWORD Run();

   void Init(CDebugManager* pManager);

   CDebugManager* m_pManager;
};


////////////////////////////////////////////////////////
//

/**
 * @class CDebugManager
 * Manages a debug session.
 */
class CDebugManager : 
   public IOutputLineListener
{
public:
   CDebugManager();
   virtual ~CDebugManager();

// Attributtes
public:
   CShellManager m_ShellManager;
   CDebugStopThread m_StopThread;

private:
   CRemoteProject* m_pProject;
   IDebuggerAdaptor* m_pAdaptor;
   CSimpleMap<CString, long> m_aBreakpoints;   /// Breakpoints; key=<filename:lineno> value=<break-nr>
   CEvent m_eventAck;                          /// New debug information is available?
   volatile int m_nDebugAck;                   /// No of debug acknoledge
   volatile int m_nLastAck;                    /// Last known acknoledge
   volatile int m_nIgnoreErrors;               /// Ignore GDB error report?
   volatile int m_nIgnoreBreaks;               /// Ignore doing visual updates when breaking?
   bool m_bBreaked;                            /// Is debugging, but currently breaked?
   bool m_bDebugging;                          /// Is currently debugging?
   bool m_bCommandMode;                        /// In Command mode?
   bool m_bRunning;                            /// Process or debugger is running?
   bool m_bSeenExit;                           /// Did we see a proper GDB exit?
   bool m_bDebugEvents;                        /// Listens for GDB debug events?
   CString m_sVarName;                         /// Data-evaluation variable name
   //
   CString m_sDebuggerType;                    /// What type of debugger do we speak to?
   CString m_sCommandCD;                       /// Syntax for changing path in shell
   CString m_sAppExecutable;                   /// Syntax for launching the application
   CString m_sAppArgs;                         /// Current arguments for debugged application
   CString m_sDebuggerExecutable;              /// Syntax for launching the debugger
   CString m_sAttachExe;                       /// Syntax for attaching to application in debugger
   CString m_sAttachCore;                      /// Syntax for attaching to core file in debugger
   CString m_sAttachPid;                       /// Syntax for attaching to PID in debugger
   CString m_sDebugMain;                       /// Name of "main" function in application (for first temp. breakpoint)
   CString m_sCoreProcess;                     /// Name of process being debugger in core-debug session
   CString m_sCoreFile;                        /// Name of core-file being debugger in core-debug session
   CString m_sSearchPath;                      /// Include path
   long m_lStartTimeout;                       /// Seconds to wait for debugger to start
   BOOL m_bMaintainSession;                    /// Should we shut down shell after debugger exit?
   double m_dblDebuggerVersion;                /// Version number of debugger

// Operations
public:
   void Clear();
   void Init(CRemoteProject* pProject);
   bool Load(ISerializable* pArc);
   bool Save(ISerializable* pArc);

   bool RunNormal();
   bool AttachProcess(long lPID);
   bool AttachCoreFile(LPCTSTR pstrProcess, LPCTSTR pstrCoreFilename);
   bool RunContinue();
   bool RunDebug();
   bool Break();
   bool DoDebugCommand(LPCTSTR pstrText);
   bool DoDebugCommandV(LPCTSTR pstrText, ...);
   bool DoSignal(BYTE bCmd);
   void ProgramStop();
   void SignalStop();
   bool Stop();
   bool IsBusy() const;
   bool IsBreaked() const;
   bool IsDebugging() const;

   bool ClearBreakpoints();
   bool AddBreakpoint(LPCTSTR pstrFilename, int iLineNum);
   bool RemoveBreakpoint(LPCTSTR pstrFilename, int iLineNum);
   bool GetBreakpoints(LPCTSTR pstrFilename, CSimpleArray<int>& aLines) const;
   bool SetBreakpoints(LPCTSTR pstrFilename, CSimpleArray<int>& aLines);

   bool RunTo(LPCTSTR pstrFilename, int iLineNum);
   bool SetNextStatement(LPCTSTR pstrFilename, int iLineNum);

   bool EvaluateExpression(LPCTSTR pstrValue);

   CString GetParam(LPCTSTR pstrName) const;
   void SetParam(LPCTSTR pstrName, LPCTSTR pstrValue);

// IOutputLineListener
public:
   void OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText);

// Implementation
private:
   bool _CheckStatus();
   bool _PauseDebugger();
   void _ResumeDebugger();
   bool _WaitForDebuggerStart();
   void _ClearLink();
   bool _AttachDebugger(CSimpleArray<CString>& aCommands, bool bExternalProcess);
   CString _TranslateCommand(LPCTSTR pstrCommand, LPCTSTR pstrParam = NULL);
   void _ParseNewFrame(CMiInfo& info);
   void _UpdateBreakpoint(CMiInfo& info);
   void _ParseOutOfBand(LPCTSTR pstrText);
   void _ParseResultRecord(LPCTSTR pstrText);
   void _ParseConsoleOutput(LPCTSTR pstrText);
   void _ParseTargetOutput(LPCTSTR pstrText);
   void _ParseNotifyAsync(LPCTSTR pstrText);
   void _ParseLogOutput(LPCTSTR pstrText);
   void _ParseKeyPrompt(LPCTSTR pstrText);
};


#endif // !defined(AFX_DEBUGMANAGER_H__20030506_F7F6_1BCA_D2DB_0080AD509054__INCLUDED_)
