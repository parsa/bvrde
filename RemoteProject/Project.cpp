
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"
#include "RemoteFileDlg.h"
#include "ViewSerializer.h"


EXTERN_C IView* WINAPI Plugin_CreateView(LPCTSTR, IProject*, IElement*);


// Constructor / destructor

CRemoteProject::CRemoteProject() :
   m_Dispatch(this),
   m_pQuickWatchDlg(NULL),
   m_bLoaded(false),
   m_bIsDirty(false)
{
   SecClearPassword();
}

CRemoteProject::~CRemoteProject()
{
   Close();
}

// IProject

BOOL CRemoteProject::Initialize(IDevEnv* pEnv, LPCTSTR pstrPath)
{
   m_bLoaded = true;

   m_wndMain = pEnv->GetHwnd(IDE_HWND_MAIN);
   ATLASSERT(m_wndMain.IsWindow());

   m_sPath = pstrPath;

   _InitializeData();

   _pDevEnv->AddAppListener(this);
   _pDevEnv->AddTreeListener(this);
   _pDevEnv->AddWizardListener(this);

   m_CompileManager.Init(this);
   m_DebugManager.Init(this);
   m_TagManager.Init(this);

   return TRUE;
}

BOOL CRemoteProject::Close()
{
   if( !m_bLoaded ) return TRUE;
   m_bLoaded = false;

   _pDevEnv->RemoveAppListener(this);
   _pDevEnv->RemoveTreeListener(this);
   _pDevEnv->RemoveIdleListener(this);
   _pDevEnv->RemoveWizardListener(this);

   m_FileManager.SignalStop();
   m_DebugManager.SignalStop();
   m_CompileManager.SignalStop();

   m_viewDebugLog.Close();
   m_viewClassTree.Close();
   m_viewCompileLog.Close();

   /*
   _pDevEnv->RemoveDockView(m_viewStack);
   _pDevEnv->RemoveDockView(m_viewVariable);
   _pDevEnv->RemoveDockView(m_viewRegister);
   _pDevEnv->RemoveDockView(m_viewMemory);
   _pDevEnv->RemoveDockView(m_viewDisassembly);
   _pDevEnv->RemoveDockView(m_viewWatch);
   _pDevEnv->RemoveDockView(m_viewThread);
   _pDevEnv->RemoveDockView(m_viewBreakpoint);
   _pDevEnv->RemoveDockView(m_viewDebugLog);
   _pDevEnv->RemoveDockView(m_viewCompileLog);

   if( m_viewStack.IsWindow() ) m_viewStack.DestroyWindow();
   if( m_viewVariable.IsWindow() ) m_viewVariable.DestroyWindow();
   if( m_viewRegister.IsWindow() ) m_viewRegister.DestroyWindow();
   if( m_viewMemory.IsWindow() ) m_viewMemory.DestroyWindow();
   if( m_viewDisassembly.IsWindow() ) m_viewDisassembly.DestroyWindow();
   if( m_viewWatch.IsWindow() ) m_viewWatch.DestroyWindow();
   if( m_viewThread.IsWindow() ) m_viewThread.DestroyWindow();
   if( m_viewBreakpoint.IsWindow() ) m_viewBreakpoint.DestroyWindow();
   if( m_viewDebugLog.IsWindow() ) m_viewDebugLog.DestroyWindow();
   if( m_viewCompileLog.IsWindow() ) m_viewCompileLog.DestroyWindow();
   */

   Reset();
   m_FileManager.Stop();
   m_DebugManager.Stop();
   m_CompileManager.Stop();

   if( m_pQuickWatchDlg ) delete m_pQuickWatchDlg;
   m_pQuickWatchDlg = NULL;

   return TRUE;
}

BOOL CRemoteProject::GetName(LPTSTR pstrName, UINT cchMax) const
{
   _tcsncpy(pstrName, m_sName, cchMax);
   return TRUE;
}

BOOL CRemoteProject::SetName(LPCTSTR pstrName)
{
   ATLASSERT(!::IsBadStringPtr(pstrName,-1));
   m_sName = pstrName;
   m_bIsDirty = true;
   return TRUE;
}

BOOL CRemoteProject::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Project"), cchMax);
   return TRUE;
}

BOOL CRemoteProject::GetClass(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T(PLUGIN_NAME), cchMax);
   return TRUE;
}

BOOL CRemoteProject::IsDirty() const
{
   if( !m_bLoaded ) return FALSE;
   if( m_bIsDirty ) return TRUE;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      if( m_aFiles[i]->IsDirty() ) return TRUE;
   }
   return FALSE;
}

IDispatch* CRemoteProject::GetDispatch()
{
   CRemoteProject* pThis = const_cast<CRemoteProject*>(this);
   return &pThis->m_Dispatch;
}

INT CRemoteProject::GetItemCount() const
{
   return m_aFiles.GetSize();
}

IView* CRemoteProject::GetItem(INT iIndex)
{
   if( iIndex < 0 || iIndex > m_aFiles.GetSize() ) return NULL;
   return m_aFiles[iIndex];
}

BOOL CRemoteProject::Reset()
{
   m_sName.Empty();
   // Close the views nicely before killing them off
   int i;
   for( i = 0; i < m_aFiles.GetSize(); i++ ) m_aFiles[i]->CloseView();
   for( i = 0; i < m_aDependencies.GetSize(); i++ ) m_aDependencies[i]->CloseView();
   m_aFiles.RemoveAll();
   m_aLazyData.RemoveAll();
   m_aDependencies.RemoveAll();
   m_bIsDirty = false;
   return TRUE;
}

BOOL CRemoteProject::Load(ISerializable* pArc)
{
   Reset();

   pArc->Read(_T("name"), m_sName.GetBufferSetLength(128), 128);
   m_sName.ReleaseBuffer();

   if( !_LoadSettings(pArc) ) return FALSE;
   if( !_LoadFiles(pArc, this) ) return FALSE;

   return TRUE;
}

BOOL CRemoteProject::Save(ISerializable* pArc)
{
   pArc->Write(_T("name"), m_sName);
   pArc->Write(_T("type"), _T(PLUGIN_NAME));

   if( !_SaveSettings(pArc) ) return FALSE;
   if( !_SaveFiles(pArc, this) ) return FALSE;

   m_bIsDirty = false;
   return TRUE;
}

BOOL CRemoteProject::CreateProject()
{
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);

   CString sName;
   GetName(sName.GetBufferSetLength(128), 128);
   sName.ReleaseBuffer();

   HTREEITEM hRoot = ctrlTree.GetRootItem();
   HTREEITEM hItem = ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
         sName, 
         6, 6, 
         0, 0,
         (LPARAM) this,
         hRoot, TVI_LAST);
   _PopulateTree(ctrlTree, this, hItem);
   ctrlTree.Expand(hRoot);
   ctrlTree.Expand(hItem);
   
   return TRUE;
}

void CRemoteProject::ActivateProject()
{
   _pDevEnv->AddIdleListener(this);
   _pDevEnv->AddCommandListener(this);

   if( !m_FileManager.IsConnected() ) m_FileManager.Start();
   if( !m_CompileManager.IsConnected() ) m_CompileManager.Start();

   m_viewCompileLog.Init(&m_CompileManager.m_ShellManager);
   m_viewDebugLog.Init(&m_DebugManager.m_ShellManager);
   m_viewClassTree.Init(this);
   m_viewStack.Init(this);
   m_viewRegister.Init(this);
   m_viewDisassembly.Init(this);
   m_viewMemory.Init(this);
   m_viewBreakpoint.Init(this);
   m_viewVariable.Init(this);
   m_viewThread.Init(this);
   m_viewWatch.Init(this);

   m_viewClassTree.Populate();

   // Project filenames are relative to the solution, so
   // we need to change current Windows path.
   ::SetCurrentDirectory(m_sPath);
}

void CRemoteProject::DeactivateProject()
{
   // Need to remove the classbrowser view because
   // activating a new project should display new items
   m_viewClassTree.Clear();

   // Don't keep dependencies around
   for( int i = 0; i < m_aDependencies.GetSize(); i++ ) m_aDependencies[i]->CloseView();
   m_aDependencies.RemoveAll();

   _pDevEnv->RemoveIdleListener(this);
   _pDevEnv->RemoveCommandListener(this);
}

void CRemoteProject::ActivateUI()
{
   CMenu menu;
   menu.LoadMenu(IDR_MAIN);
   ATLASSERT(menu.IsMenu());
   CMenuHandle menuMain = _pDevEnv->GetMenuHandle(IDE_HWND_MAIN);
   CMenuHandle menuFile = menuMain.GetSubMenu(0);
   CMenuHandle menuView = menuMain.GetSubMenu(2);
   MergeMenu(menuFile.GetSubMenu(1), menu.GetSubMenu(5), 2);
   MergeMenu(menuMain, menu.GetSubMenu(0), 3);
   MergeMenu(menuMain, menu.GetSubMenu(1), 4);
   MergeMenu(menuView, menu.GetSubMenu(3), 3);
   MergeMenu(menuView.GetSubMenu(4), menu.GetSubMenu(4), 2);

   // Propagate dirty flag to project itself.
   // This will allow us to memorize if the project was ever changed
   // and is especially useful when we need to determine if a compile
   // is needed before the process can be run.
   m_bIsDirty |= (IsDirty() == TRUE);
}

void CRemoteProject::DeactivateUI()
{
}

// IAppListener

LRESULT CRemoteProject::OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   LRESULT lResult = 0;
   bHandled = ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult);
   return lResult;
}

BOOL CRemoteProject::PreTranslateMessage(MSG* pMsg)
{
   if( _pDevEnv->GetSolution()->GetFocusProject() != this ) return FALSE;
   if( !m_accel.IsNull() && m_accel.TranslateAccelerator(m_wndMain, pMsg) ) return TRUE;
   return FALSE;
}

// IIdleListener

