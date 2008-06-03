
#include "stdafx.h"
#include "resource.h"

#include "Project.h"
#include "Commands.h"

#include "ArgumentsDlg.h"
#include "RemoteFileDlg.h"
#include "RebuildLexDlg.h"
#include "AttachProcessDlg.h"
#include "AttachCoreFileDlg.h"

#include "ViewSerializer.h"


// Message handlers

LRESULT CRemoteProject::OnFileRemove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   HTREEITEM hItem = NULL;
   IElement* pElement = _GetSelectedTreeElement(&hItem);
   if( pElement == NULL ) { bHandled = FALSE; return 0; }
   // Remove tree item
   CTreeViewCtrl ctrlTree = _pDevEnv->GetHwnd(IDE_HWND_EXPLORER_TREE);
   // Remove entire project or file
   if( pElement == this ) {
      ctrlTree.DeleteItem(hItem);
      ISolution* pSolution = _pDevEnv->GetSolution();
      pSolution->RemoveProject(this);
      // NOTE: Never use class-scope variables from this point
      //       since this instance has been nuked!
   }
   else {
      IView* pView = static_cast<IView*>(pElement);
      _RemoveView(pView);
      pView->CloseView();
      ctrlTree.DeleteItem(hItem);
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

   // Make sure the project is saved to a file.
   // Need a project filename so we can make local filenames relative.
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
      //       convert it back to Windows filename-slashes because the Windows API
      //       doesn't work with unix-type slashes (/ vs \).
      // TODO: Do a real implementation of PathRelativePathTo().
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
   if( m_DebugManager.IsDebugging() ) m_DebugManager.Break();
   if( m_CompileManager.IsCompiling() ) m_CompileManager.DoAction(_T("Stop"));
   bHandled = FALSE;
   return 0;
}

