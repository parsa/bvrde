
#include "stdafx.h"
#include "resource.h"

#include "CommandView.h"
#include "MainFrm.h"

#include "Macro.h"
#include "Thread.h"


#define COLOR_RED RGB(100,0,0)
#define COLOR_BLUE RGB(0,0,100)


/////////////////////////////////////////////////////
// Globals

void AppendRtfText(CRichEditCtrl ctrlEdit, LPCTSTR pstrText, DWORD dwMask = 0, DWORD dwEffects = 0, COLORREF clrText = RGB(0,0,0))
{
   ATLASSERT(ctrlEdit.IsWindow());
   ATLASSERT(!::IsBadStringPtr(pstrText,-1));
   // Yield control?
   if( ::InSendMessage() ) ::ReplyMessage(TRUE);
   ctrlEdit.HideSelection(TRUE);
   // Remove top lines if we've filled out the buffer
   GETTEXTLENGTHEX gtlx = { GTL_DEFAULT | GTL_CLOSE, 1200 };
   while( ctrlEdit.GetTextLengthEx(&gtlx) > ctrlEdit.GetLimitText() - 3000 ) {     
      LONG iStartPos = ctrlEdit.LineIndex(0);
      LONG iEndPos = ctrlEdit.LineIndex(1);
      ctrlEdit.SetSel(iStartPos, iEndPos);
      ctrlEdit.ReplaceSel(_T(""));
   }
   // Append line
   LONG iStartPos = 0;
   LONG iEndPos = 0;
   LONG iDummy = 0;
   CHARFORMAT cf;
   cf.cbSize = sizeof(CHARFORMAT);
   cf.dwMask = dwMask;;
   cf.dwEffects = dwEffects;
   cf.crTextColor = clrText;
   ctrlEdit.SetSel(-1, -1);
   ctrlEdit.GetSel(iStartPos, iDummy);
   ctrlEdit.ReplaceSel(pstrText);
   ctrlEdit.GetSel(iDummy, iEndPos);
   ctrlEdit.SetSel(iStartPos, iEndPos);
   ctrlEdit.SetSelectionCharFormat(cf);
   ctrlEdit.HideSelection(FALSE);
   ctrlEdit.SetSel(-1, -1);
}

CString GetSystemErrorText(DWORD dwErr)
{
   LPVOID lpMsgBuf = NULL;
   ::FormatMessage( 
       FORMAT_MESSAGE_ALLOCATE_BUFFER | 
       FORMAT_MESSAGE_FROM_SYSTEM | 
       FORMAT_MESSAGE_IGNORE_INSERTS,
       NULL,
       dwErr,
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
       (LPTSTR) &lpMsgBuf,
       0,
       NULL);
   CString s = (LPCTSTR) lpMsgBuf;
   if( lpMsgBuf != NULL ) ::LocalFree(lpMsgBuf);
   return s;
}


/////////////////////////////////////////////////////
// CCommandThread

class CCommandThread : public CThreadImpl<CCommandThread>
{
public:
   CRichEditCtrl m_ctrlEdit;
   CMainFrame* m_pMainFrame;
   CString m_sCommandLine;

   DWORD Run()
   {
      ATLASSERT(m_pMainFrame);
      ATLASSERT(m_ctrlEdit.IsWindow());
      g_pDevEnv->SetThreadLanguage();
      // NOTE: Cannot initialize as multi-threaded; see Q287087
      CCoInitialize cominit;
      // Lock view before executing
      m_ctrlEdit.SetReadOnly(TRUE);
      // Allow plugins to respond
      if( !_BroadcastCommand(m_sCommandLine) 
          && m_sCommandLine.CompareNoCase(_T("help")) != 0
          && !m_sCommandLine.IsEmpty() )
      {
         AppendRtfText(m_ctrlEdit, CString(MAKEINTRESOURCE(IDS_ERR_UNKNOWNCOMMAND)), CFM_COLOR, 0, COLOR_RED);
         // On first time an un-interpreted (errornous) command is entered, print the
         // "Type Help to see commands" text to help the user along.
         static int s_nErrors = 0;
         if( ++s_nErrors == 1 ) AppendRtfText(m_ctrlEdit, CString(MAKEINTRESOURCE(IDS_TYPEHELP)), CFM_COLOR, 0, COLOR_RED);
      }
      // Print prompt and unlock
      AppendRtfText(m_ctrlEdit, _T("\r\n> "), CFM_BOLD, CFE_BOLD);
      m_ctrlEdit.SetReadOnly(FALSE);
      return 0;
   }