void CRemoteProject::OnIdle(IUpdateUI* pUIBase)
{
   CString sFileType;
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement ) {
      pElement->GetType(sFileType.GetBufferSetLength(64), 64);
      sFileType.ReleaseBuffer();
   }

   BOOL bCompiling = m_CompileManager.IsBusy();
   BOOL bProcessStarted = m_DebugManager.IsBusy();
   BOOL bDebugging = m_DebugManager.IsDebugging();
   BOOL bBusy = bCompiling || bProcessStarted;
   BOOL bDebugBreak = m_DebugManager.IsBreaked();
   BOOL bIsFolder = sFileType == _T("Project") || sFileType == _T("Folder");

   pUIBase->UIEnable(ID_PROJECT_SET_DEFAULT, !bBusy);
   pUIBase->UIEnable(ID_FILE_OPENSOLUTION, !bBusy);
   pUIBase->UIEnable(ID_FILE_CLOSESOLUTION, !bBusy);
   pUIBase->UIEnable(ID_NEW_SOLUTION, !bBusy);
   pUIBase->UIEnable(ID_NEW_PROJECT, !bBusy);
   pUIBase->UIEnable(ID_ADD_PROJECT, !bBusy);

   pUIBase->UIEnable(ID_FILE_ADD_FOLDER, bIsFolder && !bBusy);
   pUIBase->UIEnable(ID_FILE_ADD_LOCAL, bIsFolder && !bBusy);
   pUIBase->UIEnable(ID_FILE_ADD_REMOTE, bIsFolder && !bBusy);
   pUIBase->UIEnable(ID_FILE_RENAME, pElement != NULL && !bBusy);
   pUIBase->UIEnable(ID_FILE_DELETE, !bBusy);

   pUIBase->UIEnable(ID_EDIT_BREAK, bCompiling | bDebugging);

   pUIBase->UIEnable(ID_VIEW_OPEN, pElement != NULL && !bIsFolder);
   pUIBase->UIEnable(ID_VIEW_PROPERTIES, pElement != NULL);

   pUIBase->UIEnable(ID_BUILD_COMPILE, sFileType == "CPP" && !bCompiling);
   pUIBase->UIEnable(ID_BUILD_CHECKSYNTAX, sFileType == "CPP" && !bCompiling);
   pUIBase->UIEnable(ID_BUILD_PROJECT, !bCompiling);
   pUIBase->UIEnable(ID_BUILD_REBUILD, !bCompiling);
   pUIBase->UIEnable(ID_BUILD_SOLUTION, !bCompiling);   
   pUIBase->UIEnable(ID_BUILD_CLEAN, !bCompiling);
   pUIBase->UIEnable(ID_BUILD_BUILDTAGS, !bCompiling);
   pUIBase->UIEnable(ID_BUILD_BUILDMAKEFILE, !bBusy);
   pUIBase->UIEnable(ID_BUILD_FILEWIZARD, pElement != NULL && !bBusy && !bIsFolder);   
   pUIBase->UIEnable(ID_BUILD_STOP, bCompiling);

   pUIBase->UIEnable(ID_DEBUG_START, (!bCompiling && !bProcessStarted) || bDebugBreak);
   pUIBase->UIEnable(ID_DEBUG_DEBUG, !bCompiling && !bProcessStarted);
   pUIBase->UIEnable(ID_DEBUG_STEP_INTO, bDebugging);
   pUIBase->UIEnable(ID_DEBUG_STEP_OVER, bDebugging);
   pUIBase->UIEnable(ID_DEBUG_STEP_OUT, bDebugging);
   pUIBase->UIEnable(ID_DEBUG_STEP_RUN, FALSE);
   pUIBase->UIEnable(ID_DEBUG_BREAK, bDebugging && !bDebugBreak);
   pUIBase->UIEnable(ID_DEBUG_STOP, bProcessStarted);
   pUIBase->UIEnable(ID_DEBUG_BREAKPOINT, FALSE);
   pUIBase->UIEnable(ID_DEBUG_CLEAR_BREAKPOINTS, TRUE);
   pUIBase->UIEnable(ID_DEBUG_QUICKWATCH, FALSE);
   pUIBase->UIEnable(ID_DEBUG_PROCESSES, FALSE);

   pUIBase->UIEnable(ID_VIEW_COMPILE_LOG, TRUE);
   pUIBase->UIEnable(ID_VIEW_DEBUG_LOG, TRUE);
   pUIBase->UIEnable(ID_VIEW_REGISTERS, bDebugging);
   pUIBase->UIEnable(ID_VIEW_MEMORY, bDebugging);
   pUIBase->UIEnable(ID_VIEW_DISASM, bDebugging);
   pUIBase->UIEnable(ID_VIEW_THREADS, bDebugging);
   pUIBase->UIEnable(ID_VIEW_VARIABLES, bDebugging);
   pUIBase->UIEnable(ID_VIEW_WATCH, bDebugging);
   pUIBase->UIEnable(ID_VIEW_CALLSTACK, bDebugging);
   pUIBase->UIEnable(ID_VIEW_BREAKPOINTS, bDebugging);

   pUIBase->UISetCheck(ID_VIEW_COMPILE_LOG, m_viewCompileLog.IsWindowVisible());
   pUIBase->UISetCheck(ID_VIEW_DEBUG_LOG, m_viewDebugLog.IsWindowVisible());
   pUIBase->UISetCheck(ID_VIEW_REGISTERS, m_viewRegister.IsWindow() && m_viewRegister.IsWindowVisible());
   pUIBase->UISetCheck(ID_VIEW_MEMORY, m_viewMemory.IsWindow() && m_viewMemory.IsWindowVisible());
   pUIBase->UISetCheck(ID_VIEW_DISASM, m_viewDisassembly.IsWindow() && m_viewDisassembly.IsWindowVisible());
   pUIBase->UISetCheck(ID_VIEW_THREADS, m_viewThread.IsWindow() && m_viewThread.IsWindowVisible());
   pUIBase->UISetCheck(ID_VIEW_VARIABLES, m_viewVariable.IsWindow() && m_viewVariable.IsWindowVisible());
   pUIBase->UISetCheck(ID_VIEW_WATCH, m_viewWatch.IsWindow() && m_viewWatch.IsWindowVisible());
   pUIBase->UISetCheck(ID_VIEW_CALLSTACK, m_viewStack.IsWindow() && m_viewStack.IsWindowVisible());
   pUIBase->UISetCheck(ID_VIEW_BREAKPOINTS, m_viewBreakpoint.IsWindow() && m_viewBreakpoint.IsWindowVisible());
}

// ITreeListener

LRESULT CRemoteProject::OnTreeMessage(LPNMHDR pnmh, BOOL& bHandled)
{
   if( _GetSelectedTreeElement() == NULL ) return 0;
   LRESULT lResult = 0;
   bHandled = ProcessWindowMessage(pnmh->hwndFrom, WM_NOTIFY, (WPARAM) pnmh->idFrom, (LPARAM) pnmh, lResult);
   return lResult;
}

// IWizardListener

BOOL CRemoteProject::OnInitWizard(IWizardManager* pManager, IProject* pProject, LPCTSTR pstrName)
{
   if( pProject != this ) return TRUE;

   static CFileTransferPage s_pageTransfer;
   static CCompilerPage s_pageCompiler;
   static CDebuggerPage s_pageDebugger;

   static CString sTransferTitle(MAKEINTRESOURCE(IDS_WIZARD_TITLE_TRANSFER));
   static CString sTransferSubTitle(MAKEINTRESOURCE(IDS_WIZARD_SUBTITLE_TRANSFER));
   static CString sStartTitle(MAKEINTRESOURCE(IDS_WIZARD_TITLE_STARTAPP));
   static CString sStartSubTitle(MAKEINTRESOURCE(IDS_WIZARD_SUBTITLE_STARTAPP));
   static CString sDebuggerTitle(MAKEINTRESOURCE(IDS_WIZARD_TITLE_DEBUGGER));
   static CString sDebuggerSubTitle(MAKEINTRESOURCE(IDS_WIZARD_SUBTITLE_DEBUGGER));
   
   s_pageTransfer.m_pProject = this;
   s_pageTransfer.SetHeaderTitle(sTransferTitle);
   s_pageTransfer.SetHeaderSubTitle(sTransferSubTitle);
   s_pageCompiler.m_pProject = this;
   s_pageCompiler.SetHeaderTitle(sStartTitle);
   s_pageCompiler.SetHeaderSubTitle(sStartSubTitle);
   s_pageDebugger.m_pProject = this;
   s_pageDebugger.SetHeaderTitle(sDebuggerTitle);
   s_pageDebugger.SetHeaderSubTitle(sDebuggerSubTitle);

   pManager->AddWizardPage(s_pageTransfer.IDD, s_pageTransfer);
   pManager->AddWizardPage(s_pageCompiler.IDD, s_pageCompiler);
   pManager->AddWizardPage(s_pageDebugger.IDD, s_pageDebugger);

   SetName(pstrName);
   return TRUE;
}

