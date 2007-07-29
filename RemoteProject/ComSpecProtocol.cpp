
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"

#include "ComSpecProtocol.h"

#pragma code_seg( "PROTOCOLS" )


////////////////////////////////////////////////////////
//

CComSpecThread::CComSpecThread()
{
   m_hPipeRead = NULL;
   m_hPipeWrite = NULL;
   m_hPipeStdOut = NULL;
   m_hPipeStdIn = NULL;
   m_dwProcessId = 0;
}

DWORD CComSpecThread::Run()
{
   USES_CONVERSION;

   ATLASSERT(m_pManager);
   ATLASSERT(m_pCallback);

   ::SetThreadLocale(_pDevEnv->GetLCID());

   CString sBinPath = m_pManager->GetParam(_T("Host"));
   CString sPath = m_pManager->GetParam(_T("Path"));
   CString sExtraCommands = m_pManager->GetParam(_T("Extra"));

   // If user supplied a path to the MinGW /bin folder, let's make
   // sure to actually find the needed processes there.
   // We'll currently just look for gdb.exe since we don't know what
   // compilation environment is eventually used.
   if( !sBinPath.IsEmpty() ) {
      TCHAR szFilename[MAX_PATH];
      _tcscpy(szFilename, sBinPath);
      ::PathAppend(szFilename, _T("gdb.exe"));
      if( !::PathFileExists(szFilename) ) {
         m_pManager->m_dwErrorCode = ERROR_PATH_BUSY;
         return 0;
      }
      
   }

   m_pManager->m_dwErrorCode = 0;
   m_pManager->m_dwSpawnedProcessId = 0;
   m_pManager->m_bConnected = false;

   // Collect sub-processes in a job
   HANDLE hJob = ::CreateJobObject(NULL, NULL);

   SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), 0, 0 };
   sa.bInheritHandle = TRUE;
   sa.lpSecurityDescriptor = NULL;

   // Make a real security thing to allow inheriting handles
   SECURITY_DESCRIPTOR sd;
   ::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
   ::SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
   sa.nLength = sizeof(SECURITY_ATTRIBUTES);
   sa.lpSecurityDescriptor = &sd;

   TCHAR szCommandLine[MAX_PATH] = { 0 };
   ::ExpandEnvironmentStrings(_T("%ComSpec%"), szCommandLine, MAX_PATH);

   // Create pipe for output redirection
   // Read handle, write handle, security attributes, number of bytes reserved for pipe - 0 default
   ::CreatePipe(&m_hPipeRead, &m_hPipeStdOut, &sa, 0);
   ::CreatePipe(&m_hPipeStdIn, &m_hPipeWrite, &sa, 0);
   ::SetHandleInformation(m_hPipeRead, HANDLE_FLAG_INHERIT, 0);
   ::SetHandleInformation(m_hPipeWrite, HANDLE_FLAG_INHERIT, 0);

   STARTUPINFO si = { sizeof(STARTUPINFO) };
   si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
   si.wShowWindow = SW_HIDE;
   si.hStdInput = m_hPipeStdIn;
   si.hStdOutput = m_hPipeStdOut;
   si.hStdError = m_hPipeStdOut;

   PROCESS_INFORMATION pi = { 0 };
   BOOL bRunning = ::CreateProcess(
           NULL,
           szCommandLine,
           NULL, 
           NULL,
           TRUE, 
           CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP | NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED,
           NULL,
           NULL,
           &si, 
           &pi);
   if( !bRunning ) {
      m_pManager->m_dwErrorCode = ::GetLastError();
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_COMSPEC)));
      return 0;
   }
   m_dwProcessId = pi.dwProcessId;
   ::AssignProcessToJobObject(hJob, pi.hProcess);
   ::ResumeThread(pi.hThread); 

   m_pManager->m_bConnected = true;

   // Prepare buffers
   const int MAX_BUFFER_SIZE = 400;
   LPBYTE pBuffer = (LPBYTE) malloc(MAX_BUFFER_SIZE);
   BYTE bReadBuffer[MAX_BUFFER_SIZE + 2] = { 0 };
   DWORD dwPos = 0;
   DWORD dwStartLinePos = 0;
   DWORD dwBufferSize = MAX_BUFFER_SIZE;
   VT100COLOR nColor = VT100_DEFAULT;

   // Adding the path to the compile tools is important
   if( !sBinPath.IsEmpty() ) {
      CString sPathEnv;
      sPathEnv.Format(_T("SET PATH=%%PATH%%;%s"), sBinPath);
      m_pManager->WriteData(sPathEnv);
   }

   m_pManager->WriteData(sExtraCommands);

   while( bRunning ) 
   {
      if( ShouldStop() ) break;

      DWORD dwBytesRead = 0;
      DWORD dwBytesAvail = 0;
      if( !::PeekNamedPipe(m_hPipeRead, bReadBuffer, sizeof(bReadBuffer) - 1, &dwBytesRead, &dwBytesAvail, NULL) ) {
         dwBytesAvail = 0;
      }
      if( dwBytesAvail > 0 ) {
         BOOL bTest = ::ReadFile(m_hPipeRead, bReadBuffer, sizeof(bReadBuffer) - 1, &dwBytesRead, NULL);
         if( !bTest ) break;
         bReadBuffer[dwBytesRead] = '\0';
         // We might need to grow the data buffer
         if( dwPos + dwBytesRead > dwBufferSize ) {
            dwBufferSize = dwPos + dwBytesRead;
            pBuffer = (LPBYTE) realloc(pBuffer, dwBufferSize);
            ATLASSERT(pBuffer);
         }
         // Scan received buffer...
         DWORD iPos = 0;
         while( iPos < dwBytesRead ) {
            ATLASSERT(iPos<sizeof(bReadBuffer));
            BYTE b = bReadBuffer[iPos++];
            switch( b ) {
            case 0x07:
            case '\v':
            case '\f':
            case '\r':
               break;
            case '\0':
            case '\n':
               {
                  CString sLine = _GetLine(pBuffer, dwStartLinePos, dwPos);
                  if( sLine.Left(sPath.GetLength()) == sPath && sLine.Find('>') > 0 ) nColor = VT100_PROMPT;
                  dwPos = 0;
                  dwStartLinePos = 0;
                  m_pCallback->BroadcastLine(nColor, sLine);
                  nColor = VT100_DEFAULT;
               }
               break;
            default:
               *(pBuffer + dwPos) = b;
               dwPos++;
            }
         }
      }

      // Check for process death...
      DWORD dwExitCode = 0;
      if( ::GetExitCodeProcess(pi.hProcess, &dwExitCode) ) {
         if( STILL_ACTIVE != dwExitCode ) break;
      }

      // TODO: Hmm, nasty data polling delay! Sadly Windows will block on the ReadFile()
      //       and won't let us close the handle in the other thread.
      ::Sleep(80L);
   }

   // Play it nicely and terminate by CTRL+C
   m_pManager->GenerateConsoleEvent(pi.dwProcessId, CTRL_C_EVENT);
   m_pManager->GenerateConsoleEvent(pi.dwProcessId, CTRL_CLOSE_EVENT);

   // Close the stdin/stdout. This could actually also stall some applications
   // if they're still running.
   ::CloseHandle(m_hPipeStdIn);   m_hPipeStdIn = NULL;
   ::CloseHandle(m_hPipeStdOut);  m_hPipeStdOut = NULL;
   ::CloseHandle(m_hPipeRead);    m_hPipeRead = NULL;
   ::CloseHandle(m_hPipeWrite);   m_hPipeWrite = NULL;       

   m_dwProcessId = 0;

   // "All those moments will be lost in time, like tears in rain. Time to die." (Roy Batty)
   ::TerminateJobObject(hJob, 0);
   ::CloseHandle(hJob);

   // Kill it off explicitly if nessecary. The Job object should have done its
   // toll, but we want to be sure.
   DWORD dwExitCode = 0;
   if( ::GetExitCodeProcess(pi.hProcess, &dwExitCode) ) {
      if( STILL_ACTIVE == dwExitCode ) ::TerminateProcess(pi.hProcess, 0);
   }

   ::CloseHandle(pi.hProcess);
   ::CloseHandle(pi.hThread);

   free(pBuffer);

   m_pManager->m_bConnected = false;
   return 0;
}

