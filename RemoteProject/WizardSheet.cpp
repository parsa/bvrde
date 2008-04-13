
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

#define WRITE_CHECKBOX(id, name) \
     m_pArc->Write(name, CButton(GetDlgItem(id)).GetCheck() == BST_CHECKED ? _T("true") : _T("false"));


///////////////////////////////////////////////////////////////
//

enum
{
   FILETRANSFER_FTP = 0,
   FILETRANSFER_SFTP,
   FILETRANSFER_NETWORK,
};

enum
{
   SHELLTRANSFER_TELNET = 0,
   SHELLTRANSFER_RLOGIN,
   SHELLTRANSFER_SSH,
   SHELLTRANSFER_COMSPEC,
};

enum
{
   DEBUGGERTYPE_GDB = 0,
   DEBUGGERTYPE_DBX,
};

// This structure contains the choices of the initial page's settings, so
// that we can pick similar settings on the remaining page in Wizard-mode.
// Meaning Telnet automatically picks FTP, SSH leads to SFTP, etc.
struct WIZDATA
{
   WIZDATA() : bIsWizard(false), iFileTransferType(-1), iShellType(-1) { };
   bool bIsWizard;                // Is pages shown in a Wizard?
   int iFileTransferType;         // File transfer type (FILETRANSFER_FTP etc)
   int iShellType;                // Shell transfer type (SHELLTRANSFER_TELNET etc)
   CString sHost;                 // Name of host
   CString sUsername;             // Username
   CString sPassword;             // Password
   CString sPath;                 // Source Path 
   CString sExtra;                // Additional commands during logon
   CString sServerType;           // Server type
} g_data;


static void CheckCryptLib(HWND hWnd)
{
   static bool s_bFirstTime = true;
   if( !s_bFirstTime ) return;
   s_bFirstTime = false;
   CString sFilename;
   sFilename.Format(_T("%sCL32.DLL"), CModulePath());
   if( CFile::FileExists(sFilename) ) return;
   CCryptLibDlg dlg;
   dlg.DoModal(hWnd);
}

static void CheckDebugPriv(HWND hWnd)
{
   static bool s_bFirstTime = true;
   if( !s_bFirstTime ) return;
   s_bFirstTime = false;
   if( EnableSystemAccessPriveledge(SE_DEBUG_NAME) ) return;
   _pDevEnv->ShowMessageBox(hWnd, CString(MAKEINTRESOURCE(IDS_ERR_DEBUGPRIV)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONWARNING);
}

static void CheckAttachConsole(HWND hWnd)
{
   typedef BOOL (WINAPI* PFNATTACHCONSOLE)(DWORD);
   if( ::GetProcAddress( ::GetModuleHandle(_T("kernel32.dll")), "AttachConsole" ) != NULL ) return;
   _pDevEnv->ShowMessageBox(hWnd, CString(MAKEINTRESOURCE(IDS_ERR_WINXP)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONERROR);
}

static void SetShellManagerDefaults(CShellManager& sm, CString sServerType)
{
   if( sServerType.Find(_T("LynxOS")) >= 0 ) sm.SetParam(_T("LoginPrompt"), _T("USER"));
}

static void SetCompileManagerDefaults(CCompileManager& cm, CString sServerType, int iShellType)
{
   if( sServerType.Find(_T("UNIX")) >= 0 ) cm.SetParam(_T("ProcessList"), _T("ps -ef"));
   if( sServerType.Find(_T("LINUX")) >= 0 ) cm.SetParam(_T("ProcessList"), _T("ps -ef"));
   // In Cygwin mode, we're dealing with Windows syntax
   if( iShellType == SHELLTRANSFER_COMSPEC ) {
      cm.SetParam(_T("ChangeDir"), _T("$DRIVE$&cd $PATH$"));
      cm.SetParam(_T("DebugExport"), _T("SET DEBUG_OPTIONS=-g -D_DEBUG"));
      cm.SetParam(_T("ReleaseExport"), _T("SET DEBUG_OPTIONS="));
   }
}

static void SetDebugManagerDefaults(CDebugManager& dm, CString sServerType, CString sDebuggerType, int iShellType)
{
   if( iShellType == SHELLTRANSFER_COMSPEC ) {
      dm.SetParam(_T("ChangeDir"), _T("$DRIVE$&cd $PATH$"));   
      dm.SetParam(_T("App"), _T("$PROJECTNAME$.exe"));
   }
   else {
      dm.SetParam(_T("ChangeDir"), _T("cd $PATH$"));
      dm.SetParam(_T("App"), _T("./$PROJECTNAME$"));
   }
   if( sDebuggerType == _T("gdb") ) {
      dm.SetParam(_T("Debugger"), _T("gdb"));
      dm.SetParam(_T("AttachExe"), _T("-i=mi ./$PROJECTNAME$"));
      dm.SetParam(_T("AttachCore"), _T("-i=mi $PROCESS$ -c $COREFILE$"));
      dm.SetParam(_T("AttachPid"), _T("-i=mi -pid $PID$"));
   }
   if( sDebuggerType == _T("dbx") ) {
      dm.SetParam(_T("Debugger"), _T("dbx"));
      dm.SetParam(_T("AttachExe"), _T("./$PROJECTNAME$"));
      dm.SetParam(_T("AttachCore"), _T("- $COREFILE$"));
      dm.SetParam(_T("AttachPid"), _T("- $PID$"));
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

   m_ctrlHost.LimitText(128);
   m_ctrlUsername.LimitText(128);
   m_ctrlPassword.LimitText(30);
   m_ctrlPath.LimitText(MAX_PATH);

   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_FTP)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_SFTP)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_NETWORKDRIVE)));

   CString sType = m_pProject->m_FileManager.GetParam(_T("Type"));
   int iTypeIndex = FILETRANSFER_FTP;
   if( sType == _T("sftp") )    iTypeIndex = FILETRANSFER_SFTP;
   if( sType == _T("network") ) iTypeIndex = FILETRANSFER_NETWORK;
   m_ctrlType.SetCurSel(iTypeIndex);

   g_data.bIsWizard = false;
   g_data.iFileTransferType = iTypeIndex;
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

   BOOL bDummy = FALSE;
   OnTypeChange(0, 0, NULL, bDummy);
   OnTextChange(0, 0, NULL, bDummy);

   return 0;
}

