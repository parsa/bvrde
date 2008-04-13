
#include "StdAfx.h"
#include "resource.h"

#include "Files.h"
#include "Project.h"

#pragma code_seg( "MISC" )


////////////////////////////////////////////////////////
// CRememberErr

class CRememberErr
{
public:
   DWORD m_dwErr;
   CRememberErr() : m_dwErr(0) { };
   ~CRememberErr() { if( m_dwErr != 0 ) ::SetLastError(m_dwErr); };
   BOOL SetLastError() { m_dwErr = ::GetLastError(); return FALSE; };
   BOOL SetLastError(DWORD dwErr) { m_dwErr = dwErr; return FALSE; };
};


////////////////////////////////////////////////////////
// CViewImpl

CViewImpl::CViewImpl(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) : 
   m_pCppProject(pCppProject),
   m_pProject(pProject),
   m_pParent(pParent),
   m_bIsDirty(TRUE)
{
   ::ZeroMemory(&m_ftCurrent, sizeof(FILETIME));
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
   m_bIsDirty = TRUE;
   return TRUE;
}

BOOL CViewImpl::Load(ISerializable* pArc)
{
   pArc->Read(_T("name"), m_sName.GetBufferSetLength(128), 128);
   m_sName.ReleaseBuffer();
   m_bIsDirty = FALSE;
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
   ::ZeroMemory(&m_ftCurrent, sizeof(FILETIME));
   m_bIsDirty = FALSE;
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
   CMenuHandle menuEdit = menuMain.GetSubMenu(MENUPOS_EDIT_FB);
   MergeMenu(menuEdit, menu.GetSubMenu(2), menuEdit.GetMenuItemCount());

   // C++ Edit-menu has an "Insert/Remove Breakpoint" menuitem
   if( m_sLanguage == _T("cpp") ) MergeMenu(menuEdit, menu.GetSubMenu(6), menuEdit.GetMenuItemCount() - 4);
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
      TCHAR szPath[MAX_PATH] = { 0 };
      m_pCppProject->GetPath(szPath, MAX_PATH);
      TCHAR szExpanded[MAX_PATH] = { 0 };
      ::ExpandEnvironmentStrings(szPath, szExpanded, MAX_PATH);
      sPath = szExpanded;
   }
   return sPath + m_sFilename;
}

bool CViewImpl::_IsValidFile(LPBYTE pData, DWORD dwSize) const
{
   ATLASSERT(!::IsBadReadPtr(pData,dwSize));
   if( dwSize <= 2 ) return true;
   if( pData[0] == 0xFF && pData[1] == 0xFE ) return false;   // UNICODE BOM
   if( pData[0] == 0xFE && pData[1] == 0xFF ) return false;   // UNICODE BOM
   if( pData[0] == 0xEF && pData[1] == 0xBB ) return false;   // UTF-8 BOM
   return true;
}


////////////////////////////////////////////////////////
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

