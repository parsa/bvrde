
#include "StdAfx.h"
#include "resource.h"

#include "Globals.h"
#include "Commands.h"

#include "UpdateDlg.h"
#include "AddFileDlg.h"
#include "CheckInDlg.h"
#include "LoginCvsDlg.h"


////////////////////////////////////////////////////////////////7
//
//

void CCommandThread::SetCommand(UINT nCmd, LPCTSTR pstrCommand)
{
   m_nCmd = nCmd;
   m_sCommand = pstrCommand;
}

CString CCommandThread::GetResult() const
{
   return m_sResult;
}

DWORD CCommandThread::Run()
{
   // Build command prompt and execute command through
   // project's scripting mode.
   m_sResult.Empty();
   ISolution* pSolution = _pDevEnv->GetSolution();
   if( pSolution == NULL ) return 0;
   IProject* pProject = pSolution->GetActiveProject();
   if( pProject == NULL ) return 0;
   IDispatch* pDisp = pProject->GetDispatch();
   CComDispatchDriver dd = pDisp;
   CComBSTR bstrCommand = L"cd $PATH$\n";
   bstrCommand += CComBSTR(m_sCommand);
   CComVariant vCommand = bstrCommand;
   CComVariant vCallback = (IUnknown*) this;
   dd.Invoke2(OLESTR("ExecCommand"), &vCommand, &vCallback);

   // Let's look at the result
   CSimpleArray<CString> aLines;
   _SplitResult(m_sResult, aLines);
   CString sMessage = _T("\r\n");
   for( int i = 0; i < aLines.GetSize(); i++ ) {
      sMessage += aLines[i];
      sMessage += _T("\r\n");
   }
   // Output it to the Command View
   CRichEditCtrl ctrlEdit = _pDevEnv->GetHwnd(IDE_HWND_COMMANDVIEW);
   ctrlEdit.SetReadOnly(TRUE);
   AppendRtfText(ctrlEdit, sMessage, CFM_BOLD, 0);
   AppendRtfText(ctrlEdit, _T("\r\n> "), CFM_BOLD, CFE_BOLD);
   ctrlEdit.SetReadOnly(FALSE);
   _pDevEnv->ActivateAutoHideView(ctrlEdit);
   return 0;
}

// Implementation

void CCommandThread::_SplitResult(CString sResult, CSimpleArray<CString>& aLines) const
{
   while( !sResult.IsEmpty() ) {
      CString sToken = sResult.SpanExcluding(_T("\n"));
      if( sToken.GetLength() == 0 ) sToken = sResult;
      sResult = sResult.Mid(sToken.GetLength() + 1);
      // TODO: A little format helper; some of these source-control systems spit
      //       out stdout-prefixes (like "cvs server:") to mark its output.
      //       We'll just strip these for now. In the future we could consider
      //       only grabbing these (relevant) strings.
      if( !_Commands.sOutput.IsEmpty() ) sToken.Replace(_Commands.sOutput, _T(""));
      sToken.TrimRight();
      aLines.Add(sToken);
   }
}

// ILineCallback

HRESULT CCommandThread::OnIncomingLine(BSTR bstr)
{
   m_sResult += bstr;
   m_sResult += _T("\n");
   return S_OK;
}

// IUnknown

HRESULT CCommandThread::QueryInterface(REFIID riid, void** ppvObject)
{
   if( riid == __uuidof(ILineCallback) ) {
      *ppvObject = (ILineCallback*) this;
      return S_OK;
   }
   return E_NOINTERFACE;
}

ULONG CCommandThread::AddRef(void)
{
   return 1;
}

ULONG CCommandThread::Release(void)
{
   return 1;
}



////////////////////////////////////////////////////////////////7
//
//

bool CScCommands::Init()
{
   TCHAR szBuffer[128] = { 0 };

   _pDevEnv->GetProperty(_T("sourcecontrol.type"), szBuffer, 127);
   sType = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.enable"), szBuffer, 127);
   bEnabled = _tcscmp(szBuffer, _T("true")) == 0;

   _pDevEnv->GetProperty(_T("sourcecontrol.program"), szBuffer, 127);
   sProgram = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.output"), szBuffer, 127);
   sOutput = szBuffer;

   _pDevEnv->GetProperty(_T("sourcecontrol.cmd.checkin"), szBuffer, 127);
   sCmdCheckIn = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.cmd.checkout"), szBuffer, 127);
   sCmdCheckOut = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.cmd.update"), szBuffer, 127);
   sCmdUpdate = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.cmd.addfile"), szBuffer, 127);
   sCmdAddFile = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.cmd.removefile"), szBuffer, 127);
   sCmdRemoveFile = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.cmd.status"), szBuffer, 127);
   sCmdStatus = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.cmd.diff"), szBuffer, 127);
   sCmdDiff = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.cmd.login"), szBuffer, 127);
   sCmdLogIn = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.cmd.logout"), szBuffer, 127);
   sCmdLogOut = szBuffer;

   _pDevEnv->GetProperty(_T("sourcecontrol.opt.message"), szBuffer, 127);
   sOptMessage = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.opt.recursive"), szBuffer, 127);
   sOptRecursive = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.opt.sticky"), szBuffer, 127);
   sOptStickyTag = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.opt.updatedirs"), szBuffer, 127);
   sOptUpdateDirs = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.opt.branch"), szBuffer, 127);
   sOptBranch = szBuffer;

   return true;
}

