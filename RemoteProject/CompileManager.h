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

// Compil flags
#define COMPFLAG_COMMANDMODE    0x00000001
#define COMPFLAG_IGNOREOUTPUT   0x00000002
#define COMPFLAG_BUILDSESSION   0x00000004
#define COMPFLAG_RELOADFILE     0x00000008
#define COMPFLAG_SILENT         0x00000010
#define COMPFLAG_COMPILENOTIFY  0x00000020


////////////////////////////////////////////////////////
//

/**
 * @class CRebuildThread
 * Handles rebuild of an entire solution.
 */
class CRebuildThread : public CThreadImpl<CRebuildThread>
{
public:
   DWORD Run();

   CRemoteProject* m_pProject;
   CCompileManager* m_pManager;
};


////////////////////////////////////////////////////////
//

/**
 * @class CCompileThread
 * Handle compile commands (rebuild, compile, check syntax) etc. It executes
 * a queue of command-line commands.
 */
class CCompileThread : public CThreadImpl<CCompileThread>
{
public:
   CCompileThread();

   DWORD Run();

   bool IsQueueEmpty();

   CRemoteProject* m_pProject;
   CCompileManager* m_pManager;
   CComAutoCriticalSection m_cs;           /// Thread synch lock
   CSimpleArray<CString> m_aCommands;      /// List of commands waiting for execution
   CSimpleArray<UINT> m_aFlags;            /// List of compiler flags
   SIZE m_szWindow;                        /// Initial size of output window
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

   void Clear();
   bool Load(ISerializable* pArc);
   bool Save(ISerializable* pArc);
   bool Start();
   bool Stop();
   void SignalStop();
   bool IsBusy();
   bool IsCompiling();
   bool IsConnected();

   bool DoAction(LPCTSTR pstrName, LPCTSTR pstrParams = NULL, UINT Flags = 0);
   bool DoRebuild();

   CString GetParam(LPCTSTR pstrName) const;
   void SetParam(LPCTSTR pstrName, LPCTSTR pstrValue);

   void AppendOutputText(IDE_HWND_TYPE WindowType, LPCTSTR pstrText, VT100COLOR Color);

   // Implementation

   SIZE _GetViewWindowSize() const;
   bool _PrepareProcess(LPCTSTR /*pstrName*/);
   bool _StartProcess(LPCTSTR pstrName, CSimpleArray<CString>& aCommands, UINT Flags);
   CString _TranslateCommand(LPCTSTR pstrAction, LPCTSTR pstrParams = NULL) const;

   // IOutputLineListener

   void OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText);

public:
   CRemoteProject* m_pProject;             /// Reference to project
   CShellManager m_ShellManager;           /// Command Prompt connection (protocol specific)
   CCompileThread m_CompileThread;         /// Thread that pumps Compile commands
   CRebuildThread m_RebuildThread;         /// Thread that controls a Solution Rebuild
   CString m_sProcessName;                 /// Name of program (Compile, Rebuild etc)
   CString m_sUserName;                    /// Name of login user
   CEvent m_event;                         /// Event that triggers command/batch execution
   static volatile bool s_bBusy;           /// Flag signals thread busy state
   volatile bool m_bCompiling;             /// Are we currently compiling?
   bool m_bWarningPlayed;                  /// Error warning sound played once?
   CString m_sBuildMode;                   /// Currently set to Debug or Release?
   UINT m_Flags;                           /// Various state flags for current compile batch
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
   CString m_sCommandPreStep;
   CString m_sCommandPostStep;
   CString m_sCommandProcessList;
   CString m_sPromptPrefix;
   CString m_sCompileFlags;
   CString m_sLinkFlags;
};


#endif // !defined(AFX_COMPILETHREAD_H__20030330_ADCE_5CFE_D70A_0080AD509054__INCLUDED_)

