
#include "StdAfx.h"
#include "resource.h"

#include "Files.h"
#include "Project.h"

#pragma code_seg( "MISC" )


///////////////////////////////////////////////////////7
// CViewImpl

CViewImpl::CViewImpl(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) : 
   m_pLocalProject(pLocalProject),
   m_pProject(pProject),
   m_pParent(pParent),
   m_bIsDirty(false)
{
}

BOOL CViewImpl::GetName(LPTSTR pstrName, UINT cchMax) const
{
   ATLASSERT(pstrName);
   _tcsncpy(pstrName, m_sName, cchMax);
   return TRUE;
}

BOOL CViewImpl::GetText(BSTR* pbstrText)
{
   *pbstrText = ::SysAllocString(L"");
   return TRUE;
}

BOOL CViewImpl::GetFileName(LPTSTR pstrName, UINT cchMax) const
{
   ATLASSERT(pstrName);
   _tcscpy(pstrName, _T(""));
   return TRUE;
}

IElement* CViewImpl::GetParent() const
{
   return m_pParent;
}

BOOL CViewImpl::SetName(LPCTSTR pstrName)
{
   ATLASSERT(pstrName);
   m_sName = pstrName;
   m_bIsDirty = true;
   return TRUE;
}

BOOL CViewImpl::Load(ISerializable* pArc)
{
   pArc->Read(_T("name"), m_sName.GetBufferSetLength(128), 128);
   m_sName.ReleaseBuffer();
   m_bIsDirty = false;
   return TRUE;
}

BOOL CViewImpl::Save(ISerializable* pArc)
{
   CString sType;
   GetType(sType.GetBufferSetLength(64), 64);
   sType.ReleaseBuffer();

   pArc->Write(_T("name"), m_sName);
   pArc->Write(_T("type"), sType);

   return TRUE;
}

BOOL CViewImpl::Save()
{
   return TRUE;
}

BOOL CViewImpl::Reload()
{
   return FALSE;
}

BOOL CViewImpl::IsDirty() const
{
   return m_bIsDirty;
}

void CViewImpl::ActivateUI()
{
   CMenu menu;
   menu.LoadMenu(IDR_MAIN);
   ATLASSERT(menu.IsMenu());

   CMenuHandle menuMain = _pDevEnv->GetMenuHandle(IDE_HWND_MAIN);
   CMenuHandle menuEdit = menuMain.GetSubMenu(1);
   MergeMenu(menuEdit, menu.GetSubMenu(0), menuEdit.GetMenuItemCount());
}

void CViewImpl::DeactivateUI()
{
}

void CViewImpl::EnableModeless(BOOL /*bEnable*/)
{
}

IDispatch* CViewImpl::GetDispatch()
{
   return _pDevEnv->CreateStdDispatch(_T("View"), this);
}

LRESULT CViewImpl::PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   return 0;
}

LRESULT CViewImpl::SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   return 0;
}

CString CViewImpl::_GetRealFilename() const
{
   // Ok, the only reason we need the CEmptyProject* reference is
   // that the filename may be relative to the project. So here we
   // expand the filename.
   CString sPath;
   if( m_sFilename.Left(1) == _T(".") ) {
      if( m_pLocalProject ) {
         m_pLocalProject->GetPath(sPath.GetBufferSetLength(MAX_PATH), MAX_PATH);
         sPath.ReleaseBuffer();
      }
   }
   return sPath + m_sFilename;
}


///////////////////////////////////////////////////////7
//

CFolderFile::CFolderFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) : 
   CViewImpl(pLocalProject, pProject, pParent)
{
}

BOOL CFolderFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Folder"), cchMax);
   return TRUE;
}

BOOL CFolderFile::Load(ISerializable* pArc)
{
   if( !CViewImpl::Load(pArc) ) return FALSE;
   return TRUE;
}

BOOL CFolderFile::Save(ISerializable* pArc)
{
   if( !CViewImpl::Save(pArc) ) return FALSE;
   return TRUE;
}

BOOL CFolderFile::OpenView(long /*lLineNum*/)
{
   return TRUE;
}

void CFolderFile::CloseView()
{
   delete this;
}


///////////////////////////////////////////////////////7
//

CTextFile::CTextFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CViewImpl(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("text");
}

