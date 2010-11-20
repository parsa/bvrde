
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"

#include "Authen.h"

#include "TelnetProtocol.h"

#pragma code_seg( "PROTOCOLS" )


////////////////////////////////////////////////////////
//

DWORD CTelnetThread::Run()
{
   ATLASSERT(m_pManager);
   ATLASSERT(m_pCallback);

   _pDevEnv->SetThreadLanguage();

   CSocket& socket = m_pManager->m_socket;

   // Get connect parameters
   // TODO: Protect these guys with thread-lock
   CString sHost = m_pManager->GetParam(_T("Host"));
   CString sUsername = m_pManager->GetParam(_T("Username"));
   CString sPassword = m_pManager->GetParam(_T("Password"));
   u_short iPort = (u_short) _ttol( m_pManager->GetParam(_T("Port")) );
   CString sPath = m_pManager->GetParam(_T("Path"));
   CString sExtraCommands = m_pManager->GetParam(_T("Extra"));
   CString sLoginPrompt = m_pManager->GetParam(_T("LoginPrompt"));
   CString sPasswordPrompt = m_pManager->GetParam(_T("PasswordPrompt"));

   if( iPort == 0 ) iPort = 23; // default telnet port

   // Prompt for password?
   if( sPassword.IsEmpty() ) sPassword = SecGetPassword();

   m_pManager->m_dwErrorCode = 0;
   m_pManager->m_bConnected = false;

   BOOL bRes = socket.Open(iPort, sHost);
   if( !bRes ) {
      m_pManager->m_dwErrorCode = ::WSAGetLastError();
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_TELNETHOST)));
      socket.Close();
      return 0;
   }

   // Tune the socket connection
   int nodelay = TRUE;
   ::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*) &nodelay, sizeof(nodelay));
   LINGER linger = { 1, 5 };
   ::setsockopt(socket, SOL_SOCKET, SO_LINGER, (char*) &linger, sizeof(linger));
   int keepalive = TRUE;
   ::setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (char*) &keepalive, sizeof(keepalive));
   int oob = TRUE;
   ::setsockopt(socket, SOL_SOCKET, SO_OOBINLINE, (char*) &oob, sizeof(oob));

   // Prepare buffers
   const int MAX_BUFFER_SIZE = 400;
   LPBYTE pBuffer = (LPBYTE) malloc(MAX_BUFFER_SIZE);
   BYTE bReadBuffer[MAX_BUFFER_SIZE + 32] = { 0 };
   DWORD dwPos = 0;
   DWORD dwStartLinePos = 0;
   DWORD dwBufferSize = MAX_BUFFER_SIZE;

   // Prepare negotiation and states...
   BYTE aNegotiated[256] = { 0 };
   bool bNegotiated = false;       // Initial client settings negotiated?
   bool bNextIsPrompt = false;     // Next line is likely to be prompt/command line prefix
   bool bAuthenticated = false;    // Session fully authenticated?
   bool bLoginSent = false;        // Seen Login prompt?
   bool bPasswordSent = false;     // Seen Password prompt?

   CEvent event;
   event.Create();
   ::WSAEventSelect(socket, event, FD_READ);

   HANDLE aHandles[] = { event, m_pManager->m_event };

   while( !ShouldStop() ) 
   {
      DWORD dwRes = ::WaitForMultipleObjects(2, aHandles, FALSE, INFINITE);
      if( ShouldStop() ) break;

      DWORD dwSize = socket.GetAvailableDataCount();

      // We might need to grow the data buffer
      if( dwPos + dwSize > dwBufferSize ) {
         dwBufferSize = dwPos + dwSize;
         pBuffer = (LPBYTE) realloc(pBuffer, dwBufferSize);
         ATLASSERT(pBuffer);
      }

      CSimpleValArray<BYTE> aSend;
      VT100COLOR nColor = VT100_DEFAULT;

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
         ATLTRACE(_T("TELNET: '%hs' len=%ld\n"), bReadBuffer, dwRead);
#endif // _DEBUG

         if( !bAuthenticated ) 
         {
            CString sLine = _GetLine(bReadBuffer, 0, dwRead);
            m_pCallback->PreAuthenticatedLine(sLine);
            sLine.MakeUpper();
            if( !bLoginSent && sLine.Find(sLoginPrompt) >= 0 ) {
               // Need to send login?
               CLockStaticDataInit lock;
               CHAR szBuffer[200] = { 0 };
               ::wnsprintfA(szBuffer, 199, "%ls\r\n", sUsername);
               socket.Write(szBuffer, strlen(szBuffer));
               ::Sleep(100L);
               bLoginSent = true;
            }
            if( !bPasswordSent && sLine.Find(sPasswordPrompt) >= 0 ) {
               // Need to send password?
               CLockStaticDataInit lock;
               CHAR szBuffer[200] = { 0 };
               ::wnsprintfA(szBuffer, 199, "%ls\r\n", sPassword);
               socket.Write(szBuffer, strlen(szBuffer));
               ::Sleep(100L);
               bPasswordSent = true;
               bAuthenticated = true;
            }
         }
         else if( !m_pManager->m_bConnected ) 
         {
            CString sLine = _GetLine(bReadBuffer, 0, dwRead);
            sLine.MakeUpper();
            // Did authorization fail?
            // BUG: Localization!!!
            static LPCTSTR aFailed[] =
            {
               _T("AUTHORIZATION FAILED"),
               _T("PERMISSION DENIED"),
               _T("INVALID PASSWORD"),
               _T("LOGIN INCORRECT"),
               _T("LOGIN FAILED"),
               _T("UNKNOWN USER"),
            };
            for( int i = 0; i < sizeof(aFailed) / sizeof(aFailed[0]); i++ ) {
               if( sLine.Find(aFailed[i]) >= 0 ) {
                  m_pManager->m_pProject->DelayedMessage(CString(MAKEINTRESOURCE(IDS_ERR_LOGIN_FAILED)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONERROR | MB_MODELESS);
                  m_pManager->m_dwErrorCode = ERROR_LOGIN_WKSTA_RESTRICTION;
                  SignalStop();
                  break;
               }
            }
            // Send our own post-login commands
            if( !sExtraCommands.IsEmpty() ) {
               CLockStaticDataInit lock;
               CHAR szBuffer[1025] = { 0 };
               ::wnsprintfA(szBuffer, 1024, "%ls\r\n", sExtraCommands);
               socket.Write(szBuffer, strlen(szBuffer));
               ::Sleep(100L);
            }
            m_pManager->m_bConnected = true;
         }

         DWORD iPos = 0;
         while( iPos < dwRead ) {
            ATLASSERT(iPos<sizeof(bReadBuffer));
            BYTE b = bReadBuffer[iPos++];
            switch( b ) {
            case 0x07:
            case '\v':
            case '\f':
            case '\r':
            case TELNET_NOP:
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
                     // Skip OSC
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
            case '\0':
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
            case TELNET_IAC:
               b = _GetByte(socket, bReadBuffer, dwRead, iPos);
               switch( b ) {
               case TELNET_XEOF:
                  SignalStop();
                  break;
               case TELNET_IAC:
                  *(pBuffer + dwPos) = b;  // NOTE: variable 'b' still contains IAC (escape)
                  dwPos++;
                  break;
               case TELNET_DO:
               case TELNET_DONT:
                  {
                     ATLTRACE(_T("TELNET %s: %ld\n"), b == TELNET_DO ? _T("DO") : _T("DONT"), (long) bReadBuffer[iPos]);
                     BYTE nCmd = _GetByte(socket, bReadBuffer, dwRead, iPos);
                     if( (aNegotiated[nCmd] & NEGOTIATED_WILL) == 0) {
                        // Send answer to server
                        BYTE nCode = TELNET_WONT;
                        switch( nCmd ) {
                        case TELOPT_TERM:
                        case TELOPT_NAWS:
                           nCode = TELNET_WILL;
                           break;
                        }
                        aSend.Add( (BYTE) TELNET_IAC );
                        aSend.Add( nCode );
                        aSend.Add( nCmd );

                        // Send window size immediately
                        if( nCmd == TELOPT_NAWS && nCode == TELNET_WILL ) 
                        {
                           const WORD SCR_WIDTH = 80;
                           const WORD SCR_HEIGHT = 24;
                           aSend.Add( (BYTE) TELNET_IAC );
                           aSend.Add( (BYTE) TELNET_SB );
                           aSend.Add( nCmd );
                           aSend.Add( (BYTE) ((SCR_WIDTH >> 8) & 0xFF) );
                           aSend.Add( (BYTE) (SCR_WIDTH & 0xFF) );
                           aSend.Add( (BYTE) ((SCR_HEIGHT >> 8) & 0xFF) );
                           aSend.Add( (BYTE) (SCR_HEIGHT & 0xFF) );
                           aSend.Add( (BYTE) TELNET_IAC );
                           aSend.Add( (BYTE) TELNET_SE );
                           m_pManager->m_bCanWindowSize = true;
                        }
                     }
                  }
                  break;
               case TELNET_WILL:
               case TELNET_WONT:
                  {
                     ATLTRACE(_T("TELNET %s: %ld\n"), b == TELNET_WILL ? _T("WILL") : _T("WONT"), (long) bReadBuffer[iPos]);
                     BYTE nCmd = _GetByte(socket, bReadBuffer, dwRead, iPos);
                     if( (aNegotiated[nCmd] & NEGOTIATED_DO) == 0 ) {
                        // Deny everything
                        aSend.Add( (BYTE) TELNET_IAC );
                        aSend.Add( TELNET_DONT );
                        aSend.Add( nCmd );
                     }
                  }
                  break;
               case TELNET_SB:
                  {
                     ATLTRACE(_T("TELNET SB: %ld %ld\n"), (long) bReadBuffer[iPos], (long) bReadBuffer[iPos+1]);
                     BYTE bType = _GetByte(socket, bReadBuffer, dwRead, iPos);
                     BYTE bCmd = _GetByte(socket, bReadBuffer, dwRead, iPos);
                     _GetByte(socket, bReadBuffer, dwRead, iPos); // Skip IAC
                     _GetByte(socket, bReadBuffer, dwRead, iPos); // Skip SE
                     switch( bType ) {
                     case TELOPT_TERM:
                        if( bCmd == 1 ) {
                           aSend.Add( (BYTE) TELNET_IAC );
                           aSend.Add( (BYTE) TELNET_SB );
                           aSend.Add( bType );
                           aSend.Add( 0 );
                           // NOTE: We could send "VT100" here because even the most
                           //       crappy TELNET server will understand this. Actually
                           //       the DUMB terminal is mentioned in the original RFC
                           //       docs and should be good enough for all servers!
                           // RANT: No, not for LINUX - it still sends its stupid
                           //       ANSI escape codes....
                           aSend.Add('D');
                           aSend.Add('U');
                           aSend.Add('M');
                           aSend.Add('B');
                           aSend.Add( (BYTE) TELNET_IAC );
                           aSend.Add( (BYTE) TELNET_SE );
                        }
                        break;
                     case TELOPT_RFC:
                        break;
                     default:
                        ATLASSERT(false);
                        break;
                     }
                  }
                  break;
               case TELNET_GA:
               case TELNET_EOR:
                  ATLTRACE("TELNET EOR\n");
                  break;
               case TELNET_DM:
               case TELNET_NOP:
                  break;
               default:
                  ATLASSERT(false);
                  break;
               }
               ATLASSERT(iPos<=dwRead);
               break;
            default:
               *(pBuffer + dwPos) = b;
               dwPos++;
            }
         }
      }

      if( !bNegotiated ) {
         // Negotiate some options we'd like to have...
         ATLTRACE("TELNET - Sending negotiation parameters...\n");
         _NegotiateOption(aNegotiated, aSend, TELNET_DO, TELOPT_BIN);
         _NegotiateOption(aNegotiated, aSend, TELNET_DO, TELOPT_LINE);
         _NegotiateOption(aNegotiated, aSend, TELNET_DO, TELOPT_ECHO);
         _NegotiateOption(aNegotiated, aSend, TELNET_WILL, TELOPT_NAWS);
         bNegotiated = true;
      }

      if( aSend.GetSize() > 0 ) {
         CLockStaticDataInit lock;
         socket.Write(aSend.GetData(), aSend.GetSize());
         aSend.RemoveAll();
      }
   }

   // HACK: Some telnet servers will not abort properly when connection
   //       is killed, so we send a stream of logoff attempts:
   //       exit - TELNET_IAC DO TELOPT_LOGO - TELNET_ABORT - TELNET_XEOF
   LPSTR pstrExit = "exit\0x0FF\x0FD\x012\x0FF\x0EE\x0FF\x0EC\x004\r\n";
   socket.Write(pstrExit, sizeof(pstrExit));
   //       hurry - TELNET_DM
   LPSTR pstrHurry = "\0xFF\0xF2";
   socket.Write(pstrHurry, sizeof(pstrHurry), NULL, MSG_OOB);

   socket.Close();

   free(pBuffer);

   m_pManager->m_bConnected = false;
   return 0;
}