   bool _BroadcastCommand(LPCTSTR pstrText)
   {
      __try
      {
         BOOL bHandled = FALSE;
         int nCount = m_pMainFrame->m_aCommandListeners.GetSize();
         for( int i = 0; i < nCount; i++ ) {
            m_pMainFrame->m_aCommandListeners[i]->OnUserCommand(pstrText, bHandled);
            if( bHandled ) return true;
         }
      }
      __except(1)
      {         
         ATLASSERT(false);   // Bluntly ignore it...
      }
      return false;
   }
};


/////////////////////////////////////////////////////
// CCommandView

// Constructor

CCommandView::CCommandView() :
   m_pMainFrame(NULL)
{
}

// Operations
   
void CCommandView::Clear()
{
   SetWindowText(_T(""));
   AppendRtfText(m_hWnd, _T("> "), CFM_BOLD, CFE_BOLD);
}

// Implementation

CString CCommandView::_ParseLine() const
{
   // Format of command prompt is:
   //   > Stuff
   // so that's pretty simple.
   LONG iStart, iEnd;
   GetSel(iStart, iEnd);
   int iLine = LineFromChar(iStart);
   int iLen = LineLength(iStart);
   ATLASSERT(iLine<GetLineCount());
   // Ok, the RTF GetLine() is kind of weird and needs the
   // first WORD to be the length...
   WORD wSize = (WORD)((iLen + 1) * sizeof(TCHAR));
   LPTSTR pstr = (LPTSTR) _alloca(wSize  + sizeof(WORD));
   * (WORD*) pstr = (WORD) iLen;
   iLen = GetLine(iLine, pstr);
   pstr[iLen] = '\0';
   CString s = pstr;
   s.TrimLeft(_T(" \t>"));
   s.TrimRight(_T("\r\n \t"));
   return s;
}