BOOL CTextFile::Load(ISerializable* pArc)
{
   if( !CViewImpl::Load(pArc) ) return FALSE;
   
   pArc->Read(_T("filename"), m_sFilename.GetBufferSetLength(MAX_PATH), MAX_PATH);
   m_sFilename.ReleaseBuffer();
   pArc->Read(_T("location"), m_sLocation.GetBufferSetLength(64), 64);
   m_sLocation.ReleaseBuffer();
   
   if( m_sLocation.IsEmpty() ) m_sLocation = _T("local");
   return TRUE;
}

BOOL CTextFile::Save(ISerializable* pArc)
{
   if( !CViewImpl::Save(pArc) ) return FALSE;

   pArc->Write(_T("filename"), m_sFilename);
   pArc->Write(_T("location"), m_sLocation);

   if( pArc->WriteExternal(_T("text")) ) {
      if( IsDirty() ) {
         if( !Save() ) return FALSE;
      }
   }

   return TRUE;
}

BOOL CTextFile::Save()
{  
   ATLASSERT(m_view.IsWindow());
   if( !m_view.IsWindow() ) return TRUE;

   LPSTR pstrText = NULL;
   if( !m_view.GetText(pstrText) ) return FALSE;

   CString sFilename = _GetRealFilename();

   // Protect Read Only files
   TCHAR szBuffer[32] = { 0 };
   _pDevEnv->GetProperty(_T("gui.document.protectReadOnly"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("false")) == 0 ) CFile::Delete(sFilename);

   CFile f;
   if( !f.Create(sFilename) ) {
      DWORD dwErr = ::GetLastError();
      free(pstrText);
      ::SetLastError(dwErr);
      return FALSE;
   }
   f.Write(pstrText, strlen(pstrText));
   f.Close();

   m_view.m_ctrlEdit.SetSavePoint();

   // Save Clears Undo stack?
   _pDevEnv->GetProperty(_T("gui.document.clearUndo"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("true")) == 0 ) m_view.m_ctrlEdit.EmptyUndoBuffer();

   free(pstrText);
   return TRUE;
}

BOOL CTextFile::IsDirty() const
{
   if( ::IsWindow(m_wndFrame) ) return m_view.m_ctrlEdit.GetModify();
   return FALSE;
}

BOOL CTextFile::GetText(BSTR* pbstrText)
{
   ATLASSERT(*pbstrText==NULL);
   // View is open, so grab the text from the editor...
   if( m_wndFrame.IsWindow() ) 
   {
      LPSTR pstrSource = NULL;
      m_view.GetText(pstrSource);
      CComBSTR bstr = pstrSource;
      *pbstrText = bstr.Detach();
      free(pstrSource);
   }
   else 
   {
      // View is a local file; just load it...
      CFile f;
      if( !f.Open(_GetRealFilename()) ) return FALSE;
      DWORD dwSize = f.GetSize();
      LPSTR pstrData = (LPSTR) malloc(dwSize + 1);
      if( pstrData == NULL ) {
         ::SetLastError(ERROR_OUTOFMEMORY);
         return FALSE;
      }
      if( !f.Read(pstrData, dwSize) ) {
         DWORD dwErr = ::GetLastError();
         free(pstrData);
         ::SetLastError(dwErr);
         return FALSE;
      }
      pstrData[dwSize] = '\0';
      f.Close();
      CComBSTR bstr = pstrData;
      *pbstrText = bstr.Detach();
      free(pstrData);
   }
   return TRUE;
}

void CTextFile::EnableModeless(BOOL bEnable)
{
   if( m_wndFrame.IsWindow() ) m_wndFrame.EnableWindow(bEnable);
}

BOOL CTextFile::IsOpen() const
{
   return ::IsWindow(m_wndFrame);
}

