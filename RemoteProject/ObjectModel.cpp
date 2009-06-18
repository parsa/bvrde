
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"
#include "Files.h"


/////////////////////////////////////////////////////////////////////////////
// CCommandExecute

class CCommandExecute : public IOutputLineListener
{
public:
   CCompileManager* m_pManager;
   ILineCallback* m_pCallback;
   volatile DWORD m_dwLastTime;

   void Run(CCompileManager* pManager, LPCTSTR pstrCommand, ILineCallback* pCallback, LONG lTimeout)
   {
      ATLASSERT(pManager);
      ATLASSERT(pCallback);
      ATLASSERT(pstrCommand);
      if( pManager->IsBusy() ) {
         ::MessageBeep((UINT)-1);
         return;
      }
      m_pCallback = pCallback;
      pManager->m_ShellManager.AddLineListener(this);
      // Build command to execute in shell
      // The "cz" is an internal prefix for submitting "unparsed"
      // commands to the compiler shell.
      CString sCommand;
      sCommand.Format(_T("cz %s"), pstrCommand);
      pManager->DoAction(sCommand);
      // Wait for it to start and complete
      m_dwLastTime = ::GetTickCount();
      while( pManager->GetParam(_T("InCommand")) == _T("true") ) {
         ::Sleep(200UL);
         if( lTimeout > 0 && ::GetTickCount() - m_dwLastTime > (DWORD) lTimeout ) break;
      }
      pManager->m_ShellManager.RemoveLineListener(this);
   }

   // IOutputLineListener

   void OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText)
   {
      if( *pstrText == '[' ) return;                            // Ignore prompt?
      if( _tcsstr(pstrText, TERM_MARKER) != NULL ) return;      // Ignore termination marker?
      m_dwLastTime = ::GetTickCount();
      m_pCallback->OnIncomingLine(CComBSTR(pstrText));
   }
};


/////////////////////////////////////////////////////////////////////////////
// CProjectOM

CProjectOM::CProjectOM(CRemoteProject* pProject) :
   m_pOwner(pProject),
   m_Files(pProject)
{
}

BSTR CProjectOM::get_Name()
{
   TCHAR szName[128] = { 0 };
   m_pOwner->GetName(szName, 127);
   return ::SysAllocString(szName);
}

BSTR CProjectOM::get_Type()
{
   TCHAR szType[64] = { 0 };
   m_pOwner->GetType(szType, 63);
   return ::SysAllocString(szType);
}

BSTR CProjectOM::get_Class()
{
   TCHAR szClass[64] = { 0 };
   m_pOwner->GetClass(szClass, 63);
   return ::SysAllocString(szClass);
}

BSTR CProjectOM::get_CurDir()
{
   CString sPath = m_pOwner->m_CompileManager.GetParam(_T("Path"));
   return ::SysAllocString(sPath);
}

BSTR CProjectOM::get_Server()
{
   CString sPath = m_pOwner->m_CompileManager.GetParam(_T("Host"));
   return ::SysAllocString(sPath);
}

BSTR CProjectOM::get_Username()
{
   CString sPath = m_pOwner->m_CompileManager.GetParam(_T("Username"));
   return ::SysAllocString(sPath);
}

BSTR CProjectOM::get_Password()
{
   CString sPath = m_pOwner->m_CompileManager.GetParam(_T("Password"));
   return ::SysAllocString(sPath);
}

BSTR CProjectOM::GetParam(BSTR Type, BSTR Key)
{
   CString sType = Type;
   CString sValue;
   if( sType == _T("Compiler") ) sValue = m_pOwner->m_CompileManager.GetParam(CString(Key));
   if( sType == _T("Debugger") ) sValue = m_pOwner->m_DebugManager.GetParam(CString(Key));
   return sValue.AllocSysString();
}

VOID CProjectOM::SetParam(BSTR Type, BSTR Key, BSTR Value)
{
   CString sType = Type;
   if( sType == _T("Compiler") ) m_pOwner->m_CompileManager.SetParam(CString(Key), CString(Value));
   if( sType == _T("Debugger") ) m_pOwner->m_DebugManager.SetParam(CString(Key), CString(Value));
}

VARIANT_BOOL CProjectOM::get_IsConnected()
{
   return m_pOwner->m_CompileManager.IsConnected() ? VARIANT_TRUE : VARIANT_FALSE;
}

VARIANT_BOOL CProjectOM::get_IsBusy()
{
   return m_pOwner->m_CompileManager.IsBusy() || m_pOwner->m_DebugManager.IsDebugging() ? VARIANT_TRUE : VARIANT_FALSE;
}

IDispatch* CProjectOM::get_Files()
{
   return &m_Files;
}