BOOL CRemoteProject::OnInitProperties(IWizardManager* pManager, IElement* pElement)
{
   if( pElement != this ) return TRUE;

   static CFileTransferPage s_pageTransfer;
   static CFileOptionsPage s_pageTransferOptions;
   static CCompilerPage s_pageCompiler;
   static CCompilerCommandsPage s_pageCompilerCommands;
   static CDebuggerPage s_pageDebugger;
   static CDebuggerCommandsPage s_pageDebuggerCommands;

   static CString sTransferTitle(MAKEINTRESOURCE(IDS_CONFIG_SETTINGS));
   static CString sTransferOptionsTitle(MAKEINTRESOURCE(IDS_CONFIG_OPTIONS));
   static CString sStartTitle(MAKEINTRESOURCE(IDS_CONFIG_SETTINGS));
   static CString sCompilerCommandsTitle(MAKEINTRESOURCE(IDS_CONFIG_COMMANDS));
   static CString sDebuggerTitle(MAKEINTRESOURCE(IDS_CONFIG_SETTINGS));
   static CString sDebuggerCommandsTitle(MAKEINTRESOURCE(IDS_CONFIG_COMMANDS));

   s_pageTransfer.m_pProject = this;
   s_pageTransfer.SetTitle((LPCTSTR)sTransferTitle);
   s_pageTransferOptions.m_pProject = this;
   s_pageTransferOptions.SetTitle((LPCTSTR)sTransferOptionsTitle);
   s_pageCompiler.m_pProject = this;
   s_pageCompiler.SetTitle((LPCTSTR)sStartTitle);
   s_pageCompilerCommands.m_pProject = this;
   s_pageCompilerCommands.SetTitle((LPCTSTR)sCompilerCommandsTitle);
   s_pageDebugger.m_pProject = this;
   s_pageDebugger.SetTitle((LPCTSTR)sDebuggerTitle);
   s_pageDebuggerCommands.m_pProject = this;
   s_pageDebuggerCommands.SetTitle((LPCTSTR)sDebuggerCommandsTitle);

   CString sConfiguration(MAKEINTRESOURCE(IDS_CONFIG_CONFIGURATION));
   pManager->AddWizardGroup(sConfiguration, CString(MAKEINTRESOURCE(IDS_CONFIG_TRANSFER)));
   pManager->AddWizardPage(s_pageTransfer.IDD, s_pageTransfer);
   pManager->AddWizardPage(s_pageTransferOptions.IDD, s_pageTransferOptions);
   //
   pManager->AddWizardGroup(sConfiguration, CString(MAKEINTRESOURCE(IDS_CONFIG_COMPILER)));
   pManager->AddWizardPage(s_pageCompiler.IDD, s_pageCompiler);
   pManager->AddWizardPage(s_pageCompilerCommands.IDD, s_pageCompilerCommands);
   //
   pManager->AddWizardGroup(sConfiguration, CString(MAKEINTRESOURCE(IDS_CONFIG_DEBUGGER)));
   pManager->AddWizardPage(s_pageDebugger.IDD, s_pageDebugger);
   pManager->AddWizardPage(s_pageDebuggerCommands.IDD, s_pageDebuggerCommands);

   return TRUE;
}

BOOL CRemoteProject::OnInitOptions(IWizardManager* pManager, ISerializable* pArc)
{
   return TRUE;
}

// ICommandListener

void CRemoteProject::OnUserCommand(LPCTSTR pstrCommand, BOOL& bHandled)
{
   bHandled = FALSE;
   CRichEditCtrl ctrlEdit = _pDevEnv->GetHwnd(IDE_HWND_COMMANDVIEW);
   if( _tcsncmp(pstrCommand, _T("cc "), 3) == 0 ) 
   {
      if( !m_CompileManager.IsConnected() ) {
         // Compiler session not connected?
         AppendRtfText(ctrlEdit, CString(MAKEINTRESOURCE(IDS_ERR_COMPILER_NOT_RUNNING)), CFM_COLOR, 0, RGB(100,0,0));
      }
      else if( m_CompileManager.IsBusy() ) {
         // Already compiling another project?
         AppendRtfText(ctrlEdit, CString(MAKEINTRESOURCE(IDS_ERR_COMPILERBUSY)), CFM_COLOR, 0, RGB(100,0,0));
      }
      else {
         // Execute command and wait for completion
         m_CompileManager.DoAction(pstrCommand);
         DWORD dwStartTick = ::GetTickCount();
         while( m_CompileManager.GetParam(_T("InCommand")) == _T("true") ) {
            ::Sleep(100L);
            // HACK: Don't wait more than 5 secs!
            //       This timeout value may have to be adjusted in the future.
            if( ::GetTickCount() - dwStartTick > 5UL * 1000UL ) break;
         }
      }
      bHandled = TRUE;
   }
   if( _tcsncmp(pstrCommand, _T("gdb "), 4) == 0 ) 
   {
      if( !m_DebugManager.IsDebugging() ) {
         // Debug session not connected?
         AppendRtfText(ctrlEdit, CString(MAKEINTRESOURCE(IDS_ERR_DEBUGGER_NOT_RUNNING)), CFM_COLOR, 0, RGB(100,0,0));
      }
      else {
         // Execute command in debugger and wait for completion
         m_DebugManager.SetParam(_T("InCommand"), _T("true"));
         CString sCommand;
         sCommand.Format(_T("-interpreter-exec console \"%s\""), pstrCommand + 4);
         m_DebugManager.DoDebugCommand(sCommand);
         m_DebugManager.DoDebugCommand(_T(""));
         DWORD dwStartTime = ::GetTickCount();
         while( m_DebugManager.GetParam(_T("InCommand")) == _T("true") ) {
            ::Sleep(100L);
            // NOTE: We'll only wait 5 secs for completion. This is a
            //       bit of a hack, but we have very little control over
            //       what the debugger might be spitting out. At least, stopping
            //       the tight loop will allow the user to abort the session!
            if( ::GetTickCount() - dwStartTime > 5000UL ) break;
         }
      }
      bHandled = TRUE;
   }
   if( _tcsicmp(pstrCommand, _T("help")) == 0 ) 
   {
      AppendRtfText(ctrlEdit, CString(MAKEINTRESOURCE(IDS_HELP)));
   }
}

// Message handlers

LRESULT CRemoteProject::OnFileRemove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   HTREEITEM hItem = NULL;
   IElement* pElement = _GetSelectedTreeElement(&hItem);
   if( pElement == NULL ) { bHandled = FALSE; return 0; }
   // Remove tree item
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   ctrlTree.DeleteItem(hItem);
   // Remove entire project or file
   if( pElement == this ) {
      ISolution* pSolution = _pDevEnv->GetSolution();
      pSolution->RemoveProject(this);
      // NOTE: Never use class-scope variables from this point
      //       since the object has been nuked!
   }
   else {
      // BUG: Leaks all the child views.
      // TODO: Recursively remove all children
      IView* pView = static_cast<IView*>(pElement);
      pView->CloseView();
      m_aFiles.Remove(pView);
      m_bIsDirty = true;
   }
   return 0;
}

LRESULT CRemoteProject::OnFileRename(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   HTREEITEM hItem = NULL;
   if( _GetSelectedTreeElement(&hItem) == NULL ) { bHandled = FALSE; return 0; }
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   ctrlTree.SelectItem(hItem);
   ctrlTree.EditLabel(hItem);
   return 0;
}

LRESULT CRemoteProject::OnFileAddFolder(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   HTREEITEM hItem = { 0 };
   IElement* pElement = _GetSelectedTreeElement(&hItem);
   if( pElement == NULL ) { bHandled = FALSE; return 0; }
   // Create new object
   IView* pFolder = new CFolderFile(this, this, pElement);
   if( pFolder == NULL ) return 0;
   CString sName(MAKEINTRESOURCE(IDS_NEW_FOLDER));
   pFolder->SetName(sName);
   m_aFiles.Add(pFolder);
   // Add new tree item
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   int iImage = 0;
   HTREEITEM hNew = ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
         sName, 
         iImage, iImage, 
         0, 0,
         (LPARAM) pFolder,
         hItem, TVI_LAST);
   ctrlTree.Expand(hItem);
   ctrlTree.SelectItem(hNew);
   ctrlTree.EditLabel(hNew);
   m_bIsDirty = true;
   return 0;
}

LRESULT CRemoteProject::OnFileAddLocal(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   // Find the tree-item, which will be our new parent
   HTREEITEM hItem = NULL;
   IElement* pElement = _GetSelectedTreeElement(&hItem);
   if( pElement == NULL )  { bHandled = FALSE; return 0; }

   // Make sure the project is saved to a file
   // Need the project filename so we can make local filenames relative.
   if( m_aFiles.GetSize() == 0 ) m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE_ALL, 0));

   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);

   // Browse for filename
   CString sFilter(MAKEINTRESOURCE(IDS_FILTER_FILES));
   for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == '|' ) sFilter.SetAt(i, '\0');
   TCHAR szBuffer[MAX_PATH * 10] = { 0 };
   DWORD dwStyle = OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ENABLESIZING | OFN_ALLOWMULTISELECT;
   CFileDialog dlg(TRUE, _T(""), NULL, dwStyle, sFilter, m_wndMain);
   dlg.m_ofn.Flags &= ~OFN_ENABLEHOOK;
   dlg.m_ofn.lpstrFile = szBuffer;
   dlg.m_ofn.nMaxFile = sizeof(szBuffer) / sizeof(TCHAR);
   dlg.m_ofn.lpstrInitialDir = m_sPath;
   if( dlg.DoModal() != IDOK ) return 0;
   
   // Compute paths and names
   CString sPath = dlg.m_ofn.lpstrFile;
   if( sPath.Right(1) != _T("\\") ) sPath += '\\';
   HTREEITEM hFirst = NULL;

   TCHAR szFilename[MAX_PATH] = { 0 };
   LPTSTR src = &szBuffer[dlg.m_ofn.nFileOffset];
   LPTSTR dst = &szFilename[dlg.m_ofn.nFileOffset];
   _tcsncpy(szFilename, szBuffer, dlg.m_ofn.nFileOffset);
   szFilename[dlg.m_ofn.nFileOffset - 1] = '\\';
   while (*src != '\0') {
      _tcscpy(dst, src);
      src = &src[_tcslen(src) + 1];

      TCHAR szName[MAX_PATH];
      _tcscpy(szName, szFilename);
      ::PathStripPath(szName);

      TCHAR szPath[MAX_PATH];
      _tcscpy(szPath, m_sPath);
      ::PathRemoveBackslash(szPath);

      TCHAR szRelativeFilename[MAX_PATH] = { 0 };
      ::PathRelativePathTo(szRelativeFilename, szPath, FILE_ATTRIBUTE_DIRECTORY, szFilename, FILE_ATTRIBUTE_NORMAL);
      if( _tcslen(szRelativeFilename) == 0 ) _tcscpy(szRelativeFilename, szFilename);

      if( !_CheckProjectFile(szRelativeFilename, szName, false) ) return 0;

      // Create new object
      IView* pView = _pDevEnv->CreateView(szFilename, this, pElement);
      if( pView == NULL ) return 0;

      // Load some default properties
      CViewSerializer arc;
      arc.Add(_T("name"), szName);
      arc.Add(_T("filename"), szRelativeFilename);
      arc.Add(_T("location"), _T("local"));
      pView->Load(&arc);

      // Add view to collection
      m_aFiles.Add(pView);

      // Add new tree item
      int iImage = _GetElementImage(pView);
      HTREEITEM hNew = ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
            szName, 
            iImage, iImage, 
            0, 0,
            (LPARAM) pView,
            hItem, TVI_LAST);

      if( hFirst == NULL ) hFirst = hNew;
   }
   ctrlTree.Expand(hItem);
   if( hFirst != NULL ) ctrlTree.SelectItem(hFirst);

   m_bIsDirty = true;
   return 0;
}