bool CScCommands::CollectFiles(CSimpleArray<CString>& aFiles)
{
   extern IElement* g_pCurElement;

   IElement* pElement = g_pCurElement;

   TCHAR szType[100] = { 0 };
   pElement->GetType(szType, 99);

   bool bIsFolder = _tcscmp(szType, _T("Folder")) == 0;

   CComDispatchDriver dd = pElement->GetDispatch();
   if( dd == NULL ) return false;

   // Does it have a collection of files?
   CComVariant vFiles;
   dd.GetPropertyByName(OLESTR("Files"), &vFiles);
   if( vFiles.vt == VT_DISPATCH ) {
      dd = vFiles.pdispVal;
      bIsFolder = true;
   }

   if( bIsFolder ) {
      // It's a folder with a files collection
      CComVariant vCount;
      dd.GetPropertyByName(OLESTR("Count"), &vCount);
      if( vCount.vt != VT_I4 ) return false;
      long nCount = vCount.lVal;
      for( long i = 1; i <= nCount; i++ ) {
         CComVariant vItem;
         CComVariant vIndex = i;
         dd.Invoke1(OLESTR("Item"), &vIndex, &vItem);
         if( vItem.vt != VT_DISPATCH || vItem.pdispVal == NULL ) return false;

         CComDispatchDriver ddItem = vItem.pdispVal;
         if( ddItem == NULL ) return false;

         CComVariant vFilename;
         ddItem.GetPropertyByName(OLESTR("Filename"), &vFilename);
         if( vFilename.vt != VT_BSTR || ::SysStringLen(vFilename.bstrVal) == 0 ) continue;
         CString sFilename = vFilename.bstrVal;
         aFiles.Add(sFilename);
      }
   }
   else {
      // It's a regular element; let's interrogate the filename
      CComVariant vFilename;
      dd.GetPropertyByName(OLESTR("Filename"), &vFilename);
      if( vFilename.vt != VT_BSTR || ::SysStringLen(vFilename.bstrVal) == 0 ) return false;
      CString sFilename = vFilename.bstrVal;
      aFiles.Add(sFilename);
   }
   return true;
}

bool CScCommands::CheckIn(CSimpleArray<CString>& aFiles)
{
   CCheckInDlg dlg;
   dlg.m_pOwner = this;
   dlg.m_pFiles = &aFiles;
   if( dlg.DoModal() != IDOK ) return false;

   CString sCmd;
   sCmd.Format(_T("%s %s"), sProgram, sCmdCheckIn);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   sCmd += _T(" ");
   sCmd += dlg.m_sOptions;
   sCmd += _T("\n");
   m_thread.Stop();
   m_thread.SetCommand(ID_SC_CHECKIN, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::CheckOut(CSimpleArray<CString>& aFiles)
{
   CString sCmd;
   sCmd.Format(_T("%s %s"), sProgram, sCmdCheckOut);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   sCmd += _T("\n");
   m_thread.Stop();
   m_thread.SetCommand(ID_SC_CHECKOUT, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::Update(CSimpleArray<CString>& aFiles)
{
   CUpdateDlg dlg;
   dlg.m_pOwner = this;
   dlg.m_pFiles = &aFiles;
   if( dlg.DoModal() != IDOK ) return false;

   CString sCmd;
   sCmd.Format(_T("%s %s"), sProgram, sCmdUpdate);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   sCmd += _T("\n");
   m_thread.Stop();
   m_thread.SetCommand(ID_SC_UPDATE, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::AddFile(CSimpleArray<CString>& aFiles)
{
   CAddFileDlg dlg;
   dlg.m_pOwner = this;
   dlg.m_pFiles = &aFiles;
   if( dlg.DoModal() != IDOK ) return false;
   
   CString sCmd;
   sCmd.Format(_T("%s %s"), sProgram, sCmdAddFile);
   sCmd += dlg.m_sOptions;
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   sCmd += _T("\n");
   m_thread.Stop();
   m_thread.SetCommand(ID_SC_ADDFILE, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::RemoveFile(CSimpleArray<CString>& aFiles)
{
   CString sCmd;
   sCmd.Format(_T("%s %s"), sProgram, sCmdRemoveFile);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   sCmd += _T("\n");
   m_thread.Stop();
   m_thread.SetCommand(ID_SC_REMOVEFILE, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::LogIn(CSimpleArray<CString>& aFiles)
{
   CString sCmd;
   sCmd.Format(_T("%s %s"), sProgram, sCmdLogIn);
   if( sType == _T("cvs") ) {
      CLoginCvsDlg dlg;
      if( dlg.DoModal() != IDOK ) return false;
      sCmd += dlg.m_sOptions;
      // Send password as raw input through connection
      sCmd += "\n";      
      sCmd += dlg.m_sPassword;
   }
   sCmd += _T("\n");
   m_thread.Stop();
   m_thread.SetCommand(ID_SC_LOGIN, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::LogOut(CSimpleArray<CString>& aFiles)
{
   CString sCmd;
   sCmd.Format(_T("%s %s"), sProgram, sCmdLogOut);
   sCmd += _T("\n");
   m_thread.Stop();
   m_thread.SetCommand(ID_SC_LOGOUT, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::StatusFile(CSimpleArray<CString>& aFiles)
{
   CString sCmd;
   sCmd.Format(_T("%s %s"), sProgram, sCmdStatus);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   sCmd += _T("\n");
   m_thread.Stop();
   m_thread.SetCommand(ID_SC_DIFFVIEW, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::DiffFile(CSimpleArray<CString>& aFiles)
{
   CString sCmd;
   sCmd.Format(_T("%s %s"), sProgram, sCmdDiff);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   sCmd += _T("\n");
   m_thread.Stop();
   m_thread.SetCommand(ID_SC_STATUS, sCmd);
   m_thread.Start();
   return true;
}

