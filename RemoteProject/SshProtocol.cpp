
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"

#include "SshProtocol.h"

#pragma code_seg( "PROTOCOLS" )


////////////////////////////////////////////////////////
//

static CCryptLib clib;


////////////////////////////////////////////////////////
//

DWORD CSshThread::Run()
{
   ATLASSERT(m_pManager);
   ATLASSERT(m_pCallback);

   ::SetThreadLocale(_pDevEnv->GetLCID());

   CRYPT_SESSION& cryptSession = (CRYPT_SESSION&) m_pManager->m_cryptSession;
   ATLASSERT(cryptSession==0);

   m_pManager->m_dwErrorCode = 0;
   m_pManager->m_bConnected = false;

   // Make sure CryptLib is available
   if( !clib.Init() ) {
      m_pManager->m_dwErrorCode = ERROR_DEVICE_NOT_AVAILABLE;
      return 0;
   }

   // Get connect parameters
   // TODO: Protect these guys with thread-lock
   CString sHost = m_pManager->GetParam(_T("Host"));
   CString sUsername = m_pManager->GetParam(_T("Username"));
   CString sPassword = m_pManager->GetParam(_T("Password"));
   CString sExtraCommands = m_pManager->GetParam(_T("Extra"));
   long lPort = _ttol(m_pManager->GetParam(_T("Port")));
   long lConnectTimeout = _ttol(m_pManager->GetParam(_T("ConnectTimeout")));
   CString sCertificate = m_pManager->GetParam(_T("Certificate"));

   // Check for the presence of a private key
   if( sCertificate.IsEmpty() ) {
      CString sFilename;
      sFilename.Format(_T("%sprivate.key"), CModulePath());
      if( CFile::FileExists(sFilename) ) sCertificate = sFilename;
   }

   if( lPort == 0 ) lPort = 22;
   if( sPassword.IsEmpty() && sCertificate.IsEmpty() ) sPassword = SecGetPassword();

   USES_CONVERSION;
   LPCSTR pstrHost = T2CA(sHost);
   LPCSTR pstrUsername = T2CA(sUsername);
   LPCSTR pstrPassword = T2CA(sPassword);
   LPCSTR pstrCertificate = T2CA(sCertificate);

   // Create the session
   cryptSession = 0;
   int status = clib.cryptCreateSession(&cryptSession, CRYPT_UNUSED, CRYPT_SESSION_SSH);
   if( cryptStatusError(status) ) {
      cryptSession = 0;
      m_pManager->m_dwErrorCode = NTE_BAD_VER;
      if( status == CRYPT_ERROR_NOTINITED ) m_pManager->m_dwErrorCode = ERROR_SHARING_PAUSED;
      if( status == CRYPT_ERROR_PARAM3 ) m_pManager->m_dwErrorCode = ERROR_MEDIA_NOT_AVAILABLE;
      return 0;
   }
   if( cryptStatusOK(status) ) {
      status = clib.cryptSetAttributeString(cryptSession, CRYPT_SESSINFO_SERVER_NAME, pstrHost, sHost.GetLength());
   }
   if( cryptStatusOK(status) ) {
      status = clib.cryptSetAttributeString(cryptSession, CRYPT_SESSINFO_USERNAME, pstrUsername, sUsername.GetLength());
   }
   if( cryptStatusOK(status) ) {
      status = clib.cryptSetAttribute(cryptSession, CRYPT_SESSINFO_SERVER_PORT, lPort);
   }  
   if( cryptStatusOK(status) ) {
      if( !sCertificate.IsEmpty() ) {
         CRYPT_CONTEXT privateKey;
         status = SshGetPrivateKey(clib, &privateKey, pstrCertificate, "BVRDE Certificate", pstrPassword);
         if( cryptStatusOK(status) ) {
            status = clib.cryptSetAttribute( cryptSession, CRYPT_SESSINFO_PRIVATEKEY, privateKey );
            clib.cryptDestroyContext( privateKey );
         }
      }
      else {
         status = clib.cryptSetAttributeString(cryptSession, CRYPT_SESSINFO_PASSWORD, pstrPassword, sPassword.GetLength());
      }
   }
   if( cryptStatusOK(status) ) {
      status = clib.cryptSetAttribute(cryptSession, CRYPT_SESSINFO_VERSION, 2);
   }
   if( cryptStatusError(status) ) {
      m_pManager->m_dwErrorCode = ERROR_REQUEST_REFUSED;
      if( cryptSession != 0 ) {
         clib.cryptDestroySession(cryptSession);
         cryptSession = 0;
      }
      return 0;
   }

   // Set timeout values
   clib.cryptSetAttribute(cryptSession, CRYPT_OPTION_NET_CONNECTTIMEOUT, lConnectTimeout - 2);
   //clib.cryptSetAttribute(cryptSession, CRYPT_OPTION_NET_TIMEOUT, lConnectTimeout);
   //clib.cryptSetAttribute(cryptSession, CRYPT_OPTION_NET_READTIMEOUT, lConnectTimeout);
   //clib.cryptSetAttribute(cryptSession, CRYPT_OPTION_NET_WRITETIMEOUT, lConnectTimeout);

   // Start connection
   status = clib.cryptSetAttribute(cryptSession, CRYPT_SESSINFO_ACTIVE, TRUE);
   if( cryptStatusError(status) ) {
      m_pManager->m_dwErrorCode = WSAEPROVIDERFAILEDINIT;
      if( status == CRYPT_ERROR_OPEN ) m_pManager->m_dwErrorCode = WSASERVICE_NOT_FOUND;
      if( status == CRYPT_ERROR_MEMORY ) m_pManager->m_dwErrorCode = ERROR_OUTOFMEMORY;
      if( status == CRYPT_ERROR_BADDATA ) m_pManager->m_dwErrorCode = WSAEPROTOTYPE;
      if( status == CRYPT_ERROR_INVALID ) m_pManager->m_dwErrorCode = DIGSIG_E_CRYPTO;
      if( status == CRYPT_ERROR_TIMEOUT )  m_pManager->m_dwErrorCode = ERROR_TIMEOUT;
      if( status == CRYPT_ERROR_WRONGKEY ) m_pManager->m_dwErrorCode = ERROR_NOT_AUTHENTICATED;
      if( status == CRYPT_ERROR_OVERFLOW ) m_pManager->m_dwErrorCode = WSA_QOS_ADMISSION_FAILURE;
      if( status == CRYPT_ERROR_SIGNALLED ) m_pManager->m_dwErrorCode = WSAECONNRESET;
      CHAR szError[200] = { 0 };
      int ccbSize = sizeof(szError) - 1;
      clib.cryptGetAttributeString(cryptSession, CRYPT_ATTRIBUTE_INT_ERRORMESSAGE, szError, &ccbSize);
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_SSHHOST)));
      m_pCallback->BroadcastLine(VT100_RED, GetSystemErrorText(m_pManager->m_dwErrorCode));      
      m_pCallback->BroadcastLine(VT100_RED, A2CT(szError));
      if( cryptSession != 0 ) {
         clib.cryptDestroySession(cryptSession);
         cryptSession = 0;
      }
      return 0;
   }

   const int INITIAL_BUFFER_SIZE = 400;
   LPBYTE pBuffer = (LPBYTE) malloc(INITIAL_BUFFER_SIZE);
   BYTE bReadBuffer[INITIAL_BUFFER_SIZE + 32] = { 0 };
   DWORD dwPos = 0;
   DWORD iStartLinePos = 0;
   DWORD dwBufferSize = INITIAL_BUFFER_SIZE;

   VT100COLOR nColor = VT100_DEFAULT;
   bool bNextIsPrompt = false;

   while( !ShouldStop() ) {

      int iRead = 0;
      status = clib.cryptPopData(cryptSession, bReadBuffer, INITIAL_BUFFER_SIZE, &iRead);
      if( status == 0 && iRead == 0 ) {
         // Hmm, nasty data polling delay! But CryptLib only supports read-timeouts in
         // seconds and that is too high.
         ::Sleep(200L);
         continue;
      }
      if( status == CRYPT_ERROR_TIMEOUT ) continue;
      if( cryptSession == 0 ) break;
      if( cryptStatusError(status) ) break;

#ifdef _DEBUG
      bReadBuffer[iRead] = '\0';
      ATLTRACE(_T("SSH: '%hs' len=%ld\n"), bReadBuffer, iRead);
#endif

      // We might need to grow the data buffer
      if( dwPos + iRead > dwBufferSize ) {
         dwBufferSize = dwPos + iRead;
         pBuffer = (LPBYTE) realloc(pBuffer, dwBufferSize);
         ATLASSERT(pBuffer);
      }

      if( !m_pManager->m_bConnected ) 
      {
         CString sLine = _GetLine(bReadBuffer, 0, iRead);
         m_pCallback->PreAuthenticatedLine(sLine);
         sLine.MakeUpper();
         // Did authorization fail? 
         // TODO: Do we need this check at all?
         //       What if shell is not a telnet dude!?
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
         // Ok, signal that we're connected
         m_pManager->m_bConnected = *ppstr == NULL;
         // Send our own login commands
         if( m_pManager->m_bConnected && !sExtraCommands.IsEmpty() ) {
            CString sCustom = sExtraCommands;
            sCustom.Remove('\r');
            m_pManager->WriteData(sCustom);
         }
      }

      int iPos = 0;
      while( iPos < iRead && !ShouldStop() ) {
         ATLASSERT(iPos<sizeof(bReadBuffer));
         BYTE b = bReadBuffer[iPos++];
         switch( b ) {
         case '\0x0':
         case '\0x1':
         case '\0x2':
         case '\0x7':
         case '\r':
            // Ignore...
            break;
         case '\t':
            *(pBuffer + dwPos) = ' ';
            dwPos++;
            break;
         case 27:
            {
               // Urgh, ANSI escape code for coloring and stuff.
               // Let's just filter them out.
               b = _GetByte(cryptSession, bReadBuffer, iRead, iPos); // skip [
               if( b == ']' ) 
               {
                  // Parse OSC
                  while( true ) {
                     b = _GetByte(cryptSession, bReadBuffer, iRead, iPos);
                     if( b == 94 || b == 7 || b == 0 ) break;
                  }
                  bNextIsPrompt = true;
               }
               else
               {
                  // Parse CSI
                  b = _GetByte(cryptSession, bReadBuffer, iRead, iPos);
                  bool bNewLine = false;
                  int iValue = 0;
                  while( isdigit(b) || b == ';' ) {
                     iValue = (iValue * 10) + (b - '0');
                     b = _GetByte(cryptSession, bReadBuffer, iRead, iPos);
                  }
                  if( b == 'H' ) bNewLine = true;
                  if( b == 'm' && nColor == 0 ) nColor = (VT100COLOR) iValue;
                  if( !bNewLine ) break;
               }
            }
            // FALL THROUGH
         case '\n':
            {
               CString sLine = _GetLine(pBuffer, iStartLinePos, dwPos);
               dwPos = 0;
               iStartLinePos = 0;
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

   free(pBuffer);

   // Clean up
   if( cryptSession != 0 ) {
      status = clib.cryptDestroySession(cryptSession);
      cryptSession = 0;
   }

   m_pManager->m_bConnected = false;

   return 0;
}

CHAR CSshThread::_GetByte(CRYPT_SESSION cryptSession, const LPBYTE pBuffer, int iLength, int& iPos) const
{
   ATLASSERT(pBuffer);
   // First check if the buffer contains enough data
   if( iPos < iLength ) return pBuffer[iPos++];
   // Ok, we need to receive more data...
   CHAR b = 0;
   int iRead = 0;
   clib.cryptPopData(cryptSession, &b, 1, &iRead);
   ATLASSERT(iRead==1); iRead;
   return b;
}

CString CSshThread::_GetLine(LPBYTE pBuffer, int iStart, int iEnd) const
{
   ATLASSERT(iStart<=iEnd);
   ATLASSERT(!::IsBadReadPtr(pBuffer+iStart,iEnd-iStart));
   DWORD dwLength = iEnd - iStart;
   if( dwLength == 0 ) return CString();
   LPSTR pstr = (LPSTR) _alloca(dwLength + 1);
   ATLASSERT(pstr);
   memcpy(pstr, pBuffer + iStart, dwLength);
   pstr[dwLength] = '\0';
   return pstr;
}


////////////////////////////////////////////////////////
//

CSshProtocol::CSshProtocol() :
   m_cryptSession(0),
   m_dwErrorCode(0),
   m_bConnected(false),
   m_lConnectTimeout(0),
   m_lPort(0)
{
   Clear();
   m_event.Create();
}

CSshProtocol::~CSshProtocol()
{
   Stop();
}

void CSshProtocol::Init(CRemoteProject* pProject, IShellCallback* pCallback)
{
   ATLASSERT(pProject);
   ATLASSERT(pCallback);
   m_pProject = pProject;
   m_pCallback = pCallback;
}

void CSshProtocol::Clear()
{
   m_sHost.Empty();
   m_sUsername.Empty();
   m_sPassword.Empty();
   m_lPort = 0;
   m_sPath.Empty();
   m_sExtraCommands.Empty();
   m_lConnectTimeout = 10;
}

bool CSshProtocol::Load(ISerializable* pArc)
{
   Clear();

   if( !pArc->ReadItem(_T("SSH")) ) return false;
   pArc->Read(_T("host"), m_sHost.GetBufferSetLength(128), 128);
   m_sHost.ReleaseBuffer();
   pArc->Read(_T("port"), m_lPort);
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
   m_sPassword = SecDecodePassword(m_sPassword);
   if( m_lConnectTimeout <= 0 ) m_lConnectTimeout = 10;

   return true;
}

bool CSshProtocol::Save(ISerializable* pArc)
{
   if( !pArc->WriteItem(_T("SSH")) ) return false;
   pArc->Write(_T("host"), m_sHost);
   pArc->Write(_T("port"), m_lPort);
   pArc->Write(_T("user"), m_sUsername);
   pArc->Write(_T("password"), SecEncodePassword(m_sPassword));
   pArc->Write(_T("path"), m_sPath);
   pArc->Write(_T("extra"), ConvertFromCrLf(m_sExtraCommands));
   pArc->Write(_T("connectTimeout"), m_lConnectTimeout);
   return true;
}

bool CSshProtocol::Start()
{
   Stop();
   m_thread.m_pManager = this;
   m_thread.m_pCallback = m_pCallback;
   if( !m_thread.Start() ) return false;
   return true;
}

bool CSshProtocol::Stop()
{
   SignalStop();
   if( m_cryptSession != 0 ) {
      // Clean up
      clib.cryptAsyncCancel(m_cryptSession);
      if( cryptStatusOK(clib.cryptDestroySession(m_cryptSession)) ) m_cryptSession = 0;
   }
   m_thread.Stop();
   m_dwErrorCode = 0L;
   m_bConnected = false;
   return true;
}

void CSshProtocol::SignalStop()
{
   m_thread.SignalStop();
   m_event.SetEvent();
}

bool CSshProtocol::IsConnected() const
{
   return m_bConnected;
}

bool CSshProtocol::IsBusy() const
{
   return m_thread.IsRunning() == TRUE;
}

CString CSshProtocol::GetParam(LPCTSTR pstrName) const
{
   CString sName = pstrName;
   if( sName == _T("Path") ) return m_sPath;
   if( sName == _T("Host") ) return m_sHost;
   if( sName == _T("Username") ) return m_sUsername;
   if( sName == _T("Password") ) return m_sPassword;
   if( sName == _T("Extra") ) return m_sExtraCommands;
   if( sName == _T("Port") ) return ToString(m_lPort);
   if( sName == _T("Type") ) return _T("SSH");
   if( sName == _T("ConnectTimeout") ) return ToString(m_lConnectTimeout);
   return _T("");
}

void CSshProtocol::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   if( sName == _T("Path") ) m_sPath = pstrValue;
   if( sName == _T("Host") ) m_sHost = pstrValue;
   if( sName == _T("Username") ) m_sUsername = pstrValue;
   if( sName == _T("Password") ) m_sPassword = pstrValue;
   if( sName == _T("Port") ) m_lPort = _ttol(pstrValue);
   if( sName == _T("Extra") ) m_sExtraCommands = pstrValue;
   if( sName == _T("ConnectTimeout") ) m_lConnectTimeout = _ttol(pstrValue);
   if( m_lConnectTimeout <= 0 ) m_lConnectTimeout = 10;
}

bool CSshProtocol::WriteData(LPCTSTR pstrData)
{
   ATLASSERT(!::IsBadStringPtr(pstrData,-1));
   if( m_cryptSession == 0 ) return false;
   if( !WaitForConnection() ) {
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_CONNECTERROR)));
      return false;
   }
   int nLen = _tcslen(pstrData);
   LPSTR pstr = (LPSTR) _alloca(nLen + 2);
   ATLASSERT(pstr);
   if( pstr == NULL ) return false;
   AtlW2AHelper(pstr, pstrData, nLen + 1);
   strcpy(pstr + nLen, "\n");
   CLockStaticDataInit lock;
   int iWritten = 0;
   int iTimeout = 10;
   int status = CRYPT_ERROR_TIMEOUT;
   while( status == CRYPT_ERROR_TIMEOUT && --iTimeout > 0 ) {
      status = clib.cryptPushData(m_cryptSession, pstr, nLen + 1, &iWritten);
   }
   if( cryptStatusOK(status) ) status = clib.cryptFlushData(m_cryptSession);
   if( cryptStatusError(status) || iWritten != nLen + 1 ) {
      ::SetLastError(ERROR_WRITE_FAULT);
      return false;
   }
   return true;
}