LRESULT CRemoteProject::OnFileAddRemote(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   // Find the tree-item, which will be our new parent
   HTREEITEM hItem = NULL;
   IElement* pElement = _GetSelectedTreeElement(&hItem);
   if( pElement == NULL ) { bHandled = FALSE; return 0; }

   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);

   // Browse for filename
   CString sFilter(MAKEINTRESOURCE(IDS_FILTER_FILES));
   for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == '|' ) sFilter.SetAt(i, '\0');
   CString sRemotePath = m_FileManager.GetCurPath();
   CRemoteFileDlg dlg(TRUE, &m_FileManager, _T(""), OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT, sFilter);
   dlg.m_ofn.lpstrInitialDir = sRemotePath;
   if( dlg.DoModal(m_wndMain) != IDOK ) return 0;

   CWaitCursor cursor;
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, CString(MAKEINTRESOURCE(IDS_STATUS_ADDFILE)));

   // Convert paths to Windows type so we can manipulate
   // them using the Shell API
   CString sSeparator = m_FileManager.GetParam(_T("Separator"));
   if( sSeparator != _T("\\") ) sRemotePath.Replace(sSeparator, _T("\\"));

   // Compute paths and names
   CString sPath = dlg.m_ofn.lpstrFile;
   if( sPath.Right(1) != sSeparator ) sPath += sSeparator;
   HTREEITEM hFirst = NULL;
   LPCTSTR ppstrPath = dlg.m_ofn.lpstrFile + _tcslen(dlg.m_ofn.lpstrFile) + 1;
   while( *ppstrPath ) {
      CString sFilename = sPath + ppstrPath;

      // NOTE: Before we can use PathRelativePathTo() we need to
      //       convert back to Windows filename-slashes because it doesn't
      //       work with unix-type slashes (/ vs \).
      if( sSeparator != _T("\\") ) sFilename.Replace(sSeparator, _T("\\"));
      TCHAR szName[MAX_PATH] = { 0 };
      _tcscpy(szName, sFilename);
      ::PathStripPath(szName);
      TCHAR szRelativeFilename[MAX_PATH] = { 0 };
      ::PathRelativePathTo(szRelativeFilename, sRemotePath, FILE_ATTRIBUTE_DIRECTORY, sFilename, FILE_ATTRIBUTE_NORMAL);
      if( _tcslen(szRelativeFilename) == 0 ) _tcscpy(szRelativeFilename, sFilename);
      CString sName = szName;
      CString sRelativeFilename = szRelativeFilename;     
      if( sSeparator != _T("\\") ) {         
         sName.Replace(_T("\\"), sSeparator);
         sFilename.Replace(_T("\\"), sSeparator);
         sRelativeFilename.Replace(_T("\\"), sSeparator);
      }

      if( !_CheckProjectFile(sRelativeFilename, szName, true) ) return 0;

      // Create new object
      IView* pView = Plugin_CreateView(sFilename, this, pElement);
      if( pView == NULL ) return 0;

      // Load some default properties
      CViewSerializer arc;
      arc.Add(_T("name"), szName);
      arc.Add(_T("filename"), sRelativeFilename);
      arc.Add(_T("location"), _T("remote"));
      pView->Load(&arc);

      // Add to collection
      m_aFiles.Add(pView);

      // Add new tree item
      int iImage = _GetElementImage(pView);
      HTREEITEM hNew = ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
            szName, 
            iImage, iImage, 
            0, 0,
            (LPARAM) pView,
            hItem, TVI_LAST);

      if( hFirst == NULL ) hFirst = hNew;

      ppstrPath += _tcslen(ppstrPath) + 1;
   }
   ctrlTree.Expand(hItem);
   if( hFirst != NULL ) ctrlTree.SelectItem(hFirst);

   m_bIsDirty = true;
   return 0;
}

LRESULT CRemoteProject::OnEditBreak(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   if( m_DebugManager.IsDebugging() ) OnDebugBreak(0, 0, NULL, bHandled);
   if( m_CompileManager.IsBusy() ) OnBuildStop(0, 0, NULL, bHandled);
   return 0;
}

LRESULT CRemoteProject::OnViewOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) { bHandled = FALSE; return 0; }
   if( pElement == this ) return 0;

   IView* pView = static_cast<IView*>(pElement);
   if( !pView->OpenView(0) ) {
      if( ::GetLastError() == ERROR_FILE_NOT_FOUND ) {
         HWND hWnd = ::GetActiveWindow();
         if( IDYES == _pDevEnv->ShowMessageBox(hWnd, CString(MAKEINTRESOURCE(IDS_CREATEFILE)), CString(MAKEINTRESOURCE(IDS_CAPTION_QUESTION)), MB_YESNO | MB_ICONQUESTION) ) {
            _CreateNewRemoteFile(hWnd, pView);
         }
      }
      else {
         GenerateError(_pDevEnv, IDS_ERR_OPENVIEW);
         m_FileManager.Stop();
         m_FileManager.Start();
      }
   }
   return 0;
}

LRESULT CRemoteProject::OnViewCompileLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   // Position the view
   if( !m_viewCompileLog.IsWindowVisible() ) {
      RECT rcWin = { 20, 20, 400, 400 };
      _pDevEnv->AddDockView(m_viewCompileLog, IDE_DOCK_FLOAT, rcWin);
   }
   else {
      _pDevEnv->AddDockView(m_viewCompileLog, IDE_DOCK_HIDE, CWindow::rcDefault);
   }
   return 0;
}

LRESULT CRemoteProject::OnViewDebugLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   // Position the view
   if( !m_viewDebugLog.IsWindowVisible() ) {
      RECT rcWin = { 20, 20, 400, 400 };
      _pDevEnv->AddDockView(m_viewDebugLog, IDE_DOCK_FLOAT, rcWin);
   }
   else {
      _pDevEnv->AddDockView(m_viewDebugLog, IDE_DOCK_HIDE, CWindow::rcDefault);
   }
   return 0;
}

