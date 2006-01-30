
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"

#include "RloginProtocol.h"

#pragma code_seg( "PROTOCOLS" )


////////////////////////////////////////////////////////
//

DWORD CRloginThread::Run()
{
   ATLASSERT(m_pManager);
   ATLASSERT(m_pCallback);

   ::SetThreadLocale(_pDevEnv->GetLCID());

   USES_CONVERSION;
   CSocket& socket = m_pManager->m_socket;

   // Get connect parameters
   // TODO: Protect these guys with thread-lock
   CString sHost = m_pManager->GetParam(_T("Host"));
   CString sUsername = m_pManager->GetParam(_T("Username"));
   CString sPassword = m_pManager->GetParam(_T("Password"));
   short iPort = (short) _ttoi( m_pManager->GetParam(_T("Port")) );
   long lSpeed = _ttol( m_pManager->GetParam(_T("Speed")) );
   CString sExtraCommands = m_pManager->GetParam(_T("Extra"));

   if( iPort == 0 ) iPort = 513;         // default rlogin port
   if( lSpeed == 0 ) lSpeed = 38600L;    // default terminal speed
   if( sPassword.IsEmpty() ) sPassword = SecGetPassword();

   m_pManager->m_dwErrorCode = 0;
   m_pManager->m_bConnected = false;

   // Open socket - but we need to bind to a port below 1024, since this
   // is what UNIX will require from a rlogin session...
   // See http://www.ietf.org/rfc/rfc1282.txt?number=1282
   socket.Attach( ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP) );

   // Tune the socket connection
   BOOL nodelay = TRUE;
   ::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*) &nodelay, sizeof(nodelay));
   LINGER linger = { 1, 5 };
   ::setsockopt(socket, SOL_SOCKET, SO_LINGER, (char*) &linger, sizeof(linger));
   BOOL keepalive = TRUE;
   ::setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (char*) &keepalive, sizeof(keepalive));
   BOOL oob = TRUE;
   ::setsockopt(socket, SOL_SOCKET, SO_OOBINLINE, (char*) &oob, sizeof(oob));

   // Bind to a port below 1024
   SOCKADDR_IN a = { 0 };
   a.sin_family = AF_INET;
   a.sin_addr.s_addr = INADDR_ANY;
   
   // Let's try not to go below port 512 so we don't
   // mess with all the default system port numbers.
   static short s_iBindPort = 1024;
   if( s_iBindPort <= 512 ) s_iBindPort = 1024;
   
   while( true ) {
      // Try next port
      s_iBindPort--;
      if( s_iBindPort == 512 ) {
         m_pManager->m_dwErrorCode = WSAEADDRINUSE;
         m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_RLOGINHOST)));
         return 0;
      }
      a.sin_port = htons(s_iBindPort);
      int rc = ::bind(socket, (struct sockaddr*) &a, sizeof(a));
      if( rc != SOCKET_ERROR ) break;
   }

   // Connect to remote server
   SOCKADDR_IN b = { 0 };
   LPCSTR pstrAddress = T2CA(sHost);
   b.sin_addr.s_addr = ::inet_addr(pstrAddress);
   if( b.sin_addr.s_addr == INADDR_NONE ) {
      PHOSTENT pHost = ::gethostbyname(pstrAddress);
      if( pHost == NULL ) return 0;
      ::CopyMemory(&b.sin_addr, pHost->h_addr_list[0], pHost->h_length);
   }
   b.sin_family = AF_INET;
   b.sin_port = htons(iPort);
   if( ::connect(socket, (struct sockaddr*) &b, sizeof(b)) == SOCKET_ERROR ) {
      m_pManager->m_dwErrorCode = ::WSAGetLastError();
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_RLOGINHOST)));
      return 0;
   }

   // Prepare buffers
   const int MAX_BUFFER_SIZE = 400;
   LPBYTE pBuffer = (LPBYTE) malloc(MAX_BUFFER_SIZE);
   BYTE bReadBuffer[MAX_BUFFER_SIZE + 32] = { 0 };
   DWORD dwPos = 0;
   DWORD dwStartLinePos = 0;
   DWORD dwBufferSize = MAX_BUFFER_SIZE;
   bool bInitialized = false;

   CEvent event;
   event.Create();
   ::WSAEventSelect(socket, event, FD_READ);

   HANDLE aHandles[] = { event, m_pManager->m_event };

   // Send login/terminal information
   CHAR szBuffer[400];
   int nLen = ::wsprintfA(szBuffer, "%ls|%ls|%ls|%ls/%ld|", 
      _T(""),       // NULL
      sUsername,    // <client-user-name>
      sUsername,    // <server-user-name>
      _T("vt100"),  // <terminal-type>
      lSpeed);      // <terminal-speed>
   for( LPSTR p = szBuffer; *p; p++ ) if( *p == '|' ) *p = '\0';
   int nSent = ::send(socket, szBuffer, nLen, 0); nSent;

   // NOTE: Like most of these network wrappers this is
   //       a very simplistic client implementation and
   //       we should really spend more time on fulfilling
   //       the RFC specifications.
   // TODO: We are in "cooked" mode and should probably
   //       be concerned about it until told otherwise...
   //       At least we should process "urgent" TCP packets.

   bool bNextIsPrompt = false;         // Next line is likely to be prompt/command line prefix
   VT100COLOR nColor = VT100_DEFAULT;  // Color code from terminal

   while( !ShouldStop() ) {

      DWORD dwRes = ::WaitForMultipleObjects(2, aHandles, FALSE, INFINITE);
      if( ShouldStop() ) break;

      DWORD dwSize = socket.GetAvailableDataCount();

      // We might need to grow the data buffer
      if( dwPos + dwSize > dwBufferSize ) {
         dwBufferSize = dwPos + dwSize;
         pBuffer = (LPBYTE) realloc(pBuffer, dwBufferSize);
         ATLASSERT(pBuffer);
      }

      // While there's data to read, parse stream
      while( dwSize > 0 && !ShouldStop() ) {
         DWORD dwRead = 0;
         socket.Read(bReadBuffer, min(dwSize, MAX_BUFFER_SIZE), &dwRead);
         if( dwRead == 0 ) {
            DWORD dwErr = ::WSAGetLastError();
            switch( dwErr ) {
            case WSAECONNRESET:
            case WSAECONNABORTED:
               m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_ABORTED)));
               SignalStop();
               break;
            case WSAEINPROGRESS:
            case WSAEWOULDBLOCK:
               break;
            default:
               ATLASSERT(dwRead>0);
            }
            break;
         }
         dwSize -= dwRead;

