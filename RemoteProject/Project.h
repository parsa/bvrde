#if !defined(AFX_REMOTEPROJECT_H__20030307_E777_9147_A664_0080AD509054__INCLUDED_)
#define AFX_REMOTEPROJECT_H__20030307_E777_9147_A664_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

#include "Files.h"
#include "FileProxy.h"
#include "CompileManager.h"
#include "DebugManager.h"
#include "DockManager.h"
#include "TagProxy.h"

#include "ObjectModel.h"

#include "TelnetView.h"
#include "ClassView.h"
#include "VariableView.h"
#include "StackView.h"
#include "WatchView.h"
#include "BreakpointView.h"
#include "RegisterView.h"
#include "MemoryView.h"
#include "DisasmView.h"
#include "ThreadView.h"
#include "RemoteDirView.h"
#include "QuickWatchDlg.h"

#include "Commands.h"

#include "WizardSheet.h"


class CRemoteProject : 
   public IProject,
   virtual public IAppMessageListener,
   virtual public IIdleListener,
   virtual public ITreeMessageListener,
   virtual public IWizardListener,
   virtual public ICustomCommandListener
{
public:
   CRemoteProject();
   virtual ~CRemoteProject();

// Attributes
public:
   CWindow m_wndMain;
   bool m_bLoaded;
   //
   CFileManager m_FileManager;
   CCompileManager m_CompileManager;
   CDebugManager m_DebugManager;
   CTagManager m_TagManager;
   CProjectOM m_Dispatch;
   CDockManager m_DockManager;

private:
   CString m_sName;
   CString m_sPath;
   CSimpleArray<LAZYDATA> m_aLazyData;
   CSimpleValArray<IView*> m_aFiles;
   CSimpleValArray<IView*> m_aDependencies;
   CQuickWatchDlg* m_pQuickWatchDlg;
   bool m_bIsDirty;
   //
   static CAccelerator m_accel;
   static CComboBox m_ctrlMode;
   static CToolBarCtrl m_ctrlBuild;
   static CToolBarCtrl m_ctrlDebug;
   static CToolBarCtrl m_ctrlBookmarks;
   static CToolBarCtrl m_ctrlSearch;
   static CTelnetView m_viewCompileLog;
   static CClassView m_viewClassTree;
   static CTelnetView m_viewDebugLog;
   static CWatchView m_viewWatch;
   static CStackView m_viewStack;
   static CBreakpointView m_viewBreakpoint;
   static CRegisterView m_viewRegister;
   static CMemoryView m_viewMemory;
   static CDisasmView m_viewDisassembly;
   static CVariableView m_viewVariable;
   static CThreadView m_viewThread;
   static CRemoteDirView m_viewRemoteDir;

// IElement
public:
   BOOL Load(ISerializable* pArchive);
   BOOL Save(ISerializable* pArchive);
   BOOL GetName(LPTSTR pstrName, UINT cchMax) const;
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
   IDispatch* GetDispatch();

// IProject
public:
   BOOL Initialize(IDevEnv* pEnv, LPCTSTR pstrPath);
   BOOL IsDirty() const;
   BOOL GetFileName(LPTSTR pstrFilename, UINT cchMax) const;
   BOOL GetClass(LPTSTR pstrType, UINT cchMax) const;
   BOOL Close();
   INT GetItemCount() const;
   IView* GetItem(INT iIndex);
   BOOL CreateProject();
   void ActivateProject();
   void DeactivateProject();
   void ActivateUI();
   void DeactivateUI();

// IAppMessageListener
public:
   LRESULT OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   BOOL PreTranslateMessage(MSG* pMsg);

// IIdleListener
public:
   void OnIdle(IUpdateUI* pUIBase);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);

// ITreeMessageListener
public:
   LRESULT OnTreeMessage(LPNMHDR pnmh, BOOL& bHandled);

// IWizardListener
public:
   BOOL OnInitProperties(IWizardManager* pManager, IElement* pElement);
   BOOL OnInitWizard(IWizardManager* pManager, IProject* pProject, LPCTSTR pstrName);
   BOOL OnInitOptions(IWizardManager* pManager, ISerializable* pArc);

// ICustomCommandListener
public:
   void OnUserCommand(LPCTSTR pstrCommand, BOOL& bHandled);
   void OnMenuCommand(LPCTSTR pstrType, LPCTSTR pstrCommand, LPCTSTR pstrArguments, LPCTSTR pstrPath, int iFlags, BOOL& bHandled);