LRESULT CFileTransferPage::OnTypeChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iTypeIndex = m_ctrlType.GetCurSel();
   long lPort = 21;
   switch( iTypeIndex ) {
   case FILETRANSFER_SFTP: lPort = 22; break;
   }
   if( wID != 0 ) m_ctrlPort.SetWindowText(ToString(lPort));
   m_ctrlHost.EnableWindow(iTypeIndex != FILETRANSFER_NETWORK);
   m_ctrlPort.EnableWindow(iTypeIndex != FILETRANSFER_NETWORK);
   m_ctrlUsername.EnableWindow(iTypeIndex != FILETRANSFER_NETWORK);
   m_ctrlPassword.EnableWindow(iTypeIndex != FILETRANSFER_NETWORK);
   g_data.iFileTransferType = iTypeIndex;
   return 0;
}

LRESULT CFileTransferPage::OnTextChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   BOOL bOK = TRUE;
   int iTypeIndex = m_ctrlType.GetCurSel();
   if( m_ctrlHost.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( m_ctrlPort.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( m_ctrlUsername.GetWindowTextLength() == 0 && m_ctrlPassword.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( iTypeIndex == FILETRANSFER_NETWORK ) bOK = TRUE;  // Network drive don't need host/username/password
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

   int iTypeIndex = m_ctrlType.GetCurSel();
   CString sHost = CWindowText(m_ctrlHost);
   CString sUsername = CWindowText(m_ctrlUsername);
   CString sPassword = CWindowText(m_ctrlPassword);
   CString sPath = CWindowText(m_ctrlPath);
   long lPort = _ttol(CWindowText(m_ctrlPort));

   CString sType = _T("ftp");
   switch( iTypeIndex ) {
   case FILETRANSFER_SFTP:    sType = _T("sftp"); break;
   case FILETRANSFER_NETWORK: sType = _T("network"); break;
   }

   CFileManager fm;
   fm.SetParam(_T("Type"), sType);
   fm.SetParam(_T("Host"), sHost);
   fm.SetParam(_T("Username"), sUsername);
   fm.SetParam(_T("Password"), sPassword);
   fm.SetParam(_T("Port"), ToString(lPort));
   fm.SetParam(_T("Path"), sPath);
   fm.SetParam(_T("Proxy"), m_pProject->m_FileManager.GetParam(_T("Proxy")));
   fm.SetParam(_T("Passive"), m_pProject->m_FileManager.GetParam(_T("Passive")));
   fm.SetParam(_T("ConnectTimeout"), m_pProject->m_FileManager.GetParam(_T("ConnectTimeout")));
   // The actual test of how we got connected, is to try to set the current path.
   // The file protocol should set a meaningful error code.
   if( !fm.Start() || !fm.WaitForConnection() || !fm.SetCurPath(sPath) ) {
      DWORD dwErr = ::GetLastError();
      fm.SignalStop();
      GenerateError(_pDevEnv, m_hWnd, IDS_ERR_NOCONNECTION, dwErr);
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
   BOOL bDummy = FALSE;
   OnTextChange(0, 0, NULL, bDummy);
   return 0;
}

int CFileTransferPage::OnWizardNext()
{
   CString sPath = CWindowText(m_ctrlPath);

   // Warn when using a project path that too short; don't have good results
   // when using root or no path...
   if( sPath == _T("/") || sPath.IsEmpty() ) 
   {
      if( IDNO == _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_ERR_RELPATH)), CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONWARNING | MB_YESNO) ) return -1;
   }

   // Store the values on the first page for use in the remaining pages.
   // Notice how this is done in the WizardNext event - which is not called
   // when we're showing the Property Pages, but only during the Wizard.
   g_data.bIsWizard = true;
   g_data.iFileTransferType = m_ctrlType.GetCurSel();
   g_data.sHost = CWindowText(m_ctrlHost);
   g_data.sUsername = CWindowText(m_ctrlUsername);
   g_data.sPassword = CWindowText(m_ctrlPassword);
   g_data.sPath = CWindowText(m_ctrlPath);
   return 0;
}

int CFileTransferPage::OnApply()
{
   ATLASSERT(m_pProject);

   int iTypeIndex = m_ctrlType.GetCurSel();

   CString sHost = CWindowText(m_ctrlHost);
   CString sUsername = CWindowText(m_ctrlUsername);
   CString sPassword = CWindowText(m_ctrlPassword);
   CString sPath = CWindowText(m_ctrlPath);
   long lPort = _ttol(CWindowText(m_ctrlPort));

   CString sType = _T("ftp");
   switch( iTypeIndex ) {
   case FILETRANSFER_SFTP:     sType = _T("sftp"); break;
   case FILETRANSFER_NETWORK:  sType = _T("network"); break;
   }

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
   int cxChar = (int) LOWORD(GetDialogBaseUnits());
   m_ctrlList.SetColumnWidth(16 * cxChar);
   
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

   sName.LoadString(IDS_PATHS);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_COMMAND_SEARCHPATH);
   sValue = m_pProject->m_FileManager.GetParam(_T("SearchPath"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 10));

   sName.LoadString(IDS_MISC);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_MISC_CONNECTTIMEOUT);
   sValue = m_pProject->m_FileManager.GetParam(_T("ConnectTimeout"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 20));
   sName.LoadString(IDS_MISC_COMPATIBILITY);
   bValue = m_pProject->m_FileManager.GetParam(_T("CompatibilityMode")) == _T("true");
   m_ctrlList.AddItem(PropCreateSimple(sName, bValue, 21));

   return 0;
}

int CFileOptionsPage::OnSetActive()
{   
   m_ctrlList.SetItemEnabled(m_ctrlList.FindProperty(1), g_data.iFileTransferType == FILETRANSFER_FTP);
   m_ctrlList.SetItemEnabled(m_ctrlList.FindProperty(2), g_data.iFileTransferType == FILETRANSFER_FTP);
   m_ctrlList.SetItemEnabled(m_ctrlList.FindProperty(20), g_data.iFileTransferType != FILETRANSFER_NETWORK);
   return 0;
}