LRESULT CRemoteProject::OnViewBreakpoints(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Breakpoints view
   if( !m_viewBreakpoint.IsWindow() ) {
      CString s(MAKEINTRESOURCE(IDS_CAPTION_BREAKPOINTS));
      DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      m_viewBreakpoint.Init(this);
      m_viewBreakpoint.Create(m_wndMain, CWindow::rcDefault, s, dwStyle, WS_EX_CLIENTEDGE);
      _pDevEnv->AddDockView(m_viewBreakpoint, IDE_DOCK_HIDE, CWindow::rcDefault);
   }
   
   // Position the view
   if( !m_viewBreakpoint.IsWindowVisible() ) {
      RECT s_rcWin = { 420, 40, 780, 300 };
      _pDevEnv->AddDockView(m_viewBreakpoint, IDE_DOCK_FLOAT, s_rcWin);
      DelayedDebugEvent(LAZY_DEBUG_STOP_EVENT);
   }
   else {
      _pDevEnv->AddDockView(m_viewBreakpoint, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewRegisters(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Registers view
   if( !m_viewRegister.IsWindow() ) {
      CString s(MAKEINTRESOURCE(IDS_CAPTION_REGISTERS));
      DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      m_viewRegister.Init(this);
      m_viewRegister.Create(m_wndMain, CWindow::rcDefault, s, dwStyle, WS_EX_CLIENTEDGE);
      _pDevEnv->AddDockView(m_viewRegister, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   // Position the view
   if( !m_viewRegister.IsWindowVisible() ) {
      RECT s_rcWin = { 420, 40, 640, 300 };
      _pDevEnv->AddDockView(m_viewRegister, IDE_DOCK_FLOAT, s_rcWin);
      DelayedDebugEvent(LAZY_DEBUG_STOP_EVENT);
   }
   else {
      _pDevEnv->AddDockView(m_viewRegister, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewMemory(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Memory view
   if( !m_viewMemory.IsWindow() ) {
      CString s(MAKEINTRESOURCE(IDS_CAPTION_MEMORY));
      DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      m_viewMemory.Init(this);
      m_viewMemory.Create(m_wndMain, CWindow::rcDefault, s, dwStyle, WS_EX_CLIENTEDGE);
      _pDevEnv->AddDockView(m_viewMemory, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   // Position the view
   if( !m_viewMemory.IsWindowVisible() ) {
      RECT s_rcWin = { 120, 140, 760, 400 };
      _pDevEnv->AddDockView(m_viewMemory, IDE_DOCK_FLOAT, s_rcWin);
      DelayedDebugEvent(LAZY_DEBUG_STOP_EVENT);
   }
   else {
      _pDevEnv->AddDockView(m_viewMemory, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewDisassembly(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Disassembly view
   if( !m_viewDisassembly.IsWindow() ) {
      CString s(MAKEINTRESOURCE(IDS_CAPTION_DISASSEMBLY));
      DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      m_viewDisassembly.Init(this);
      m_viewDisassembly.Create(m_wndMain, CWindow::rcDefault, s, dwStyle, WS_EX_CLIENTEDGE);
      _pDevEnv->AddDockView(m_viewDisassembly, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   // Position the view
   if( !m_viewDisassembly.IsWindowVisible() ) {
      RECT s_rcWin = { 320, 50, 690, 400 };
      _pDevEnv->AddDockView(m_viewDisassembly, IDE_DOCK_FLOAT, s_rcWin);
      DelayedDebugEvent(LAZY_DEBUG_STOP_EVENT);
   }
   else {
      _pDevEnv->AddDockView(m_viewDisassembly, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewThreads(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Thread view
   if( !m_viewThread.IsWindow() ) {
      CString s(MAKEINTRESOURCE(IDS_CAPTION_THREADS));
      DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      m_viewThread.Init(this);
      m_viewThread.Create(m_wndMain, CWindow::rcDefault, s, dwStyle, WS_EX_CLIENTEDGE);
      _pDevEnv->AddDockView(m_viewThread, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   // Position the view
   if( !m_viewThread.IsWindowVisible() ) {
      RECT s_rcWin = { 420, 220, 700, 350 };
      _pDevEnv->AddDockView(m_viewThread, IDE_DOCK_BOTTOM, s_rcWin);
      DelayedDebugEvent(LAZY_DEBUG_STOP_EVENT);
   }
   else {
      _pDevEnv->AddDockView(m_viewThread, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewStack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Stack view
   if( !m_viewStack.IsWindow() ) {
      CString s(MAKEINTRESOURCE(IDS_CAPTION_CALLSTACK));
      DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      m_viewStack.Init(this);
      m_viewStack.Create(m_wndMain, CWindow::rcDefault, s, dwStyle, 0);
      _pDevEnv->AddDockView(m_viewStack, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   // Position the view
   if( !m_viewStack.IsWindowVisible() ) {
      RECT rcWin = { 420, 520, 780, 650 };
      _pDevEnv->AddDockView(m_viewStack, IDE_DOCK_BOTTOM, rcWin);
      DelayedDebugEvent(LAZY_DEBUG_STOP_EVENT);
   }
   else {
      _pDevEnv->AddDockView(m_viewStack, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewVariables(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Variables view
   if( !m_viewVariable.IsWindow() ) {
      CString s(MAKEINTRESOURCE(IDS_CAPTION_VARIABLES));
      DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      m_viewVariable.Init(this);
      m_viewVariable.Create(m_wndMain, CWindow::rcDefault, s, dwStyle, WS_EX_CLIENTEDGE);
      _pDevEnv->AddDockView(m_viewVariable, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   // Position the view
   if( !m_viewVariable.IsWindowVisible() ) {
      RECT rcWin = { 450, 520, 760, 750 };
      _pDevEnv->AddDockView(m_viewVariable, IDE_DOCK_BOTTOM, rcWin);
      DelayedDebugEvent(LAZY_DEBUG_STOP_EVENT);
   }
   else {
      _pDevEnv->AddDockView(m_viewVariable, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewWatch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Watches view
   if( !m_viewWatch.IsWindow() ) {
      CString s(MAKEINTRESOURCE(IDS_CAPTION_WATCH));
      DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      m_viewWatch.Init(this);
      m_viewWatch.Create(m_wndMain, CWindow::rcDefault, s, dwStyle, WS_EX_CLIENTEDGE);
      _pDevEnv->AddDockView(m_viewWatch, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   // Position the view
   if( !m_viewWatch.IsWindowVisible() ) {
      RECT rcWin = { 320, 180, 750, 450 };
      _pDevEnv->AddDockView(m_viewWatch, IDE_DOCK_FLOAT, rcWin);
      DelayedDebugEvent(LAZY_DEBUG_STOP_EVENT);
      m_viewWatch.ActivateWatches();
   }
   else {
      _pDevEnv->AddDockView(m_viewWatch, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewProperties(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   // Find the tree-item, which will be our new parent
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement != this ) { bHandled = FALSE; return 0; }
   if( _pDevEnv->ShowConfiguration(pElement) ) m_bIsDirty = true;
   return 0;
}

LRESULT CRemoteProject::OnProjectSetDefault(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) { bHandled = FALSE; return 0; };
   IProject* pProject = static_cast<IProject*>(pElement);
   _pDevEnv->GetSolution()->SetActiveProject(pProject);
   return 0;
}

LRESULT CRemoteProject::OnDebugStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   if( m_DebugManager.IsDebugging() ) return m_DebugManager.RunContinue(); 
   return m_DebugManager.RunNormal();
}

LRESULT CRemoteProject::OnDebugDebug(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   return m_DebugManager.RunDebug();
}

LRESULT CRemoteProject::OnDebugBreak(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   return m_DebugManager.Break();
}

LRESULT CRemoteProject::OnDebugStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   m_DebugManager.SignalStop();
   return 0;
}

LRESULT CRemoteProject::OnDebugStepOver(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   DelayedDebugCommand(_T("-exec-next"));
   return 0;
}

LRESULT CRemoteProject::OnDebugStepInto(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   DelayedDebugCommand(_T("-exec-step"));
   return 0;
}

LRESULT CRemoteProject::OnDebugStepOut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   DelayedDebugCommand(_T("-exec-finish"));
   return 0;
}

LRESULT CRemoteProject::OnDebugClearBreakpoints(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   m_DebugManager.ClearBreakpoints();
   return 0;
}

LRESULT CRemoteProject::OnDebugQuickWatch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   ATLASSERT(m_DebugManager.IsDebugging());
   if( !m_DebugManager.IsDebugging() ) return 0;
   if( m_pQuickWatchDlg ) delete m_pQuickWatchDlg;

   // Ask active editor window to retrieve the selected/caret text
   LAZYDATA data;
   data.Action = LAZY_SEND_VIEW_MESSAGE;
   data.wParam = DEBUG_CMD_GET_CARET_TEXT;
   m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_DEBUG_EDIT_LINK, data.wParam), (LPARAM) &data);
   
   m_pQuickWatchDlg = new CQuickWatchDlg(_pDevEnv, this, data.szMessage);
   ATLASSERT(m_pQuickWatchDlg);
   m_pQuickWatchDlg->Create(m_wndMain);
   m_pQuickWatchDlg->ShowWindow(SW_SHOW);
   return 0;
}

LRESULT CRemoteProject::OnBuildClean(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   return m_CompileManager.DoAction(_T("Clean"));
}

LRESULT CRemoteProject::OnBuildProject(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   return m_CompileManager.DoAction(_T("Build"));
}

LRESULT CRemoteProject::OnBuildRebuild(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   return m_CompileManager.DoAction(_T("Rebuild"));
}

LRESULT CRemoteProject::OnBuildSolution(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   while( m_CompileManager.IsBusy() ) {
      PumpIdleMessages();
      ::Sleep(500);
   }
   m_CompileManager.DoAction(_T("Rebuild"));
   bHandled = FALSE; // Let other projects get a shot at this message
   return 0;
}

LRESULT CRemoteProject::OnBuildCompile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   bHandled = FALSE;
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) return 0;

   CString sType;
   pElement->GetType(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();
   if( sType != _T("CPP") ) return 0;

   IView* pFile = NULL;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      if( m_aFiles[i] == pElement ) {
         pFile = m_aFiles[i];
         break;
      }
   }
   if( pFile == NULL ) return 0;

   CString sFilename;
   pFile->GetFileName(sFilename.GetBufferSetLength(MAX_PATH), MAX_PATH);
   sFilename.ReleaseBuffer();
   m_CompileManager.DoAction(_T("Compile"), sFilename);

   bHandled = TRUE;
   return 0;
}

LRESULT CRemoteProject::OnBuildCheckSyntax(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   bHandled = FALSE;
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) return 0;

   CString sType;
   pElement->GetType(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();
   if( sType != _T("CPP") ) return 0;

   IView* pFile = NULL;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      if( m_aFiles[i] == pElement ) {
         pFile = m_aFiles[i];
         break;
      }
   }
   if( pFile == NULL ) return 0;

   CString sFilename;
   pFile->GetFileName(sFilename.GetBufferSetLength(MAX_PATH), MAX_PATH);
   sFilename.ReleaseBuffer();
   m_CompileManager.DoAction(_T("CheckSyntax"), sFilename);

   bHandled = TRUE;
   return 0;
}

LRESULT CRemoteProject::OnBuildTags(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   return m_CompileManager.DoAction(_T("BuildTags"));
}

LRESULT CRemoteProject::OnBuildMakefile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   HWND hWnd = ::GetActiveWindow();

   // Find project in tree
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   HTREEITEM hItem = ctrlTree.GetRootItem();
   if( hItem ) hItem = ctrlTree.GetChildItem(hItem);
   while( hItem != NULL ) {
      if( ctrlTree.GetItemData(hItem) == (DWORD_PTR) this ) break;
      hItem = ctrlTree.GetNextSiblingItem(hItem);
   }
   if( hItem == NULL ) return 0;

   // Check if a Makefile already exists
   IView* pView = NULL;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      IView* pFile = m_aFiles[i];
      CString sType;
      pFile->GetType(sType.GetBufferSetLength(64), 64);
      sType.ReleaseBuffer();
      if( sType == _T("Makefile") ) pView = pFile;
   }
   // It does not? Let's create a new file in the project list...
   if( pView == NULL ) {
      if( IDNO == _pDevEnv->ShowMessageBox(hWnd, CString(MAKEINTRESOURCE(IDS_CREATEMAKEFILE)), CString(MAKEINTRESOURCE(IDS_CAPTION_QUESTION)), MB_YESNO | MB_ICONQUESTION) ) return 0;
      // Create new file
      pView = _pDevEnv->CreateView(_T("Makefile"), this, this);
      if( pView == NULL ) return false;
      // Load some default properties
      CViewSerializer arc;
      arc.Add(_T("name"), _T("Makefile"));
      arc.Add(_T("filename"), _T("./Makefile"));
      arc.Add(_T("location"), _T("remote"));
      pView->Load(&arc);
      m_aFiles.Add(pView);
      // Add to tree
      CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
      int iImage = _GetElementImage(pView);
      ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
            _T("Makefile"), 
            iImage, iImage, 
            0, 0,
            (LPARAM) pView,
            hItem, 
            TVI_LAST);
      m_bIsDirty = true;
   }
   // Finally create makefile contents!
   _RunFileWizard(hWnd, _T("Makefile Wizard"), pView);
   return 0;
}

LRESULT CRemoteProject::OnBuildFileWizard(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) return 0;
   CString sName;
   pElement->GetName(sName.GetBufferSetLength(100), 100);
   sName.ReleaseBuffer();
   if( !_RunFileWizard(::GetActiveWindow(), sName, (IView*) pElement) ) {
      GenerateError(_pDevEnv, IDS_ERR_NOFILEWIZARD);
   }
   return 0;
}

LRESULT CRemoteProject::OnBuildStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   return m_CompileManager.DoAction(_T("Stop"));
}

LRESULT CRemoteProject::OnToolBarDropDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   LPNMTOOLBAR lptb = (LPNMTOOLBAR) pnmh;
   if( lptb->iItem != ID_BOOKMARKS_TOGGLE && lptb->iItem != ID_BOOKMARKS_GOTO ) {
      bHandled = FALSE;
      return 0;
   }
   CToolBarCtrl tb = lptb->hdr.hwndFrom;
   RECT rcItem;
   tb.GetItemRect(tb.CommandToIndex(lptb->iItem), &rcItem);
   POINT pt = { rcItem.left, rcItem.bottom };
   tb.ClientToScreen(&pt);
   CMenu menu;
   CMenuHandle submenu;
   if( lptb->iItem == ID_BOOKMARKS_TOGGLE ) {
      menu.LoadMenu(IDR_MAIN);
      submenu = menu.GetSubMenu(2).GetSubMenu(1);
   }
   else {
      menu.LoadMenu(IDM_BOOKMARKS);
      submenu = menu.GetSubMenu(0);
   }
   _pDevEnv->ShowPopupMenu(NULL, submenu, pt);
   return TBDDRET_DEFAULT;
}

LRESULT CRemoteProject::OnTreeLabelBegin(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) return TRUE;
   CString sType;
   pElement->GetType(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();
   if( pElement == this ) return FALSE;
   if( sType == _T("Project") ) return FALSE;
   if( sType == _T("Folder") ) return FALSE;
   return TRUE;
}

LRESULT CRemoteProject::OnTreeLabelEdit(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   bHandled = FALSE;
   LPNMTVDISPINFO lpNMTVDI = (LPNMTVDISPINFO) pnmh;
   if( lpNMTVDI->item.pszText == NULL ) return FALSE;
   // Commit rename action
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) return FALSE;
   if( pElement == this ) {
      SetName(lpNMTVDI->item.pszText);
   }
   else {
      IView* pView = static_cast<IView*>(pElement);
      pView->SetName(lpNMTVDI->item.pszText);
   }
   // Refresh property window (if visible)
   _pDevEnv->ShowProperties(pElement, FALSE);
   m_bIsDirty = true;
   bHandled = TRUE;
   return TRUE;
}

LRESULT CRemoteProject::OnTreeKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   bHandled = FALSE;
   LPNMTVKEYDOWN lpNMTVKD = (LPNMTVKEYDOWN) pnmh;
   if( lpNMTVKD->wVKey == VK_DELETE ) {
      m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_DELETE, 0));
   }
   if( lpNMTVKD->wVKey == VK_RETURN ) {
      m_wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(ID_VIEW_OPEN, 0));
   }
   if( lpNMTVKD->wVKey == VK_F2 ) {
      CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
      HTREEITEM hItem = NULL;
      if( _GetSelectedTreeElement(&hItem) != NULL ) ctrlTree.EditLabel(hItem);
   }
   return TRUE;
}

LRESULT CRemoteProject::OnTreeDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
   return OnViewOpen(0, 0, NULL, bHandled);
}

LRESULT CRemoteProject::OnTreeRClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
   bHandled = FALSE;
   HTREEITEM hItem = NULL;
   IElement* pElement = _GetSelectedTreeElement(&hItem);
   if( pElement == NULL ) return 0;
   CString sType;
   pElement->GetType(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();
   UINT nRes = 0;
   if( sType == _T("Folder") ) nRes = IDR_FOLDER;
   else if( sType == _T("CPP") ) nRes = IDR_CPP;
   else if( sType == _T("Project") ) nRes = IDR_PROJECT;
   else nRes = IDR_TEXT;
   CMenu menu;
   menu.LoadMenu(nRes);
   CMenuHandle submenu = menu.GetSubMenu(0);
   DWORD dwPos = ::GetMessagePos();
   POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
   _pDevEnv->ShowPopupMenu(pElement, submenu, pt, TRUE, this);
   bHandled = TRUE;
   return 0;
}

// Operations

BOOL CRemoteProject::GetPath(LPTSTR pstrPath, UINT cchMax) const
{
   _tcsncpy(pstrPath, m_sPath, cchMax);
   return TRUE;
}

CTelnetView* CRemoteProject::GetDebugView() const
{
   return &m_viewDebugLog;
}

CClassView* CRemoteProject::GetClassView() const
{
   return &m_viewClassTree;
}

CString CRemoteProject::GetBuildMode() const
{
   return m_ctrlMode.GetCurSel() == 0 ? _T("Release") : _T("Debug");
}

void CRemoteProject::SendViewMessage(UINT nCmd, LAZYDATA* pData)
{
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      DWORD dwRes = 0;
      m_aFiles[i]->SendMessage(WM_COMMAND, MAKEWPARAM(ID_DEBUG_EDIT_LINK, nCmd), (LPARAM) pData);
   }
}

bool CRemoteProject::OpenView(LPCTSTR pstrFilename, long lLineNum)
{
   IView* pView = FindView(pstrFilename, false);
   if( pView == NULL ) pView = _CreateDependencyFile(pstrFilename, ::PathFindFileName(pstrFilename));
   if( pView == NULL ) return false;
   return pView->OpenView(lLineNum) == TRUE;
}

IView* CRemoteProject::FindView(LPCTSTR pstrFilename, bool bLocally /*= false*/) const
{
   ATLASSERT(!::IsBadStringPtr(pstrFilename,-1));
   // Prepare filename (strip path)
   TCHAR szSearchFile[MAX_PATH + 1] = { 0 };
   _tcscpy(szSearchFile, pstrFilename);
   ::PathStripPath(szSearchFile);
   // No need to look for dummy filenames
   if( _tcslen(pstrFilename) < 2 ) return false;
   // Scan through all views and see if there's a filename-match
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      IView* pView = m_aFiles[i];
      TCHAR szFilename[MAX_PATH + 1] = { 0 };
      pView->GetFileName(szFilename, MAX_PATH);
      ::PathStripPath(szFilename);
      if( _tcsicmp(szSearchFile, szFilename) == 0 ) return pView;
   }
   // Scan for project files only?
   if( bLocally ) return NULL;
   // Locate file in another project
   ISolution* pSolution = _pDevEnv->GetSolution();
   for( int a = 0; a < pSolution->GetItemCount(); a++ ) {
      IProject* pProject = pSolution->GetItem(a);
      for( INT b = 0; b < pProject->GetItemCount(); b++ ) {
         IView* pView = pProject->GetItem(b);
         TCHAR szFilename[MAX_PATH + 1] = { 0 };
         pView->GetFileName(szFilename, MAX_PATH);
         ::PathStripPath(szFilename);
         if( _tcsicmp(szSearchFile, szFilename) == 0 ) return pView;
      }
   }
   // Look for the file in the dependencies
   for( int x = 0; x < m_aDependencies.GetSize(); x++ ) {
      IView* pView = m_aDependencies[x];
      TCHAR szFilename[MAX_PATH + 1] = { 0 };
      pView->GetFileName(szFilename, MAX_PATH);
      ::PathStripPath(szFilename);
      if( _tcsicmp(szSearchFile, szFilename) == 0 ) return pView;
   }
   return NULL;
}

CString CRemoteProject::GetTagInfo(LPCTSTR pstrValue, LPCTSTR pstrOwner /*= NULL*/)
{
   ATLASSERT(!::IsBadStringPtr(pstrValue,-1));
   // NOTE: GetTagInfo() is designed so that it *may* return a result
   //       immediately - but it may also delay the retrieval of information
   //       (which happens when it queries debug-information from the debugger).
   if( m_DebugManager.IsDebugging() ) return m_DebugManager.GetTagInfo(pstrValue);
   return m_TagManager.GetItemDeclaration(pstrValue, pstrOwner);
}

void CRemoteProject::InitializeToolBars()
{
   // NOTE: The toolbars are static members of this
   //       class so we need to initialize them only once.
   if( m_ctrlBuild.IsWindow() ) return;

   CString s;
   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);

   m_ctrlDebug = CFrameWindowImplBase<>::CreateSimpleToolBarCtrl(wndMain, IDR_DEBUG, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
   m_ctrlBuild = CFrameWindowImplBase<>::CreateSimpleToolBarCtrl(wndMain, IDR_BUILD, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE | TBSTYLE_LIST);
   m_ctrlBookmarks = CFrameWindowImplBase<>::CreateSimpleToolBarCtrl(wndMain, IDR_BOOKMARKS, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE | TBSTYLE_LIST);
   m_ctrlSearch = CFrameWindowImplBase<>::CreateSimpleToolBarCtrl(wndMain, IDR_SEARCH, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE | TBSTYLE_LIST);

   m_ctrlMode.Create(m_ctrlBuild, CWindow::rcDefault, NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS);
   m_ctrlMode.SetFont(AtlGetDefaultGuiFont());
   s.LoadString(IDS_RELEASE);
   m_ctrlMode.AddString(s);
   s.LoadString(IDS_DEBUG);
   m_ctrlMode.AddString(s);
   m_ctrlMode.SetCurSel(1);

   m_ctrlBuild.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);
   _AddButtonText(m_ctrlBuild, ID_BUILD_PROJECT, IDS_BUILD);
   _AddControlToToolbar(m_ctrlBuild, m_ctrlMode, 90, 4, false);
   m_ctrlMode.SetWindowPos(NULL, 0, 0, 90, 120, SWP_NOMOVE | SWP_NOREDRAW);

   m_ctrlBookmarks.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);
   _AddDropDownButton(m_ctrlBookmarks, ID_BOOKMARKS_TOGGLE);
   _AddDropDownButton(m_ctrlBookmarks, ID_BOOKMARKS_GOTO);
   _AddButtonText(m_ctrlBookmarks, ID_BOOKMARKS_TOGGLE, IDS_TOGGLE);
   _AddButtonText(m_ctrlBookmarks, ID_BOOKMARKS_GOTO, IDS_GOTO);

   _pDevEnv->AddToolBar(m_ctrlBuild, CString(MAKEINTRESOURCE(IDS_CAPTION_BUILD)));
   _pDevEnv->AddToolBar(m_ctrlDebug, CString(MAKEINTRESOURCE(IDS_CAPTION_DEBUG)));
   _pDevEnv->AddToolBar(m_ctrlBookmarks, CString(MAKEINTRESOURCE(IDS_CAPTION_BOOKMARKS)));
   //_pDevEnv->AddToolBar(m_ctrlSearch, CString(MAKEINTRESOURCE(IDS_CAPTION_SEARCH)));
}

// Implementation

void CRemoteProject::_InitializeData()
{
   // Initilize data. These control are mostly statics and
   // need only be initialized one.
   if( !m_accel.IsNull() ) return;

   CString sCaption;
   DWORD dwStyle;

   m_accel.LoadAccelerators(IDR_ACCELERATOR);

   // Create the Compile Log view
   sCaption.LoadString(IDS_CAPTION_COMPILELOG);
   dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
   m_viewCompileLog.Init(&m_CompileManager.m_ShellManager);
   m_viewCompileLog.Create(m_wndMain, CWindow::rcDefault, sCaption, dwStyle, WS_EX_CLIENTEDGE);
   _pDevEnv->AddDockView(m_viewCompileLog, IDE_DOCK_HIDE, CWindow::rcDefault);

   // Create the Debug Log view
   COLORREF clrBack = BlendRGB(::GetSysColor(COLOR_WINDOW), RGB(0,0,0), 5);
   sCaption.LoadString(IDS_CAPTION_DEBUGLOG);
   dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
   m_viewDebugLog.Init(&m_DebugManager.m_ShellManager);
   m_viewDebugLog.Create(m_wndMain, CWindow::rcDefault, sCaption, dwStyle, WS_EX_CLIENTEDGE);
   m_viewDebugLog.SetColors(::GetSysColor(COLOR_WINDOWTEXT), clrBack);
   _pDevEnv->AddDockView(m_viewDebugLog, IDE_DOCK_HIDE, CWindow::rcDefault);

   // Create the Class view
   dwStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER;
   m_viewClassTree.Init(this);
   m_viewClassTree.Create(m_wndMain, CWindow::rcDefault, NULL, dwStyle);

   _AddCommandBarImages(IDR_TOOLIMAGES);

   // We'll always display the C++ toolbars
   _pDevEnv->ShowToolBar(m_ctrlBuild, TRUE);
   _pDevEnv->ShowToolBar(m_ctrlDebug, TRUE);
}

bool CRemoteProject::_LoadSettings(ISerializable* pArc)
{
   if( !pArc->ReadGroupBegin(_T("Settings")) ) return true;
   if( !m_FileManager.Load(pArc) ) return false;
   if( !m_CompileManager.Load(pArc) ) return false;
   if( !m_DebugManager.Load(pArc) ) return false;
   if( !pArc->ReadGroupEnd() ) return false;
   return true;
}

bool CRemoteProject::_SaveSettings(ISerializable* pArc)
{
   if( !pArc->WriteGroupBegin(_T("Settings")) ) return false;
   if( !m_FileManager.Save(pArc) ) return false;
   if( !m_CompileManager.Save(pArc) ) return false;
   if( !m_DebugManager.Save(pArc) ) return false;
   if( !pArc->WriteGroupEnd() ) return false;
   return true;
}

bool CRemoteProject::_LoadFiles(ISerializable* pArc, IElement* pParent)
{
   if( !pArc->ReadGroupBegin(_T("Files")) ) return true;
   while( pArc->ReadGroupBegin(_T("File")) ) {
      CString sType;
      pArc->Read(_T("type"), sType.GetBufferSetLength(64), 64);
      sType.ReleaseBuffer();
      CString sLocation;
      pArc->Read(_T("location"), sLocation.GetBufferSetLength(64), 64);
      sLocation.ReleaseBuffer();
      CString sFilename;
      pArc->Read(_T("filename"), sFilename.GetBufferSetLength(MAX_PATH), MAX_PATH);
      sFilename.ReleaseBuffer();
      // Create file object
      // NOTE: We prefer to create files marked as "remote" internally since the
      //       other plugins will not be able to load them from the remote server.
      IView* pView = NULL;
      if( sType == _T("Folder") ) pView = new CFolderFile(this, this, pParent);
      else if( sLocation == _T("remote") ) pView = Plugin_CreateView(sFilename, this, pParent);
      else pView = _pDevEnv->CreateView(sFilename, this, pParent);
      ATLASSERT(pView);
      if( pView == NULL ) return false;
      // Load file properties
      if( !pView->Load(pArc) ) {
         pView->CloseView();
         return false;
      }
      // Add file to collection
      m_aFiles.Add(pView);
      // Recurse into subfiles
      if( !_LoadFiles(pArc, pView) ) {
         m_aFiles.Remove(pView);
         pView->CloseView();
         return false;
      }
      if( !pArc->ReadGroupEnd() ) return false;
   }
   if( !pArc->ReadGroupEnd() ) return false;
   return true;
}

bool CRemoteProject::_SaveFiles(ISerializable* pArc, IElement* pParent)
{
   if( !pArc->WriteGroupBegin(_T("Files")) ) return false;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      IView* pFile = m_aFiles[i];
      if( pFile->GetParent() == pParent ) {
         if( !pArc->WriteGroupBegin(_T("File")) ) return false;
         // Save file properties
         IElement* pElement = pFile;
         if( !pElement->Save(pArc) ) return false;
         // Recurse into sub-folders
         TCHAR szType[64] = { 0 };
         pElement->GetType(szType, 63);
         if( _tcscmp(szType, _T("Folder")) == 0 ) {
            if( !_SaveFiles(pArc, pFile) ) return false;
         }
         if( !pArc->WriteGroupEnd() ) return false;
      }
   }
   if( !pArc->WriteGroupEnd() ) return false;
   return true;
}

void CRemoteProject::_PopulateTree(CTreeViewCtrl& ctrlTree, 
                                   IElement* pParent, 
                                   HTREEITEM hParent) const
{
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      IView* pFile = m_aFiles[i];
      if( pFile->GetParent() == pParent ) {
         int iImage = _GetElementImage(pFile);
         CString sName;
         pFile->GetName(sName.GetBufferSetLength(128), 128);
         sName.ReleaseBuffer();
         HTREEITEM hNew = ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
               sName, 
               iImage, iImage, 
               0, 0,
               (LPARAM) pFile,
               hParent, TVI_LAST);
         _PopulateTree(ctrlTree, pFile, hNew);
      }
   }
   TVSORTCB tvscb = { 0 };
   tvscb.hParent = hParent;
   tvscb.lParam = (LPARAM) this;
   tvscb.lpfnCompare = _SortTreeCB;
   ctrlTree.SortChildrenCB(&tvscb);
}

int CRemoteProject::_GetElementImage(IElement* pElement) const
{
   ATLASSERT(pElement);
   TCHAR szType[64] = { 0 };
   pElement->GetType(szType, 63);
   if( _tcscmp(szType, _T("Folder")) == 0 ) return IMAGE_FOLDER;
   if( _tcscmp(szType, _T("CPP")) == 0 ) return IMAGE_CPP;
   if( _tcscmp(szType, _T("XML")) == 0 ) return IMAGE_XML;
   if( _tcscmp(szType, _T("Java")) == 0 ) return IMAGE_JAVA;
   if( _tcscmp(szType, _T("HTML")) == 0 ) return IMAGE_HTML;
   if( _tcscmp(szType, _T("BASIC")) == 0 ) return IMAGE_BASIC;
   if( _tcscmp(szType, _T("Header")) == 0 ) return IMAGE_HEADER;
   if( _tcscmp(szType, _T("Makefile")) == 0 ) return IMAGE_MAKEFILE;
   return IMAGE_TEXT;
}

IElement* CRemoteProject::_GetSelectedTreeElement(HTREEITEM* phItem /*= NULL*/) const
{
   // Get the active/selected tree-item
   if( phItem ) *phItem = NULL;
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   HTREEITEM hItem = ctrlTree.GetDropHilightItem();
   if( hItem == NULL ) hItem = ctrlTree.GetSelectedItem();
   if( hItem == NULL ) return NULL;
   if( phItem ) *phItem = hItem;
   // Let's check that this tree-item actually belongs to this project
   LPARAM lParam = ctrlTree.GetItemData(hItem);
   if( lParam == NULL ) return NULL;
   bool bLocal = false;
   if( lParam == (LPARAM) this ) bLocal = true;
   for( int i = 0; !bLocal && i < m_aFiles.GetSize(); i++ ) if( (LPARAM) m_aFiles[i] == lParam ) bLocal = true;
   if( !bLocal ) return NULL;
   return (IElement*) lParam;
}

IElement* CRemoteProject::_GetDropTreeElement(HTREEITEM* phItem /*= NULL*/) const
{
   if( phItem ) *phItem = NULL;
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   HTREEITEM hItem = ctrlTree.GetDropHilightItem();
   if( hItem == NULL ) return NULL;
   if( phItem ) *phItem = hItem;
   LPARAM lParam = ctrlTree.GetItemData(hItem);
   if( lParam == NULL ) return NULL;
   bool bLocal = false;
   if( lParam == (LPARAM) this ) bLocal = true;
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) if( (LPARAM) m_aFiles[i] == lParam ) bLocal = true;
   if( !bLocal ) return NULL;
   return (IElement*) lParam;
}

UINT CRemoteProject::_GetMenuPosFromID(HMENU hMenu, UINT ID) const
{
   UINT nCount = ::GetMenuItemCount(hMenu);
   for( UINT i = 0; i < nCount; i++ ) {
      if( ::GetMenuItemID(hMenu, i) == ID ) return i;
   }
   return (UINT) -1;
}

int CALLBACK CRemoteProject::_SortTreeCB(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   IView* pItem1 = (IView*) lParam1;
   IView* pItem2 = (IView*) lParam2;
   TCHAR szName1[128];
   TCHAR szName2[128];
   TCHAR szType1[64];
   TCHAR szType2[64];
   pItem1->GetName(szName1, 127);
   pItem2->GetName(szName2, 127);
   pItem1->GetType(szType1, 63);
   pItem2->GetType(szType2, 63);
   bool bIsFolder1 = _tcscmp(szType1, _T("Folder")) == 0;
   bool bIsFolder2 = _tcscmp(szType2, _T("Folder")) == 0;
   if( bIsFolder1 && !bIsFolder2 ) return -1;
   if( !bIsFolder1 && bIsFolder2 ) return 1;
   if( bIsFolder1 && bIsFolder2 ) return 0; // Folders are not sorted alphabetically!
   return _tcsicmp(szName1, szName2);
}

void CRemoteProject::_AddButtonText(CToolBarCtrl tb, UINT nID, UINT nRes)
{
#ifndef BTNS_SHOWTEXT
   const UINT BTNS_SHOWTEXT = 0x0040;
#endif  // BTNS_SHOWTEXT
   TBBUTTONINFO tbi = { 0 };
   tbi.cbSize = sizeof(TBBUTTONINFO),
   tbi.dwMask  = TBIF_STYLE;
   tb.GetButtonInfo(nID, &tbi);
   tbi.dwMask = TBIF_STYLE | TBIF_TEXT;
   tbi.fsStyle |= TBSTYLE_AUTOSIZE | BTNS_SHOWTEXT;
   CString s(MAKEINTRESOURCE(nRes));
   tbi.pszText = (LPTSTR) (LPCTSTR) s;
   tb.SetButtonInfo(nID, &tbi);
}

void CRemoteProject::_AddDropDownButton(CToolBarCtrl tb, UINT nID)
{
   tb.SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS); 

   TBBUTTONINFO tbi = { 0 };
   tbi.cbSize  = sizeof(TBBUTTONINFO);
   tbi.dwMask  = TBIF_STYLE;
   tb.GetButtonInfo(nID, &tbi);
   tbi.fsStyle |= BTNS_WHOLEDROPDOWN;
   tb.SetButtonInfo(nID, &tbi);
}

void CRemoteProject::_AddControlToToolbar(CToolBarCtrl tb, HWND hWnd, USHORT cx, UINT nCmdPos, bool bIsCommandId, bool bInsertBefore /*= true*/)
{
   ATLASSERT(::IsWindow(hWnd));
   static UINT s_nCmd = 45000; // Unique number for new toolbar separator
   int iIndex = nCmdPos;
   if( bIsCommandId ) iIndex = tb.CommandToIndex(nCmdPos);
   if( !bInsertBefore ) iIndex++;
   
   // Create a separator toolbar button
   TBBUTTON but = { 0 };
   but.fsStyle = TBSTYLE_SEP;
   but.fsState = TBSTATE_ENABLED;
   but.idCommand = s_nCmd;
   BOOL bRes = tb.InsertButton(iIndex, &but);
   ATLASSERT(bRes); bRes;
   TBBUTTONINFO info;
   info.cbSize = sizeof(info);
   info.dwMask = TBIF_SIZE;
   info.cx = cx;
   tb.SetButtonInfo(s_nCmd++, &info);

   // Chain the control to its new parent
   ::SetParent(hWnd, tb);
   // Position the new control on top of the separator
   RECT rc;
   tb.GetItemRect(iIndex, &rc);
   ::SetWindowPos(hWnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE);
}

bool CRemoteProject::_AddCommandBarImages(UINT nRes) const
{
   CImageListCtrl Images;
   Images.Create(16, 16, ILC_COLOR | ILC_MASK, 40, 1);
   CBitmap bmp;
   bmp.LoadBitmap(nRes);
   ATLASSERT(!bmp.IsNull());
   if( bmp.IsNull() ) return false;

   HRSRC hRsrc = ::FindResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(nRes), (LPTSTR) RT_TOOLBAR);
   ATLASSERT(hRsrc);
   if( hRsrc == NULL ) return false;
   HGLOBAL hGlobal = ::LoadResource(_Module.GetResourceInstance(), hRsrc);
   ATLASSERT(hGlobal);
   if( hGlobal == NULL ) return false;

   struct _ToolBarData   // toolbar resource data
   {
      WORD wVersion;
      WORD wWidth;
      WORD wHeight;
      WORD wItemCount;
      WORD* GetItems() { return (WORD*)(this + 1); }
   };
   _ToolBarData* pData = (_ToolBarData*) ::LockResource(hGlobal);
   if( pData == NULL ) return FALSE;
   ATLASSERT(pData->wVersion==1);

   if( Images.Add(bmp, RGB(192,192,192)) == -1) return false;

   // Fill the array with command IDs
   WORD* pItems = pData->GetItems();
   int nItems = pData->wItemCount;
   for( int i = 0; i < nItems; i++ ) {
      CIcon icon = Images.GetIcon(i);
      ATLASSERT(!icon.IsNull());
      _pDevEnv->AddCommandBarImage(pItems[i], icon);
   }
   return true;
}

bool CRemoteProject::_CreateNewRemoteFile(HWND hWnd, IView* pView)
{
   CString sModule;
   sModule.Format(_T("%sTemplates.dll"), CModulePath());
   HINSTANCE hInst = ::LoadLibrary(sModule);
   if( hInst == NULL ) return false;

   typedef BOOL (APIENTRY* LPFNNEWFILE)(HWND, IDevEnv*, ISolution*, IProject*, IView*);
   LPFNNEWFILE fnCreate = (LPFNNEWFILE) ::GetProcAddress(hInst, "Templates_NewFile");
   if( fnCreate == NULL ) return false;
   if( !fnCreate(hWnd, _pDevEnv, _pDevEnv->GetSolution(), this, pView) ) return false;

   return TRUE;
}

bool CRemoteProject::_RunFileWizard(HWND hWnd, LPCTSTR pstrName, IView* pView)
{
   CString sModule;
   sModule.Format(_T("%sTemplates.dll"), CModulePath());
   HINSTANCE hInst = ::LoadLibrary(sModule);
   if( hInst == NULL ) return false;

   typedef BOOL (APIENTRY* LPFNRUNWIZARD)(HWND, LPCTSTR, IDevEnv*, ISolution*, IProject*, IView*);
   LPFNRUNWIZARD fnRunWizard = (LPFNRUNWIZARD) ::GetProcAddress(hInst, "Templates_RunWizard");
   if( fnRunWizard == NULL ) return false;
   if( !fnRunWizard(hWnd, pstrName, _pDevEnv, _pDevEnv->GetSolution(), this, pView) ) return false;

   return TRUE;
}

bool CRemoteProject::_CheckProjectFile(LPCTSTR pstrFilename, LPCTSTR pstrName, bool bRemote) 
{
   CString sUpperName = pstrName;
   sUpperName.MakeUpper();

   // Check that the file doesn't already exist in the
   // project filelist.
   for( int i = 0; i < m_aFiles.GetSize(); i++ ) {
      CString sName;
      m_aFiles[i]->GetName(sName.GetBufferSetLength(128), 128);
      sName.ReleaseBuffer();
      if( sName.CompareNoCase(pstrName) == 0 ) {
         ::SetLastError(0L);
         GenerateError(_pDevEnv, IDS_ERR_FILEINCLUDED);
         return false;
      }       
   }

   // Remove current tag information if it looks like a new
   // tag file has been added...
   if( sUpperName.Find(_T("TAGS")) >= 0 ) {
      m_viewClassTree.Lock();
      m_TagManager.m_TagInfo.MergeFile(pstrName);
      m_viewClassTree.Rebuild();
      m_viewClassTree.Unlock();
   }

   // New C++ files are added to class-view immediately if online-scanner
   // is running...
   if( bRemote 
       && (sUpperName.Find(_T(".H")) > 0 || sUpperName.Find(_T(".C")) > 0) ) 
   {
      if( m_TagManager.m_LexInfo.IsAvailable() ) {
         LPSTR pstrText = NULL;
         DWORD dwSize = 0;
         if( m_FileManager.LoadFile(pstrFilename, true, (LPBYTE*) &pstrText, &dwSize) ) {
            m_TagManager.m_LexInfo.MergeFile(pstrFilename, pstrText);
            free(pstrText);
         }
      }
   }

   return true;
}

IView* CRemoteProject::_CreateDependencyFile(LPCTSTR pstrFilename, LPCTSTR pstrName)
{
   // See if we can find the file...
   CString sFilename = m_FileManager.FindFile(pstrFilename);
   if( sFilename.IsEmpty() ) return NULL;
   
   // Create new object
   IView* pView = _pDevEnv->CreateView(pstrFilename, this, this);
   if( pView == NULL ) return false;

   // Load some default properties
   CViewSerializer arc;
   arc.Add(_T("name"), pstrName);
   arc.Add(_T("filename"), sFilename);
   arc.Add(_T("location"), _T("remote"));
   pView->Load(&arc);

   // Add view to collection
   m_aDependencies.Add(pView);

   return pView;
}

// Static members

CAccelerator CRemoteProject::m_accel;
CToolBarCtrl CRemoteProject::m_ctrlBuild;
CToolBarCtrl CRemoteProject::m_ctrlDebug;
CToolBarCtrl CRemoteProject::m_ctrlBookmarks;
CToolBarCtrl CRemoteProject::m_ctrlSearch;
CComboBox CRemoteProject::m_ctrlMode;
CClassView CRemoteProject::m_viewClassTree;
CTelnetView CRemoteProject::m_viewDebugLog;
CTelnetView CRemoteProject::m_viewCompileLog;
CWatchView CRemoteProject::m_viewWatch;
CStackView CRemoteProject::m_viewStack;
CBreakpointView CRemoteProject::m_viewBreakpoint;
CRegisterView CRemoteProject::m_viewRegister;
CMemoryView CRemoteProject::m_viewMemory;
CDisasmView CRemoteProject::m_viewDisassembly;
CVariableView CRemoteProject::m_viewVariable;
CThreadView CRemoteProject::m_viewThread;