// Operations
public:
   BOOL Reset();
   bool OpenView(LPCTSTR pstrFilename, long lLineNum);
   IView* FindView(LPCTSTR pstrFilename, bool bLocally = false) const;
   void SendViewMessage(UINT nCmd, LAZYDATA* pData);
   CClassView* GetClassView() const;
   CTelnetView* GetDebugView() const;
   BOOL SetName(LPCTSTR pstrName);
   BOOL GetPath(LPTSTR pstrPath, UINT cchMax) const;
   bool GetTagInfo(LPCTSTR pstrValue, bool bAskDebugger, CSimpleArray<CString>& aResult, LPCTSTR pstrOwner /*= NULL*/);
   //
   void DelayedStatusBar(LPCTSTR pstrText);
   void DelayedOpenView(LPCTSTR pstrFilename, long lLineNum);
   void DelayedDebugCommand(LPCTSTR pstrCommand);
   void DelayedMessage(LPCTSTR pstrMessage, LPCTSTR pstrCaption, UINT iFlags);
   void DelayedGuiAction(UINT iAction, HWND hWnd);
   void DelayedGuiAction(UINT iAction, LPCTSTR pstrFilename = NULL, long lLineNum = -1);
   void DelayedViewMessage(WPARAM wCmd, LPCTSTR pstrFilename = NULL, long lLineNum = -1, UINT iFlags = 0);
   void DelayedDebugBreakpoint(LPCTSTR pstrFilename, long lLineNum);
   void DelayedDebugEvent(LAZYACTION event = LAZY_DEBUG_BREAK_EVENT);
   void DelayedDebugInfo(LPCTSTR pstrCommand, CMiInfo& info);
   //
   static void InitializeToolBars();

