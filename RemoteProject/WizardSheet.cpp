
#include "stdafx.h"
#include "resource.h"

#include "WizardSheet.h"
#include "Project.h"

#pragma code_seg( "WIZARDS" )

#include "CryptLibDlg.h"


////////////////////////////////////////////////////////////////////////
//

#define SET_CHECK(id, prop) \
   { TCHAR szBuf[32] = { 0 }; _pDevEnv->GetProperty(prop, szBuf, 31); \
     if( _tcscmp(szBuf, _T("false"))!=0 ) CButton(GetDlgItem(id)).Click(); }

#define GET_CHECK(id, prop) \
   _pDevEnv->SetProperty(prop, CButton(GetDlgItem(id)).GetCheck() == BST_CHECKED ? _T("true") : _T("false"))

#define TRANSFER_PROP(name, prop) \
   { TCHAR szBuf[32] = { 0 }; _pDevEnv->GetProperty(prop, szBuf, 31); \
     m_pArc->Write(name, szBuf); }


///////////////////////////////////////////////////////////////
//

struct WIZDATA
{
   long lType;
   CString sHost;
   CString sUsername;
   CString sPassword;
   CString sPath;
   CString sExtra;
} g_data;


static void CheckCryptLib()
{
   static bool s_bFirstTime = true;
   if( !s_bFirstTime ) return;
   s_bFirstTime = false;
   CString sFilename;
   sFilename.Format(_T("%sCL32.DLL"), CModulePath());
   if( !CFile::FileExists(sFilename) ) {
      CCryptLibDlg dlg;
      dlg.DoModal();
   }
}


///////////////////////////////////////////////////////////////
//

LRESULT CFileTransferPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlTest = GetDlgItem(IDC_TEST);
   m_ctrlType = GetDlgItem(IDC_TYPE);
   m_ctrlHost = GetDlgItem(IDC_HOST);
   m_ctrlPort = GetDlgItem(IDC_PORT);
   m_ctrlUsername = GetDlgItem(IDC_USERNAME);
   m_ctrlPassword = GetDlgItem(IDC_PASSWORD);
   m_ctrlPath = GetDlgItem(IDC_PATH);

   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_FTP)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_SFTP)));

   CString sType = m_pProject->m_FileManager.GetParam(_T("Type"));
   long lType = 0; // ftp
   if( sType == _T("sftp") ) lType = 1;
   m_ctrlType.SetCurSel(lType);

   g_data.sHost.Empty();
   g_data.sUsername.Empty();
   g_data.sPassword.Empty();
   g_data.sPath.Empty();
   g_data.sExtra.Empty();

   m_ctrlHost.SetWindowText(m_pProject->m_FileManager.GetParam(_T("Host")));
   m_ctrlUsername.SetWindowText(m_pProject->m_FileManager.GetParam(_T("Username")));
   m_ctrlPassword.SetWindowText(m_pProject->m_FileManager.GetParam(_T("Password")));
   m_ctrlPath.SetWindowText(m_pProject->m_FileManager.GetParam(_T("Path")));
   m_ctrlPort.SetWindowText(m_pProject->m_FileManager.GetParam(_T("Port")));

   if( _ttol(CWindowText(m_ctrlPort)) == 0 ) m_ctrlPort.SetWindowText(_T("21"));

   return 0;
}

LRESULT CFileTransferPage::OnTypeChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   long lPort = 21;
   switch( m_ctrlType.GetCurSel() ) {
   case 1: lPort = 22; break; // sftp
   }
   m_ctrlPort.SetWindowText(ToString(lPort));
   return 0;
}

