#if !defined(AFX_DEBUGMANAGER_H__20030506_F7F6_1BCA_D2DB_0080AD509054__INCLUDED_)
#define AFX_DEBUGMANAGER_H__20030506_F7F6_1BCA_D2DB_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ShellProxy.h"


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

private:
   CRemoteProject* m_pProject;
   CSimpleMap<CString, long> m_aBreakpoints;   // Breakpoints; key=<filename:lineno> value=<break-nr>
   volatile int m_nDebugAck;                   // No of debug acknoledge
   volatile int m_nLastAck;                    // Last known acknoledge
   volatile int m_nIgnoreErrors;               // Ignore GDB error report?
   bool m_bBreaked;                            // Is debugging, but currently breaked?
   bool m_bDebugging;                          // Is debugging?
   bool m_bCommandMode;                        // In Command mode?
   CString m_sVarName;                         // Data-evaluation variable name
   //
   CString m_sCommandCD;
   CString m_sAppExecutable;
   CString m_sAppArgs;
   CString m_sDebuggerExecutable;
   CString m_sDebuggerArgs;
   CString m_sDebugMain;

// Operations
public:
   void Clear();
   void Init(CRemoteProject* pProject);
   bool Load(ISerializable* pArc);
   bool Save(ISerializable* pArc);

   bool RunNormal();
   bool RunContinue();
   bool RunDebug();
   bool Break();
   bool DoDebugCommand(LPCTSTR pstrText);
   bool DoSignal(BYTE bCmd);
   void SignalStop();
   bool Stop();
   bool IsBusy() const;
   bool IsBreaked() const;
   bool IsDebugging() const;

   bool ClearBreakpoints();
   bool AddBreakpoint(LPCTSTR pstrText);
   bool RemoveBreakpoint(LPCTSTR pstrText);
   bool GetBreakpoints(LPCTSTR pstrFilename, CSimpleArray<long>& aLines) const;
   bool SetBreakpoints(LPCTSTR pstrFilename, CSimpleArray<CString>& aBreakpoints);

   bool RunTo(LPCTSTR pstrFilename, long lLineNum);
   bool SetNextStatement(LPCTSTR pstrFilename, long lLineNum);

   CString GetTagInfo(LPCTSTR pstrValue);

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
   CString _TranslateCommand(LPCTSTR pstrCommand, LPCTSTR pstrParam = NULL);
   void _ParseNewFrame(CMiInfo& info);
   void _UpdateBreakpoint(CMiInfo& info);
   void _ParseOutOfBand(LPCTSTR pstrText);
   void _ParseResultRecord(LPCTSTR pstrText);
   void _ParseConsoleOutput(LPCTSTR pstrText);
   void _ParseTargetOutput(LPCTSTR pstrText);
   void _ParseLogOutput(LPCTSTR pstrText);
};


#endif // !defined(AFX_DEBUGMANAGER_H__20030506_F7F6_1BCA_D2DB_0080AD509054__INCLUDED_)