CHAR CTelnetThread::_GetByte(CSocket& socket, const LPBYTE pBuffer, DWORD dwRead, DWORD& iPos) const
{
   ATLASSERT(pBuffer);
   // First check if the cache contains the data
   if( iPos < dwRead ) return pBuffer[iPos++];
   // Ok, we need to receive more data!
   CHAR b = 0;
   if( !socket.WaitForData(200) ) return b;
   BOOL bRet = socket.Read(&b, 1, NULL);
   switch( ::WSAGetLastError() ) {
   case WSAEINPROGRESS:
   case WSAEWOULDBLOCK:
      ::Sleep(100L);
      bRet = socket.Read(&b, 1, NULL);
      break;
   }
   ATLASSERT(bRet); bRet;
   return b;
}

CString CTelnetThread::_GetLine(LPBYTE pBuffer, DWORD dwStart, DWORD dwEnd) const
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

void CTelnetThread::_NegotiateOption(LPBYTE pList, CSimpleValArray<BYTE>& aSend, BYTE bAction, BYTE nCmd)
{
   aSend.Add( (BYTE) TELNET_IAC );
   aSend.Add( bAction );
   aSend.Add( nCmd );
   pList[nCmd] |= bAction == TELNET_DO ? NEGOTIATED_DO : NEGOTIATED_WILL;
}


