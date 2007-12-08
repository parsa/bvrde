
#include "StdAfx.h"
#include "resource.h"

#include "Globals.h"
#include "Commands.h"

#pragma code_seg( "MISC" )

#include "UpdateDlg.h"
#include "AddFileDlg.h"
#include "CheckInDlg.h"
#include "LoginCvsDlg.h"

#include "DiffCvsView.h"


////////////////////////////////////////////////////////////////7
//
//

void CCommandThread::ChangePath()
{
   CString sCommand = _T("cd $PATH");

   // Attempt to lure the ChangePath command out
   // of the project settings.
   IProject* pProject = _GetProject();
   if( pProject != NULL ) {
      CComDispatchDriver dd = pProject->GetDispatch();
      CComVariant vRet;
      CComVariant aParams[2];
      aParams[1] = L"Compiler";
      aParams[0] = L"ChangeDir";
      dd.InvokeN(OLESTR("GetParam"), aParams, 2, &vRet);
      if( vRet.vt == VT_BSTR ) sCommand = vRet.bstrVal;
   }

   AddCommand(IDC_SC_CHANGEDIR, sCommand);
}

void CCommandThread::AddCommand(UINT nCmd, LPCTSTR pstrCommand, LONG lTimeout /*= 4000L*/)
{
   CString sCommand = pstrCommand;
   m_aCmdIds.Add(nCmd);
   m_aCommands.Add(sCommand);
   m_lTimeout = lTimeout;
}

CString CCommandThread::GetResult() const
{
   return m_sResult;
}

DWORD CCommandThread::Run()
{
   _pDevEnv->SetThreadLanguage();

   CCoInitialize cominit;

   // Construct and execute commands through the
   // project's scripting mode.
   m_sResult.Empty();

   UINT nCmd = 0;
   bool bActivateOutputView = true;

   int i;
   CSimpleArray<CString> aCommands;
   for( i = 0; i < m_aCommands.GetSize(); i++ ) {
      CString sCommand = m_aCommands[i];
      aCommands.Add(sCommand);

      switch( m_aCmdIds[i] ) {
      case ID_SC_DIFFVIEW:
         bActivateOutputView = false;
         nCmd = ID_VIEW_CVSDIFF;
         break;
      }
   }
   m_aCommands.RemoveAll();
   m_aCmdIds.RemoveAll();

   // Bring up the Command View so we can see it all...
   CRichEditCtrl ctrlEdit = _pDevEnv->GetHwnd(IDE_HWND_COMMANDVIEW);
   if( bActivateOutputView ) _pDevEnv->ActivateAutoHideView(ctrlEdit);

   for( i = 0; i < aCommands.GetSize(); i++ ) {
      // HACK: Racing to submit all commands may obscure the telnet output
      //       with stdout and new commands being mixed on the same stream.
      //       We'll play nicely and allow the commands to appear on the screen
      //       sequentially.
      if( i >= 2 ) ::Sleep(1000L);
      // Send new command to remote
      IProject* pProject = _GetProject();
      if( pProject == NULL ) return 0;
      IDispatch* pDisp = pProject->GetDispatch();
      if( pDisp == NULL ) return 0;
      CComDispatchDriver dd = pDisp;
      CString sCommand = aCommands[i];
      CComBSTR bstrCommand = sCommand;
      CComVariant aParams[3];
      aParams[2] = bstrCommand;
      aParams[1] = static_cast<IUnknown*>(this);
      aParams[0] = m_lTimeout;
      dd.InvokeN(OLESTR("ExecCommand"), aParams, 3);
   }

   // Let's look at the result
   _SplitResult(m_sResult, m_aLines);

   CString sMessage = _T("\r\n");
   for( i = 0; i < m_aLines.GetSize(); i++ ) {
      CString sLine = m_aLines[i];
      sLine.TrimRight();
      sMessage += sLine;
      sMessage += _T("\r\n");
   }

   // Output it to the Command View; end with new prompt
   ctrlEdit.SetReadOnly(TRUE);
   AppendRtfText(ctrlEdit, sMessage, CFM_BOLD, 0);
   AppendRtfText(ctrlEdit, _T("\r\n> "), CFM_BOLD, CFE_BOLD);
   ctrlEdit.SetReadOnly(FALSE);

   // May need to trigger action again. This would for instance
   // be to display the diff. in a window.
   if( nCmd != 0 ) {
      CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
      wndMain.PostMessage(WM_COMMAND, MAKEWPARAM(nCmd, 0));
   }

   return 0;
}