int CFileOptionsPage::OnApply()
{
   ATLASSERT(m_pProject);

   CComVariant v;
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(1), &v);
   m_pProject->m_FileManager.SetParam(_T("Passive"), v.lVal != 0 ? _T("true") : _T("false"));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(2), &v);
   m_pProject->m_FileManager.SetParam(_T("Proxy"), CString(v.bstrVal));

   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(10), &v);
   m_pProject->m_FileManager.SetParam(_T("SearchPath"), CString(v.bstrVal));

   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(20), &v);
   m_pProject->m_FileManager.SetParam(_T("ConnectTimeout"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(21), &v);
   m_pProject->m_FileManager.SetParam(_T("CompatibilityMode"), v.lVal != 0 ? _T("true") : _T("false"));

   return PSNRET_NOERROR;
}


///////////////////////////////////////////////////////////////
//

LRESULT CCompilerPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlServer = GetDlgItem(IDC_SERVER);
   m_ctrlType = GetDlgItem(IDC_TYPE);
   m_ctrlTest = GetDlgItem(IDC_TEST);
   m_ctrlHost = GetDlgItem(IDC_HOST);
   m_ctrlHostLabel = GetDlgItem(IDC_HOST_LABEL);
   m_ctrlPort = GetDlgItem(IDC_PORT);
   m_ctrlUsername = GetDlgItem(IDC_USERNAME);
   m_ctrlPassword = GetDlgItem(IDC_PASSWORD);
   m_ctrlPath = GetDlgItem(IDC_PATH);
   m_ctrlExtra = GetDlgItem(IDC_EXTRA);

   m_ctrlHost.LimitText(128);
   m_ctrlUsername.LimitText(128);
   m_ctrlPassword.LimitText(30);
   m_ctrlPath.LimitText(MAX_PATH);
   m_ctrlExtra.LimitText(1400);

   m_ctrlServer.AddString(_T("Generic"));
   m_ctrlServer.AddString(_T("LINUX"));
   m_ctrlServer.AddString(_T("LINUX (Ubuntu)"));
   m_ctrlServer.AddString(_T("LINUX (Gentoo)"));
   m_ctrlServer.AddString(_T("LINUX (Debian)"));
   m_ctrlServer.AddString(_T("LINUX (CentOS)"));
   m_ctrlServer.AddString(_T("LINUX (Red Hat)"));
   m_ctrlServer.AddString(_T("UNIX"));
   m_ctrlServer.AddString(_T("UNIX (AIX)"));
   m_ctrlServer.AddString(_T("UNIX (HP-UX)"));
   m_ctrlServer.AddString(_T("BSD (FreeBSD)"));
   m_ctrlServer.AddString(_T("BSD (OpenBSD)"));
   m_ctrlServer.AddString(_T("Soliaris"));
   m_ctrlServer.AddString(_T("Cygwin"));
   m_ctrlServer.AddString(_T("Windows"));
   m_ctrlServer.AddString(_T("MacOS"));
   m_ctrlServer.AddString(_T("LynxOS"));

   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_TELNET)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_RLOGIN)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_SSH)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_COMSPEC)));

   m_ctrlHost.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("Host")));
   m_ctrlUsername.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("Username")));
   m_ctrlPassword.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("Password")));
   m_ctrlPath.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("Path")));
   m_ctrlPort.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("Port")));
   m_ctrlExtra.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("Extra")));

   CString sShellType = m_pProject->m_CompileManager.GetParam(_T("Type"));
   CString sServerType = m_pProject->m_CompileManager.GetParam(_T("ServerType"));

   // If this is the first time we're visiting the page, fill
   // out some defaults...
   if( g_data.bIsWizard ) 
   {
      m_ctrlHost.SetWindowText(g_data.sHost);
      m_ctrlPort.SetWindowText(_T("23"));
      m_ctrlUsername.SetWindowText(g_data.sUsername);
      m_ctrlPassword.SetWindowText(g_data.sPassword);
      m_ctrlPath.SetWindowText(g_data.sPath);
      if( g_data.iFileTransferType == FILETRANSFER_SFTP ) {
         sShellType = _T("ssh");
         m_ctrlPort.SetWindowText(_T("22"));
      }
   }

   // Choose generic server
   m_ctrlServer.SelectString(0, sServerType);
   if( m_ctrlServer.GetCurSel() < 0 ) m_ctrlServer.SetCurSel(0);

   int iTypeIndex = SHELLTRANSFER_TELNET;
   if( sShellType == _T("ssh") )     iTypeIndex = SHELLTRANSFER_SSH;
   if( sShellType == _T("rlogin") )  iTypeIndex = SHELLTRANSFER_RLOGIN;
   if( sShellType == _T("comspec") ) iTypeIndex = SHELLTRANSFER_COMSPEC;
   m_ctrlType.SetCurSel(iTypeIndex);

   g_data.iShellType = iTypeIndex;

   BOOL bDummy = FALSE;
   OnTypeChange(0, 0, NULL, bDummy);
   OnTextChange(0, 0, NULL, bDummy);

   return 0;
}