////////////////////////////////////////////////////////
//

CTelnetProtocol::CTelnetProtocol() :
   m_bConnected(false),
   m_lConnectTimeout(0),
   m_lPort(0)
{
   Clear();
   m_event.Create();
}

CTelnetProtocol::~CTelnetProtocol()
{
   Stop();
}

void CTelnetProtocol::Init(CRemoteProject* pProject, IShellCallback* pCallback)
{
   ATLASSERT(pProject);
   ATLASSERT(pCallback);
   m_pProject = pProject;
   m_pCallback = pCallback;
}

void CTelnetProtocol::Clear()
{
   m_sHost.Empty();
   m_sUsername.Empty();
   m_sPassword.Empty();
   m_lPort = 0;
   m_sPath.Empty();
   m_sExtraCommands.Empty();
   m_sLoginPrompt = _T("LOGIN");
   m_sPasswordPrompt = _T("PASSWORD");
   m_lConnectTimeout = 8;
   m_bCanWindowSize = false;
}

bool CTelnetProtocol::Load(ISerializable* pArc)
{
   Clear();

   if( !pArc->ReadItem(_T("Telnet")) ) return false;
   pArc->Read(_T("host"), m_sHost.GetBufferSetLength(128), 128);
   m_sHost.ReleaseBuffer();
   pArc->Read(_T("port"), m_lPort);
   pArc->Read(_T("user"), m_sUsername.GetBufferSetLength(128), 128);
   m_sUsername.ReleaseBuffer();
   pArc->Read(_T("password"), m_sPassword.GetBufferSetLength(128), 128);
   m_sPassword.ReleaseBuffer();
   pArc->Read(_T("path"), m_sPath.GetBufferSetLength(MAX_PATH), MAX_PATH);
   m_sPath.ReleaseBuffer();
   pArc->Read(_T("extra"), m_sExtraCommands.GetBufferSetLength(1400), 1400);
   m_sExtraCommands.ReleaseBuffer();
   pArc->Read(_T("loginPrompt"), m_sLoginPrompt.GetBufferSetLength(100), 100);
   m_sLoginPrompt.ReleaseBuffer();
   pArc->Read(_T("passwordPrompt"), m_sPasswordPrompt.GetBufferSetLength(100), 100);
   m_sPasswordPrompt.ReleaseBuffer();
   pArc->Read(_T("connectTimeout"), m_lConnectTimeout);

   ConvertToCrLf(m_sExtraCommands);
   m_sPassword = SecDecodePassword(m_sPassword);
   if( m_lConnectTimeout <= 0 ) m_lConnectTimeout = 8;
   if( m_sLoginPrompt.IsEmpty() ) m_sLoginPrompt = _T("LOGIN");
   if( m_sPasswordPrompt.IsEmpty() ) m_sPasswordPrompt = _T("PASSWORD");

   return true;
}