bool CSshProtocol::WriteSignal(BYTE bCmd)
{
   // TODO: Write SSH_MSG_CHANNEL_REQUEST instead
   if( m_cryptSession == 0 ) return false;
   if( !WaitForConnection() ) {
      m_pCallback->BroadcastLine(VT100_RED, CString(MAKEINTRESOURCE(IDS_LOG_CONNECTERROR)));
      return false;
   }
   switch( bCmd ) {
   case TERMINAL_BREAK:
      bCmd = 3;
      break;
   default:
      ATLASSERT(false);
      return false;
   }
   // NOTE: Can't use WriteData() since byte might get lost in
   //       MBCS translation.
   CLockStaticDataInit lock;
   int iWritten = 0;
   int status = clib.cryptPushData(m_cryptSession, &bCmd, 1, &iWritten);
   if( cryptStatusOK(status) ) status = clib.cryptFlushData(m_cryptSession);
   return true;
}

bool CSshProtocol::WriteScreenSize(int w, int h)
{
   // TODO: Send SSH_MSG_CHANNEL_WINDOW_ADJUST message
   return true;
}

bool CSshProtocol::WaitForConnection()
{
   // Wait for the thread to connect to the SSH host.
   const DWORD SPAWNTIMEOUT = 2UL;
   DWORD dwTickStart = ::GetTickCount();
   BOOL bHasRun = FALSE;
   while( !m_bConnected ) {

      // Idle wait a bit
      PumpIdleMessages(200L);

      DWORD dwTick = ::GetTickCount();

      // Did the server return some kind of error?
      if( m_dwErrorCode != 0 ) {
         ::SetLastError(m_dwErrorCode);
         return false;
      }

      // Timeout after 10 sec.
      if( dwTick - dwTickStart > (DWORD) m_lConnectTimeout * 1000UL ) {
         if( m_dwErrorCode == 0 ) ::Sleep(2000L);
         ::SetLastError(m_dwErrorCode == 0 ? ERROR_TIMEOUT : m_dwErrorCode);
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
   ATLASSERT(m_cryptSession!=0);
   return true;
}