void CCommandView::_ExecAndCapture(LPCTSTR pstrCommandLine)
{
   SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), 0, 0 };
   sa.bInheritHandle = TRUE;
   sa.lpSecurityDescriptor = NULL;

   // Make a real security thing to allow inheriting handles
   SECURITY_DESCRIPTOR sd;
   ::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
   ::SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
   sa.nLength = sizeof(SECURITY_ATTRIBUTES);
   sa.lpSecurityDescriptor = &sd;

   // Create pipe for output redirection
   // Read handle, write handle, security attributes, number of bytes reserved for pipe - 0 default
   HANDLE hPipeWrite = NULL;
   HANDLE hPipeRead = NULL;
   ::CreatePipe(&hPipeRead, &hPipeWrite, &sa, 0);

   HANDLE hWriteSubProcess = NULL;

   // Read handle, write handle, security attributes, number of bytes reserved for pipe - 0 default
   HANDLE hRead2 = NULL;
   ::CreatePipe(&hRead2, &hWriteSubProcess, &sa, 0);

   ::SetHandleInformation(hPipeRead, HANDLE_FLAG_INHERIT, 0);
   ::SetHandleInformation(hWriteSubProcess, HANDLE_FLAG_INHERIT, 0);

   STARTUPINFO si = { sizeof(STARTUPINFO) };
   si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
   si.wShowWindow = SW_SHOWNOACTIVATE;
   si.hStdInput = hRead2;
   si.hStdOutput = hPipeWrite;
   si.hStdError = hPipeWrite;

   PROCESS_INFORMATION pi = { 0 };
   BOOL bRunning = ::CreateProcess(
           NULL,
           const_cast<LPTSTR>(pstrCommandLine),
           NULL, 
           NULL,
           TRUE, 
           CREATE_NEW_PROCESS_GROUP,
           NULL,
           NULL,
           &si, 
           &pi);
   if( !bRunning ) {
      AppendRtfText(m_hWnd, GetSystemErrorText(::GetLastError()), CFM_COLOR, 0, COLOR_RED);
   }

   while( bRunning ) 
   {
      DWORD dwBytesRead = 0;
      DWORD dwBytesAvail = 0;
      BYTE bBuffer[1024 + 1];
      if( !::PeekNamedPipe(hPipeRead, bBuffer, sizeof(bBuffer) - 1, &dwBytesRead, &dwBytesAvail, NULL) ) {
         dwBytesAvail = 0;
      }
      if( dwBytesAvail > 0 ) {
         BOOL bTest = ::ReadFile(hPipeRead, bBuffer, sizeof(bBuffer) - 1, &dwBytesRead, NULL);
         if( !bTest ) break;
         bBuffer[dwBytesRead] = '\0';
         CString sText = (LPSTR) bBuffer;
         sText.Remove('\r');
         sText.Replace(_T("\n"), _T("\r\n"));
         AppendRtfText(m_hWnd, sText);
         UpdateWindow();
      }
      DWORD dwExitCode = 0;
      if( ::GetExitCodeProcess(pi.hProcess, &dwExitCode) ) {
         if( STILL_ACTIVE != dwExitCode ) break;
      }
      ::Sleep(100L);
   }

   ::CloseHandle(pi.hProcess);
   ::CloseHandle(pi.hThread);
   ::CloseHandle(hPipeRead);
   ::CloseHandle(hPipeWrite);
   ::CloseHandle(hRead2);
   ::CloseHandle(hWriteSubProcess);
}

// Message handlers

LRESULT CCommandView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pMainFrame);
   LRESULT lRes = DefWindowProc();

   SetFont(AtlGetDefaultGuiFont());
   SetBackgroundColor(::GetSysColor(COLOR_WINDOW));
   LimitText(160000);
   SetUndoLimit(1);
   SetSel(-1, -1);

   m_pMainFrame->AddCommandListener(this);

   return lRes;
}

LRESULT CCommandView::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == VK_RETURN ) 
   {
      // Send command/line to command window. We do this
      // in another thread so we do not block the user interface
      // while waiting for the command to complete.
      static CCommandThread s_thread;
      if( s_thread.IsRunning() ) {
         // Already executing another command! Fail with a beep.
         ::MessageBeep((UINT)-1);
      }
      else {
         // Parse prompt now (in the GUI thread)
         SetModify(FALSE);
         CString sPrompt = _ParseLine();
         AppendRtfText(m_hWnd, _T("\r\n"));
         if( sPrompt == _T("mode") )
         {
            // Resetting console mode?
            m_sMode.Empty();  
            AppendRtfText(m_hWnd, _T("\r\n> "), CFM_BOLD, CFE_BOLD);
         }
         else
         {
            // Ignite thread to broadcast the event...
            s_thread.Stop();
            s_thread.m_pMainFrame = m_pMainFrame;
            s_thread.m_ctrlEdit = m_hWnd;
            s_thread.m_sCommandLine = m_sMode + sPrompt;
            s_thread.Start();
         }
      }
      return 0;
   }
   if( wParam == VK_BACK ) 
   {
      // Cannot delete prompt itself
      LONG iStart, iEnd;
      GetSel(iStart, iEnd);
      TCHAR szBuffer[5] = { 0 };
      TEXTRANGE tr = { 0 };
      tr.chrg.cpMin = max(0, iStart - 3);
      tr.chrg.cpMax = max(0, iStart);
      tr.lpstrText = szBuffer;
      GetTextRange(&tr);
      if( _tcscmp(szBuffer, _T("> ")) == 0
          || _tcscmp(szBuffer, _T("\r> ")) == 0 
          || _tcscmp(szBuffer, _T("\n> ")) == 0 ) 
      {
         ::MessageBeep((UINT)-1);
         return 0;
      }
   }
   bHandled = FALSE;
   return 0;
}