CString CComSpecThread::_GetLine(LPBYTE pBuffer, DWORD dwStart, DWORD dwEnd) const
{
   ATLASSERT(dwStart<=dwEnd);
   ATLASSERT(!::IsBadReadPtr(pBuffer+dwStart,dwEnd-dwStart));
   DWORD dwLength = dwEnd - dwStart;
   if( dwLength == 0 ) return CString();
   LPSTR pstr = (LPSTR) _alloca(dwLength + 1);
   ATLASSERT(pstr);
   memcpy(pstr, pBuffer + dwStart, dwLength);
   pstr[dwLength] = '\0';
   return pstr;
}


////////////////////////////////////////////////////////
//

CComSpecProtocol::CComSpecProtocol() :
   m_dwSpawnedProcessId(0),
   m_bConnected(false)
{
   Clear();
}

CComSpecProtocol::~CComSpecProtocol()
{
   Stop();
}

void CComSpecProtocol::Init(CRemoteProject* pProject, IShellCallback* pCallback)
{
   ATLASSERT(pProject);
   ATLASSERT(pCallback);
   m_pProject = pProject;
   m_pCallback = pCallback;
}

void CComSpecProtocol::Clear()
{
   m_sPath.Empty();
   m_sExtraCommands.Empty();
}

bool CComSpecProtocol::Load(ISerializable* pArc)
{
   Clear();

   if( !pArc->ReadItem(_T("ComSpecLink")) ) return false;
   pArc->Read(_T("bin"), m_sBinPath.GetBufferSetLength(MAX_PATH), MAX_PATH);
   m_sBinPath.ReleaseBuffer();
   pArc->Read(_T("path"), m_sPath.GetBufferSetLength(MAX_PATH), MAX_PATH);
   m_sPath.ReleaseBuffer();
   pArc->Read(_T("extra"), m_sExtraCommands.GetBufferSetLength(1400), 1400);
   m_sExtraCommands.ReleaseBuffer();

   ConvertToCrLf(m_sExtraCommands);

   return true;
}