BOOL CTextFile::OpenView(long lLineNum)
{
   if( m_wndFrame.IsWindow() ) 
   {
      // View already open. Just set focus to the window.
      m_wndFrame.SetFocus();
      if( lLineNum == 0 ) lLineNum = m_view.m_ctrlEdit.GetCurrentLine() + 1;
   }
   else 
   {
      // Load the file
      // If we request line number 0, the file will always open. This allows
      // a non-existing file to appear.
      CComBSTR bstrText;
      if( !GetText(&bstrText) && lLineNum > 0 ) return FALSE;
      CString sText = bstrText;

      DWORD dwAttribs = ::GetFileAttributes(_GetRealFilename());
      if( dwAttribs & FILE_ATTRIBUTE_READONLY ) {
         TCHAR szBuffer[32] = { 0 };
         _pDevEnv->GetProperty(_T("gui.document.protectReadOnly"), szBuffer, 31);
         if( _tcscmp(szBuffer, _T("true")) == 0 ) m_view.m_ctrlEdit.SetReadOnly(TRUE);
      }

      CString sName;
      GetName(sName.GetBufferSetLength(128), 128);
      sName.ReleaseBuffer();

      IViewFrame* pFrame = _pDevEnv->CreateClient(sName, m_pProject, this);
      ATLASSERT(pFrame);
      if( pFrame == NULL ) {
         ::SetLastError(ERROR_OUTOFMEMORY);
         return FALSE;
      }
      m_wndFrame = pFrame->GetHwnd();
      m_view.Init(m_pProject, this, m_sFilename, m_sLanguage);
      m_view.Create(m_wndFrame, CWindow::rcDefault, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
      ATLASSERT(m_view.IsWindow());
      if( !m_view.IsWindow() ) return FALSE;
      pFrame->SetClient(m_view);

      int nLen = sText.GetLength();
      LPSTR pstrData = (LPSTR) malloc(nLen + 1);
      if( pstrData == NULL ) {
         ::SetLastError(ERROR_OUTOFMEMORY);
         return FALSE;
      }
      AtlW2AHelper(pstrData, sText, nLen);
      pstrData[nLen] = '\0';
      m_view.SetText(pstrData);
      free(pstrData);

      m_view.ShowWindow(SW_NORMAL);
   }

   // Delayed activation of view
   lLineNum = max(0, lLineNum - 1);
   m_view.m_ctrlEdit.GotoLine(lLineNum);
   m_view.m_ctrlEdit.EnsureVisible(lLineNum);
   m_wndFrame.PostMessage(WM_SETFOCUS);

   return TRUE;
}

void CTextFile::CloseView()
{
   if( m_wndFrame.IsWindow() ) _pDevEnv->DestroyClient(m_wndFrame);
   delete this;
}

BOOL CTextFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Text"), cchMax);
   return TRUE;
}

BOOL CTextFile::GetFileName(LPTSTR pstrName, UINT cchMax) const
{
   ATLASSERT(pstrName);
   _tcsncpy(pstrName, m_sFilename, cchMax);
   return TRUE;
}

LRESULT CTextFile::PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   if( m_view.IsWindow() ) return m_view.PostMessage(uMsg, wParam, lParam);
   return 0;
}

LRESULT CTextFile::SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   if( m_view.IsWindow() ) return m_view.SendMessage(uMsg, wParam, lParam);
   return 0;
}


///////////////////////////////////////////////////////7
//

CCppFile::CCppFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CTextFile(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("cpp");
}

BOOL CCppFile::Load(ISerializable* pArc)
{
   if( !CTextFile::Load(pArc) ) return FALSE;
   return TRUE;
}

BOOL CCppFile::Save(ISerializable* pArc)
{
   if( !CTextFile::Save(pArc) ) return FALSE;
   return TRUE;
}

BOOL CCppFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("CPP"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CHeaderFile::CHeaderFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CCppFile(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("cpp");
}

BOOL CHeaderFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Header"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CMakeFile::CMakeFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CTextFile(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("makefile");
}

BOOL CMakeFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Makefile"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CJavaFile::CJavaFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CTextFile(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("java");
}

BOOL CJavaFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Java"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CBasicFile::CBasicFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CTextFile(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("basic");
}

BOOL CBasicFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("BASIC"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CHtmlFile::CHtmlFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CTextFile(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("html");
}

BOOL CHtmlFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("HTML"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CPhpFile::CPhpFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CTextFile(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("php");
}

BOOL CPhpFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("PHP"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CAspFile::CAspFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CTextFile(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("asp");
}

BOOL CAspFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("ASP"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CXmlFile::CXmlFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CTextFile(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("xml");
}

BOOL CXmlFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("XML"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CPerlFile::CPerlFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CTextFile(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("xml");
}

BOOL CPerlFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Perl"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CPythonFile::CPythonFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CTextFile(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("python");
}

BOOL CPythonFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Python"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CPascalFile::CPascalFile(CEmptyProject* pLocalProject, IProject* pProject, IElement* pParent) :
   CTextFile(pLocalProject, pProject, pParent)
{
   m_sLanguage = _T("pascal");
}

BOOL CPascalFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Pascal"), cchMax);
   return TRUE;
}