BOOL CFolderFile::Reload()
{
   return FALSE;
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


////////////////////////////////////////////////////////
//

CTextFile::CTextFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CViewImpl(pCppProject, pProject, pParent),
   m_Dispatch(this)
{
   m_sLanguage = _T("text");
   m_sFileType = _T("Text");
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

   return TRUE;
}

BOOL CTextFile::Save()
{  
   if( !m_view.IsWindow() ) return TRUE;

   CRememberErr err;

   LPSTR pstrText = NULL;
   if( !m_view.GetText(pstrText) ) return err.SetLastError();

   if( m_pCppProject != NULL && m_sLocation == _T("remote") ) 
   {
      // File is remote so we must use a file transfer protocol
      if( !m_pCppProject->m_FileManager.SaveFile(m_sFilename, true, (LPBYTE) pstrText, strlen(pstrText)) ) {
         err.SetLastError();
         free(pstrText);
         return FALSE;
      }
   }
   else 
   {
      // File is a local file; we can save it using standard Win32 calls...
      CFile f;
      if( !f.Create(_GetRealFilename()) ) {
         err.SetLastError();
         free(pstrText);
         return FALSE;
      }
      f.Write(pstrText, strlen(pstrText));
      f.Close();
   }

   free(pstrText);

   m_view.m_ctrlEdit.SetSavePoint();

   // Save Clears Undo stack
   TCHAR szBuffer[32] = { 0 };
   _pDevEnv->GetProperty(_T("gui.document.clearUndo"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("true")) == 0 ) m_view.m_ctrlEdit.EmptyUndoBuffer();

   return CViewImpl::Save();
}

BOOL CTextFile::Reload()
{
   if( IsOpen() ) _pDevEnv->DestroyClient(m_wndFrame);
   return OpenView(1);
}

void CTextFile::ActivateUI()
{
   if( m_sLocation == _T("local") ) 
   {
      // Check if file changed outside view?
      TCHAR szBuffer[32] = { 0 };;
      _pDevEnv->GetProperty(_T("gui.document.detectChange"), szBuffer, 31);
      if( _tcscmp(szBuffer, _T("true")) == 0 ) {
         // Detect if filetime has changed
         CFile f;
         if( f.Open(_GetRealFilename()) ) {
            FILETIME ft = { 0 };
            ::GetFileTime(f, NULL, NULL, &ft);
            if( m_ftCurrent.dwLowDateTime == 0 ) m_ftCurrent = ft;
            if( ::CompareFileTime(&m_ftCurrent, &ft) != 0 ) {
               _pDevEnv->GetProperty(_T("gui.document.autoLoad"), szBuffer, 31);           
               if( _tcscmp(szBuffer, _T("true")) == 0 || IDYES == _pDevEnv->ShowMessageBox(m_view, CString(MAKEINTRESOURCE(IDS_FILECHANGES)), CString(MAKEINTRESOURCE(IDS_CAPTION_QUESTION)), MB_YESNO | MB_ICONQUESTION) ) {
                  Reload();
               }
            }
            m_ftCurrent = ft;
            f.Close();
         }
      }
   }
   CViewImpl::ActivateUI();
}

BOOL CTextFile::IsDirty() const
{
   if( CViewImpl::IsDirty() ) return TRUE;
   if( ::IsWindow(m_wndFrame) ) return m_view.m_ctrlEdit.GetModify();
   return FALSE;
}

BOOL CTextFile::GetText(BSTR* pbstrText)
{
   ATLASSERT(*pbstrText==NULL);

   CRememberErr err;
   
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
      if( m_pCppProject != NULL && m_sLocation == _T("remote") ) 
      {
         // View is a remote file; Load it through protocol manager
         DWORD dwSize = 0;
         LPBYTE pData = NULL;
         if( !m_pCppProject->m_FileManager.LoadFile(m_sFilename, true, &pData, &dwSize) ) {
            err.SetLastError();
            if( pData != NULL ) free(pData);
            return FALSE;
         }

         if( !_IsValidFile(pData, dwSize) ) {
            err.SetLastError(ERROR_BAD_FORMAT);
            free(pData);
            _pDevEnv->ShowMessageBox(_pDevEnv->GetHwnd(IDE_HWND_MAIN), CString(MAKEINTRESOURCE(IDS_ERR_FILEENCODING)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
            return FALSE;
         }

         // FIX: We need the sz-byte for reliable MBCS/UNICODE conversion
         //      so we duplicate the buffer once again.
         LPSTR pstrData = (LPSTR) malloc(dwSize + 1);
         ATLASSERT(pstrData);
         if( pstrData == NULL ) return err.SetLastError(ERROR_OUTOFMEMORY);
         memcpy(pstrData, pData, dwSize);
         pstrData[dwSize] = '\0';

         CComBSTR bstr = pstrData;
         *pbstrText = bstr.Detach();
         free(pstrData);
         free(pData);
      }
      else 
      {
         // View is a local file; just load it...
         CFile f;
         if( !f.Open(_GetRealFilename()) ) return err.SetLastError();
         DWORD dwSize = f.GetSize();
         LPSTR pstrData = (LPSTR) malloc(dwSize + 1);
         ATLASSERT(pstrData);
         if( pstrData == NULL ) return err.SetLastError(ERROR_OUTOFMEMORY);
         if( !f.Read(pstrData, dwSize) ) {
            err.SetLastError();
            free(pstrData);
            return FALSE;
         }
         pstrData[dwSize] = '\0';
         f.Close();

         if( !_IsValidFile( (LPBYTE) pstrData, dwSize ) ) {
            err.SetLastError(ERROR_BAD_FORMAT);
            _pDevEnv->ShowMessageBox(_pDevEnv->GetHwnd(IDE_HWND_MAIN), CString(MAKEINTRESOURCE(IDS_ERR_FILEENCODING)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
            free(pstrData);
            return FALSE;
         }

         CComBSTR bstr = pstrData;
         *pbstrText = bstr.Detach();
         free(pstrData);
      }
   }

   ::SetLastError(0);
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
   CRememberErr err;

   if( m_wndFrame.IsWindow() ) 
   {
      // View already open. Just set focus to the window.
      m_wndFrame.SetFocus();
      if( lLineNum == 0 ) lLineNum = m_view.m_ctrlEdit.GetCurrentLine() + 1;
   }
   else 
   {
      CWaitCursor cursor;
      _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, CString(MAKEINTRESOURCE(IDS_STATUS_OPENFILE)));

      // Load the file (local file or from remote server)
      // As a little hack we allow the file to be opened if we request a line-number
      // as 0! This allows empty pages to be opened and later saved.
      CComBSTR bstrText;
      if( !GetText(&bstrText) && lLineNum > 0 ) return err.SetLastError();
      if( ::GetLastError() != 0 ) return err.SetLastError();
      CString sText = bstrText;

      CString sName;
      GetName(sName.GetBufferSetLength(128), 128);
      sName.ReleaseBuffer();

      IViewFrame* pFrame = _pDevEnv->CreateClient(sName, m_pProject, this);
      ATLASSERT(pFrame);
      if( pFrame == NULL ) return err.SetLastError(ERROR_OUTOFMEMORY);
      m_wndFrame = pFrame->GetHwnd();
      m_view.Init(m_pCppProject, m_pProject, this, m_sFilename, m_sLanguage);
      m_view.Create(m_wndFrame, CWindow::rcDefault, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
      ATLASSERT(m_view.IsWindow());
      if( !m_view.IsWindow() ) return err.SetLastError();
      pFrame->SetClient(m_view);

      // Convert to MBCS.
      // FIX: We need to allocate enough space for the UNICODE to MBCS translation
      //      meaning 2 bytes pr char.
      int nLen = sText.GetLength();
      LPSTR pstrData = (LPSTR) malloc((nLen * 2) + 1);
      ATLASSERT(pstrData);
      if( pstrData == NULL ) return err.SetLastError(ERROR_OUTOFMEMORY);
      AtlW2AHelper(pstrData, sText, (nLen * 2) + 1);
      m_view.SetText(pstrData);
      free(pstrData);

      // Uphold the read-only rule
      if( m_sLocation == _T("local") )
      {
         DWORD dwAttribs = ::GetFileAttributes(_GetRealFilename());
         if( dwAttribs != (DWORD) -1 && (dwAttribs & FILE_ATTRIBUTE_READONLY) != 0 ) {
            TCHAR szBuffer[32] = { 0 };
            _pDevEnv->GetProperty(_T("gui.document.protectReadOnly"), szBuffer, 31);
            if( _tcscmp(szBuffer, _T("true")) == 0 ) m_view.m_ctrlEdit.SetReadOnly(TRUE);
         }
      }

      m_view.ShowWindow(SW_NORMAL);
   }

   // Delayed activation of view
   lLineNum = max(0, lLineNum - 1);
   m_view.m_ctrlEdit.GotoLine(lLineNum);
   m_view.m_ctrlEdit.EnsureVisible(lLineNum);

   // Someone steals focus on the Project Explorer double-click.
   // We'll take focus right back again.
   m_view.PostMessage(WM_SETFOCUS);

   return TRUE;
}

void CTextFile::CloseView()
{
   if( m_wndFrame.IsWindow() ) _pDevEnv->DestroyClient(m_wndFrame);
   // BUG: Needs to somehow also clean up the 'm_aFiles' collection in CRemoteProject
   //      instance - otherwise this is a dangling invalid pointer!!
   delete this;
}

BOOL CTextFile::GetType(LPTSTR pstrType, UINT cchMax) const
{
   return _tcsncpy(pstrType, m_sFileType, cchMax) > 0;
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
   if( !m_view.IsWindow() ) return 0;
   return m_view.PostMessage(uMsg, wParam, lParam);
}

LRESULT CTextFile::SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   if( !m_view.IsWindow() )  return 0;
   DWORD dwRes = 0;
   return ::SendMessageTimeout(m_view, uMsg, wParam, lParam, SMTO_ABORTIFHUNG, 1000UL, &dwRes);
}


////////////////////////////////////////////////////////
//

CCppFile::CCppFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("cpp");
   m_sFileType = _T("CPP");
}


////////////////////////////////////////////////////////
//

CHeaderFile::CHeaderFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CCppFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("cpp");
   m_sFileType = _T("Header");
}


////////////////////////////////////////////////////////
//

CMakeFile::CMakeFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("makefile");
   m_sFileType = _T("Makefile");
}