LRESULT CFileTransferPage::OnTextChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   BOOL bOK = TRUE;
   if( m_ctrlHost.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( m_ctrlPort.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( m_ctrlUsername.GetWindowTextLength() == 0 && m_ctrlPassword.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( m_ctrlPath.GetWindowTextLength() == 0 ) bOK = FALSE;
   SetWizardButtons(bOK ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK);
   CWindow(GetDlgItem(IDC_TEST)).EnableWindow(bOK);
   return 0;
}

LRESULT CFileTransferPage::OnTest(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CWaitCursor cursor;
   m_ctrlTest.EnableWindow(FALSE);

   SecClearPassword();

   CString sHost = CWindowText(m_ctrlHost);
   CString sUsername = CWindowText(m_ctrlUsername);
   CString sPassword = CWindowText(m_ctrlPassword);
   CString sPath = CWindowText(m_ctrlPath);
   long lPort = _ttol(CWindowText(m_ctrlPort));

   CString sType = _T("ftp");
   switch( m_ctrlType.GetCurSel() ) {
   case 1: sType = _T("sftp"); break;
   }

   CFileManager fm;
   fm.SetParam(_T("Type"), sType);
   fm.SetParam(_T("Host"), sHost);
   fm.SetParam(_T("Username"), sUsername);
   fm.SetParam(_T("Password"), sPassword);
   fm.SetParam(_T("Port"), ToString(lPort));
   fm.SetParam(_T("Path"), sPath);
   fm.Start();
   if( !fm.SetCurPath(CWindowText(m_ctrlPath)) ) {
      fm.SignalStop();
      GenerateError(_pDevEnv, IDS_ERR_NOCONNECTION);
   }
   else {
      fm.SignalStop();
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_MESSAGE));
      CString sMsg(MAKEINTRESOURCE(IDS_CONNECTION_OK));
      _pDevEnv->ShowMessageBox(m_hWnd, sMsg, sCaption, MB_ICONINFORMATION);
   }
   fm.Stop();

   m_ctrlTest.EnableWindow(TRUE);
   return 0;
}

int CFileTransferPage::OnSetActive()
{
   BOOL bDummy;
   OnTextChange(0, 0, NULL, bDummy);
   return 0;
}

int CFileTransferPage::OnWizardNext()
{
   // Store the values on the first page for use in the remaining pages.
   g_data.lType = m_ctrlType.GetCurSel();
   g_data.sHost = CWindowText(m_ctrlHost);
   g_data.sUsername = CWindowText(m_ctrlUsername);
   g_data.sPassword = CWindowText(m_ctrlPassword);
   g_data.sPath = CWindowText(m_ctrlPath);
   return 0;
}

int CFileTransferPage::OnApply()
{
   ATLASSERT(m_pProject);

   CString sType = m_ctrlType.GetCurSel() == 0 ? _T("ftp") : _T("sftp");
   CString sHost = CWindowText(m_ctrlHost);
   CString sUsername = CWindowText(m_ctrlUsername);
   CString sPassword = CWindowText(m_ctrlPassword);
   CString sPath = CWindowText(m_ctrlPath);
   long lPort = _ttol(CWindowText(m_ctrlPort));
 
   m_pProject->m_FileManager.Stop();
   m_pProject->m_FileManager.SetParam(_T("Type"), sType);
   m_pProject->m_FileManager.SetParam(_T("Host"), sHost);
   m_pProject->m_FileManager.SetParam(_T("Username"), sUsername);
   m_pProject->m_FileManager.SetParam(_T("Password"), sPassword);
   m_pProject->m_FileManager.SetParam(_T("Port"), ToString(lPort));
   m_pProject->m_FileManager.SetParam(_T("Path"), sPath);
   m_pProject->m_FileManager.Start();

   return PSNRET_NOERROR;
}


///////////////////////////////////////////////////////////////
//

LRESULT CFileOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlList.SubclassWindow(GetDlgItem(IDC_LIST));
   m_ctrlList.SetExtendedListStyle(PLS_EX_XPLOOK|PLS_EX_CATEGORIZED);
   m_ctrlList.SetColumnWidth(130);
   
   CString sName;
   CString sValue;
   bool bValue;
   
   sName.LoadString(IDS_OPTIONS);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_COMMAND_PASSIVE);
   bValue = m_pProject->m_FileManager.GetParam(_T("Passive")) == _T("true");
   m_ctrlList.AddItem(PropCreateSimple(sName, bValue, 1));
   sName.LoadString(IDS_COMMAND_PROXY);
   sValue = m_pProject->m_FileManager.GetParam(_T("Proxy"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 2));

   return 0;
}

int CFileOptionsPage::OnSetActive()
{
   return 0;
}

int CFileOptionsPage::OnApply()
{
   ATLASSERT(m_pProject);

   CComVariant v;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(1), &v);
   m_pProject->m_FileManager.SetParam(_T("Passive"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(2), &v);
   m_pProject->m_FileManager.SetParam(_T("Proxy"), CString(v.bstrVal));

   return PSNRET_NOERROR;
}