bool CComSpecProtocol::Save(ISerializable* pArc)
{
   if( !pArc->WriteItem(_T("ComSpecLink")) ) return false;
   pArc->Write(_T("bin"), m_sBinPath);
   pArc->Write(_T("path"), m_sPath);
   pArc->Write(_T("extra"), ConvertFromCrLf(m_sExtraCommands));
   return true;
}

bool CComSpecProtocol::Start()
{
   Stop();
   m_thread.m_pManager = this;
   m_thread.m_pCallback = m_pCallback;
   if( !m_thread.Start() ) return false;
   return true;
}

bool CComSpecProtocol::Stop()
{
   SignalStop();
   m_thread.Stop();
   m_dwErrorCode = 0;
   m_dwSpawnedProcessId = 0;
   m_bConnected = false;
   return true;
}

void CComSpecProtocol::SignalStop()
{
   m_thread.SignalStop();
}

bool CComSpecProtocol::IsConnected() const
{
   return m_bConnected;
}

bool CComSpecProtocol::IsBusy() const
{
   return m_thread.IsRunning() == TRUE;
}

CString CComSpecProtocol::GetParam(LPCTSTR pstrName) const
{
   CString sName = pstrName;
   if( sName == _T("Type") ) return _T("comspec");
   if( sName == _T("Host") ) return m_sBinPath;
   if( sName == _T("Path") ) return m_sPath;
   if( sName == _T("Extra") ) return m_sExtraCommands;
   if( sName == _T("Certificate") ) return _T("Windows ComSpec Link");
   return _T("");
}

void CComSpecProtocol::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   if( sName == _T("Host") ) m_sBinPath = pstrValue;
   if( sName == _T("Path") ) m_sPath = pstrValue;
   if( sName == _T("Extra") ) m_sExtraCommands = pstrValue;
   if( sName == _T("ProcessID") ) m_dwSpawnedProcessId = (DWORD) _ttol(pstrValue);
}

bool CComSpecProtocol::WriteData(LPCTSTR pstrData)
{
   USES_CONVERSION;
   ATLASSERT(!::IsBadStringPtr(pstrData,-1));
   if( !m_thread.IsRunning() ) return false;
   if( m_thread.ShouldStop() ) return false;
   if( !WaitForConnection() ) {
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_CONNECTERROR)));
      return false;
   }
   int nLen = (_tcslen(pstrData) * 2) + 3;  // MBCS + \r\n + \0
   LPSTR pstr = (LPSTR) _alloca(nLen);
   ATLASSERT(pstr);
   if( pstr == NULL ) return false;
   ::ZeroMemory(pstr, nLen);
   AtlW2AHelper(pstr, pstrData, nLen - 3);
   strcat(pstr, "\r\n");
   DWORD dwWritten = 0;
   return ::WriteFile(m_thread.m_hPipeWrite, pstr, strlen(pstr), &dwWritten, NULL) == TRUE;
}