LRESULT CCompilerPage::OnTest(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CWaitCursor cursor;
   m_ctrlTest.EnableWindow(FALSE);

   int iTypeIndex = m_ctrlType.GetCurSel();

   SecClearPassword();

   CString sHost = CWindowText(m_ctrlHost);
   CString sUsername = CWindowText(m_ctrlUsername);
   CString sPassword = CWindowText(m_ctrlPassword);
   CString sPath = CWindowText(m_ctrlPath);
   long lPort = _ttol(CWindowText(m_ctrlPort));
   CString sExtra = CWindowText(m_ctrlExtra);
   CString sServerType = CWindowText(m_ctrlServer);

   CString sShellType = _T("telnet");
   switch( iTypeIndex ) {
   case SHELLTRANSFER_SSH:     sShellType = _T("ssh"); break;
   case SHELLTRANSFER_RLOGIN:  sShellType = _T("rlogin"); break;
   case SHELLTRANSFER_COMSPEC: sShellType = _T("comspec"); break;
   }

   if( iTypeIndex == SHELLTRANSFER_SSH ) CheckCryptLib(m_hWnd);

   CShellManager sm;
   sm.Init(m_pProject);
   sm.SetParam(_T("Type"), sShellType);
   sm.SetParam(_T("Host"), sHost);
   sm.SetParam(_T("Username"), sUsername);
   sm.SetParam(_T("Password"), sPassword);
   sm.SetParam(_T("Port"), ToString(lPort));
   sm.SetParam(_T("Path"), sPath);
   sm.SetParam(_T("Extra"), sExtra);
   sm.SetParam(_T("LoginPrompt"), m_pProject->m_CompileManager.GetParam(_T("LoginPrompt")));
   sm.SetParam(_T("PasswordPrompt"), m_pProject->m_CompileManager.GetParam(_T("PasswordPrompt")));
   sm.SetParam(_T("ConnectTimeout"), m_pProject->m_CompileManager.GetParam(_T("ConnectTimeout")));
   if( g_data.bIsWizard ) SetShellManagerDefaults(sm, sServerType);
   if( !sm.Start() || !sm.WaitForConnection() ) {
      DWORD dwErr = ::GetLastError();
      sm.SignalStop();
      GenerateError(_pDevEnv, m_hWnd, IDS_ERR_NOCONNECTION, dwErr);
   }
   else {
      sm.SignalStop();
      CString sCaption(MAKEINTRESOURCE(IDS_CAPTION_MESSAGE));
      CString sMsg(MAKEINTRESOURCE(IDS_CONNECTION_OK));
      _pDevEnv->ShowMessageBox(m_hWnd, sMsg, sCaption, MB_ICONINFORMATION);
   }
   sm.Stop();

   // Did we also detect server OS type?
   if( g_data.bIsWizard ) 
   {
      sServerType = sm.GetParam(_T("ServerType"));
      m_ctrlServer.SelectString(0, sServerType);
   }

   m_ctrlTest.EnableWindow(TRUE);
   return 0;
}

LRESULT CCompilerPage::OnTypeChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iTypeIndex = m_ctrlType.GetCurSel();
   long lPort = 23;
   switch( iTypeIndex ) {
   case SHELLTRANSFER_SSH:     lPort = 22; break;
   case SHELLTRANSFER_RLOGIN:  lPort = 513; break;
   }
   if( wID != 0 ) m_ctrlPort.SetWindowText(ToString(lPort));
   if( wID != 0 && iTypeIndex == SHELLTRANSFER_COMSPEC ) m_ctrlServer.SetCurSel(m_ctrlServer.FindString(0, _T("Cygwin")));
   m_ctrlPort.EnableWindow(iTypeIndex != SHELLTRANSFER_COMSPEC);
   m_ctrlUsername.EnableWindow(iTypeIndex != SHELLTRANSFER_COMSPEC);
   m_ctrlPassword.EnableWindow(iTypeIndex != SHELLTRANSFER_COMSPEC);
   UINT uLabel = IDS_HOST_LABEL;
   if( iTypeIndex == SHELLTRANSFER_COMSPEC ) uLabel = IDS_MINGWBIN_LABEL;
   m_ctrlHostLabel.SetWindowText(CString(MAKEINTRESOURCE(uLabel)));
   g_data.iShellType = iTypeIndex;
   return 0;
}

LRESULT CCompilerPage::OnTextChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   BOOL bOK = TRUE;
   int iTypeIndex = m_ctrlType.GetCurSel();
   if( m_ctrlHost.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( m_ctrlPort.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( m_ctrlUsername.GetWindowTextLength() == 0 && m_ctrlPassword.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( iTypeIndex == SHELLTRANSFER_COMSPEC ) bOK = TRUE;  // Batchfile don't need username/password/host
   if( m_ctrlPath.GetWindowTextLength() == 0 ) bOK = FALSE;
   SetWizardButtons(bOK ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK);
   if( m_ctrlHost.GetWindowTextLength() == 0 ) bOK = FALSE;
   CWindow(GetDlgItem(IDC_TEST)).EnableWindow(bOK);
   return 0;
}

int CCompilerPage::OnWizardNext()
{
   int iTypeIndex = m_ctrlType.GetCurSel();
   CString sHost =  CWindowText(m_ctrlHost);
   CString sPath = CWindowText(m_ctrlPath);
   CString sExtra =  CWindowText(m_ctrlExtra);
   CString sServerType = CWindowText(m_ctrlServer);
  
   // Warn when not specifying a correct path to the /bin folder when
   // the MinGW compilation environment has been chosen.
   if( !sHost.IsEmpty()
       && iTypeIndex == SHELLTRANSFER_COMSPEC )
   {
      TCHAR szFilename[MAX_PATH];
      _tcscpy(szFilename, sHost);
      ::PathAppend(szFilename, _T("gdb.exe"));
      if( !::PathFileExists(szFilename) ) {
         if( IDNO == _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_ERR_MINGWPATH)), CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONWARNING | MB_YESNO) ) return -1;
      }
   }

   // Warn when no path to MinGW /bin folder is specified. This is a valid
   // configuration but user needs to know that the system must be setup correctly.
   if( sHost.IsEmpty()
       && iTypeIndex == SHELLTRANSFER_COMSPEC )
   {
      if( IDNO == _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_ERR_MINGWEMPTY)), CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONINFORMATION | MB_YESNO) ) return -1;
   }

   // Warn when using a project path that is not relative. It should be a
   // complete path to the remote folder.
   if( !sPath.IsEmpty() 
       && sPath.Left(1) != _T("/") 
       && sPath.FindOneOf(_T("\\:")) < 0 ) 
   {
      if( IDNO == _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_ERR_RELPATH)), CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONWARNING | MB_YESNO) ) return -1;
   }

   // Store the values on the compiler page for use in the remaining pages.
   g_data.iShellType = iTypeIndex;
   g_data.sHost = sHost;
   g_data.sExtra = sExtra;
   g_data.sServerType = sServerType;

   return 0;
}

