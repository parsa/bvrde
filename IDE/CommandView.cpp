
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

void AppendRtfText(CRichEditCtrl ctrlEdit, LPCTSTR pstrText, DWORD dwMask = 0, DWORD dwEffects = 0, COLORREF clrText = 0)
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
      m_ctrlEdit.SetReadOnly(TRUE);
      if( !_BroadcastCommand(m_sCommandLine) 
          && _tcsicmp(m_sCommandLine, _T("help")) != 0 ) 
      {
         AppendRtfText(m_ctrlEdit, CString(MAKEINTRESOURCE(IDS_ERR_UNKNOWNCOMMAND)), CFM_COLOR, 0, COLOR_RED);
         // On first time an un-interpreted (errornous) command is entered, print the
         // "Type Help to see commands" text to help the user along.
         static int s_nErrors = 0;
         if( ++s_nErrors == 1 ) AppendRtfText(m_ctrlEdit, CString(MAKEINTRESOURCE(IDS_TYPEHELP)), CFM_COLOR, 0, COLOR_RED);
      }
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
         // Bluntly ignore it...
         ATLASSERT(false);
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
   WORD wSize = (iLen + 1) * sizeof(TCHAR);
   LPTSTR pstr = (LPTSTR) _alloca(wSize  + sizeof(WORD));
   * (WORD*) pstr = iLen;
   iLen = GetLine(iLine, pstr);
   pstr[iLen] = _T('\0');
   CString s = pstr;
   s.TrimLeft(_T(" \t>"));
   s.TrimRight(_T("\r\n \t"));
   return s;
}

void CCommandView::_ExecAndCapture(LPCTSTR pstrCommandLine)
{
   USES_CONVERSION;

   OSVERSIONINFO osv = { sizeof(OSVERSIONINFO) };
   ::GetVersionEx(&osv);
   bool bWin95 = osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS;

   SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), 0, 0 };
   sa.bInheritHandle = TRUE;
   sa.lpSecurityDescriptor = NULL;

   // If NT make a real security thing to allow inheriting handles
   SECURITY_DESCRIPTOR sd;
   if( !bWin95 ) {
      ::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
      ::SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
      sa.nLength = sizeof(SECURITY_ATTRIBUTES);
      sa.lpSecurityDescriptor = &sd;
   }

   // Create pipe for output redirection
   // read handle, write handle, security attributes, number of bytes reserved for pipe - 0 default
   HANDLE hPipeWrite = NULL;
   HANDLE hPipeRead = NULL;
   ::CreatePipe(&hPipeRead, &hPipeWrite, &sa, 0);

   HANDLE hWriteSubProcess = NULL;

   // Read handle, write handle, security attributes,  number of bytes reserved for pipe - 0 default
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
   DWORD dwTimeDetectedDeath = 0;
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
      AppendRtfText(m_hWnd, _GetSystemErrorText(::GetLastError()), CFM_COLOR, 0, COLOR_RED);
   }
   while( bRunning ) {
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
      if( dwTimeDetectedDeath > 0 && ::GetTickCount()  - dwTimeDetectedDeath > 500 ) bRunning = false;
      DWORD dwExitCode = 0;
      if( ::GetExitCodeProcess(pi.hProcess, &dwExitCode) ) {
         if( STILL_ACTIVE != dwExitCode ) {
            if( bWin95 ) {
               // Process is dead, but wait a second in case there is some output in transit
               dwTimeDetectedDeath = ::GetTickCount();
            } 
            else {   
               // NT; so it's dead already...
               break;
            }
         }
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

CString CCommandView::_GetSystemErrorText(DWORD dwErr) const
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
   if( lpMsgBuf ) ::LocalFree(lpMsgBuf);
   return s;
}

// Message handlers

LRESULT CCommandView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pMainFrame);
   LRESULT lRes = DefWindowProc();

   SetFont(AtlGetDefaultGuiFont());
   SetBackgroundColor(::GetSysColor(COLOR_WINDOW));
   LimitText(60000);
   SetSel(-1, -1);

   m_pMainFrame->AddCommandListener(this);

   return lRes;
}

LRESULT CCommandView::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == VK_RETURN ) {
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
         CString sPrompt = _ParseLine();
         AppendRtfText(m_hWnd, _T("\r\n"));
         // Ignite thread to broadcast the event...
         s_thread.Stop();
         s_thread.m_pMainFrame = m_pMainFrame;
         s_thread.m_ctrlEdit = m_hWnd;
         s_thread.m_sCommandLine = sPrompt;
         s_thread.Start();
      }
      return 0;
   }
   bHandled = FALSE;
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
   
   FORMATRANGE fr;
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

// ICommandListener

void CCommandView::OnUserCommand(LPCTSTR pstrCommand, BOOL& bHandled)
{
   bHandled = FALSE;
   // Support for the following commands:
   //    run <filename>
   //    print <filename>
   //    exec <filename>
   //    ? <expression>
   //    help
   if( _tcsncmp(pstrCommand, _T("run "), 4) == 0 )
   {
      TCHAR szBuffer[500] = { 0 };
      _tcscpy(szBuffer, pstrCommand + 4);
      LPTSTR pstrArgs = ::PathGetArgs(szBuffer);
      if( pstrArgs && *pstrArgs != '\0' ) *(pstrArgs - 1) = '\0';
      ::PathUnquoteSpaces(szBuffer);
      DWORD dwRes = (DWORD) ::ShellExecute(m_hWnd, _T("open"), szBuffer, pstrArgs, NULL, SW_SHOW);
      if( dwRes <= 32 ) AppendRtfText(m_hWnd, _GetSystemErrorText(dwRes), CFM_COLOR, 0, COLOR_RED);
      bHandled = TRUE;
   }
   else if( _tcsncmp(pstrCommand, _T("print "), 6) == 0 )
   {
      DWORD dwRes = (DWORD) ::ShellExecute(m_hWnd, _T("print"), pstrCommand + 6, NULL, NULL, SW_SHOW);
      if( dwRes <= 32 ) AppendRtfText(m_hWnd, _GetSystemErrorText(dwRes), CFM_COLOR, 0, COLOR_RED);
      bHandled = TRUE;
   }
   else if( _tcsnicmp(pstrCommand, _T("exec "), 5) == 0 )
   {
      _ExecAndCapture(pstrCommand + 5);
      SetSel(-1, -1);
      bHandled = TRUE;
   }
   else if( _tcsnicmp(pstrCommand, _T("? "), 2) == 0 ) 
   {
      ::CoInitialize(NULL);
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
      ::CoUninitialize();
      bHandled = TRUE;
   }
   else if( _tcsicmp(pstrCommand, _T("help")) == 0 )
   {
      AppendRtfText(m_hWnd, CString(MAKEINTRESOURCE(IDS_COMMAND_HELP)));
   }
}