bool CComSpecProtocol::WriteSignal(BYTE bCmd)
{
   if( m_thread.ShouldStop() ) return false;
   if( !WaitForConnection() ) return false;
   switch( bCmd ) {
   case TERMINAL_BREAK:
      {
         BOOL bRes = FALSE;
         if( !bRes && m_dwSpawnedProcessId != 0 ) {
            bRes = GenerateBreakEvent(m_dwSpawnedProcessId);
         }
         if( !bRes && m_dwSpawnedProcessId == 0 ) {
            bRes = GenerateConsoleEvent(m_thread.m_dwProcessId, CTRL_C_EVENT);
         }
         if( !bRes ) {
            m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_GDBSTOP)));
            return false;
         }
      }
      break;
   case TERMINAL_QUIT:
      {
         // While CTRL+C doesn't work with CYGWIN, CTRL+BREAK does - and it shuts
         // down most of the batch-file processing.
         GenerateConsoleEvent(m_thread.m_dwProcessId, CTRL_BREAK_EVENT);
         GenerateConsoleEvent(m_thread.m_dwProcessId, CTRL_CLOSE_EVENT);
      }
      break;
   default:
      break;
   }
   return true;
}

bool CComSpecProtocol::WriteScreenSize(int w, int h)
{
   return false;
}

bool CComSpecProtocol::WaitForConnection()
{
   // Wait for the thread to connect to the command-line link.
   const DWORD SPAWNTIMEOUT = 2;
   const DWORD IDLETIMEOUT = 3;
   DWORD dwTickStart = ::GetTickCount();
   BOOL bHasRun = FALSE;
   while( !m_bConnected ) {

      // Did the server return some kind of error?
      if( m_dwErrorCode != 0 ) {
         ::SetLastError(m_dwErrorCode);
         return false;
      }

      // Wait a little while
      PumpIdleMessages(200L);

      DWORD dwTick = ::GetTickCount();

      // Timeout after 3 sec.
      if( dwTick - dwTickStart > IDLETIMEOUT * 1000UL ) {
         ::SetLastError(ERROR_TIMEOUT);
         return false;
      }

      bHasRun |= m_thread.IsRunning();

      // If the connect thread hasn't run after 2 secs, we might
      // as well assume it failed or didn't run at all...
      if( !bHasRun
          && dwTick - dwTickStart > SPAWNTIMEOUT * 1000UL ) 
      {
         // Attempt to reconnect to the remote host.
         // This will allow a periodically downed server to recover the connection.
         Start();
         ::SetLastError(m_dwErrorCode == 0 ? ERROR_NOT_CONNECTED : m_dwErrorCode);
         return false;
      }
   }
   return true;
}

BOOL CComSpecProtocol::GenerateBreakEvent(DWORD dwProcessId)
{
   BOOL bRes = FALSE;
   // After many tries <sigh> it seems that the only way to send a CTRL+C to the
   // GDB console window, is not to send it at all, but to cause the debug-break
   // manually. It appears to be the Cygwin/MinGW wrapper that prevents the
   // scheduling of signals from other than keyboard.
   // Outside GDB the standard procedure seems to work just fine.
   if( !bRes ) {
      typedef BOOL (WINAPI *PFNDEBUGBREAKPROCESS)(HANDLE);
      PFNDEBUGBREAKPROCESS fnDebugBreakProcess = 
         (PFNDEBUGBREAKPROCESS) ::GetProcAddress( ::GetModuleHandle(_T("kernel32.dll")), "DebugBreakProcess" );
      if( fnDebugBreakProcess != NULL ) {
         HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
         if( hProcess != NULL ) {
            bRes = fnDebugBreakProcess(hProcess);
            ::CloseHandle(hProcess);
         }
      }
   }
   if( !bRes ) {
      HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
      if( hProcess != NULL ) {
         DWORD dwThreadId = 0;
         HANDLE hThread = ::CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) ::GetProcAddress( ::GetModuleHandle(_T("kernel32.dll")), "DebugBreak" ), 0, 0, &dwThreadId);
         if( hThread != NULL ) {
            ::CloseHandle(hThread);
            bRes = TRUE;
         }
         ::CloseHandle(hProcess);
      }
   }
   return bRes;
}