bool CTelnetProtocol::Save(ISerializable* pArc)
{
   if( !pArc->WriteItem(_T("Telnet")) ) return false;
   pArc->Write(_T("host"), m_sHost);
   pArc->Write(_T("port"), m_lPort);
   pArc->Write(_T("user"), m_sUsername);
   pArc->Write(_T("password"), SecEncodePassword(m_sPassword));
   pArc->Write(_T("path"), m_sPath);
   pArc->Write(_T("extra"), ConvertFromCrLf(m_sExtraCommands));
   pArc->Write(_T("loginPrompt"), m_sLoginPrompt);
   pArc->Write(_T("passwordPrompt"), m_sPasswordPrompt);
   pArc->Write(_T("connectTimeout"), m_lConnectTimeout);
   return true;
}

bool CTelnetProtocol::Start()
{
   Stop();
   m_event.ResetEvent();
   m_thread.m_pManager = this;
   m_thread.m_pCallback = m_pCallback;
   if( !m_thread.Start() ) return false;
   return true;
}

bool CTelnetProtocol::Stop()
{
   SignalStop();
   m_thread.Stop();
   m_socket.Close();
   m_dwErrorCode = 0;
   m_bConnected = false;
   return true;
}

void CTelnetProtocol::SignalStop()
{
   m_thread.SignalStop();
   m_event.SetEvent();
}