///////////////////////////////////////////////////////////////
//

LRESULT CCompilerPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlTest = GetDlgItem(IDC_TEST);
   m_ctrlType = GetDlgItem(IDC_TYPE);
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_TELNET)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_RLOGIN)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_SSH)));
   m_ctrlHost = GetDlgItem(IDC_HOST);
   m_ctrlPort = GetDlgItem(IDC_PORT);
   m_ctrlUsername = GetDlgItem(IDC_USERNAME);
   m_ctrlPassword = GetDlgItem(IDC_PASSWORD);
   m_ctrlPath = GetDlgItem(IDC_PATH);
   m_ctrlExtra = GetDlgItem(IDC_EXTRA);

   m_ctrlHost.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("Host")));
   m_ctrlUsername.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("Username")));
   m_ctrlPassword.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("Password")));
   m_ctrlPath.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("Path")));
   m_ctrlPort.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("Port")));
   m_ctrlExtra.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("Extra")));

   if( m_ctrlHost.GetWindowTextLength() == 0 ) {
      m_ctrlHost.SetWindowText(g_data.sHost);
      m_ctrlPort.SetWindowText(_T("23"));
      m_ctrlUsername.SetWindowText(g_data.sUsername);
      m_ctrlPassword.SetWindowText(g_data.sPassword);
      m_ctrlPath.SetWindowText(g_data.sPath);
   }

   CString sType = m_pProject->m_CompileManager.GetParam(_T("Type"));
   long lType = 0; // telnet
   if( sType == _T("rlogin") ) lType = 1;
   if( sType == _T("ssh") ) lType = 2;
   m_ctrlType.SetCurSel(lType);

   return 0;
}

LRESULT CCompilerPage::OnTest(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CWaitCursor cursor;
   m_ctrlTest.EnableWindow(FALSE);

   SecClearPassword();

   CString sHost = CWindowText(m_ctrlHost);
   CString sUsername = CWindowText(m_ctrlUsername);
   CString sPassword = CWindowText(m_ctrlPassword);
   CString sPath = CWindowText(m_ctrlPath);
   long lPort = _ttol(CWindowText(m_ctrlPort));
   CString sExtra = CWindowText(m_ctrlExtra);

   CString sType = _T("telnet");
   switch( m_ctrlType.GetCurSel() ) {
   case 1: sType = _T("rlogin"); break;
   case 2: sType = _T("ssh"); break;
   }

   if( sType == _T("ssh") ) CheckCryptLib();

   CShellManager sm;
   sm.Init(m_pProject);
   sm.SetParam(_T("Type"), sType);
   sm.SetParam(_T("Host"), sHost);
   sm.SetParam(_T("Username"), sUsername);
   sm.SetParam(_T("Password"), sPassword);
   sm.SetParam(_T("Port"), ToString(lPort));
   sm.SetParam(_T("Path"), sPath);
   sm.SetParam(_T("Extra"), sExtra);
   sm.Start();
   if( !sm.WaitForConnection() ) {
      sm.SignalStop();
      GenerateError(_pDevEnv, IDS_ERR_NOCONNECTION);
   }
   else {
      sm.SignalStop();
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_MESSAGE));
      CString sMsg(MAKEINTRESOURCE(IDS_CONNECTION_OK));
      _pDevEnv->ShowMessageBox(m_hWnd, sMsg, sCaption, MB_ICONINFORMATION);
   }
   sm.Stop();

   m_ctrlTest.EnableWindow(TRUE);
   return 0;
}

LRESULT CCompilerPage::OnTypeChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   long lPort = 23;
   switch( m_ctrlType.GetCurSel() ) {
   case 1: lPort = 513; break; // rlogin
   case 2: lPort = 22; break;  // ssh
   }
   m_ctrlPort.SetWindowText(ToString(lPort));
   return 0;
}

LRESULT CCompilerPage::OnTextChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   bool bOK = true;
   if( m_ctrlHost.GetWindowTextLength() == 0 ) bOK = false;
   if( m_ctrlPort.GetWindowTextLength() == 0 ) bOK = false;
   if( m_ctrlUsername.GetWindowTextLength() == 0 && m_ctrlPassword.GetWindowTextLength() == 0 ) bOK = false;
   if( m_ctrlPath.GetWindowTextLength() == 0 ) bOK = false;
   SetWizardButtons(bOK ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK);
   return 0;
}