// This nasty piece of code was snipped from www.latenighthacking.com
// which grabs the internal kernel32.dll|CtrlRoutine() API address.
// We assume that kernel32.dll is mapped on the same base address in
// all processes and we run Win32. This is just the fallback method.
HANDLE g_hCtrlEvent = NULL;
volatile LPVOID g_pCtrlRoutineAddr = NULL;
BOOL WINAPI TempCtrlHandler(DWORD dwCtrlType) 
{
   if( CTRL_C_EVENT != dwCtrlType) return FALSE;   
   // Read the stack-base address from the x386 TEB (NtCurrentTeb)
   // then grab the calling address.
   //DWORD* pStackBase = (DWORD*) NtCurrentTeb()->Tib.StackBase;
   #define TEB_STACKBASE_OFFSET 4
   DWORD* pStackBase;
   __asm { mov eax, fs:[TEB_STACKBASE_OFFSET] }
   __asm { mov pStackBase, eax }
   #define PARAM_0_OF_BASE_THEAD_START_OFFSET -3
   g_pCtrlRoutineAddr = (LPVOID) pStackBase[PARAM_0_OF_BASE_THEAD_START_OFFSET];  // Read the parameter off the stack
   ::SetEvent(g_hCtrlEvent);  // Signal that we got it
   return TRUE;
}

BOOL CComSpecProtocol::GenerateConsoleEvent(DWORD dwProcessId, DWORD dwEvent)
{
   BOOL bRes = FALSE;
   if( !bRes ) {
      // First we'll try to just fire the event. This will not work
      // because the console must be attached to our process. Maybe
      // in future versions of Windows.
      bRes = ::GenerateConsoleCtrlEvent(dwEvent, dwProcessId);
   }
   if( !bRes ) {
      // I've discovered that on Vista we can attach the remote process'
      // console and send the event. A good thing that we added the
      // CREATE_NEW_PROCESS_GROUP to the process when created.
      typedef BOOL (WINAPI* PFNATTACHCONSOLE)(DWORD);
      PFNATTACHCONSOLE fnAttachConsole = 
         (PFNATTACHCONSOLE) ::GetProcAddress( ::GetModuleHandle(_T("kernel32.dll")), "AttachConsole" );
      if( fnAttachConsole != NULL ) {
         if( fnAttachConsole(dwProcessId) ) {
            ::GenerateConsoleCtrlEvent(dwEvent, dwProcessId);
            ::FreeConsole();
         }
         bRes = TRUE;
      }
   }
   if( !bRes ) {
      // Finally we'll try to generate the event by calling into the remote
      // process. We'll use the undocumented kernel32.dll|CtrlRoutine() API for this.
      // First we need to snatch the CtrlRoutine() from a locally triggered event.
      // See comments in TempCtrlHandler() above.
      if( g_pCtrlRoutineAddr == NULL ) {
         if( ::AllocConsole() ) {
            g_hCtrlEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
            ::SetConsoleCtrlHandler(TempCtrlHandler, TRUE);
            if( ::GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0) ) ::WaitForSingleObject(g_hCtrlEvent, 4000);
            ::SetConsoleCtrlHandler(TempCtrlHandler, FALSE);
            ::FreeConsole();
            ::CloseHandle(g_hCtrlEvent);
         }
      }
      // Now that we have the CtrlRoutine start-address, we'll inject and run
      // it in the remote process.
      if( g_pCtrlRoutineAddr != NULL ) {
         HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
         if( hProcess != NULL ) {
            DWORD dwThreadId = 0;
            HANDLE hThread = ::CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) g_pCtrlRoutineAddr, (LPVOID) dwEvent, 0, &dwThreadId);
            if( hThread != NULL ) {
               ::CloseHandle(hThread);
               bRes = TRUE;
            }
            ::CloseHandle(hProcess);
         }
      }
   }
   return bRes;
}