int CCompilerPage::OnSetActive()
{
   BOOL bDummy = FALSE;
   OnTextChange(0, 0, NULL, bDummy);
   return 0;
}

int CCompilerPage::OnApply()
{
   ATLASSERT(m_pProject);

   int iTypeIndex = m_ctrlType.GetCurSel();

   CString sServerType = CWindowText(m_ctrlServer);
   CString sHost = CWindowText(m_ctrlHost);
   CString sUsername = CWindowText(m_ctrlUsername);
   CString sPassword = CWindowText(m_ctrlPassword);
   CString sPath = CWindowText(m_ctrlPath);
   long lPort = _ttol(CWindowText(m_ctrlPort));
   CString sExtra = CWindowText(m_ctrlExtra);

   CString sShellType = _T("telnet");
   switch( iTypeIndex ) {
   case SHELLTRANSFER_SSH:     sShellType = _T("ssh"); break;
   case SHELLTRANSFER_RLOGIN:  sShellType = _T("rlogin"); break;
   case SHELLTRANSFER_COMSPEC: sShellType = _T("comspec"); break;
   }

   m_pProject->m_CompileManager.Stop();
   m_pProject->m_CompileManager.SetParam(_T("Type"), sShellType);
   m_pProject->m_CompileManager.SetParam(_T("ServerType"), sServerType);
   m_pProject->m_CompileManager.SetParam(_T("Host"), sHost);
   m_pProject->m_CompileManager.SetParam(_T("Username"), sUsername);
   m_pProject->m_CompileManager.SetParam(_T("Password"), sPassword);
   m_pProject->m_CompileManager.SetParam(_T("Port"), ToString(lPort));
   m_pProject->m_CompileManager.SetParam(_T("Path"), sPath);
   m_pProject->m_CompileManager.SetParam(_T("Extra"), sExtra);
   // Adjust some commands based on the server type.
   // This get around some of the quirks on some gnu util implementations.
   if( g_data.bIsWizard ) {
      SetShellManagerDefaults(m_pProject->m_CompileManager.m_ShellManager, sServerType);
      SetCompileManagerDefaults(m_pProject->m_CompileManager, sServerType, iTypeIndex);
   }
   m_pProject->m_CompileManager.Start();

   // Finally some file checking...
   if( g_data.bIsWizard ) {
      if( iTypeIndex == SHELLTRANSFER_SSH ) CheckCryptLib(m_hWnd);     
   }

   return PSNRET_NOERROR;
}


///////////////////////////////////////////////////////////////
//

LRESULT CCompilerCommandsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlList.SubclassWindow(GetDlgItem(IDC_LIST));
   m_ctrlList.SetExtendedListStyle(PLS_EX_XPLOOK | PLS_EX_CATEGORIZED);
   int cxChar = (int) LOWORD(GetDialogBaseUnits());
   m_ctrlList.SetColumnWidth(16 * cxChar);
   
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
   sName.LoadString(IDS_COMMAND_CHECKSYNTAX);
   sValue = m_pProject->m_CompileManager.GetParam(_T("CheckSyntax"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 5));
   sName.LoadString(IDS_COMMAND_CLEAN);
   sValue = m_pProject->m_CompileManager.GetParam(_T("Clean"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 6));
   sName.LoadString(IDS_MISC_PROCESSLIST);
   sValue = m_pProject->m_CompileManager.GetParam(_T("ProcessList"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 7));

   sName.LoadString(IDS_DIRECTIVES);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_COMMAND_DEBUGEXPORT);
   sValue = m_pProject->m_CompileManager.GetParam(_T("DebugExport"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 10));
   sName.LoadString(IDS_COMMAND_RELEASEEXPORT);
   sValue = m_pProject->m_CompileManager.GetParam(_T("ReleaseExport"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 11));

   sName.LoadString(IDS_FLAGS);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_COMMAND_BUILDCTAGS);
   sValue = m_pProject->m_CompileManager.GetParam(_T("BuildTags"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 20));
   sName.LoadString(IDS_COMMAND_COMPILEFLAGS);
   sValue = m_pProject->m_CompileManager.GetParam(_T("CompileFlags"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 21));
   sName.LoadString(IDS_COMMAND_LINKFLAGS);
   sValue = m_pProject->m_CompileManager.GetParam(_T("LinkFlags"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 22));

   sName.LoadString(IDS_MISC);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_MISC_CONNECTTIMEOUT);
   sValue = m_pProject->m_CompileManager.GetParam(_T("ConnectTimeout"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 30));

   return 0;
}

int CCompilerCommandsPage::OnSetActive()
{   
   m_ctrlList.SetItemEnabled(m_ctrlList.FindProperty(30), g_data.iShellType != SHELLTRANSFER_COMSPEC);
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
   m_pProject->m_CompileManager.SetParam(_T("CheckSyntax"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(6), &v);
   m_pProject->m_CompileManager.SetParam(_T("Clean"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(7), &v);
   m_pProject->m_CompileManager.SetParam(_T("ProcessList"), CString(v.bstrVal));

   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(10), &v);
   m_pProject->m_CompileManager.SetParam(_T("DebugExport"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(11), &v);
   m_pProject->m_CompileManager.SetParam(_T("ReleaseExport"), CString(v.bstrVal));

   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(20), &v);
   m_pProject->m_CompileManager.SetParam(_T("BuildTags"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(21), &v);
   m_pProject->m_CompileManager.SetParam(_T("CompileFlags"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(22), &v);
   m_pProject->m_CompileManager.SetParam(_T("LinkFlags"), CString(v.bstrVal));

   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(30), &v);
   m_pProject->m_CompileManager.SetParam(_T("ConnectTimeout"), CString(v.bstrVal));

   return PSNRET_NOERROR;
}


///////////////////////////////////////////////////////////////
//

LRESULT CCompilerStepsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlPreStep = GetDlgItem(IDC_PRESTEP);
   m_ctrlPostStep = GetDlgItem(IDC_POSTSTEP);
   m_ctrlPreStep.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("PreStep")));
   m_ctrlPostStep.SetWindowText(m_pProject->m_CompileManager.GetParam(_T("PostStep")));
   return 0;
}

