#if !defined(AFX_COMPILETHREAD_H__20030330_ADCE_5CFE_D70A_0080AD509054__INCLUDED_)
#define AFX_COMPILETHREAD_H__20030330_ADCE_5CFE_D70A_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ShellProxy.h"


////////////////////////////////////////////////////////
//

// Forward declare
class CCompileManager;

#define COMPFLAG_COMMANDMODE    1
#define COMPFLAG_IGNOREOUTPUT   2
#define COMPFLAG_BUILDSESSION   4
#define COMPFLAG_RELOADFILE     8


class CCompileThread : public CThreadImpl<CCompileThread>
{
public:
   CCompileThread();

   DWORD Run();

   CRemoteProject* m_pProject;
   CCompileManager* m_pManager;
   CComAutoCriticalSection m_cs;           // Thread synch lock
   SIZE m_szWindow;                        // Initial size of output window
   CSimpleArray<CString> m_aCommands;      // List of commands waiting for execution
   UINT m_Flags;                           // Various COMPFLAG_xxx flags
};


////////////////////////////////////////////////////////
//

/**
 * @class CCompileManager
 * Manages a build/compile session.
 */
class CCompileManager : 
   public IOutputLineListener
{
friend CCompileThread;
public:
   CCompileManager();

   // Operations

   void Init(CRemoteProject* pProject);

   // ICompileManager

   void Clear();
   bool Load(ISerializable* pArc);
   bool Save(ISerializable* pArc);
   bool Start();
   bool Stop();
   void SignalStop();
   bool IsBusy() const;
   bool IsCompiling() const;
   bool IsConnected() const;

   bool DoAction(LPCTSTR pstrName, LPCTSTR pstrParams = NULL);
   CString GetParam(LPCTSTR pstrName) const;
   void SetParam(LPCTSTR pstrName, LPCTSTR pstrValue);

   // Implementation

   SIZE _GetViewWindowSize() const;
   bool _PrepareProcess(LPCTSTR /*pstrName*/);
   bool _StartProcess(LPCTSTR pstrName, CSimpleArray<CString>& aCommands, UINT Flags);
   CString _TranslateCommand(LPCTSTR pstrAction, LPCTSTR pstrParams = NULL) const;

   // IOutputLineListener

   void OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText);

public:
   CRemoteProject* m_pProject;             // Reference to project
   CShellManager m_ShellManager;           // Command Prompt connection (protocol specific)
   CCompileThread m_thread;                // Thread that pumps commands
   CString m_sProcessName;                 // Name of program (Compile, Rebuild etc)
   CEvent m_event;                         // Event that triggers command/batch execution
   static volatile bool s_bBusy;           // Flag signals thread busy state
   bool m_bReleaseMode;                    // Project is in Release (combobox on toolbar)
   bool m_bCompiling;                      // Are we currently compiling?
   bool m_bWarningPlayed;                  // Error warning sound played once?
   UINT m_Flags;                           // Various state flags for current compile batch
   //
   CString m_sCommandCD;
   CString m_sCommandBuild;
   CString m_sCommandRebuild;
   CString m_sCommandCompile;
   CString m_sCommandClean;
   CString m_sCommandCheckSyntax;
   CString m_sCommandBuildTags;
   CString m_sCommandDebug;
   CString m_sCommandRelease;
   CString m_sPromptPrefix;
   CString m_sCompileFlags;
   CString m_sLinkFlags;
};


#endif // !defined(AFX_COMPILETHREAD_H__20030330_ADCE_5CFE_D70A_0080AD509054__INCLUDED_)