VOID CProjectOM::Clean()
{
   _StartCompiler();
   m_pOwner->m_CompileManager.DoAction(_T("Clean"));
}

VOID CProjectOM::Build()
{
   _StartCompiler();
   m_pOwner->m_CompileManager.DoAction(_T("Build"));
}

VOID CProjectOM::Rebuild()
{
   // Start build (silently; see CBuildSolutionThread)
   _StartCompiler();
   m_pOwner->m_CompileManager.DoAction(_T("Rebuild"), NULL, COMPFLAG_SILENT);
}

VOID CProjectOM::StartApp()
{
   if( m_pOwner->m_DebugManager.IsDebugging() ) m_pOwner->m_DebugManager.RunContinue(); 
   else m_pOwner->m_DebugManager.RunNormal();
}

VOID CProjectOM::DebugApp()
{
   m_pOwner->m_DebugManager.RunDebug();
}

VOID CProjectOM::ExecCommand(BSTR Command, IUnknown* pUnk, LONG lTimeout)
{
   CComQIPtr<ILineCallback> spCallback = pUnk;
   if( spCallback == NULL ) return;
   USES_CONVERSION;
   CCommandExecute cmd;
   cmd.Run(&m_pOwner->m_CompileManager, OLE2CT(Command), spCallback, lTimeout);
}

void CProjectOM::_StartCompiler()
{
   // Silently attempt to connect to server
   if( !m_pOwner->m_CompileManager.IsConnected() ) {
      m_pOwner->m_CompileManager.Start();
      long lTimeout = _ttol(m_pOwner->m_CompileManager.GetParam(_T("ConnectTimeout")));
      if( lTimeout <= 0 ) lTimeout = 3L;
      while( lTimeout-- > 0 ) {
         if( m_pOwner->m_CompileManager.IsConnected() ) break;
         ::Sleep(1000L);
      }
   }
}


/////////////////////////////////////////////////////////////////////////////
// CFilesOM

CFilesOM::CFilesOM(CRemoteProject* pProject) :
   m_pOwner(pProject)
{
}

LONG CFilesOM::get_Count()
{
   return m_pOwner->GetItemCount();
}

IDispatch* CFilesOM::Item(LONG iIndex)
{
   IView* pView = m_pOwner->GetItem(iIndex - 1);
   if( pView == NULL ) return NULL;
   return pView->GetDispatch();
}


/////////////////////////////////////////////////////////////////////////////
// CFolderOM

CFolderOM::CFolderOM(CRemoteProject* pProject, IElement* pElement) :
   m_pProject(pProject),
   m_pOwner(pElement)
{
}

BSTR CFolderOM::get_Name()
{
   TCHAR szName[128] = { 0 };
   m_pOwner->GetName(szName, 127);
   return ::SysAllocString(szName);
}

BSTR CFolderOM::get_Type()
{
   TCHAR szType[64] = { 0 };
   m_pOwner->GetType(szType, 63);
   return ::SysAllocString(szType);
}

LONG CFolderOM::get_Count()
{
   if( m_pProject == NULL ) return 0;
   int nCount = 0;
   for( int i = 0; i < m_pProject->GetItemCount(); i++ ) if( m_pProject->GetItem(i)->GetParent() == m_pOwner ) nCount++;
   return nCount;
}