int CCompilerStepsPage::OnApply()
{
   ATLASSERT(m_pProject);
   m_pProject->m_CompileManager.SetParam(_T("PreStep"), CWindowText(m_ctrlPreStep));
   m_pProject->m_CompileManager.SetParam(_T("PostStep"), CWindowText(m_ctrlPostStep));
   return PSNRET_NOERROR;
}


///////////////////////////////////////////////////////////////
//

LRESULT CDebuggerPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlTest = GetDlgItem(IDC_TEST);

   m_ctrlDebugger = GetDlgItem(IDC_DEBUGGER);

   m_ctrlType = GetDlgItem(IDC_TYPE);
   m_ctrlHost = GetDlgItem(IDC_HOST);
   m_ctrlHostLabel = GetDlgItem(IDC_HOST_LABEL);
   m_ctrlPort = GetDlgItem(IDC_PORT);
   m_ctrlUsername = GetDlgItem(IDC_USERNAME);
   m_ctrlPassword = GetDlgItem(IDC_PASSWORD);
   m_ctrlPath = GetDlgItem(IDC_PATH);
   m_ctrlExtra = GetDlgItem(IDC_EXTRA);

   m_ctrlHost.LimitText(128);
   m_ctrlUsername.LimitText(128);
   m_ctrlPassword.LimitText(30);
   m_ctrlPath.LimitText(MAX_PATH);
   m_ctrlExtra.LimitText(1400);

   m_ctrlDebugger.AddString(CString(MAKEINTRESOURCE(IDS_GDB)));
   m_ctrlDebugger.AddString(CString(MAKEINTRESOURCE(IDS_DBX)));
   m_ctrlDebugger.SetCurSel(0);

   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_TELNET)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_RLOGIN)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_SSH)));
   m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_COMSPEC)));

   m_ctrlHost.SetWindowText(m_pProject->m_DebugManager.GetParam(_T("Host")));
   m_ctrlUsername.SetWindowText(m_pProject->m_DebugManager.GetParam(_T("Username")));
   m_ctrlPassword.SetWindowText(m_pProject->m_DebugManager.GetParam(_T("Password")));
   m_ctrlPath.SetWindowText(m_pProject->m_DebugManager.GetParam(_T("Path")));
   m_ctrlPort.SetWindowText(m_pProject->m_DebugManager.GetParam(_T("Port")));
   m_ctrlExtra.SetWindowText(m_pProject->m_DebugManager.GetParam(_T("Extra")));

   CString sShellType = m_pProject->m_DebugManager.GetParam(_T("Type"));
   CString sDebuggerType = m_pProject->m_DebugManager.GetParam(_T("DebuggerType"));

   // If this is the first time we're visiting the page, fill
   // out some defaults...
   if( g_data.bIsWizard ) 
   {
      m_ctrlHost.SetWindowText(g_data.sHost);
      m_ctrlPort.SetWindowText(_T("23"));
      m_ctrlUsername.SetWindowText(g_data.sUsername);
      m_ctrlPassword.SetWindowText(g_data.sPassword);
      m_ctrlPath.SetWindowText(g_data.sPath);
      m_ctrlExtra.SetWindowText(g_data.sExtra);
      if( g_data.iFileTransferType == FILETRANSFER_SFTP || g_data.iShellType == SHELLTRANSFER_SSH ) {
         sShellType = _T("ssh");
         m_ctrlPort.SetWindowText(_T("22"));
      }
      if( g_data.iShellType == SHELLTRANSFER_COMSPEC ) {
         sShellType = _T("comspec");
      }
      if( g_data.sServerType == _T("Solaris") ) {
         sDebuggerType = _T("dbx");
      }
   }

   int iTypeIndex = SHELLTRANSFER_TELNET;
   if( sShellType == _T("ssh") )     iTypeIndex = SHELLTRANSFER_SSH;
   if( sShellType == _T("rlogin") )  iTypeIndex = SHELLTRANSFER_RLOGIN;
   if( sShellType == _T("comspec") ) iTypeIndex = SHELLTRANSFER_COMSPEC;
   m_ctrlType.SetCurSel(iTypeIndex);

   int iDebuggerIndex = DEBUGGERTYPE_GDB;
   if( sDebuggerType == _T("dbx") ) iDebuggerIndex = DEBUGGERTYPE_DBX;
   m_ctrlDebugger.SetCurSel(iDebuggerIndex);

   BOOL bDummy = FALSE;
   OnTypeChange(0, 0, NULL, bDummy);
   OnTextChange(0, 0, NULL, bDummy);

   return 0;
}

LRESULT CDebuggerPage::OnTypeChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iTypeIndex = m_ctrlType.GetCurSel();
   long lPort = 23;
   switch( iTypeIndex ) {
   case SHELLTRANSFER_SSH:    lPort = 22; break;
   case SHELLTRANSFER_RLOGIN: lPort = 513; break;
   }
   if( wID != 0 ) m_ctrlPort.SetWindowText(ToString(lPort));
   m_ctrlPort.EnableWindow(iTypeIndex != SHELLTRANSFER_COMSPEC);
   m_ctrlUsername.EnableWindow(iTypeIndex != SHELLTRANSFER_COMSPEC);
   m_ctrlPassword.EnableWindow(iTypeIndex != SHELLTRANSFER_COMSPEC);
   UINT uLabel = IDS_HOST_LABEL;
   if( iTypeIndex == SHELLTRANSFER_COMSPEC ) uLabel = IDS_MINGWBIN_LABEL;
   m_ctrlHostLabel.SetWindowText(CString(MAKEINTRESOURCE(uLabel)));
   return 0;
}