// Implementation

void CCommandThread::_SplitResult(CString sResult, CSimpleArray<CString>& aLines) const
{
   aLines.RemoveAll();
   while( !sResult.IsEmpty() ) {
      CString sToken = sResult.SpanExcluding(_T("\n"));
      if( sToken.GetLength() == 0 ) sToken = sResult;
      sResult = sResult.Mid(sToken.GetLength() + 1);
      // TODO: A little format helper; some of these source-control systems spit
      //       out stdout-prefixes (like "cvs server:") to mark its output.
      //       We'll just strip these for now. In the future we could consider
      //       only grabbing these (relevant) strings.
      if( !_Commands.sOutput.IsEmpty() ) sToken.Replace(_Commands.sOutput, _T(""));
      aLines.Add(sToken);
   }
}

IProject* CCommandThread::_GetProject() const
{
   IElement* pElement = _Commands.m_pCurElement;
   while( pElement != NULL ) {
      WCHAR szType[60] = { 0 };
      pElement->GetType(szType, 59);
      if( wcscmp(szType, L"Project") == 0 ) return static_cast<IProject*>(pElement);
      pElement = pElement->GetParent();
   }
   return NULL;
}

// ILineCallback

HRESULT CCommandThread::OnIncomingLine(BSTR bstr)
{
   if( ShouldStop() ) return E_ABORT;
   m_sResult += bstr;
   m_sResult += _T("\n");
   return S_OK;
}

// IUnknown

HRESULT CCommandThread::QueryInterface(REFIID riid, void** ppvObject)
{
   if( riid == __uuidof(ILineCallback) || riid == IID_IUnknown ) {
      *ppvObject = static_cast<ILineCallback*>(this);
      return S_OK;
   }
   return E_NOINTERFACE;
}

ULONG CCommandThread::AddRef()
{
   return 1;
}

ULONG CCommandThread::Release()
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
   _pDevEnv->GetProperty(_T("sourcecontrol.opt.general"), szBuffer, 127);
   sOptCommon = szBuffer;

   _pDevEnv->GetProperty(_T("sourcecontrol.browse.all"), szBuffer, 127);
   sBrowseAll = szBuffer;
   _pDevEnv->GetProperty(_T("sourcecontrol.browse.single"), szBuffer, 127);
   sBrowseSingle = szBuffer;

   return true;
}

bool CScCommands::SetCurElement(IElement* pElement)
{
   m_bIsFolder = false;
   m_pCurElement = pElement;
   if( pElement == NULL ) return false;
   // Ok, figure out if the element is a container (folder)
   // or not...
   CComDispatchDriver dd = pElement->GetDispatch();
   if( dd == NULL ) return false;
   DISPID dispItem = 0;
   if( SUCCEEDED( dd.GetIDOfName(L"Item", &dispItem) ) ) m_bIsFolder = true;
   if( SUCCEEDED( dd.GetIDOfName(L"Files", &dispItem) ) ) m_bIsFolder = true;
   return true;
}