#ifdef _DEBUG
         bReadBuffer[dwRead] = '\0';
         ATLTRACE(_T("RLOGIN: '%hs' len=%ld\n"), bReadBuffer, dwRead);
#endif

         if( !bInitialized ) 
         {
            CString sLine = _GetLine(bReadBuffer, 0, dwRead);
            sLine.MakeUpper();
            // Server answered with 0-byte?
            if( sLine.IsEmpty() ) {
               if( dwRead > 0 ) {
                  // Skip the 0-answer
                  BYTE b = _GetByte(socket, bReadBuffer, dwRead, dwPos);
                  if( b == 0 ) {
                     if( dwRead > 1 ) {
                        // Negotiate window size may be initiated
                        b = _GetByte(socket, bReadBuffer, dwRead, dwPos);
                        if( b == 0x80 ) {
                           CLockStaticDataInit lock;
                           int iTermWidth = 80;
                           int iTermHeight = 24;
                           BYTE b[12] = { 0xFF, 0xFF, 0x73, 0x73, 0, 0, 0, 0, 0, 0, 0, 0 };
                           b[4] = (BYTE) ((iTermHeight >> 8) & 0xFF);
                           b[5] = (BYTE) (iTermHeight & 0xFF);
                           b[6] = (BYTE) ((iTermWidth >> 8) & 0xFF);
                           b[7] = (BYTE) (iTermWidth & 0xFF);
                           socket.Write(b, sizeof(b));
                           m_pManager->m_bCanWindowSize = true;
                        }
                     }
                     bInitialized = true;
                  }
               }
            }
         }
         else if( !m_pManager->m_bConnected ) 
         {
            CString sLine = _GetLine(bReadBuffer, 0, dwRead);
            sLine.MakeUpper();
            // Did authorization fail?
            static LPCTSTR pstrFailed[] =
            {
               _T("AUTHORIZATION FAILED"),
               _T("PERMISSION DENIED"),
               _T("INVALID PASSWORD"),
               _T("LOGIN INCORRECT"),
               _T("LOGIN FAILED"),
               _T("UNKNOWN USER"),
               NULL,
            };
            const LPCTSTR* ppstr = pstrFailed;
            while( *ppstr ) {
               if( sLine.Find(*ppstr) >= 0 ) {
                  m_pManager->m_pProject->DelayedMessage(CString(MAKEINTRESOURCE(IDS_ERR_LOGIN_FAILED)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONERROR | MB_MODELESS);
                  m_pManager->m_dwErrorCode = ERROR_LOGIN_WKSTA_RESTRICTION;
                  SignalStop();
                  break;
               }
               ppstr++;
            }
            if( sLine.Find(_T("LOGIN:")) >= 0 ) {
               CLockStaticDataInit lock;
               CHAR szBuffer[200];
               ::wsprintfA(szBuffer, "%ls\r\n", sUsername);
               ::send(socket, szBuffer, strlen(szBuffer), 0);
               ::Sleep(500L);
            }
            else if( sLine.Find(_T("PASSWORD:")) >= 0 ) {
               CLockStaticDataInit lock;
               CHAR szBuffer[200];
               ::wsprintfA(szBuffer, "%ls\r\n", sPassword);
               ::send(socket, szBuffer, strlen(szBuffer), 0);
            }
            else if( *ppstr == NULL ) {
               // Not a login prompt or error; we assume it's the login banner and
               // we successfully entered the system.
               // Send additional login commands
               if( !sExtraCommands.IsEmpty() ) {
                  CLockStaticDataInit lock;
                  CHAR szBuffer[1024];
                  ::wsprintfA(szBuffer, "%ls\r\n", sExtraCommands);
                  ::send(socket, szBuffer, strlen(szBuffer), 0);
               }
               m_pManager->m_bConnected = true;
            }
         }

         DWORD iPos = 0;
         while( iPos < dwRead ) {
            ATLASSERT(iPos<sizeof(bReadBuffer));
            BYTE b = bReadBuffer[iPos++];
            switch( b ) {
            case '\0':
            case '\0x1':
            case '\0x2':
            case '\0x10':
               // TODO: Handle flow-control and other Rlogin features!
               break;
            case '\r':
            case '\0x7':
               // Ignore...
               break;
            case 0x08:
               if( dwPos > 0 ) dwPos--;
               break;
            case '\t':
               *(pBuffer + dwPos) = ' ';
               dwPos++;
               break;
            case 27:
               {
                  // Urgh, ANSI escape code for coloring and stuff.
                  // Let's just filter them out.
                  b = _GetByte(socket, bReadBuffer, dwRead, iPos); // skip [
                  if( b == ']' ) 
                  {
                     // Parse OSC
                     while( true ) {
                        b = _GetByte(socket, bReadBuffer, dwRead, iPos);
                        if( b == 94 || b == 7 || b == 0 ) break;
                     }
                     bNextIsPrompt = true;
                  }
                  else
                  {
                     // Parse CSI
                     b = _GetByte(socket, bReadBuffer, dwRead, iPos);
                     bool bNewLine = false;
                     int iValue = 0;
                     while( isdigit(b) || b == ';' ) {
                        iValue = (iValue * 10) + (b - '0');
                        b = _GetByte(socket, bReadBuffer, dwRead, iPos);
                     }
                     if( b == 'H' ) bNewLine = true;
                     if( b == 'm' && nColor == 0 ) nColor = (VT100COLOR) iValue;
                     if( !bNewLine ) break;
                  }
               }
               // FALL THROUGH
            case '\n':
               {
                  CString sLine = _GetLine(pBuffer, dwStartLinePos, dwPos);
                  dwPos = 0;
                  dwStartLinePos = 0;
                  if( bNextIsPrompt ) {
                     if( sLine.IsEmpty() ) break;
                     bNextIsPrompt = false;
                     if( sLine.Find(TERM_MARKER) < 0 ) break;
                  }
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
   }

   socket.Close();

   free(pBuffer);

   m_pManager->m_bConnected = false;
   return 0;
};

CHAR CRloginThread::_GetByte(CSocket& socket, const LPBYTE pBuffer, DWORD dwRead, DWORD& iPos) const
{
   ATLASSERT(pBuffer);
   // First check if the cache contains the data
   if( iPos < dwRead ) return pBuffer[iPos++];
   // Ok, we need to receive the data!
   // BUG: Could hang the system in a blocking loop!!!
   CHAR b = 0;
   BOOL bRet = socket.Read(&b, 1, NULL);
   switch( ::WSAGetLastError() ) {
   case WSAEINPROGRESS:
   case WSAEWOULDBLOCK:
      // Oops, socket is busy. This is a really bad time for this
      // to happen (but not unusual) because the function really
      // expects to return some data! Try again...
      // BUG: This is bad code.
      ::Sleep(100L);
      bRet = socket.Read(&b, 1, NULL);
      break;
   }
   ATLASSERT(bRet); bRet;
   return b;
}

CString CRloginThread::_GetLine(LPBYTE pBuffer, DWORD dwStart, DWORD dwEnd) const
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

CRloginProtocol::CRloginProtocol() :
   m_dwErrorCode(0),   
   m_bConnected(false),
   m_lSpeed(38400L),
   m_lConnectTimeout(0),
   m_lPort(0)
{
   Clear();
   m_event.Create();
}

CRloginProtocol::~CRloginProtocol()
{
   Stop();
}

void CRloginProtocol::Init(CRemoteProject* pProject, IShellCallback* pCallback)
{
   ATLASSERT(pProject);
   ATLASSERT(pCallback);
   m_pProject = pProject;
   m_pCallback = pCallback;
}

void CRloginProtocol::Clear()
{
   m_sHost.Empty();
   m_sUsername.Empty();
   m_sPassword.Empty();
   m_lPort = 0;
   m_lSpeed = 38400L;
   m_sPath.Empty();
   m_sExtraCommands.Empty();
   m_lConnectTimeout = 8;
   m_bCanWindowSize = false;
}

bool CRloginProtocol::Load(ISerializable* pArc)
{
   Clear();

   if( !pArc->ReadItem(_T("Rlogin")) ) return false;
   pArc->Read(_T("host"), m_sHost.GetBufferSetLength(128), 128);
   m_sHost.ReleaseBuffer();
   pArc->Read(_T("port"), m_lPort);
   pArc->Read(_T("speed"), m_lSpeed);
   pArc->Read(_T("user"), m_sUsername.GetBufferSetLength(128), 128);
   m_sUsername.ReleaseBuffer();
   pArc->Read(_T("password"), m_sPassword.GetBufferSetLength(128), 128);
   m_sPassword.ReleaseBuffer();
   pArc->Read(_T("path"), m_sPath.GetBufferSetLength(MAX_PATH), MAX_PATH);
   m_sPath.ReleaseBuffer();
   pArc->Read(_T("extra"), m_sExtraCommands.GetBufferSetLength(200), 200);
   m_sExtraCommands.ReleaseBuffer();
   pArc->Read(_T("connectTimeout"), m_lConnectTimeout);

   ConvertToCrLf(m_sExtraCommands);
   if( m_lConnectTimeout == 0 ) m_lConnectTimeout = 8;
   
   return true;
}

bool CRloginProtocol::Save(ISerializable* pArc)
{
   if( !pArc->WriteItem(_T("Rlogin")) ) return false;
   pArc->Write(_T("host"), m_sHost);
   pArc->Write(_T("port"), m_lPort);
   pArc->Write(_T("speed"), m_lSpeed);
   pArc->Write(_T("user"), m_sUsername);
   pArc->Write(_T("password"), m_sPassword);
   pArc->Write(_T("path"), m_sPath);
   pArc->Write(_T("extra"), ConvertFromCrLf(m_sExtraCommands));
   pArc->Write(_T("connectTimeout"), m_lConnectTimeout);
   return true;
}

bool CRloginProtocol::Start()
{
   Stop();
   m_thread.m_pManager = this;
   m_thread.m_pCallback = m_pCallback;
   if( !m_thread.Start() ) return false;
   return true;
}

bool CRloginProtocol::Stop()
{
   SignalStop();
   m_thread.Stop();
   m_socket.Close();
   m_dwErrorCode = 0;
   m_bConnected = false;
   return true;
}

void CRloginProtocol::SignalStop()
{
   m_thread.SignalStop();
   m_event.SetEvent();
}

bool CRloginProtocol::IsConnected() const
{
   return m_bConnected;
}

bool CRloginProtocol::IsBusy() const
{
   return m_thread.IsRunning() == TRUE;
}

CString CRloginProtocol::GetParam(LPCTSTR pstrName) const
{
   CString sName = pstrName;
   if( sName == _T("Path") ) return m_sPath;
   if( sName == _T("Host") ) return m_sHost;
   if( sName == _T("Username") ) return m_sUsername;
   if( sName == _T("Password") ) return m_sPassword;
   if( sName == _T("Port") ) return ToString(m_lPort);
   if( sName == _T("Speed") ) return ToString(m_lSpeed);
   if( sName == _T("Extra") ) return m_sExtraCommands;
   if( sName == _T("Type") ) return _T("RLogin");
   if( sName == _T("ConnectTimeout") ) return ToString(m_lConnectTimeout);
   return _T("");
}

void CRloginProtocol::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   if( sName == _T("Path") ) m_sPath = pstrValue;
   if( sName == _T("Host") ) m_sHost = pstrValue;
   if( sName == _T("Username") ) m_sUsername = pstrValue;
   if( sName == _T("Password") ) m_sPassword = pstrValue;
   if( sName == _T("Port") ) m_lPort = _ttol(pstrValue);
   if( sName == _T("Speed") ) m_lSpeed = _ttol(pstrValue);
   if( sName == _T("Extra") ) m_sExtraCommands = pstrValue;
   if( sName == _T("ConnectTimeout") ) m_lConnectTimeout = _ttol(pstrValue);
   if( m_lConnectTimeout <= 0 ) m_lConnectTimeout = 8;
}

bool CRloginProtocol::WriteData(LPCTSTR pstrData)
{
   ATLASSERT(!::IsBadStringPtr(pstrData,-1));
   if( m_socket.IsNull() ) return false;
   if( m_thread.ShouldStop() ) return false;
   if( !WaitForConnection() ) {
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_CONNECTERROR)));
      return false;
   }
   int nLen = _tcslen(pstrData);
   LPSTR pstr = (LPSTR) _alloca(nLen + 2);
   ATLASSERT(pstr);
   AtlW2AHelper(pstr, pstrData, nLen + 1);
   strcpy(pstr + nLen, "\r\n");
   CLockStaticDataInit lock;
   if( !m_socket.Write(pstr, nLen + 1) ) {
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_SENDERROR)));
      return false;
   }
   return true;
}