LRESULT CCommandView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{   
   POINT pt = { 0 };
   ::GetCursorPos(&pt);
   CMenu menu;
   menu.LoadMenu(IDR_COMMANDVIEW);
   ATLASSERT(menu.IsMenu());
   CMenuHandle submenu = menu.GetSubMenu(0);
   UINT nCmd = g_pDevEnv->ShowPopupMenu(NULL, submenu, pt, FALSE, this);
   if( nCmd != 0 ) PostMessage(WM_COMMAND, MAKEWPARAM(nCmd, 0), (LPARAM) m_hWnd);
   return 0;
}

LRESULT CCommandView::OnPrintClient(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   // NOTE: We need to handle WM_PRINTCLIENT because we're using this
   //       window with the AnimateWindow() API and there's a bug in
   //       the RichEdit control which doesn't seem to handle this
   //       message correctly.
   CDCHandle dc = (HDC) wParam;
   RECT rcClient;
   GetClientRect(&rcClient);

   dc.FillSolidRect(&rcClient, ::GetSysColor(COLOR_WINDOW));

   RECT rcPage = rcClient;
   rcPage.right = ::MulDiv(rcPage.right, 1440, dc.GetDeviceCaps(LOGPIXELSX));
   rcPage.bottom = ::MulDiv(rcPage.bottom, 1440, dc.GetDeviceCaps(LOGPIXELSY));

   RECT rcMargin = { 8, 8, 8, 8 };
   RECT rcOutput = rcPage;
   // Convert from 1/1000" to twips
   rcOutput.left += ::MulDiv(rcMargin.left, 1440, 1000);
   rcOutput.right -= ::MulDiv(rcMargin.right, 1440, 1000);
   rcOutput.top += ::MulDiv(rcMargin.top, 1440, 1000);
   //rcOutput.bottom -= ::MulDiv(rcMargin.bottom, 1440, 1000);
   
   FORMATRANGE fr = { 0 };
   fr.hdc = dc;
   fr.hdcTarget = dc;
   fr.rc = rcOutput;
   fr.rcPage = rcPage;
   fr.chrg.cpMin = LineIndex(GetFirstVisibleLine());
   fr.chrg.cpMax = -1;

   // We have to adjust the origin because 0,0 is not at the corner of the paper
   // but is at the corner of the printable region
   int nOffsetX = dc.GetDeviceCaps(PHYSICALOFFSETX);
   int nOffsetY = dc.GetDeviceCaps(PHYSICALOFFSETY);
   dc.SetViewportOrg(-nOffsetX, -nOffsetY, NULL);
   FormatRange(&fr, TRUE);
   DisplayBand(&rcOutput);

   // Cleanup cache in richedit
   FormatRange(NULL, FALSE);
   return 0;
}

LRESULT CCommandView::OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   Copy();
   return 0;
}

// IIdleListener

void CCommandView::OnIdle(IUpdateUI* pUIBase)
{
   pUIBase->UIEnable(ID_EDIT_COPY, TRUE);
}

void CCommandView::OnGetMenuText(UINT /*wID*/, LPTSTR /*pstrText*/, int /*cchMax*/)
{
}

// ICustomCommandListener

