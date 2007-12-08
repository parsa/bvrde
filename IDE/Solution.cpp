
#include "StdAfx.h"
#include "resource.h"

#include "Solution.h"
#include "MainFrm.h"
#include "XmlSerializer.h"


CSolution::CSolution() :
   m_Dispatch(this)
{
   Init(NULL);
}

void CSolution::Init(CMainFrame* pFrame)
{
   m_pMainFrame = pFrame;
   m_pCurProject = NULL;
   m_sName.LoadString(IDS_SOLUTIONNAME);
   m_bIsLoaded = FALSE;
   m_bIsDirty = FALSE;
}

bool CSolution::CreateExplorerUI()
{
   CLockStaticDataInit lock;

   // Allow projects to create the nessecary tree items and stuff
   for( int i = 0; i < m_aProjects.GetSize(); i++ ) {
      IProject* pProject = m_aProjects[i].pProject;
      pProject->CreateProject();
   }

   // Set the default project if needed
   IProject* pProject = m_pCurProject;
   if( pProject == NULL && GetItemCount() > 0 ) pProject = GetItem(GetItemCount() - 1);
   SetActiveProject(pProject);

   return true;
}

// ISolution

void CSolution::Close()
{
   // Close first
   g_pDevEnv->ShowStatusText(ID_DEFAULT_PANE, CString(MAKEINTRESOURCE(IDS_STATUS_CLOSING)));
   // Clear attributes
   m_sFilename.Empty();
   m_sName.LoadString(IDS_SOLUTIONNAME);
   // Deactivate
   SetActiveProject(NULL);
   // Delete projects
   for( int i = 0; i < m_aProjects.GetSize(); i++ ) {
      PROJECT& Project = m_aProjects[i];
      Project.pProject->Close();
      Project.pPlugin->DestroyProject(Project.pProject);
   }
   m_aProjects.RemoveAll();
   m_bIsLoaded = FALSE;
   m_bIsDirty = FALSE;
}

BOOL CSolution::Load(ISerializable* pArc)
{
   pArc->Read(_T("name"), m_sName.GetBufferSetLength(MAX_PATH), MAX_PATH);
   m_sName.ReleaseBuffer();

   for( int i = 0; i < m_aProjects.GetSize(); i++ ) {
      if( !m_aProjects[i].pProject->Load(pArc) ) return FALSE;
   }

   return TRUE;
}

BOOL CSolution::Save(ISerializable* pArc)
{
   pArc->Write(_T("name"), m_sName);

   if( pArc->WriteExternal(_T("Project")) ) {
      for( int i = 0; i < m_aProjects.GetSize(); i++ ) {
         if( !m_aProjects[i].pProject->Save(pArc) ) return FALSE;
      }
   }

   return TRUE;
}

BOOL CSolution::LoadSolution(LPCTSTR pstrFilename)
{
   ATLASSERT(m_pMainFrame);

   CLockStaticDataInit lock;

   Close();

   IProject* pDefaultProject = NULL;
   if( !_LoadSolution(pstrFilename, pDefaultProject) ) {
      Close();
      return FALSE;
   }

   m_sFilename = pstrFilename;
   m_bIsLoaded = TRUE;
   m_bIsDirty = FALSE;
   
   if( pDefaultProject ) SetActiveProject(pDefaultProject);
   m_pMainFrame->PostMessage(WM_APP_BUILDSOLUTIONUI);

   return TRUE;
}

