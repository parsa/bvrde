
#include "StdAfx.h"
#include "resource.h"

#include "Files.h"
#include "Project.h"

#pragma code_seg( "MISC" )


///////////////////////////////////////////////////////7
// CViewImpl

CViewImpl::CViewImpl(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) : 
   m_pCppProject(pCppProject),
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
   MergeMenu(menuEdit, menu.GetSubMenu(2), menuEdit.GetMenuItemCount());
}

void CViewImpl::DeactivateUI()
{
}

void CViewImpl::EnableModeless(BOOL /*bEnable*/)
{
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
   // Resolve filename if it has a relative path
   CString sPath;
   if( m_pCppProject != NULL && m_sFilename.Left(1) == _T(".") ) {
      m_pCppProject->GetPath(sPath.GetBufferSetLength(MAX_PATH), MAX_PATH);
      sPath.ReleaseBuffer();
   }
   return sPath + m_sFilename;
}


///////////////////////////////////////////////////////7
//

CFolderFile::CFolderFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) : 
   CViewImpl(pCppProject, pProject, pParent),
   m_Dispatch(pCppProject, this)
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

IDispatch* CFolderFile::GetDispatch()
{
   CFolderFile* pThis = const_cast<CFolderFile*>(this);
   return &pThis->m_Dispatch;
}


///////////////////////////////////////////////////////7
//

CTextFile::CTextFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CViewImpl(pCppProject, pProject, pParent),
   m_Dispatch(this)
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

   if( m_pCppProject != NULL && m_sLocation == _T("remote") ) 
   {
      // File is remote so we must use a file transfer protocol
      if( !m_pCppProject->m_FileManager.SaveFile(m_sFilename, true, (LPBYTE) pstrText, strlen(pstrText)) ) {
         free(pstrText);
         return FALSE;
      }
   }
   else 
   {
      // File is a local file; we can save it using standard Win32 calls...
      CFile f;
      if( !f.Create(_GetRealFilename()) ) {
         DWORD dwErr = ::GetLastError();
         free(pstrText);
         ::SetLastError(dwErr);
         return FALSE;
      }
      f.Write(pstrText, strlen(pstrText));
      f.Close();
   }

   m_view.m_ctrlEdit.SetSavePoint();

   // Save Clears Undo stack
   TCHAR szBuffer[32] = { 0 };
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
      // View is a remote file; Load it through protocol manager
      if( m_pCppProject != NULL && m_sLocation == _T("remote") ) 
      {
         LPBYTE pData = NULL;
         DWORD dwSize = 0;
         if( !m_pCppProject->m_FileManager.LoadFile(m_sFilename, true, &pData, &dwSize) ) {
            if( pData ) free(pData);
            return FALSE;
         }
         CString s;
         AtlA2WHelper(s.GetBufferSetLength(dwSize), (LPCSTR) pData, dwSize);
         s.ReleaseBuffer();
         CComBSTR bstr = s;
         *pbstrText = bstr.Detach();
         free(pData);
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
      // Load the file (local file or from remote server)
      CComBSTR bstrText;
      if( !GetText(&bstrText) ) return FALSE;
      CString sText = bstrText;

      if( m_sLocation != _T("remote") )
      {
         DWORD dwAttribs = ::GetFileAttributes(_GetRealFilename());
         if( dwAttribs & FILE_ATTRIBUTE_READONLY ) {
            TCHAR szBuffer[32] = { 0 };
            _pDevEnv->GetProperty(_T("gui.document.protectReadOnly"), szBuffer, 31);
            if( _tcscmp(szBuffer, _T("true")) == 0 ) m_view.m_ctrlEdit.SetReadOnly(TRUE);
         }
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
      m_view.Init(m_pCppProject, m_pProject, this, m_sFilename, m_sLanguage);
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

IDispatch* CTextFile::GetDispatch()
{
   CTextFile* pThis = const_cast<CTextFile*>(this);
   return &pThis->m_Dispatch;
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

CCppFile::CCppFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
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

CHeaderFile::CHeaderFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CCppFile(pCppProject, pProject, pParent)
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

CMakeFile::CMakeFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
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

CJavaFile::CJavaFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
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

CBasicFile::CBasicFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
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

CHtmlFile::CHtmlFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
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

CXmlFile::CXmlFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
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

CPerlFile::CPerlFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("perl");
}

BOOL CPerlFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Perl"), cchMax);
   return TRUE;
}


///////////////////////////////////////////////////////7
//

CPythonFile::CPythonFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
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

CPascalFile::CPascalFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("pascal");
}

BOOL CPascalFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   _tcsncpy(pstrType, _T("Pascal"), cchMax);
   return TRUE;
}

