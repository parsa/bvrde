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

#define COMPFLAG_COMMANDMODE    0x00000001
#define COMPFLAG_IGNOREOUTPUT   0x00000002
#define COMPFLAG_BUILDSESSION   0x00000004
#define COMPFLAG_RELOADFILE     0x00000008
#define COMPFLAG_SILENT         0x00000010


////////////////////////////////////////////////////////
//

class CRebuildThread : public CThreadImpl<CRebuildThread>
{
public:
   DWORD Run();

   CRemoteProject* m_pProject;
   CCompileManager* m_pManager;
};


////////////////////////////////////////////////////////
//

class CCompileThread : public CThreadImpl<CCompileThread>
{
public:
   CCompileThread();

   DWORD Run();

   CRemoteProject* m_pProject;
   CCompileManager* m_pManager;
   CComAutoCriticalSection m_cs;           // Thread synch lock
   CSimpleArray<CString> m_aCommands;      // List of commands waiting for execution
   SIZE m_szWindow;                        // Initial size of output window
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

   bool DoAction(LPCTSTR pstrName, LPCTSTR pstrParams = NULL, UINT Flags = 0);
   bool DoRebuild();

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
   CCompileThread m_CompileThread;         // Thread that pumps Compile commands
   CRebuildThread m_RebuildThread;         // Thread that controls a Solution Rebuild
   CString m_sProcessName;                 // Name of program (Compile, Rebuild etc)
   CEvent m_event;                         // Event that triggers command/batch execution
   static volatile bool s_bBusy;           // Flag signals thread busy state
   volatile bool m_bCompiling;             // Are we currently compiling?
   bool m_bWarningPlayed;                  // Error warning sound played once?
   CString m_sBuildMode;                   // Currently set to Debug or Release?
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