BOOL CSolution::SaveSolution(LPCTSTR pstrFilename)
{
   HWND hWndMain = g_pDevEnv->GetHwnd(IDE_HWND_MAIN);

   // Make sure we have a filename for the solution
   CString sFilename = pstrFilename;
   if( sFilename.IsEmpty() ) sFilename = m_sFilename;   
   if( sFilename.IsEmpty() ) {
      // Warn user first
      m_pMainFrame->_ShowMessageBox(hWndMain, IDS_SAVE_FIRST, IDS_CAPTION_MESSAGE, MB_ICONINFORMATION);
      // Browse for the project file (locally)
      CString sPath;
      sPath.Format(_T("%s%s"), CModulePath(), _T(SOLUTIONDIR));
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_SAVESOLUTION));
      CString sFilter(MAKEINTRESOURCE(IDS_FILTER_SOLUTION));
      for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == '|' ) sFilter.SetAt(i, '\0');
      DWORD dwStyle = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ENABLESIZING;
      CFileDialog dlg(FALSE, _T("sln"), NULL, dwStyle, sFilter, hWndMain);
      dlg.m_ofn.lpstrTitle = sCaption;
      dlg.m_ofn.Flags &= ~OFN_ENABLEHOOK;
      dlg.m_ofn.lpstrInitialDir = sPath;
      if( dlg.DoModal(hWndMain) != IDOK ) return FALSE;
      sFilename = dlg.m_ofn.lpstrFile;
   }
   m_sFilename = sFilename;

   // Store in Recent Used Files
   m_pMainFrame->m_mru.AddToList(m_sFilename);
   m_pMainFrame->m_mru.WriteToRegistry(REG_BVRDE _T("\\Mru"));

   // Make sure that all projects have filenames as well...
   for( int i = 0; i < m_aProjects.GetSize(); i++ ) {
      PROJECT& Project = m_aProjects[i];
      if( Project.sFilename.IsEmpty() ) {
         CString sPath;
         sPath.Format(_T("%s%s"), CModulePath(), _T(SOLUTIONDIR));
         CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_SAVEPROJECT));
         CString sFilter(MAKEINTRESOURCE(IDS_FILTER_PROJECT));
         for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == '|' ) sFilter.SetAt(i, '\0');
         DWORD dwStyle = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ENABLESIZING;
         CFileDialog dlg(FALSE, _T("prj"), NULL, dwStyle, sFilter, hWndMain);
         dlg.m_ofn.lpstrTitle = sCaption;
         dlg.m_ofn.Flags &= ~OFN_ENABLEHOOK;
         dlg.m_ofn.lpstrInitialDir = sPath;
         if( dlg.DoModal(hWndMain) != IDOK ) return FALSE;
         Project.sFilename = dlg.m_ofn.lpstrFile;
      }
   }

   // Finally, we can save our data
   if( !_SaveSolution() ) {
      CString sText(MAKEINTRESOURCE(IDS_ERR_SAVESOLUTION));
      sText += _T("\n");
      sText += _GetComErrorText();
      m_pMainFrame->ShowMessageBox(hWndMain, sText, CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONERROR);
      return FALSE;
   }

   // It's not dirty anymore
   m_bIsDirty = FALSE;

   return TRUE;
}

BOOL CSolution::IsLoaded() const
{
   return m_bIsLoaded;
}

BOOL CSolution::IsDirty() const
{
   CLockStaticDataInit lock;
   if( !IsLoaded() ) return FALSE;
   if( m_bIsDirty ) return TRUE;
   // The solution's dirty flag inherits the state of the project-files.
   for( int i = 0; i < m_aProjects.GetSize(); i++ ) {
      if( m_aProjects[i].pProject->IsDirty() ) return TRUE;
   }
   return FALSE;
}

BOOL CSolution::GetName(LPTSTR pstrName, UINT cchMax) const
{
   _tcsncpy(pstrName, m_sName, cchMax);
   return TRUE;
}

BOOL CSolution::GetFileName(LPTSTR pstrFilename, UINT cchMax) const
{
   _tcsncpy(pstrFilename, m_sFilename, cchMax);
   return TRUE;
}

BOOL CSolution::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Solution"), cchMax);
   return TRUE;
}

IElement* CSolution::GetParent() const
{
   return NULL;
}

IDispatch* CSolution::GetDispatch()
{
   CSolution* pThis = const_cast<CSolution*>(this);
   return &pThis->m_Dispatch;
}

IProject* CSolution::GetItem(int iIndex)
{
   CLockStaticDataInit lock;
   if( iIndex < 0 || iIndex >= m_aProjects.GetSize() ) return NULL;
   return m_aProjects[iIndex].pProject;
}

int CSolution::GetItemCount() const
{
   CLockStaticDataInit lock;
   return m_aProjects.GetSize();
}