int CCompilerPage::OnWizardNext()
{
   CString sPath = CWindowText(m_ctrlPath);

   static bool s_bWarned = false;
   if( !s_bWarned
       && !sPath.IsEmpty() 
       && sPath.Left(1) != _T("/") 
       && sPath.FindOneOf(_T("\\:")) < 0 ) 
   {
      if( IDNO == _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_ERR_RELPATH)), CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONWARNING | MB_YESNO) ) return -1;
      s_bWarned = true;
   }

   // Store the values on the first page for use in the remaining pages.
   g_data.sExtra = CWindowText(m_ctrlExtra);
   return 0;
}

int CCompilerPage::OnSetActive()
{
   BOOL bDummy;
   OnTextChange(0, 0, NULL, bDummy);
   return 0;
}

int CCompilerPage::OnApply()
{
   ATLASSERT(m_pProject);

   CString sHost = CWindowText(m_ctrlHost);
   CString sUsername = CWindowText(m_ctrlUsername);
   CString sPassword = CWindowText(m_ctrlPassword);
   CString sPath = CWindowText(m_ctrlPath);
   long lPort = _ttol(CWindowText(m_ctrlPort));
   CString sExtra = CWindowText(m_ctrlExtra);

   CString sType = _T("telnet");
   switch( m_ctrlType.GetCurSel() ) {
   case 1: sType = _T("rlogin"); break;
   case 2: sType = _T("ssh"); break;
   }

   m_pProject->m_CompileManager.Stop();
   m_pProject->m_CompileManager.SetParam(_T("Type"), sType);
   m_pProject->m_CompileManager.SetParam(_T("Host"), sHost);
   m_pProject->m_CompileManager.SetParam(_T("Username"), sUsername);
   m_pProject->m_CompileManager.SetParam(_T("Password"), sPassword);
   m_pProject->m_CompileManager.SetParam(_T("Port"), ToString(lPort));
   m_pProject->m_CompileManager.SetParam(_T("Path"), sPath);
   m_pProject->m_CompileManager.SetParam(_T("Extra"), sExtra);
   m_pProject->m_CompileManager.Start();

   if( sType == _T("ssh") ) CheckCryptLib();

   return PSNRET_NOERROR;
}


///////////////////////////////////////////////////////////////
//

LRESULT CCompilerCommandsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlList.SubclassWindow(GetDlgItem(IDC_LIST));
   m_ctrlList.SetExtendedListStyle(PLS_EX_XPLOOK | PLS_EX_CATEGORIZED);
   m_ctrlList.SetColumnWidth(130);
   
   CString sName;
   CString sValue;
   
   sName.LoadString(IDS_COMMANDS);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_COMMAND_CD);
   sValue = m_pProject->m_CompileManager.GetParam(_T("ChangeDir"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 1));
   sName.LoadString(IDS_COMMAND_BUILD);
   sValue = m_pProject->m_CompileManager.GetParam(_T("Build"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 2));
   sName.LoadString(IDS_COMMAND_REBUILD);
   sValue = m_pProject->m_CompileManager.GetParam(_T("Rebuild"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 3));
   sName.LoadString(IDS_COMMAND_COMPILE);
   sValue = m_pProject->m_CompileManager.GetParam(_T("Compile"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 4));
   sName.LoadString(IDS_COMMAND_CLEAN);
   sValue = m_pProject->m_CompileManager.GetParam(_T("Clean"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 5));

   sName.LoadString(IDS_DIRECTIVES);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_COMMAND_DEBUGEXPORT);
   sValue = m_pProject->m_CompileManager.GetParam(_T("DebugExport"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 6));
   sName.LoadString(IDS_COMMAND_RELEASEEXPORT);
   sValue = m_pProject->m_CompileManager.GetParam(_T("ReleaseExport"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 7));

   return 0;
}

int CCompilerCommandsPage::OnSetActive()
{
   return 0;
}

