
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

   void Run(CCompileManager* pManager, LPCTSTR pstrCommand, ILineCallback* pCallback)
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
      CString sCommand;
      sCommand.Format(_T("cz %s"), pstrCommand);
      pManager->DoAction(sCommand);
      DWORD dwStartTick = ::GetTickCount();
      ::Sleep(200L); // HACK: Wait for thread to start!
      // Idle wait for command completion
      while( pManager->GetParam(_T("InCommand")) == _T("true") ) ::Sleep(200L);
      pManager->m_ShellManager.RemoveLineListener(this);
   }

   void OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText)
   {
      if( *pstrText == '[' ) return;                        // Is prompt?
      if( _tcsstr(pstrText, TERM_MARKER) != NULL ) return;  // Is termination marker?
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

VARIANT_BOOL CProjectOM::get_IsConnected()
{
   return m_pOwner->m_CompileManager.IsConnected() ? VARIANT_TRUE : VARIANT_FALSE;
}

IDispatch* CProjectOM::get_Files()
{
   return &m_Files;
}

VOID CProjectOM::Clean()
{
   BOOL bDummy;
   m_pOwner->OnBuildClean(0, 0, NULL, bDummy);
}

VOID CProjectOM::Build()
{
   BOOL bDummy;
   m_pOwner->OnBuildProject(0, 0, NULL, bDummy);
}

VOID CProjectOM::Rebuild()
{
   BOOL bDummy;
   m_pOwner->OnBuildRebuild(0, 0, NULL, bDummy);
}

VOID CProjectOM::StartApp()
{
   BOOL bDummy;
   m_pOwner->OnDebugStart(0, 0, NULL, bDummy);
}

VOID CProjectOM::DebugApp()
{
   BOOL bDummy;
   m_pOwner->OnDebugDebug(0, 0, NULL, bDummy);
}

VOID CProjectOM::ExecCommand(BSTR Command, IUnknown* pUnk)
{
   CComQIPtr<ILineCallback> spCallback = pUnk;
   if( spCallback == NULL ) return;
   USES_CONVERSION;
   CCommandExecute cmd;
   cmd.Run(&m_pOwner->m_CompileManager, OLE2CT(Command), spCallback);
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
   TCHAR szFilename[MAX_PATH] = { 0 };
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
   return m_pOwner->OpenView(0) ? VARIANT_TRUE : VARIANT_FALSE;
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
   TCHAR szFilename[MAX_PATH] = { 0 };
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
   return m_pOwner->OpenView(0) ? VARIANT_TRUE : VARIANT_FALSE;
}

VOID CTextFileOM::Close()
{
   m_pOwner->CloseView();
}

VARIANT_BOOL CTextFileOM::Save()
{
   return m_pOwner->Save() ? VARIANT_TRUE : VARIANT_FALSE;
}

INT CTextFileOM::get_TextLength()
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0);
   return (INT) m_pOwner->m_view.m_ctrlEdit.GetTextLength();
}

INT CTextFileOM::get_Lines()
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0);
   return (INT) m_pOwner->m_view.m_ctrlEdit.GetLineCount();
}

INT CTextFileOM::get_CurPos()
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0);
   return (INT) m_pOwner->m_view.m_ctrlEdit.GetCurrentPos();
}

INT CTextFileOM::get_CurLine()
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0);
   return (INT) m_pOwner->m_view.m_ctrlEdit.GetCurrentLine() + 1;
}

VOID CTextFileOM::SetSelection(INT iStart, INT iEnd)
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0);
   m_pOwner->m_view.m_ctrlEdit.SetSel(iStart, iEnd);
}

BSTR CTextFileOM::GetSelection()
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0);
   CharacterRange cr = m_pOwner->m_view.m_ctrlEdit.GetSelection();
   LPSTR pstrBuffer = (LPSTR) malloc((cr.cpMax - cr.cpMin) + 1);
   if( pstrBuffer == NULL ) return NULL;
   m_pOwner->m_view.m_ctrlEdit.GetSelText(pstrBuffer);
   CComBSTR bstr = pstrBuffer;
   free(pstrBuffer);
   return bstr.Detach();
}

VOID CTextFileOM::ReplaceSelection(BSTR Text)
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0);
   USES_CONVERSION;
   if( Text == NULL ) Text = L"";
   m_pOwner->m_view.m_ctrlEdit.ReplaceSel(OLE2CA(Text));
}

INT CTextFileOM::PosFromLine(INT iLine)
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0);
   return (INT) m_pOwner->m_view.m_ctrlEdit.PositionFromLine(iLine - 1);
}

INT CTextFileOM::LineLength(INT iLine)
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0);
   return (INT) m_pOwner->m_view.m_ctrlEdit.LineLength(iLine - 1);
}

VARIANT_BOOL CTextFileOM::FindText(BSTR Text)
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0);
   USES_CONVERSION;
   if( Text == NULL ) Text = L"";
   int iPos = m_pOwner->m_view._FindNext(FR_DOWN | FR_WRAP, OLE2CA(Text), false);
   if( iPos >= 0 ) m_pOwner->m_view.m_ctrlEdit.EnsureVisible(iPos);
   return iPos >= 0 ? VARIANT_TRUE : VARIANT_FALSE;
}

VOID __stdcall CTextFileOM::SendRawMessage(long uMsg, long wParam, BSTR lParam)
{
   if( !m_pOwner->IsOpen() ) m_pOwner->OpenView(0);
   USES_CONVERSION;
   m_pOwner->m_view.m_ctrlEdit.SetFocus(TRUE);
   m_pOwner->m_view.m_ctrlEdit.SendMessage((UINT) uMsg, (WPARAM) wParam, (LPARAM) OLE2CT(lParam));
}