bool CTelnetProtocol::IsConnected() const
{
   return m_bConnected;
}

bool CTelnetProtocol::IsBusy() const
{
   return m_thread.IsRunning() == TRUE;
}

CString CTelnetProtocol::GetParam(LPCTSTR pstrName) const
{
   CString sName = pstrName;
   if( sName == _T("Type") ) return _T("Telnet");
   if( sName == _T("Path") ) return m_sPath;
   if( sName == _T("Host") ) return m_sHost;
   if( sName == _T("Port") ) return ToString(m_lPort);
   if( sName == _T("Username") ) return m_sUsername;
   if( sName == _T("Password") ) return m_sPassword;
   if( sName == _T("Extra") ) return m_sExtraCommands;
   if( sName == _T("LoginPrompt") ) return m_sLoginPrompt;
   if( sName == _T("PasswordPrompt") ) return m_sPasswordPrompt;
   if( sName == _T("ConnectTimeout") ) return ToString(m_lConnectTimeout);
   return _T("");
}

void CTelnetProtocol::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   if( sName == _T("Path") ) m_sPath = pstrValue;
   if( sName == _T("Host") ) m_sHost = pstrValue;
   if( sName == _T("Username") ) m_sUsername = pstrValue;
   if( sName == _T("Password") ) m_sPassword = pstrValue;
   if( sName == _T("Port") ) m_lPort = _ttol(pstrValue);
   if( sName == _T("Extra") ) m_sExtraCommands = pstrValue;
   if( sName == _T("LoginPrompt") ) m_sLoginPrompt = pstrValue;
   if( sName == _T("PasswordPrompt") ) m_sPasswordPrompt = pstrValue;
   if( sName == _T("ConnectTimeout") ) m_lConnectTimeout = _ttol(pstrValue);
   if( m_lConnectTimeout <= 0 ) m_lConnectTimeout = 8;
}