////////////////////////////////////////////////////////
//

CBashFile::CBashFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("bash");
   m_sFileType = _T("ShellScript");
}


////////////////////////////////////////////////////////
//

CJavaFile::CJavaFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("java");
   m_sFileType = _T("Java");
}


////////////////////////////////////////////////////////
//

CBasicFile::CBasicFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("basic");
   m_sFileType = _T("BASIC");
}


////////////////////////////////////////////////////////
//

CHtmlFile::CHtmlFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("html");
   m_sFileType = _T("HTML");
}


////////////////////////////////////////////////////////
//

CPhpFile::CPhpFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("php");
   m_sFileType = _T("PHP");
}


////////////////////////////////////////////////////////
//

CAspFile::CAspFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("asp");
   m_sFileType = _T("ASP Script");
}


////////////////////////////////////////////////////////
//

CXmlFile::CXmlFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("xml");
   m_sFileType = _T("XML");
}


////////////////////////////////////////////////////////
//

CPerlFile::CPerlFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("perl");
   m_sFileType = _T("Perl");
}


////////////////////////////////////////////////////////
//

CPythonFile::CPythonFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("python");
   m_sFileType = _T("Python");
}


////////////////////////////////////////////////////////
//

CPascalFile::CPascalFile(CRemoteProject* pCppProject, IProject* pProject, IElement* pParent) :
   CTextFile(pCppProject, pProject, pParent)
{
   m_sLanguage = _T("pascal");
   m_sFileType = _T("Pascal");
}