int CCompilerCommandsPage::OnApply()
{
   ATLASSERT(m_pProject);

   CComVariant v;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(1), &v);
   m_pProject->m_CompileManager.SetParam(_T("ChangeDir"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(2), &v);
   m_pProject->m_CompileManager.SetParam(_T("Build"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(3), &v);
   m_pProject->m_CompileManager.SetParam(_T("Rebuild"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(4), &v);
   m_pProject->m_CompileManager.SetParam(_T("Compile"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(5), &v);
   m_pProject->m_CompileManager.SetParam(_T("Clean"), CString(v.bstrVal));

   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(6), &v);
   m_pProject->m_CompileManager.SetParam(_T("DebugExport"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(7), &v);
   m_pProject->m_CompileManager.SetParam(_T("ReleaseExport"), CString(v.bstrVal));

   return PSNRET_NOERROR;
}


///////////////////////////////////////////////////////////////
//

LRESULT CDebuggerPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlTest = GetDlgItem(IDC_TEST);

   m_ctrlDebugger = GetDlgItem(IDC_DEBUGGER);
   m_ctrlDebugger.AddString(CString(MAKEINTRESOURCE(IDS_GDB)));
   m_ctrlDebugger.SetCurSel(0);

   m_ctrlType = GetDlgItem(IDC_TYPE);
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_TELNET)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_RLOGIN)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_SSH)));
   m_ctrlHost = GetDlgItem(IDC_HOST);
   m_ctrlPort = GetDlgItem(IDC_PORT);
   m_ctrlUsername = GetDlgItem(IDC_USERNAME);
   m_ctrlPassword = GetDlgItem(IDC_PASSWORD);
   m_ctrlPath = GetDlgItem(IDC_PATH);
   m_ctrlExtra = GetDlgItem(IDC_EXTRA);

   m_ctrlHost.SetWindowText(m_pProject->m_DebugManager.GetParam(_T("Host")));
   m_ctrlUsername.SetWindowText(m_pProject->m_DebugManager.GetParam(_T("Username")));
   m_ctrlPassword.SetWindowText(m_pProject->m_DebugManager.GetParam(_T("Password")));
   m_ctrlPath.SetWindowText(m_pProject->m_DebugManager.GetParam(_T("Path")));
   m_ctrlPort.SetWindowText(m_pProject->m_DebugManager.GetParam(_T("Port")));
   m_ctrlExtra.SetWindowText(m_pProject->m_DebugManager.GetParam(_T("Extra")));

   if( m_ctrlHost.GetWindowTextLength() == 0 ) {
      m_ctrlHost.SetWindowText(g_data.sHost);
      m_ctrlPort.SetWindowText(_T("23"));
      m_ctrlUsername.SetWindowText(g_data.sUsername);
      m_ctrlPassword.SetWindowText(g_data.sPassword);
      m_ctrlPath.SetWindowText(g_data.sPath);
      m_ctrlExtra.SetWindowText(g_data.sExtra);
   }

   CString sType = m_pProject->m_DebugManager.GetParam(_T("Type"));
   long lType = 0; // telnet
   if( sType == _T("rlogin") ) lType = 1;
   if( sType == _T("ssh") ) lType = 2;
   m_ctrlType.SetCurSel(lType);

   return 0;
}

LRESULT CDebuggerPage::OnTypeChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   long lPort = 23;
   switch( m_ctrlType.GetCurSel() ) {
   case 1: lPort = 513; break; // rlogin
   case 2: lPort = 22; break;  // ssh
   }
   m_ctrlPort.SetWindowText(ToString(lPort));
   return 0;
}

LRESULT CDebuggerPage::OnTextChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   bool bOK = true;
   if( m_ctrlHost.GetWindowTextLength() == 0 ) bOK = false;
   if( m_ctrlPort.GetWindowTextLength() == 0 ) bOK = false;
   if( m_ctrlUsername.GetWindowTextLength() == 0 && m_ctrlPassword.GetWindowTextLength() == 0 ) bOK = false;
   if( m_ctrlPath.GetWindowTextLength() == 0 ) bOK = false;
   SetWizardButtons(bOK ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK);
   return 0;
}