bool CTelnetProtocol::WriteData(LPCTSTR pstrData)
{
   ATLASSERT(!::IsBadStringPtr(pstrData,-1));
   if( m_socket.IsNull() ) return false;
   if( m_thread.ShouldStop() ) return false;
   if( !WaitForConnection() ) {
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_CONNECTERROR)));
      return false;
   }
   size_t nLen = (_tcslen(pstrData) * 2) + 3;  // MBCS + \r\n + \0
   LPSTR pstr = (LPSTR) _alloca(nLen);
   ATLASSERT(pstr);
   if( pstr == NULL ) return false;
   ::ZeroMemory(pstr, nLen);
   AtlW2AHelper(pstr, pstrData, (int) nLen - 3);
   strcat(pstr, "\r\n");
   CLockStaticDataInit lock;
   if( !m_socket.Write(pstr, strlen(pstr)) ) {
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_SENDERROR)));
      return false;
   }
   return true;
}

bool CTelnetProtocol::WriteSignal(BYTE bCmd)
{
   if( m_socket.IsNull() ) return false;
   if( m_thread.ShouldStop() ) return false;
   if( !WaitForConnection() ) return false;
   switch( bCmd ) {
   case TERMINAL_BREAK:
      bCmd = TELNET_IP;
      break;
   case TERMINAL_QUIT:
      bCmd = TELNET_ABORT;
      break;
   default:
      ATLASSERT(false);
      return false;
   }
   CLockStaticDataInit lock;
   BYTE bData[] = { TELNET_IAC, bCmd };
   if( !m_socket.Write(bData, sizeof(bData)) ) {
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_SENDERROR)));
      return false;
   }
   bData[1] = TELNET_DM;
   m_socket.Write(bData, sizeof(bData), NULL, MSG_OOB);
   return true;
}

bool CTelnetProtocol::WriteScreenSize(int w, int h)
{
   if( !m_bCanWindowSize ) return false;
   if( m_socket.IsNull() ) return false;
   if( m_thread.ShouldStop() ) return false;
   if( !WaitForConnection() ) {
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_CONNECTERROR)));
      return false;
   }
   BYTE bData[] = 
   {      
      TELNET_IAC, 
      TELNET_SB,
      TELOPT_NAWS,
      (BYTE)((w >> 8) & 0xFF),
      (BYTE)(w & 0xFF),
      (BYTE)((h >> 8) & 0xFF),
      (BYTE)(h & 0xFF),
      TELNET_IAC, 
      TELNET_SE,
   };
   CLockStaticDataInit lock;
   if( !m_socket.Write(bData, sizeof(bData)) ) {
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_SENDERROR)));
      return false;
   }
   return true;
}

bool CTelnetProtocol::WaitForConnection()
{
   // Wait for the thread to connect to the TELNET host.
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

      // Wait a little while
      PumpIdleMessages(200L);

      DWORD dwTick = ::GetTickCount();

      // Timeout after x sec.
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
   }
   ATLASSERT(!m_socket.IsNull());
   return true;
}