LRESULT CRemoteProject::OnViewOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) { bHandled = FALSE; return 0; }
   if( pElement == this ) return 0;

   IView* pView = static_cast<IView*>(pElement);
   if( !pView->OpenView(1) ) {
      DWORD dwErr = ::GetLastError();
      HWND hWnd = ::GetActiveWindow();
      // View didn't exist. Let's ask the user if he wants to create a new file
      // with that name...
      if( dwErr == ERROR_FILE_NOT_FOUND ) {
         if( IDYES == _pDevEnv->ShowMessageBox(hWnd, CString(MAKEINTRESOURCE(IDS_CREATEFILE)), CString(MAKEINTRESOURCE(IDS_CAPTION_QUESTION)), MB_YESNO | MB_ICONQUESTION) ) {
            _CreateNewRemoteFile(hWnd, pView);
         }
      }
      else {
         GenerateError(_pDevEnv, hWnd, IDS_ERR_OPENVIEW, dwErr);
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
   ATLASSERT(m_viewCompileLog.IsWindow());
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
   ATLASSERT(m_viewDebugLog.IsWindow());
   if( !m_viewDebugLog.IsWindowVisible() ) {
      RECT rcWin = { 20, 20, 400, 400 };
      _pDevEnv->AddDockView(m_viewDebugLog, IDE_DOCK_FLOAT, rcWin);
   }
   else {
      _pDevEnv->AddDockView(m_viewDebugLog, IDE_DOCK_HIDE, CWindow::rcDefault);
   }
   
   return 0;
}

template< typename TView >
bool ShowOrHideViewWindow(CRemoteProject* pProject, TView& view, UINT nCaption, IDE_DOCK_TYPE DockDefault, CRect rcStartup, DWORD dwExtStyle)
{
   // Create the view if needed
   if( !view.IsWindow() ) {
      CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
      CString sTitle(MAKEINTRESOURCE(nCaption));
      DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      view.Init(pProject);
      view.Create(wndMain, CWindow::rcDefault, sTitle, dwStyle, dwExtStyle);
      _pDevEnv->AddDockView(view, IDE_DOCK_HIDE, CWindow::rcDefault);
   }
   ATLASSERT(view.IsWindow());

   // Dock and position the view
   if( !view.IsWindowVisible() ) {
      IDE_DOCK_TYPE DockType;
      RECT rcWindow = { 0 };
      if( pProject->m_DockManager.GetInfo(view, DockType, rcWindow) ) {
         _pDevEnv->AddDockView(view, DockType, rcWindow);
      }
      else {
         pProject->m_DockManager.SetInfo(view, DockDefault, rcStartup);
         _pDevEnv->AddDockView(view, DockDefault, rcStartup);
      }
      return true;
   }
   else {
      _pDevEnv->AddDockView(view, IDE_DOCK_HIDE, CWindow::rcDefault);
      return false;
   }
}

LRESULT CRemoteProject::OnViewBreakpoints(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Breakpoint view
   if( ShowOrHideViewWindow(this, m_viewBreakpoint, IDS_CAPTION_BREAKPOINTS, IDE_DOCK_BOTTOM, CRect(420, 40, 780, 300), WS_EX_CLIENTEDGE) ) {
      DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewRegisters(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the CPU Registers view
   if( ShowOrHideViewWindow(this, m_viewRegister, IDS_CAPTION_REGISTERS, IDE_DOCK_FLOAT, CRect(420, 40, 640, 300), WS_EX_CLIENTEDGE) ) {
      DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewMemory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Memory Dump view
   if( ShowOrHideViewWindow(this, m_viewMemory, IDS_CAPTION_MEMORY, IDE_DOCK_FLOAT, CRect(60, 140, 780, 400), WS_EX_CLIENTEDGE) ) {
      DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewDisassembly(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Dissembly view
   if( ShowOrHideViewWindow(this, m_viewDisassembly, IDS_CAPTION_DISASSEMBLY, IDE_DOCK_FLOAT, CRect(320, 50, 690, 400), WS_EX_CLIENTEDGE) ) {
      DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewThreads(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Thread List view
   if( ShowOrHideViewWindow(this, m_viewThread, IDS_CAPTION_THREADS, IDE_DOCK_BOTTOM, CRect(420, 220, 600, 450), WS_EX_CLIENTEDGE) ) {
      DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewStack(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Stack Backtrace view
   if( ShowOrHideViewWindow(this, m_viewStack, IDS_CAPTION_CALLSTACK, IDE_DOCK_BOTTOM, CRect(420, 520, 780, 650), 0) ) {
      DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewVariables(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Variables view
   if( ShowOrHideViewWindow(this, m_viewVariable, IDS_CAPTION_VARIABLES, IDE_DOCK_BOTTOM, CRect(450, 520, 760, 750), WS_EX_CLIENTEDGE) ) {
      DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewWatch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Watches view
   if( ShowOrHideViewWindow(this, m_viewWatch, IDS_CAPTION_WATCH, IDE_DOCK_FLOAT, CRect(320, 180, 750, 450), WS_EX_CLIENTEDGE) ) {
      // We'll need to add the watches again.
      // This should be the first this we do, because otherwise the debugger views
      // will try to evaluate the watches before they have been created.
      // BUG: GDB doesn't allow us to add "delayed" watches in this version, so
      //      we'll get errors complaining about undefined variables. A fix
      //      for this is placed inside the GDB command handler.
      m_viewWatch.ActivateWatches();
      DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   }

   return 0;
}

LRESULT CRemoteProject::OnViewOutput(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }

   // Create the Console Output view
   if( !m_viewOutput.IsWindow() ) {
      CString sTitle(MAKEINTRESOURCE(IDS_CAPTION_DEBUGOUTPUT));
      DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      m_viewOutput.Create(m_wndMain, CWindow::rcDefault, sTitle, dwStyle, WS_EX_CLIENTEDGE);
      m_viewOutput.SetColors(::GetSysColor(COLOR_WINDOWTEXT), ::GetSysColor(COLOR_WINDOW));
      _pDevEnv->AddDockView(m_viewOutput, IDE_DOCK_HIDE, CWindow::rcDefault);
   }

   // Position the view
   if( !m_viewOutput.IsWindowVisible() ) {
      IDE_DOCK_TYPE DockType;
      RECT rcWindow = { 0 };
      if( m_DockManager.GetInfo(m_viewOutput, DockType, rcWindow) ) {
         _pDevEnv->AddDockView(m_viewOutput, DockType, rcWindow);
      }
      else {
         RECT rcDefault = { 120, 40, 640, 400 };
         m_DockManager.SetInfo(m_viewOutput, IDE_DOCK_TOP, rcDefault);
         _pDevEnv->AddDockView(m_viewOutput, IDE_DOCK_TOP, rcDefault);
      }
      DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   }
   else {
      _pDevEnv->AddDockView(m_viewOutput, IDE_DOCK_HIDE, CWindow::rcDefault);
      m_viewOutput.Close();
   }

   return 0;
}

LRESULT CRemoteProject::OnViewRemoteDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   // Create/remove Remote File view
   if( !m_viewRemoteDir.IsWindow() ) {
      CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
      CString sTitle(MAKEINTRESOURCE(IDS_FILEMANAGER));
      DWORD dwStyle = WS_CHILD | WS_VISIBLE;
      m_viewRemoteDir.Create(wndMain, CWindow::rcDefault, sTitle, dwStyle);
      _pDevEnv->AddAutoHideView(m_viewRemoteDir, IDE_DOCK_LEFT, 7);
      _pDevEnv->ActivateAutoHideView(m_viewRemoteDir);
   }
   else {
      _pDevEnv->RemoveAutoHideView(m_viewRemoteDir);
      m_viewRemoteDir.PostMessage(WM_CLOSE);
   }
   return 0;
}

LRESULT CRemoteProject::OnViewSymbols(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   // Create/remove Symbols view
   if( !m_viewSymbols.IsWindow() ) {
      CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
      CString sTitle(MAKEINTRESOURCE(IDS_SYMBOLVIEW));
      DWORD dwStyle = WS_CHILD | WS_VISIBLE;
      m_viewSymbols.Create(wndMain, CWindow::rcDefault, sTitle, dwStyle);
      _pDevEnv->AddExplorerView(m_viewSymbols, sTitle, 7);
      _pDevEnv->ActivateExplorerView(m_viewSymbols);
   }
   else {
      _pDevEnv->RemoveExplorerView(m_viewSymbols);
      m_viewSymbols.PostMessage(WM_CLOSE);
   }
   return 0;
}

LRESULT CRemoteProject::OnViewProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   // Show Project Settings dialog
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement != this ) { bHandled = FALSE; return 0; }
   if( _pDevEnv->ShowConfiguration(pElement) ) m_bIsDirty = true;
   return 0;
}

LRESULT CRemoteProject::OnProjectSetDefault(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   IElement* pElement = _GetSelectedTreeElement();
   if( pElement == NULL ) { bHandled = FALSE; return 0; };
   // The selected element must be a project type
   CString sType;
   pElement->GetType(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();
   if( sType != _T("Project") ) return 0;
   // Cast it back and mark as active
   IProject* pProject = static_cast<IProject*>(pElement);
   if( _pDevEnv->GetSolution()->SetActiveProject(pProject) ) m_bIsDirty = true;
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
   m_DebugManager.ProgramStop();
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

LRESULT CRemoteProject::OnDebugStepInstruction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   DelayedDebugCommand(_T("-exec-step-instruction"));
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

   // Remove previous modeless dialog
   if( m_pQuickWatchDlg != NULL ) delete m_pQuickWatchDlg;

   // Ask active editor window to retrieve the selected/caret text
   LAZYDATA data;
   data.Action = LAZY_SEND_GLOBAL_VIEW_MESSAGE;
   data.wParam = DEBUG_CMD_GET_CARET_TEXT;
   m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_DEBUG_EDIT_LINK, data.wParam), (LPARAM) &data);

   m_pQuickWatchDlg = new CQuickWatchDlg(_pDevEnv, this, data.szMessage);
   ATLASSERT(m_pQuickWatchDlg);
   m_pQuickWatchDlg->Create(m_wndMain);
   m_pQuickWatchDlg->ShowWindow(SW_SHOW);

   return 0;
}

LRESULT CRemoteProject::OnDebugProcesses(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   if( m_DebugManager.IsDebugging() ) return 0;
   CAttachProcessDlg dlg;
   dlg.Init(this, m_DebugManager.GetParam(_T("App")));
   if( dlg.DoModal() != IDOK ) return 0;
   return m_DebugManager.AttachProcess(dlg.GetSelectedPid());
}

LRESULT CRemoteProject::OnDebugCoreFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   if( m_DebugManager.IsDebugging() ) return 0;
   CAttachCoreFileDlg dlg;
   dlg.Init(this, m_DebugManager.GetParam(_T("CoreProcess")), m_DebugManager.GetParam(_T("CoreFile")));
   if( dlg.DoModal() != IDOK ) return 0;
   return m_DebugManager.AttachCoreFile(dlg.GetSelectedProcess(), dlg.GetSelectedCoreFile());
}

LRESULT CRemoteProject::OnDebugArguments(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   if( m_DebugManager.IsDebugging() ) return 0;
   CRunArgumentsDlg dlg;
   dlg.Init(this);
   if( dlg.DoModal() == IDOK ) m_bIsDirty = true;
   return 0;
}

LRESULT CRemoteProject::OnBuildClean(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( !_ShouldProcessMessage() ) { bHandled = FALSE; return 0; }
   return (LRESULT) m_CompileManager.DoAction(_T("Clean"));
}

LRESULT CRemoteProject::OnBuildProject(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( !_ShouldProcessMessage() ) { bHandled = FALSE; return 0; }
   return (LRESULT) m_CompileManager.DoAction(_T("Build"));
}

LRESULT CRemoteProject::OnBuildRebuild(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( !_ShouldProcessMessage() ) { bHandled = FALSE; return 0; }
   return (LRESULT) m_CompileManager.DoAction(_T("Rebuild"));
}

LRESULT CRemoteProject::OnBuildSolution(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   // The "Build Solution" command imposes a bit of a problem, because it is really a system wide
   // command. The trick is to start the command in a separate thread which will iterate all projects
   // and call the "Rebuild" scripting command (if avaiable). We eat the message here, because we (the
   // first handler) is responsible for calling Rebuild on all other projects!
   // Open and clear the compile output view
   return (LRESULT) m_CompileManager.DoRebuild();
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

   // We'll need the filename of the selected file to compile...
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

   // We'll need the filename of the selected file...
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

LRESULT CRemoteProject::OnBuildCTags(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   m_CompileManager.DoAction(_T("BuildTags"));
   m_TagManager.m_TagInfo.Clear();
   return 0;
}

LRESULT CRemoteProject::OnBuildLexTags(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
   if( _pDevEnv->GetSolution()->GetActiveProject() != this ) { bHandled = FALSE; return 0; }
   CRebuildLexDlg dlg;
   dlg.Init(this);
   dlg.DoModal();
   return 0;
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
      GenerateError(_pDevEnv, NULL, IDS_ERR_NOFILEWIZARD, 0);
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