LRESULT CDebuggerPage::OnTextChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   BOOL bOK = TRUE;
   int iTypeIndex = m_ctrlType.GetCurSel();
   if( m_ctrlHost.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( m_ctrlPort.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( m_ctrlUsername.GetWindowTextLength() == 0 && m_ctrlPassword.GetWindowTextLength() == 0 ) bOK = FALSE;
   if( iTypeIndex == SHELLTRANSFER_COMSPEC ) bOK = TRUE;  // Batchfile don't need username/password/host
   if( m_ctrlPath.GetWindowTextLength() == 0 ) bOK = FALSE;
   SetWizardButtons(bOK ? PSWIZB_BACK | PSWIZB_NEXT : PSWIZB_BACK);
   if( m_ctrlHost.GetWindowTextLength() == 0 ) bOK = FALSE;
   CWindow(GetDlgItem(IDC_TEST)).EnableWindow(bOK);
   return 0;
}

LRESULT CDebuggerPage::OnTest(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CWaitCursor cursor;
   m_ctrlTest.EnableWindow(FALSE);

   int iTypeIndex = m_ctrlType.GetCurSel();
   int iDebuggerIndex = m_ctrlDebugger.GetCurSel();

   SecClearPassword();

   CString sHost = CWindowText(m_ctrlHost);
   CString sUsername = CWindowText(m_ctrlUsername);
   CString sPassword = CWindowText(m_ctrlPassword);
   CString sPath = CWindowText(m_ctrlPath);
   long lPort = _ttol(CWindowText(m_ctrlPort));
   CString sExtra = CWindowText(m_ctrlExtra);

   CString sShellType = _T("telnet");
   switch( iTypeIndex ) {
   case SHELLTRANSFER_SSH:     sShellType = _T("ssh"); break;
   case SHELLTRANSFER_RLOGIN:  sShellType = _T("rlogin"); break;
   case SHELLTRANSFER_COMSPEC: sShellType = _T("comspec"); break;
   }

   CString sDebuggerType = _T("gdb");
   switch( iDebuggerIndex ) {
   case DEBUGGERTYPE_DBX:      sDebuggerType = _T("dbx"); break;
   }

   if( iTypeIndex == SHELLTRANSFER_SSH ) CheckCryptLib(m_hWnd);

   CShellManager sm;
   sm.Init(m_pProject);
   sm.SetParam(_T("Type"), sShellType);
   sm.SetParam(_T("DebuggerType"), sDebuggerType);
   sm.SetParam(_T("Host"), sHost);
   sm.SetParam(_T("Username"), sUsername);
   sm.SetParam(_T("Password"), sPassword);
   sm.SetParam(_T("Port"), ToString(lPort));
   sm.SetParam(_T("Path"), sPath);
   sm.SetParam(_T("Extra"), sExtra);
   sm.SetParam(_T("LoginPrompt"), m_pProject->m_DebugManager.GetParam(_T("LoginPrompt")));
   sm.SetParam(_T("PasswordPrompt"), m_pProject->m_DebugManager.GetParam(_T("PasswordPrompt")));
   sm.SetParam(_T("StartTimeout"), m_pProject->m_DebugManager.GetParam(_T("StartTimeout")));
   sm.SetParam(_T("ConnectTimeout"), m_pProject->m_DebugManager.GetParam(_T("ConnectTimeout")));
   if( g_data.bIsWizard ) SetShellManagerDefaults(sm, g_data.sServerType);
   if( !sm.Start() || !sm.WaitForConnection() ) {
      DWORD dwErr = ::GetLastError();
      sm.SignalStop();
      GenerateError(_pDevEnv, m_hWnd, IDS_ERR_NOCONNECTION, dwErr);
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
   BOOL bDummy = FALSE;
   OnTextChange(0, 0, NULL, bDummy);
   return 0;
}

int CDebuggerPage::OnApply()
{
   ATLASSERT(m_pProject);

   int iTypeIndex = m_ctrlType.GetCurSel();
   int iDebuggerIndex = m_ctrlDebugger.GetCurSel();

   CString sHost = CWindowText(m_ctrlHost);
   CString sUsername = CWindowText(m_ctrlUsername);
   CString sPassword = CWindowText(m_ctrlPassword);
   CString sPath = CWindowText(m_ctrlPath);
   long lPort = _ttol(CWindowText(m_ctrlPort));
   CString sExtra = CWindowText(m_ctrlExtra);

   CString sShellType = _T("telnet");
   switch( iTypeIndex ) {
   case SHELLTRANSFER_SSH:     sShellType = _T("ssh"); break;
   case SHELLTRANSFER_RLOGIN:  sShellType = _T("rlogin"); break;
   case SHELLTRANSFER_COMSPEC: sShellType = _T("comspec"); break;
   }

   CString sDebuggerType = _T("gdb");
   switch( iDebuggerIndex ) {
   case DEBUGGERTYPE_DBX:      sDebuggerType = _T("dbx"); break;
   }

   m_pProject->m_DebugManager.Stop();
   m_pProject->m_DebugManager.SetParam(_T("Type"), sShellType);
   m_pProject->m_DebugManager.SetParam(_T("DebuggerType"), sDebuggerType);
   m_pProject->m_DebugManager.SetParam(_T("Host"), sHost);
   m_pProject->m_DebugManager.SetParam(_T("Username"), sUsername);
   m_pProject->m_DebugManager.SetParam(_T("Password"), sPassword);
   m_pProject->m_DebugManager.SetParam(_T("Port"), ToString(lPort));
   m_pProject->m_DebugManager.SetParam(_T("Path"), sPath);
   m_pProject->m_DebugManager.SetParam(_T("Extra"), sExtra);

   // Adjust some known problems...
   if( g_data.bIsWizard )
   {
      SetShellManagerDefaults(m_pProject->m_DebugManager.m_ShellManager, g_data.sServerType);
      SetDebugManagerDefaults(m_pProject->m_DebugManager, g_data.sServerType, sDebuggerType, iTypeIndex);

      if( iTypeIndex == SHELLTRANSFER_SSH ) CheckCryptLib(m_hWnd);
      if( iTypeIndex == SHELLTRANSFER_COMSPEC ) CheckDebugPriv(m_hWnd);
   }

   return PSNRET_NOERROR;
}


///////////////////////////////////////////////////////////////
//

LRESULT CDebuggerCommandsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlList.SubclassWindow(GetDlgItem(IDC_LIST));
   m_ctrlList.SetExtendedListStyle(PLS_EX_XPLOOK|PLS_EX_CATEGORIZED);
   int cxChar = (int) LOWORD(GetDialogBaseUnits());
   m_ctrlList.SetColumnWidth(16 * cxChar);
   
   CString sName;
   CString sValue;
   bool bValue;

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
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 10));
   sName.LoadString(IDS_COMMAND_ARGS);
   sValue = m_pProject->m_DebugManager.GetParam(_T("AttachExe"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 11));
   sName.LoadString(IDS_COMMAND_CORE);
   sValue = m_pProject->m_DebugManager.GetParam(_T("AttachCore"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 12));
   sName.LoadString(IDS_COMMAND_PID);
   sValue = m_pProject->m_DebugManager.GetParam(_T("AttachPid"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 13));
   sName.LoadString(IDS_COMMAND_MAIN);
   sValue = m_pProject->m_DebugManager.GetParam(_T("DebugMain"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 14));

   sName.LoadString(IDS_MISC);
   m_ctrlList.AddItem(PropCreateCategory(sName));
   sName.LoadString(IDS_MISC_STARTTIMEOUT);
   sValue = m_pProject->m_DebugManager.GetParam(_T("StartTimeout"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 20));
   sName.LoadString(IDS_MISC_CONNECTTIMEOUT);
   sValue = m_pProject->m_DebugManager.GetParam(_T("ConnectTimeout"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 21));
   sName.LoadString(IDS_MISC_MAINTAINSESSION);
   bValue = m_pProject->m_DebugManager.GetParam(_T("MaintainSession")) == _T("true");
   m_ctrlList.AddItem(PropCreateSimple(sName, bValue, 22));
   sName.LoadString(IDS_MISC_SEARCHPATH);
   sValue = m_pProject->m_DebugManager.GetParam(_T("SearchPath"));
   m_ctrlList.AddItem(PropCreateSimple(sName, sValue, 23));

   return 0;
}