LRESULT CDebuggerPage::OnTest(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CWaitCursor cursor;
   m_ctrlTest.EnableWindow(FALSE);

   SecClearPassword();

   CString sHost = CWindowText(m_ctrlHost);
   CString sUsername = CWindowText(m_ctrlUsername);
   CString sPassword = CWindowText(m_ctrlPassword);
   CString sPath = CWindowText(m_ctrlPath);
   long lPort = _ttol(CWindowText(m_ctrlPort));
   CString sExtra = CWindowText(m_ctrlExtra);

   CString sType = _T("telnet");
   switch( m_ctrlType.GetCurSel() ) {
   case 1: sType = _T("rlogin"); break;
   case 2: sType = _T("ssh"); break;
   }

   if( sType == _T("ssh") ) CheckCryptLib();

   CShellManager sm;
   sm.Init(m_pProject);
   sm.SetParam(_T("Type"), sType);
   sm.SetParam(_T("Host"), sHost);
   sm.SetParam(_T("Username"), sUsername);
   sm.SetParam(_T("Password"), sPassword);
   sm.SetParam(_T("Port"), ToString(lPort));
   sm.SetParam(_T("Path"), sPath);
   sm.SetParam(_T("Extra"), sExtra);
   sm.Start();
   if( !sm.WaitForConnection() ) {
      sm.SignalStop();
      GenerateError(_pDevEnv, IDS_ERR_NOCONNECTION);
   }
   else {
      sm.SignalStop();
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_MESSAGE));
      CString sMsg(MAKEINTRESOURCE(IDS_CONNECTION_OK));
      _pDevEnv->ShowMessageBox(m_hWnd, sMsg, sCaption, MB_ICONINFORMATION);
   }
   sm.Stop();

   m_ctrlTest.EnableWindow(TRUE);
   return 0;
}

int CDebuggerPage::OnSetActive()
{
   BOOL bDummy;
   OnTextChange(0, 0, NULL, bDummy);
   return 0;
}

int CDebuggerPage::OnApply()
{
   ATLASSERT(m_pProject);

   CString sHost = CWindowText(m_ctrlHost);
   CString sUsername = CWindowText(m_ctrlUsername);
   CString sPassword = CWindowText(m_ctrlPassword);
   CString sPath = CWindowText(m_ctrlPath);
   long lPort = _ttol(CWindowText(m_ctrlPort));
   CString sExtra = CWindowText(m_ctrlExtra);

   CString sType = _T("telnet");
   switch( m_ctrlType.GetCurSel() ) {
   case 1: sType = _T("rlogin"); break;
   case 2: sType = _T("ssh"); break;
   }

   m_pProject->m_DebugManager.Stop();
   m_pProject->m_DebugManager.SetParam(_T("Type"), sType);
   m_pProject->m_DebugManager.SetParam(_T("Host"), sHost);
   m_pProject->m_DebugManager.SetParam(_T("Username"), sUsername);
   m_pProject->m_DebugManager.SetParam(_T("Password"), sPassword);
   m_pProject->m_DebugManager.SetParam(_T("Port"), ToString(lPort));
   m_pProject->m_DebugManager.SetParam(_T("Path"), sPath);
   m_pProject->m_DebugManager.SetParam(_T("Extra"), sExtra);

   if( sType == _T("ssh") ) CheckCryptLib();
   
   return PSNRET_NOERROR;
}


///////////////////////////////////////////////////////////////
//

LRESULT CDebuggerCommandsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlList.SubclassWindow(GetDlgItem(IDC_LIST));
   m_ctrlList.SetExtendedListStyle(PLS_EX_XPLOOK|PLS_EX_CATEGORIZED);
   m_ctrlList.SetColumnWidth(130);
   
   CString sName;
   CString sValue;

   sName.LoadString(IDS_COMMANDS);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_COMMAND_CD);
   sValue = m_pProject->m_DebugManager.GetParam(_T("ChangeDir"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 1));
   sName.LoadString(IDS_COMMAND_EXECUTE);
   sValue = m_pProject->m_DebugManager.GetParam(_T("App"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 2));
   sName.LoadString(IDS_COMMAND_PARAMS);
   sValue = m_pProject->m_DebugManager.GetParam(_T("AppArgs"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 3));
   sName.LoadString(IDS_DEBUGGER);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_COMMAND_EXECUTE);
   sValue = m_pProject->m_DebugManager.GetParam(_T("Debugger"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 4));
   sName.LoadString(IDS_COMMAND_PARAMS);
   sValue = m_pProject->m_DebugManager.GetParam(_T("DebuggerArgs"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 5));
   sName.LoadString(IDS_COMMAND_MAIN);
   sValue = m_pProject->m_DebugManager.GetParam(_T("DebugMain"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 6));

   return 0;
}