// Message map and handlers
public:
   BEGIN_MSG_MAP(CRemoteProject)
      COMMAND_ID_HANDLER(ID_FILE_DELETE, OnFileRemove)
      COMMAND_ID_HANDLER(ID_FILE_RENAME, OnFileRename)      
      COMMAND_ID_HANDLER(ID_FILE_ADD_FOLDER, OnFileAddFolder)      
      COMMAND_ID_HANDLER(ID_FILE_ADD_LOCAL, OnFileAddLocal) 
      COMMAND_ID_HANDLER(ID_FILE_ADD_REMOTE, OnFileAddRemote)            
      COMMAND_ID_HANDLER(ID_EDIT_BREAK, OnEditBreak)
      COMMAND_ID_HANDLER(ID_VIEW_OPEN, OnViewOpen)
      COMMAND_ID_HANDLER(ID_VIEW_COMPILE_LOG, OnViewCompileLog) 
      COMMAND_ID_HANDLER(ID_VIEW_DEBUG_LOG, OnViewDebugLog) 
      COMMAND_ID_HANDLER(ID_VIEW_PROPERTIES, OnViewProperties)
      COMMAND_ID_HANDLER(ID_VIEW_THREADS, OnViewThreads)
      COMMAND_ID_HANDLER(ID_VIEW_REGISTERS, OnViewRegisters)
      COMMAND_ID_HANDLER(ID_VIEW_MEMORY, OnViewMemory)
      COMMAND_ID_HANDLER(ID_VIEW_DISASM, OnViewDisassembly)
      COMMAND_ID_HANDLER(ID_VIEW_BREAKPOINTS, OnViewBreakpoints)
      COMMAND_ID_HANDLER(ID_VIEW_WATCH, OnViewWatch)
      COMMAND_ID_HANDLER(ID_VIEW_VARIABLES, OnViewVariables)
      COMMAND_ID_HANDLER(ID_VIEW_CALLSTACK, OnViewStack)
      COMMAND_ID_HANDLER(ID_VIEW_FILEMANAGER, OnViewRemoteDir)
      COMMAND_ID_HANDLER(ID_PROJECT_SET_DEFAULT, OnProjectSetDefault)      
      COMMAND_ID_HANDLER(ID_DEBUG_START, OnDebugStart)
      COMMAND_ID_HANDLER(ID_DEBUG_DEBUG, OnDebugDebug)
      COMMAND_ID_HANDLER(ID_DEBUG_BREAK, OnDebugBreak)
      COMMAND_ID_HANDLER(ID_DEBUG_STOP, OnDebugStop)
      COMMAND_ID_HANDLER(ID_DEBUG_STEP_INTO, OnDebugStepInto)
      COMMAND_ID_HANDLER(ID_DEBUG_STEP_OVER, OnDebugStepOver)      
      COMMAND_ID_HANDLER(ID_DEBUG_STEP_OUT, OnDebugStepOut)
      COMMAND_ID_HANDLER(ID_DEBUG_CLEAR_BREAKPOINTS, OnDebugClearBreakpoints)     
      COMMAND_ID_HANDLER(ID_DEBUG_QUICKWATCH, OnDebugQuickWatch)
      COMMAND_ID_HANDLER(ID_DEBUG_PROCESSES, OnDebugProcesses)
      COMMAND_ID_HANDLER(ID_DEBUG_ARGUMENTS, OnDebugArguments)
      COMMAND_ID_HANDLER(ID_BUILD_CLEAN, OnBuildClean)
      COMMAND_ID_HANDLER(ID_BUILD_PROJECT, OnBuildProject)
      COMMAND_ID_HANDLER(ID_BUILD_REBUILD, OnBuildRebuild)
      COMMAND_ID_HANDLER(ID_BUILD_SOLUTION, OnBuildSolution)
      COMMAND_ID_HANDLER(ID_BUILD_COMPILE, OnBuildCompile)
      COMMAND_ID_HANDLER(ID_BUILD_CHECKSYNTAX, OnBuildCheckSyntax)
      COMMAND_ID_HANDLER(ID_BUILD_BUILDMAKEFILE, OnBuildMakefile)
      COMMAND_ID_HANDLER(ID_BUILD_FILEWIZARD, OnBuildFileWizard)
      COMMAND_ID_HANDLER(ID_BUILD_BUILDTAGS, OnBuildTags)
      COMMAND_ID_HANDLER(ID_BUILD_STOP, OnBuildStop)
      COMMAND_ID_HANDLER(ID_PROCESS, OnProcess)
      NOTIFY_CODE_HANDLER(TBN_DROPDOWN, OnToolBarDropDown)
      NOTIFY_HANDLER(IDC_TREE, TVN_BEGINLABELEDIT, OnTreeLabelBegin)
      NOTIFY_HANDLER(IDC_TREE, TVN_ENDLABELEDIT, OnTreeLabelEdit)
      NOTIFY_HANDLER(IDC_TREE, TVN_KEYDOWN, OnTreeKeyDown)
      NOTIFY_HANDLER(IDC_TREE, NM_DBLCLK, OnTreeDblClick)
      NOTIFY_HANDLER(IDC_TREE, NM_RCLICK, OnTreeRClick)
   END_MSG_MAP()

   LRESULT OnFileRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileRename(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileAddFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileAddLocal(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileAddRemote(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditBreak(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewCompileLog(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewDebugLog(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewThreads(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewBreakpoints(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewRegisters(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewMemory(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewDisassembly(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewVariables(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewWatch(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewStack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewRemoteDir(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnProjectSetDefault(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugStart(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugDebug(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugBreak(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugStop(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugStepOver(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugStepInto(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugStepOut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugClearBreakpoints(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugQuickWatch(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugProcesses(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugArguments(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnBuildClean(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnBuildProject(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnBuildRebuild(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnBuildSolution(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnBuildCompile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnBuildCheckSyntax(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnBuildTags(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnBuildMakefile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnBuildFileWizard(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnBuildStop(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnToolBarDropDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeLabelBegin(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeLabelEdit(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeRClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnProcess(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

// Implementation
protected:
   enum
   {
      IMAGE_FOLDER = 0,
      IMAGE_TEXT = 4,
      IMAGE_JAVA = 3,
      IMAGE_BASIC = 10,
      IMAGE_HEADER = 41,
      IMAGE_CPP = 15,
      IMAGE_XML = 17,
      IMAGE_HTML = 18,
      IMAGE_MAKEFILE = 28,
   };

   void _InitializeData();
   bool _ShouldProcessMessage() const;
   bool _LoadSettings(ISerializable* pArc);
   bool _SaveSettings(ISerializable* pArc);
   bool _LoadFiles(ISerializable* pArc, IElement* pParent);
   bool _SaveFiles(ISerializable* pArc, IElement* pParent);
   void _PopulateTree(CTreeViewCtrl& ctrlTree, IElement* pParent, HTREEITEM hParent) const;
   bool _RunFileWizard(HWND hWnd, LPCTSTR pstrName, IView* pView);
   bool _CheckProjectFile(LPCTSTR pstrFilename, LPCTSTR pstrName, bool bRemote);
   IView* _CreateDependencyFile(LPCTSTR pstrFilename, LPCTSTR pstrName);
   IElement* _GetSelectedTreeElement(HTREEITEM* phItem = NULL) const;
   IElement* _GetDropTreeElement(HTREEITEM* phItem = NULL) const;
   UINT _GetMenuPosFromID(HMENU hMenu, UINT ID) const;
   int _GetElementImage(IElement* pElement) const;
   bool _CreateNewRemoteFile(HWND hWnd, IView* pElement);
   bool _AddCommandBarImages(UINT nRes) const;
   static void _AddDropDownButton(CToolBarCtrl tb, UINT nID);
   static void _AddButtonText(CToolBarCtrl tb, UINT nID, UINT nRes);
   static void _AddControlToToolbar(CToolBarCtrl tb, HWND hWnd, USHORT cx, UINT nCmdPos, bool bIsCommandId, bool bInsertBefore = true);

   static int CALLBACK _SortTreeCB(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
};

EXTERN_C IView* WINAPI Plugin_CreateView(LPCTSTR, IProject*, IElement*);


#endif // !defined(AFX_REMOTEPROJECT_H__20030307_E777_9147_A664_0080AD509054__INCLUDED_)