IDispatch* CFolderOM::Item(LONG iIndex)
{
   if( m_pProject == NULL ) return NULL;
   for( int i = 0, nCount = 0; i < m_pProject->GetItemCount(); i++ ) {
      if( m_pProject->GetItem(i)->GetParent() == m_pOwner ) {
         if( ++nCount == iIndex ) return m_pProject->GetItem(i)->GetDispatch();
      }
   }
   return NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CFileOM

CFileOM::CFileOM(IView* pView) :
   m_pOwner(pView)
{
}

BSTR CFileOM::get_Name()
{
   TCHAR szName[128] = { 0 };
   m_pOwner->GetName(szName, 127);
   return ::SysAllocString(szName);
}

BSTR CFileOM::get_Type()
{
   TCHAR szType[64] = { 0 };
   m_pOwner->GetType(szType, 63);
   return ::SysAllocString(szType);
}

BSTR CFileOM::get_Filename()
{
   TCHAR szFilename[MAX_PATH + 1] = { 0 };
   m_pOwner->GetFileName(szFilename, MAX_PATH);
   return ::SysAllocString(szFilename);
}

BSTR CFileOM::get_Text()
{
   CComBSTR bstrText;
   m_pOwner->GetText(&bstrText);
   return bstrText.Detach();
}

VARIANT_BOOL CFileOM::Open()
{
   return m_pOwner->OpenView(0L) ? VARIANT_TRUE : VARIANT_FALSE;
}

VOID CFileOM::Close()
{
   m_pOwner->CloseView();
}

VARIANT_BOOL CFileOM::Save()
{
   return m_pOwner->Save() ? VARIANT_TRUE : VARIANT_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CTextFileOM

CTextFileOM::CTextFileOM(CTextFile* pView) :
   m_pOwner(pView)
{
}

BSTR CTextFileOM::get_Name()
{
   TCHAR szName[128] = { 0 };
   m_pOwner->GetName(szName, 127);
   return ::SysAllocString(szName);
}

BSTR CTextFileOM::get_Type()
{
   TCHAR szType[64] = { 0 };
   m_pOwner->GetType(szType, 63);
   return ::SysAllocString(szType);
}

BSTR CTextFileOM::get_Filename()
{
   TCHAR szFilename[MAX_PATH + 1] = { 0 };
   m_pOwner->GetFileName(szFilename, MAX_PATH);
   return ::SysAllocString(szFilename);
}

BSTR CTextFileOM::get_Text()
{
   CComBSTR bstrText;
   m_pOwner->GetText(&bstrText);
   return bstrText.Detach();
}

VARIANT_BOOL CTextFileOM::Open()
{
   return m_pOwner->OpenView(-2L) ? VARIANT_TRUE : VARIANT_FALSE;
}

VOID CTextFileOM::Close()
{
   m_pOwner->CloseView();
}

VARIANT_BOOL CTextFileOM::Save()
{
   if( !m_pOwner->IsOpen() ) return VARIANT_FALSE;
   return m_pOwner->Save() ? VARIANT_TRUE : VARIANT_FALSE;
}

INT CTextFileOM::get_TextLength()
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0L);
   return (INT) m_pOwner->m_view.m_ctrlEdit.GetTextLength();
}

INT CTextFileOM::get_Lines()
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0L);
   return (INT) m_pOwner->m_view.m_ctrlEdit.GetLineCount();
}

INT CTextFileOM::get_CurPos()
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0L);
   return (INT) m_pOwner->m_view.m_ctrlEdit.GetCurrentPos();
}

INT CTextFileOM::get_CurLine()
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0L);
   return (INT) m_pOwner->m_view.m_ctrlEdit.GetCurrentLine() + 1;
}

VOID CTextFileOM::SetSelection(INT iStart, INT iEnd)
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0L);
   m_pOwner->m_view.m_ctrlEdit.SetSel(iStart, iEnd);
}

BSTR CTextFileOM::GetSelection()
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0L);
   CharacterRange cr = m_pOwner->m_view.m_ctrlEdit.GetSelection();
   int cch = cr.cpMax - cr.cpMin;
   if( cch == 0 ) return ::SysAllocString(L"");
   LPSTR pstrBuffer = (LPSTR) malloc(cch + 1);
   if( pstrBuffer == NULL ) return NULL;
   m_pOwner->m_view.m_ctrlEdit.GetSelText(pstrBuffer);
   CComBSTR bstr = pstrBuffer;
   free(pstrBuffer);
   return bstr.Detach();
}

VOID CTextFileOM::ReplaceSelection(BSTR Text)
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0L);
   USES_CONVERSION;
   if( Text == NULL ) Text = L"";
   m_pOwner->m_view.m_ctrlEdit.ReplaceSel(OLE2CA(Text));
   m_pOwner->m_bIsDirty = TRUE;
}

INT CTextFileOM::PosFromLine(INT iLine)
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0L);
   return (INT) m_pOwner->m_view.m_ctrlEdit.PositionFromLine(iLine - 1);
}

INT CTextFileOM::LineLength(INT iLine)
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0L);
   return (INT) m_pOwner->m_view.m_ctrlEdit.LineLength(iLine - 1);
}

VARIANT_BOOL CTextFileOM::FindText(BSTR Text)
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0L);
   USES_CONVERSION;
   if( Text == NULL ) Text = L"";
   int iPos = m_pOwner->m_view.FindText(FR_DOWN | FR_WRAP, CString(Text), false);
   if( iPos >= 0 ) m_pOwner->m_view.m_ctrlEdit.EnsureVisible(iPos);
   return iPos >= 0 ? VARIANT_TRUE : VARIANT_FALSE;
}

VOID CTextFileOM::SendRawMessage(INT uMsg, INT wParam, BSTR lParam)
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0L);
   USES_CONVERSION;
   m_pOwner->m_view.m_ctrlEdit.SetFocus(TRUE);
   m_pOwner->m_view.m_ctrlEdit.SendMessage((UINT) uMsg, (WPARAM) wParam, (LPARAM) OLE2CT(lParam));
}