int CDebuggerCommandsPage::OnSetActive()
{
   return 0;
}

int CDebuggerCommandsPage::OnApply()
{
   ATLASSERT(m_pProject);

   CComVariant v;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(1), &v);
   m_pProject->m_DebugManager.SetParam(_T("ChangeDir"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(2), &v);
   m_pProject->m_DebugManager.SetParam(_T("App"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(3), &v);
   m_pProject->m_DebugManager.SetParam(_T("AppArgs"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(4), &v);
   m_pProject->m_DebugManager.SetParam(_T("Debugger"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(5), &v);
   m_pProject->m_DebugManager.SetParam(_T("DebuggerArgs"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(6), &v);
   m_pProject->m_DebugManager.SetParam(_T("DebugMain"), CString(v.bstrVal));

   return PSNRET_NOERROR;
}


////////////////////////////////////////////////////////////////////////
//

LRESULT CAdvancedEditOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   ATLASSERT(!m_sLanguage.IsEmpty());

   CString sKey;
   sKey.Format(_T("editors.%s."), m_sLanguage);
   SET_CHECK(IDC_NOTIPS, sKey + _T("noTips"));
   SET_CHECK(IDC_PROTECTFILES, sKey + _T("protectDebugged"));
   SET_CHECK(IDC_MATCHBRACES, sKey + _T("matchBraces"));
   SET_CHECK(IDC_AUTOCOMPLETE, sKey + _T("autoComplete"));
   SET_CHECK(IDC_CLASSBROWSER, sKey + _T("classBrowser"));
   SET_CHECK(IDC_ONLINESCANNER, sKey + _T("onlineScanner"));
   SET_CHECK(IDC_AUTOSUGGEST, sKey + _T("autoSuggest"));
   SET_CHECK(IDC_AUTOCLOSE, sKey + _T("autoClose"));

   return 0;
}

int CAdvancedEditOptionsPage::OnApply()
{
   CString sKey;
   sKey.Format(_T("editors.%s."), m_sLanguage);
   GET_CHECK(IDC_NOTIPS, sKey + _T("noTips"));
   GET_CHECK(IDC_PROTECTFILES, sKey + _T("protectDebugged"));
   GET_CHECK(IDC_MATCHBRACES, sKey + _T("matchBraces"));
   GET_CHECK(IDC_AUTOCOMPLETE, sKey + _T("autoComplete"));
   GET_CHECK(IDC_CLASSBROWSER, sKey + _T("classBrowser"));
   GET_CHECK(IDC_ONLINESCANNER, sKey + _T("onlineScanner"));
   GET_CHECK(IDC_AUTOSUGGEST, sKey + _T("autoSuggest"));
   GET_CHECK(IDC_AUTOCLOSE, sKey + _T("autoClose"));

   if( m_pArc->ReadGroupBegin(_T("Editors")) ) 
   {
      while( m_pArc->ReadGroupBegin(_T("Editor")) ) {
         TCHAR szLanguage[32] = { 0 };
         m_pArc->Read(_T("name"), szLanguage, 31);
         if( m_sLanguage == szLanguage ) {
            m_pArc->Delete(_T("Advanced"));
            m_pArc->WriteItem(_T("Advanced"));
            TRANSFER_PROP(_T("noTips"), sKey + _T("noTips"));
            TRANSFER_PROP(_T("protectDebugged"), sKey + _T("protectDebugged"));
            TRANSFER_PROP(_T("matchBraces"), sKey + _T("matchBraces"));
            TRANSFER_PROP(_T("autoComplete"), sKey + _T("autoComplete"));
            TRANSFER_PROP(_T("classBrowser"), sKey + _T("classBrowser"));
            TRANSFER_PROP(_T("onlineScanner"), sKey + _T("onlineScanner"));
            TRANSFER_PROP(_T("autoSuggest"), sKey + _T("autoSuggest"));
            TRANSFER_PROP(_T("autoClose"), sKey + _T("autoClose"));
         }
         m_pArc->ReadGroupEnd();
      }
      m_pArc->ReadGroupEnd();
   }
   
   // HACK: To clear the iterator cache
   m_pArc->ReadGroupEnd();

   return PSNRET_NOERROR;
}