void CCommandView::OnUserCommand(LPCTSTR pstrCommand, BOOL& bHandled)
{
   bHandled = FALSE;
   // Support for the following commands:
   //    run <filename>
   //    print <filename>
   //    exec <filename>
   //    mode <prefix>
   //    ? <expression>
   //    help
   if( _tcsncmp(pstrCommand, _T("run "), 4) == 0 )
   {
      TCHAR szBuffer[500] = { 0 };
      _tcscpy(szBuffer, pstrCommand + 4);
      LPTSTR pstrArgs = ::PathGetArgs(szBuffer);
      if( pstrArgs != NULL && *pstrArgs != '\0' ) *(pstrArgs - 1) = '\0';
      ::PathUnquoteSpaces(szBuffer);
      DWORD dwRes = (DWORD) ::ShellExecute(m_hWnd, _T("open"), szBuffer, pstrArgs, NULL, SW_SHOW);
      if( dwRes <= 32 ) AppendRtfText(m_hWnd, GetSystemErrorText(dwRes), CFM_COLOR, 0, COLOR_RED);
      bHandled = TRUE;
   }
   else if( _tcsncmp(pstrCommand, _T("print "), 6) == 0 )
   {
      DWORD dwRes = (DWORD) ::ShellExecute(m_hWnd, _T("print"), pstrCommand + 6, NULL, NULL, SW_SHOW);
      if( dwRes <= 32 ) AppendRtfText(m_hWnd, GetSystemErrorText(dwRes), CFM_COLOR, 0, COLOR_RED);
      bHandled = TRUE;
   }
   else if( _tcsnicmp(pstrCommand, _T("exec "), 5) == 0 )
   {
      _ExecAndCapture(pstrCommand + 5);
      SetSel(-1, -1);
      bHandled = TRUE;
   }
   else if( _tcsnicmp(pstrCommand, _T("mode "), 5) == 0 )
   {
      m_sMode = pstrCommand + 5;
      m_sMode.TrimRight();
      m_sMode += _T(" ");
      bHandled = TRUE;
   }
   else if( _tcsnicmp(pstrCommand, _T("?"), 1) == 0 ) 
   {
      CComObjectGlobal<CMacro> macro;
      macro.Init(m_pMainFrame, &m_pMainFrame->m_Dispatch);
      CComVariant vRes;
      EXCEPINFO e = { 0 };
      macro.RunMacroFromScript(CComBSTR(pstrCommand + 2), NULL, SCRIPTTEXT_ISEXPRESSION, &vRes, &e);
      HRESULT Hr = ::VariantChangeTypeEx(&vRes, &vRes, LOCALE_USER_DEFAULT, VARIANT_ALPHABOOL | VARIANT_LOCALBOOL, VT_BSTR);
      if( Hr == S_OK && e.bstrDescription == NULL ) {
         AppendRtfText(m_hWnd, CString(vRes.bstrVal) + _T("\r\n"), CFM_COLOR, 0, COLOR_BLUE);
      }
      else {
         AppendRtfText(m_hWnd, CString(e.bstrDescription) + _T("\r\n"), CFM_COLOR, 0, COLOR_RED);
      }
      bHandled = TRUE;
   }
   else if( _tcsicmp(pstrCommand, _T("help")) == 0 )
   {
      AppendRtfText(m_hWnd, CString(MAKEINTRESOURCE(IDS_COMMAND_HELP)));
   }
}

void CCommandView::OnMenuCommand(LPCTSTR pstrType, LPCTSTR pstrCommand, LPCTSTR pstrArguments, LPCTSTR pstrPath, int iFlags, BOOL& bHandled)
{
   // Support for the following types:
   //   local
   bHandled = FALSE;
   if( _tcscmp(pstrType, _T("local")) != 0 || _tcslen(pstrType) == 0 ) return;
   if( (iFlags & TOOLFLAGS_CONSOLEOUTPUT) != 0 ) {
      // Yes, let's show this baby
      g_pDevEnv->ActivateAutoHideView(m_hWnd);
      // Run command
      AppendRtfText(m_hWnd, _T("\r\n\r\n"));
      CString sCommandline;
      sCommandline.Format(_T("%s %s"), pstrCommand, pstrArguments);
      _ExecAndCapture(sCommandline);
      // Display prompt as last item
      AppendRtfText(m_hWnd, _T("\r\n> "), CFM_BOLD, CFE_BOLD);
      SetSel(-1, -1);
   }
   else {
      // Just launch the application using the Shell
      ::ShellExecute(m_hWnd, _T("open"), pstrCommand, pstrArguments, pstrPath, SW_SHOWNORMAL);
   }
   bHandled = TRUE;
}