int CDebuggerCommandsPage::OnSetActive()
{   
   m_ctrlList.SetItemEnabled(m_ctrlList.FindProperty(20), g_data.iShellType != SHELLTRANSFER_COMSPEC);
   m_ctrlList.SetItemEnabled(m_ctrlList.FindProperty(21), g_data.iShellType != SHELLTRANSFER_COMSPEC);
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
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(10), &v);
   m_pProject->m_DebugManager.SetParam(_T("Debugger"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(11), &v);
   m_pProject->m_DebugManager.SetParam(_T("AttachExe"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(12), &v);
   m_pProject->m_DebugManager.SetParam(_T("AttachCore"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(13), &v);
   m_pProject->m_DebugManager.SetParam(_T("AttachPid"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(14), &v);
   m_pProject->m_DebugManager.SetParam(_T("DebugMain"), CString(v.bstrVal));

   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(20), &v);
   m_pProject->m_DebugManager.SetParam(_T("StartTimeout"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(21), &v);
   m_pProject->m_DebugManager.SetParam(_T("ConnectTimeout"), CString(v.bstrVal));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(22), &v);
   m_pProject->m_DebugManager.SetParam(_T("MaintainSession"), v.lVal != 0 ? _T("true") : _T("false"));
   v.Clear();
   m_ctrlList.GetItemValue(m_ctrlList.FindProperty(23), &v);
   m_pProject->m_DebugManager.SetParam(_T("SearchPath"), CString(v.bstrVal));

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
   SET_CHECK(IDC_PROTECTFILES,     sKey + _T("protectDebugged"));
   SET_CHECK(IDC_MATCHBRACES,      sKey + _T("matchBraces"));
   SET_CHECK(IDC_AUTOCOMPLETE,     sKey + _T("autoComplete"));
   SET_CHECK(IDC_CLASSBROWSER,     sKey + _T("classBrowser"));
   SET_CHECK(IDC_ONLINESCANNER,    sKey + _T("onlineScanner"));
   SET_CHECK(IDC_AUTOSUGGEST,      sKey + _T("autoSuggest"));
   SET_CHECK(IDC_AUTOCLOSE,        sKey + _T("autoClose"));
   SET_CHECK(IDC_MARKERRORS,       sKey + _T("markErrors"));
   SET_CHECK(IDC_BREAKPOINTLINES,  sKey + _T("breakpointLines"));

   return 0;
}

int CAdvancedEditOptionsPage::OnApply()
{
   if( m_pArc->ReadGroupBegin(_T("Editors")) ) 
   {
      while( m_pArc->ReadGroupBegin(_T("Editor")) ) {
         TCHAR szLanguage[32] = { 0 };
         m_pArc->Read(_T("name"), szLanguage, 31);
         if( m_sLanguage == szLanguage ) {
            m_pArc->Delete(_T("Advanced"));
            m_pArc->WriteItem(_T("Advanced"));
            WRITE_CHECKBOX(IDC_NOTIPS,          _T("noTips"));
            WRITE_CHECKBOX(IDC_PROTECTFILES,    _T("protectDebugged"));
            WRITE_CHECKBOX(IDC_MATCHBRACES,     _T("matchBraces"));
            WRITE_CHECKBOX(IDC_AUTOCOMPLETE,    _T("autoComplete"));
            WRITE_CHECKBOX(IDC_CLASSBROWSER,    _T("classBrowser"));
            WRITE_CHECKBOX(IDC_ONLINESCANNER,   _T("onlineScanner"));
            WRITE_CHECKBOX(IDC_AUTOSUGGEST,     _T("autoSuggest"));
            WRITE_CHECKBOX(IDC_AUTOCLOSE,       _T("autoClose"));
            WRITE_CHECKBOX(IDC_MARKERRORS,      _T("markErrors"));
            WRITE_CHECKBOX(IDC_BREAKPOINTLINES, _T("breakpointLines"));
         }
         m_pArc->ReadGroupEnd();
      }
      m_pArc->ReadGroupEnd();
   }

   // HACK: To clear the iterator cache
   m_pArc->ReadGroupEnd();

   return PSNRET_NOERROR;
}