bool CScCommands::CollectFiles(CSimpleArray<CString>& aFiles)
{
   IElement* pElement = m_pCurElement;

   TCHAR szType[100] = { 0 };
   pElement->GetType(szType, 99);

   CComDispatchDriver dd = pElement->GetDispatch();
   if( dd == NULL ) return false;

   // Does it have a collection of files?
   DISPID dispItem = 0;
   HRESULT HrHasItem = dd.GetIDOfName(L"Item", &dispItem);
   CComVariant vFiles;
   dd.GetPropertyByName(OLESTR("Files"), &vFiles);
   if( vFiles.vt == VT_EMPTY && HrHasItem == S_OK ) vFiles = dd;
   if( vFiles.vt == VT_DISPATCH ) {
      dd = vFiles.pdispVal;
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
         sFilename.Replace('\\', '/');
         aFiles.Add(sFilename);
      }
   }
   else {
      // It's a regular element; let's interrogate the filename
      CComVariant vFilename;
      dd.GetPropertyByName(OLESTR("Filename"), &vFilename);
      if( vFilename.vt != VT_BSTR || ::SysStringLen(vFilename.bstrVal) == 0 ) return false;
      // Let's just save the file before doing anything to it
      TCHAR szBuffer[32] = { 0 };
      _pDevEnv->GetProperty(_T("editors.general.saveBeforeTool"), szBuffer, 31);
      if( _tcscmp(szBuffer, _T("true")) == 0 ) dd.Invoke0(L"Save");      
      // Add the file to the collection
      CString sFilename = vFilename.bstrVal;
      sFilename.Replace('\\', '/');
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
   sCmd.Format(_T("%s %s %s %s"), sProgram, sOptCommon, sCmdCheckIn, dlg.m_sOptions);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   m_thread.Stop();
   m_thread.ChangePath();
   m_thread.AddCommand(ID_SC_CHECKIN, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::CheckOut(CSimpleArray<CString>& aFiles)
{
   CString sCmd;
   sCmd.Format(_T("%s %s %s"), sProgram, sOptCommon, sCmdCheckOut);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   m_thread.Stop();
   m_thread.ChangePath();
   m_thread.AddCommand(ID_SC_CHECKOUT, sCmd);
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
   sCmd.Format(_T("%s %s %s"), sProgram, sOptCommon, sCmdUpdate);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   m_thread.Stop();
   m_thread.ChangePath();
   m_thread.AddCommand(ID_SC_UPDATE, sCmd);
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
   sCmd.Format(_T("%s %s %s %s"), sProgram, sOptCommon, sCmdAddFile, dlg.m_sOptions);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   m_thread.Stop();
   m_thread.ChangePath();
   m_thread.AddCommand(ID_SC_ADDFILE, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::RemoveFile(CSimpleArray<CString>& aFiles)
{
   CString sCmd;
   sCmd.Format(_T("%s %s %s"), sProgram, sOptCommon, sCmdRemoveFile);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   m_thread.Stop();
   m_thread.ChangePath();
   m_thread.AddCommand(ID_SC_REMOVEFILE, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::LogIn(CSimpleArray<CString>& aFiles)
{
   CString sCmd;
   CString sPassword;
   sCmd.Format(_T("%s %s %s"), sProgram, sOptCommon, sCmdLogIn);
   if( sType == _T("cvs") ) {
      CLoginCvsDlg dlg;
      if( dlg.DoModal() != IDOK ) return false;
      sPassword = dlg.m_sPassword;
      sCmd += dlg.m_sOptions;
   }
   m_thread.Stop();
   m_thread.ChangePath();
   m_thread.AddCommand(ID_SC_LOGIN, sCmd, 1000);
   if( sType == _T("cvs") ) {
      // Send password in stream as well
      // Hmm, the cvs server blocks the input through
      // the Telnet shell. We'll not send the password right
      // away and do it in two steps (automatic delay in thread).
      // BUG: This is very non-portable! What if there's no telnet; or a different 
      //      cvs impl???? FIX THIS!!!
      m_thread.AddCommand(ID_SC_LOGIN, sPassword, 2000);
   }
   m_thread.Start();
   return true;
}

bool CScCommands::LogOut(CSimpleArray<CString>& aFiles)
{
   CString sCmd;
   sCmd.Format(_T("%s %s %s"), sProgram, sOptCommon, sCmdLogOut);
   m_thread.Stop();
   m_thread.ChangePath();
   m_thread.AddCommand(ID_SC_LOGOUT, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::StatusFile(CSimpleArray<CString>& aFiles)
{
   CString sCmd;
   sCmd.Format(_T("%s %s %s"), sProgram, sOptCommon, sCmdStatus);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   m_thread.Stop();
   m_thread.ChangePath();
   m_thread.AddCommand(ID_SC_STATUS, sCmd);
   m_thread.Start();
   return true;
}

bool CScCommands::DiffFile(CSimpleArray<CString>& aFiles)
{
   CString sCmd;
   sCmd.Format(_T("%s %s %s"), sProgram, sOptCommon, sCmdDiff);
   for( int i = 0; i < aFiles.GetSize(); i++ ) {
      sCmd += _T(" ");
      sCmd += aFiles[i];
   }
   m_thread.Stop();
   m_thread.ChangePath();
   m_thread.AddCommand(ID_SC_DIFFVIEW, sCmd);
   m_thread.Start();
   return true;
}

void CScCommands::ShowDiffView()
{
   CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
   CDiffCvsView* pView = new CDiffCvsView;
   pView->Create(wndMain, CWindow::rcDefault);
   if( !pView->GeneratePage(m_pCurElement, m_thread.m_aLines) ) {
      // Failed to produce anything to display
      pView->DestroyWindow();
      // Instead, display the diff output...
      CRichEditCtrl ctrlEdit = _pDevEnv->GetHwnd(IDE_HWND_COMMANDVIEW);
      _pDevEnv->ActivateAutoHideView(ctrlEdit);
   }
}