bool CRloginProtocol::WriteSignal(BYTE bCmd)
{
   if( m_socket.IsNull() ) return false;
   if( m_thread.ShouldStop() ) return false;
   if( !WaitForConnection() ) return false;
   switch( bCmd ) {
   case TERMINAL_BREAK:
      bCmd = 3;
      break;
   default:
      ATLASSERT(false);
      return false;
   }
   CLockStaticDataInit lock;
   if( !m_socket.Write(&bCmd, sizeof(bCmd), NULL, MSG_OOB) ) {
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_SENDERROR)));
      return false;
   }
   return true;
}

bool CRloginProtocol::WriteScreenSize(int w, int h)
{
   if( !m_bCanWindowSize ) return false;
   if( m_socket.IsNull() ) return false;
   if( m_thread.ShouldStop() ) return false;
   if( !WaitForConnection() ) return false;
   // Send "Window Size Marker" down the data-stream.
   BYTE b[12] = { 0xFF, 0xFF, 0x73, 0x73, 0, 0, 0, 0, 0, 0, 0, 0 };
   b[4] = (BYTE) ((w >> 8) & 0xFF);
   b[5] = (BYTE) (w & 0xFF);
   b[6] = (BYTE) ((h >> 8) & 0xFF);
   b[7] = (BYTE) (h & 0xFF);
   m_socket.Write(b, sizeof(b));
   return true;
}

bool CRloginProtocol::WaitForConnection()
{
   // Wait for the thread to connect to the Rlogin host.
   const DWORD SPAWNTIMEOUT = 2;
   DWORD dwTickStart = ::GetTickCount();
   BOOL bHasRun = FALSE;
   int iCount = 0;
   while( !m_bConnected ) {

      // Did the server return some kind of error?
      if( m_dwErrorCode != 0 ) {
         ::SetLastError(m_dwErrorCode);
         return false;
      }

      // Idle wait a bit
      ::Sleep(200L);

      DWORD dwTick = ::GetTickCount();

      // Timeout after 8 sec.
      if( dwTick - dwTickStart > (DWORD) m_lConnectTimeout * 1000UL ) {
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

      PumpIdleMessages();
   }
   ATLASSERT(!m_socket.IsNull());
   return true;
}