BOOL CSolution::AddProject(IProject* pProject)
{
   ATLASSERT(pProject);
   if( pProject == NULL ) return FALSE;
   
   CString sName;
   pProject->GetName(sName.GetBufferSetLength(MAX_PATH), MAX_PATH);
   sName.ReleaseBuffer();

   CString sType;
   pProject->GetClass(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();

   CPlugin* pPlugin = NULL;
   for( int i = 0; i < g_aPlugins.GetSize(); i++ ) {
      if( g_aPlugins[i].GetName() == sType ) pPlugin = &g_aPlugins[i];
   }
   ATLASSERT(pPlugin);
   if( pPlugin == NULL ) return FALSE;

   PROJECT Project;
   Project.pProject = pProject;
   Project.pPlugin = pPlugin;
   Project.sType = sType;
   m_aProjects.Add(Project);

   // Update Solution state
   m_bIsDirty = TRUE;
   m_bIsLoaded = TRUE;

   // Ask project tree to refresh itself
   m_pMainFrame->PostMessage(WM_APP_BUILDSOLUTIONUI);

   return TRUE;
}

BOOL CSolution::AddProject(LPCTSTR pstrFilename)
{
   ATLASSERT(!::IsBadStringPtr(pstrFilename,-1));
   
   // Attempt to load the project-file (it should at least be XML)
   // and determine the project type.
   CXmlSerializer arc;
   if( !arc.Open(_T("Project"), pstrFilename) ) return FALSE;
   CString sType;
   arc.Read(_T("type"), sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();
   if( sType.IsEmpty() ) return FALSE;
   arc.Close();

   // Now load and add it into the solution
   TCHAR szRelativeName[MAX_PATH];
   ::PathRelativePathTo(szRelativeName, m_sFilename, FILE_ATTRIBUTE_NORMAL, pstrFilename, FILE_ATTRIBUTE_NORMAL);
   if( _tcslen(szRelativeName) == 0 ) _tcscpy(szRelativeName, pstrFilename);
   if( !_AddProject(m_sFilename, szRelativeName, sType) ) return false;

   // Update Solution state
   m_bIsDirty = TRUE;
   m_bIsLoaded = TRUE;

   // Ask project tree to refresh itself
   m_pMainFrame->PostMessage(WM_APP_BUILDSOLUTIONUI);

   return TRUE;
}

BOOL CSolution::RemoveProject(IProject* pProject)
{
   ATLASSERT(pProject);
   if( pProject == NULL ) return FALSE;
   // Let's see if we can find the project first...
   int i;
   for( i = 0; i < m_aProjects.GetSize(); i++ ) {
      if( m_aProjects[i].pProject == pProject ) break;
   }
   if( i == m_aProjects.GetSize() ) return FALSE;
   // Remove activation
   SetActiveProject(NULL);
   // Destroy our project
   pProject->Close();
   m_aProjects[i].pPlugin->DestroyProject(pProject);
   m_aProjects.RemoveAt(i);
   // Active previous project in list
   SetActiveProject(GetItem(max(0, i - 1)));
   // Update Solution state
   m_bIsDirty = TRUE;
   m_pMainFrame->PostMessage(WM_APP_BUILDSOLUTIONUI);
   return TRUE;
}

IProject* CSolution::GetActiveProject() const
{
   return m_pCurProject;
}

IProject* CSolution::GetFocusProject() const
{
   ATLASSERT(m_pMainFrame);
   HWND hWnd = m_pMainFrame->MDIGetActive();
   if( ::IsWindow(hWnd) ) {
      CWinProp prop = hWnd;
      IProject* pProject = NULL;
      prop.GetProperty(_T("Project"), pProject);
      if( pProject ) return pProject;
   }
   return m_pCurProject;
}

BOOL CSolution::SetActiveProject(IProject* pProject)
{
   // Let's see if we can find the project first...
   if( pProject ) {
      int i;
      for( i = 0; i < m_aProjects.GetSize(); i++ ) if( m_aProjects[i].pProject == pProject ) break;
      if( i == m_aProjects.GetSize() ) return FALSE;
      if( !m_bIsLoaded ) return FALSE;
   }
   // Already activated?
   if( pProject == m_pCurProject ) return TRUE;
   IProject* pOldProject = m_pCurProject;
   m_pCurProject = pProject;
   // Deactivate the old project
   if( pOldProject ) {
      pOldProject->DeactivateProject();
      // NOTE: We set the dirty flag on deactivate because we'll always activate
      //       a project when the solution loads. We'll only mark it dirty then when
      //       the user actually changes the state.
      m_bIsDirty = TRUE;
   }
   // Activate the project
   if( pProject ) pProject->ActivateProject();
   // Broadcast the new project to the UI
   m_pMainFrame->SendMessage(WM_APP_PROJECTCHANGE, 0, (LPARAM) pProject);
   return TRUE;
}

// Implementation

bool CSolution::_LoadSolution(LPCTSTR pstrFilename, IProject*& pDefaultProject)
{
   ATLASSERT(!::IsBadStringPtr(pstrFilename,-1));
   // Show progress
   CString sStatus;
   sStatus.Format(IDS_STATUS_LOADING, pstrFilename);
   g_pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus);
   // Load projects
   CXmlSerializer arc;
   if( !arc.Open(_T("Solution"), pstrFilename) ) return false;
   if( !arc.Read(_T("name"), m_sName) ) return false;
   if( !arc.ReadGroupBegin(_T("Projects")) ) return false;
   while( arc.ReadGroupBegin(_T("Project")) ) {
      BOOL bDefault = FALSE;
      arc.Read(_T("default"), bDefault);
      if( !_LoadProject(pstrFilename, &arc) ) return false;
      if( !arc.ReadGroupEnd() ) return false;
      // Incidently, is this the default project?
      if( bDefault ) pDefaultProject = m_aProjects[m_aProjects.GetSize() - 1].pProject;
   }
   if( !arc.ReadGroupEnd() ) return false;
   arc.Close();
   return true;
}

bool CSolution::_SaveSolution()
{
   CXmlSerializer arc;
   if( !arc.Create(_T("Solution"), m_sFilename) ) return false;
   CString sName;
   GetName(sName.GetBufferSetLength(MAX_PATH), MAX_PATH);
   sName.ReleaseBuffer();
   if( !arc.Write(_T("name"), sName) ) return false;
   if( !arc.WriteGroupBegin(_T("Projects")) ) return false;
   for( int i = 0; i < m_aProjects.GetSize(); i++ ) {
      PROJECT& Project = m_aProjects[i];
      if( !arc.WriteGroupBegin(_T("Project")) ) return false;
      if( !_SaveProject(&arc, Project) ) return false;
      if( !arc.WriteGroupEnd() ) return false;
   }
   if( !arc.WriteGroupEnd() ) return false;
   if( !arc.Save() ) return false;
   arc.Close();
   return true;
}

bool CSolution::_LoadProject(LPCTSTR pstrSolutionFilename, ISerializable* pArc)
{
   ATLASSERT(pArc);

   CString sType;
   pArc->Read(_T("type"), sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();
   CString sFilename;
   pArc->Read(_T("filename"), sFilename.GetBufferSetLength(MAX_PATH), MAX_PATH);
   sFilename.ReleaseBuffer();

   if( !_AddProject(pstrSolutionFilename, sFilename, sType) ) return false;

   return true;
};

bool CSolution::_AddProject(LPCTSTR pstrSolutionFilename, LPCTSTR pstrFilename, LPCTSTR pstrType)
{
   HWND hWndMain = g_pDevEnv->GetHwnd(IDE_HWND_MAIN);

   // Get the actual project paths/filenames
   TCHAR szCompressedFile[MAX_PATH];
   _tcscpy(szCompressedFile, pstrSolutionFilename);
   ::PathRemoveFileSpec(szCompressedFile);
   ::PathAppend(szCompressedFile, pstrFilename);
   TCHAR szProjectFile[MAX_PATH];
   LPTSTR pFilePart = NULL;
   ::GetFullPathName(szCompressedFile, MAX_PATH, szProjectFile, &pFilePart);

   // Add a new project entry to our project list in the solution
   PROJECT Dummy;
   m_aProjects.Add(Dummy);
   PROJECT& Project = m_aProjects[ m_aProjects.GetSize() - 1 ];
   Project.sType = pstrType;
   Project.sFilename = szProjectFile;

   // See if there's a plugin that supports this project type
   for( int i = 0; i < g_aPlugins.GetSize(); i++ ) {
      CPlugin& Plugin = g_aPlugins[i];
      if( !Plugin.IsLoaded() ) continue;
      if( Plugin.GetName() != pstrType ) continue;
      // Found plugin that supports this project type
      Project.pPlugin = &Plugin;
      // Create a new project
      Project.pProject = Plugin.CreateProject();
      ATLASSERT(Project.pProject);
      if( Project.pProject == NULL ) {
         m_pMainFrame->_ShowMessageBox(hWndMain, IDS_ERR_CREATEPROJECT, IDS_CAPTION_ERROR, MB_ICONERROR);
         m_aProjects.RemoveAt(m_aProjects.GetSize() - 1);
         return true;
      }

      TCHAR szProjectPath[MAX_PATH];
      _tcscpy(szProjectPath, szProjectFile);
      ::PathRemoveFileSpec(szProjectPath);
      ::PathAddBackslash(szProjectPath);

      // Initialize with default settings
      if( !Project.pProject->Initialize(g_pDevEnv, szProjectPath) ) {
         m_pMainFrame->_ShowMessageBox(hWndMain, IDS_ERR_CREATEPROJECT, IDS_CAPTION_ERROR, MB_ICONERROR);
         m_aProjects.RemoveAt(m_aProjects.GetSize() - 1);
         return true;
      }

      // Time to actually load the project settings
      CXmlSerializer arc;
      if( !arc.Open(_T("Project"), szProjectFile) ) {
         m_pMainFrame->_ShowMessageBox(hWndMain, IDS_ERR_LOADPROJECT, IDS_CAPTION_ERROR, MB_ICONERROR);
         Project.pProject->Close();
         Plugin.DestroyProject(Project.pProject);
         m_aProjects.RemoveAt(m_aProjects.GetSize() - 1);
         return true;
      }
      if( !Project.pProject->Load(&arc) ) {
         m_pMainFrame->_ShowMessageBox(hWndMain, IDS_ERR_LOADPROJECT, IDS_CAPTION_ERROR, MB_ICONERROR);
         Project.pProject->Close();
         Plugin.DestroyProject(Project.pProject);
         m_aProjects.RemoveAt(m_aProjects.GetSize() - 1);
         return true;
      }
      arc.Close();
      return true;
   }
   // Failed to find plugin of that type!
   CString sText;
   sText.Format(IDS_ERR_PROJECTTYPE, pFilePart, Project.sType);
   m_pMainFrame->ShowMessageBox(hWndMain, sText, CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONERROR);
   m_aProjects.RemoveAt(m_aProjects.GetSize() - 1);
   return false;
}

bool CSolution::_SaveProject(ISerializable* pArc, const PROJECT& Project)
{
   HWND hWndMain = g_pDevEnv->GetHwnd(IDE_HWND_MAIN);

   // Project filenames are stored relative to the solution
   // in the solution-file
   TCHAR szRelativeName[MAX_PATH];
   ::PathRelativePathTo(szRelativeName, m_sFilename, FILE_ATTRIBUTE_NORMAL, Project.sFilename, FILE_ATTRIBUTE_NORMAL);
   if( _tcslen(szRelativeName) == 0 ) _tcscpy(szRelativeName, Project.sFilename);

   IProject* pProject = Project.pProject;
   ATLASSERT(pArc);
   pArc->Write(_T("type"), Project.pPlugin->GetName());
   pArc->Write(_T("filename"), szRelativeName);
   pArc->Write(_T("default"), m_pCurProject == pProject);

   // Has project settings changed? If not we needn't write the project file at all.
   if( !pProject->IsDirty() ) return true;

   TCHAR szFilename[MAX_PATH];
   _tcscpy(szFilename, m_sFilename);
   ::PathRemoveFileSpec(szFilename);
   ::PathAppend(szFilename, Project.sFilename);

   // Time to actually commit the project settings
   CXmlSerializer arc;
   if( !arc.Create(_T("Project"), szFilename) ) {
      CString sText(MAKEINTRESOURCE(IDS_ERR_SAVEPROJECT));
      sText += _T("\n");
      sText += _GetComErrorText();
      m_pMainFrame->ShowMessageBox(hWndMain, sText, CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONERROR);
      return false;
   }
   if( !Project.pProject->Save(&arc) ) {
      m_pMainFrame->_ShowMessageBox(hWndMain, IDS_ERR_SAVEPROJECT, IDS_CAPTION_ERROR, MB_ICONERROR);
      return false;
   }
   if( !arc.Save() ) {
      CString sText(MAKEINTRESOURCE(IDS_ERR_SAVEPROJECT));
      sText += _T("\n");
      sText += _GetComErrorText();
      m_pMainFrame->ShowMessageBox(hWndMain, sText, CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONERROR);
      return false;
   }
   arc.Close();

   return true;
}

CString CSolution::_GetComErrorText() const
{
   CComPtr<IErrorInfo> spErr;
   ::GetErrorInfo(NULL, &spErr);
   if( spErr == NULL ) return _T("");
   CComBSTR bstrText;
   spErr->GetDescription(&bstrText);
   return bstrText.m_str;
}
