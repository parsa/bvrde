
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"

#include "SftpProtocol.h"

#pragma code_seg( "PROTOCOLS" )


////////////////////////////////////////////////////////
//

static CCryptLib clib;


////////////////////////////////////////////////////////
//

#define CONVERT_INT32(x) x = ntohl((u_long)x)

#define SSH_WRITE_STR(a, x) strcpy((char*)a, T2CA(x)); a += x.GetLength()
#define SSH_WRITE_LONG(a, x) *((LPDWORD)a) = htonl((u_long)x); a += 4
#define SSH_WRITE_BYTE(a, x) *a++ = (BYTE)x
#define SSH_WRITE_DATA(a, x, s) memcpy(a, x, s); a += s

#define SSH_READ_LONG(a) htonl((u_long)*(LPDWORD)a); a += 4
#define SSH_READ_BYTE(a) *a++


enum
{
   SSH_FXP_INIT      = 1,
   SSH_FXP_VERSION   = 2,
   SSH_FXP_OPEN      = 3,
   SSH_FXP_CLOSE     = 4,
   SSH_FXP_READ      = 5,
   SSH_FXP_WRITE     = 6,
   SSH_FXP_LSTAT     = 7,
   SSH_FXP_FSTAT     = 8,
   SSH_FXP_SETSTAT   = 9,
   SSH_FXP_FSETSTAT  = 10,
   SSH_FXP_OPENDIR   = 11,
   SSH_FXP_READDIR   = 12,
   SSH_FXP_REMOVE    = 13,
   SSH_FXP_MKDIR     = 14,
   SSH_FXP_RMDIR     = 15,
   SSH_FXP_REALPATH  = 16,
   SSH_FXP_STAT      = 17,
   SSH_FXP_RENAME    = 18,
   SSH_FXP_READLINK  = 19,
   SSH_FXP_SYMLINK   = 20,
   SSH_FXP_STATUS    = 101,
   SSH_FXP_HANDLE    = 102,
   SSH_FXP_DATA      = 103,
   SSH_FXP_NAME      = 104,
   SSH_FXP_ATTRS     = 105,
};

enum
{
  SSH_FX_OK                = 0,
  SSH_FX_EOF               = 1,
  SSH_FX_NO_SUCH_FILE      = 2,
  SSH_FX_PERMISSION_DENIED = 3,
  SSH_FX_FAILURE           = 4,
  SSH_FX_BAD_MESSAGE       = 5,
  SSH_FX_NO_CONNECTION     = 6,
  SSH_FX_CONNECTION_LOST   = 7,
  SSH_FX_OP_UNSUPPORTED    = 8,
};

enum
{
   SSH_FXF_READ    = 0x00000001,
   SSH_FXF_WRITE   = 0x00000002,
   SSH_FXF_APPEND  = 0x00000004,
   SSH_FXF_CREAT   = 0x00000008,
   SSH_FXF_TRUNC   = 0x00000010,
   SSH_FXF_EXCL    = 0x00000020,
};

enum
{
   SSH_FILEXFER_ATTR_SIZE         = 0x00000001,
   SSH_FILEXFER_ATTR_UIDGID       = 0x00000002,
   SSH_FILEXFER_ATTR_PERMISSIONS  = 0x00000004,
   SSH_FILEXFER_ATTR_ACMODTIME    = 0x00000008,
   SSH_FILEXFER_ATTR_EXTENDED     = 0x80000000 ,
};



////////////////////////////////////////////////////////
//

DWORD CSftpThread::Run()
{
   ATLASSERT(m_pManager);

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
   long lPort = _ttol(m_pManager->GetParam(_T("Port")));
   CString sCertificate = m_pManager->GetParam(_T("Certificate"));
   CString sPath = m_pManager->GetParam(_T("Path"));
   CString sProxy = m_pManager->GetParam(_T("Proxy"));
   bool bPassive = m_pManager->GetParam(_T("Passive")) == _T("true");
   long lConnectTimeout = _ttol(m_pManager->GetParam(_T("ConnectTimeout")));

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
      m_pManager->m_dwErrorCode = clib.StatusToWin32Error(status, NTE_BAD_VER);
      if( status == CRYPT_ERROR_PARAM3 ) m_pManager->m_dwErrorCode = ERROR_MEDIA_NOT_AVAILABLE;
      return 0;
   }
   if( cryptStatusOK(status) ) {
      status = clib.cryptSetAttributeString(cryptSession, CRYPT_SESSINFO_SERVER_NAME, pstrHost, strlen(pstrHost));
   }
   if( cryptStatusOK(status) ) {
      status = clib.cryptSetAttributeString(cryptSession, CRYPT_SESSINFO_USERNAME, pstrUsername, strlen(pstrUsername));
   }
   if( cryptStatusOK(status) ) {
      status = clib.cryptSetAttribute(cryptSession, CRYPT_SESSINFO_SERVER_PORT, lPort);
   }  
   if( cryptStatusOK(status) ) {
      if( !sCertificate.IsEmpty() ) {
         CRYPT_CONTEXT privateKey;
         status = clib.GetPrivateKey(&privateKey, pstrCertificate, "BVRDE Certificate", pstrPassword);
         if( cryptStatusOK(status) ) {
            status = clib.cryptSetAttribute(cryptSession, CRYPT_SESSINFO_PRIVATEKEY, privateKey);
            clib.cryptDestroyContext( privateKey );
         }
      }
      else {
         status = clib.cryptSetAttributeString(cryptSession, CRYPT_SESSINFO_PASSWORD, pstrPassword, strlen(pstrPassword));
      }
   }
   if( cryptStatusOK(status) ) {
      status = clib.cryptSetAttribute(cryptSession, CRYPT_SESSINFO_VERSION, 2);
   }
   if( cryptStatusOK(status) ) {
      status = clib.cryptSetAttribute( cryptSession, CRYPT_SESSINFO_SSH_CHANNEL, CRYPT_UNUSED );
   }
   if( cryptStatusOK( status ) ) {
      status = clib.cryptSetAttributeString(cryptSession, CRYPT_SESSINFO_SSH_CHANNEL_TYPE, "subsystem", strlen("subsystem"));
   }
   if( cryptStatusOK( status ) ) {
      status = clib.cryptSetAttributeString(cryptSession, CRYPT_SESSINFO_SSH_CHANNEL_ARG1, "sftp", strlen("sftp"));
   }
   if( cryptStatusError(status) ) {
      m_pManager->m_dwErrorCode = clib.StatusToWin32Error(status, ERROR_REQUEST_REFUSED);
      if( cryptSession != 0 ) {
         clib.cryptDestroySession(cryptSession);
         cryptSession = 0;
      }
      return 0;
   }

   // Set timeout values
   clib.cryptSetAttribute(cryptSession, CRYPT_OPTION_NET_CONNECTTIMEOUT, lConnectTimeout - 2);
   clib.cryptSetAttribute(cryptSession, CRYPT_OPTION_NET_READTIMEOUT, lConnectTimeout);
   clib.cryptSetAttribute(cryptSession, CRYPT_OPTION_NET_WRITETIMEOUT, lConnectTimeout);

   // Start connection
   status = clib.cryptSetAttribute(cryptSession, CRYPT_SESSINFO_ACTIVE, TRUE);
   if( cryptStatusError(status) ) {
      m_pManager->m_dwErrorCode = clib.StatusToWin32Error(status, WSAEPROVIDERFAILEDINIT);
      if( cryptSession != 0 ) {
         clib.cryptDestroySession(cryptSession);
         cryptSession = 0;
      }
      return 0;
   }

   if( ShouldStop() ) return 0;

   // Send INIT package first
   m_pManager->m_bVersion = (BYTE) m_pManager->_SendInit();
   if( m_pManager->m_bVersion < 3 ) {
      m_pManager->m_dwErrorCode = ERROR_BAD_ENVIRONMENT;
      return 0;
   }

   if( ShouldStop() ) return 0;

   // Grab the full path of the start directory
   m_pManager->m_sCurDir = m_pManager->_ResolvePath(sPath);
   if( m_pManager->m_sCurDir.IsEmpty() ) {
      m_pManager->m_dwErrorCode = ERROR_PATH_NOT_FOUND;
      return 0;
   }

   if( ShouldStop() ) return 0;

   m_pManager->m_dwErrorCode = 0;
   m_pManager->m_bConnected = true;

   return 0;
}


////////////////////////////////////////////////////////
//

CSftpProtocol::CSftpProtocol() :
   m_cryptSession(0),
   m_lPort(22),
   m_lConnectTimeout(0),
   m_dwErrorCode(0L),
   m_bConnected(false)
{
   Clear();
}

CSftpProtocol::~CSftpProtocol()
{
   Stop();
}

void CSftpProtocol::Clear()
{
   m_sHost.Empty();
   m_sUsername.Empty();
   m_sPassword.Empty();
   m_lPort = 22;
   m_sPath.Empty();
   m_sProxy.Empty();
   m_bPassive = FALSE;
   m_bCompatibilityMode = FALSE;
   m_lConnectTimeout = 10;
}

bool CSftpProtocol::Load(ISerializable* pArc)
{
   Clear();

   if( !pArc->ReadItem(_T("Sftp")) ) return false;
   pArc->Read(_T("host"), m_sHost.GetBufferSetLength(128), 128);
   m_sHost.ReleaseBuffer();
   pArc->Read(_T("port"), m_lPort);
   pArc->Read(_T("user"), m_sUsername.GetBufferSetLength(128), 128);
   m_sUsername.ReleaseBuffer();
   pArc->Read(_T("password"), m_sPassword.GetBufferSetLength(128), 128);
   m_sPassword.ReleaseBuffer();
   pArc->Read(_T("path"), m_sPath.GetBufferSetLength(MAX_PATH), MAX_PATH);
   m_sPath.ReleaseBuffer();
   pArc->Read(_T("searchPath"), m_sSearchPath.GetBufferSetLength(128), 128);
   m_sSearchPath.ReleaseBuffer();
   pArc->Read(_T("connectTimeout"), m_lConnectTimeout);
   pArc->Read(_T("compatibility"), m_bCompatibilityMode);

   m_sPath.TrimRight(_T("/"));
   if( m_sPath.IsEmpty() ) m_sPath = _T("/");
   m_sPassword = SecDecodePassword(m_sPassword);
   if( m_lConnectTimeout <= 0 ) m_lConnectTimeout = 10;

   return true;
}

bool CSftpProtocol::Save(ISerializable* pArc)
{
   if( !pArc->WriteItem(_T("Sftp")) ) return false;
   pArc->Write(_T("host"), m_sHost);
   pArc->Write(_T("port"), m_lPort);
   pArc->Write(_T("user"), m_sUsername);
   pArc->Write(_T("password"), SecEncodePassword(m_sPassword));
   pArc->Write(_T("path"), m_sPath);
   pArc->Write(_T("searchPath"), m_sSearchPath);
   pArc->Write(_T("connectTimeout"), m_lConnectTimeout);
   pArc->Write(_T("compatibility"), m_bCompatibilityMode ? _T("true") : _T("false"));
   return true;
}

bool CSftpProtocol::Start()
{
   Stop();
   m_bVersion = 0;
   m_dwMsgId = 0;
   m_thread.m_pManager = this;
   if( !m_thread.Start() ) return false;
   return true;
}

void CSftpProtocol::SignalStop()
{
   m_thread.SignalStop();
}

bool CSftpProtocol::Stop()
{
   SignalStop();
   if( m_cryptSession != 0 ) {
      // Clean up
      clib.cryptAsyncCancel(m_cryptSession);
      if( cryptStatusOK( clib.cryptDestroySession(m_cryptSession) ) ) m_cryptSession = 0;
   }
   m_thread.Stop();
   m_dwErrorCode = 0L;
   m_bConnected = false;
   return true;
}

bool CSftpProtocol::IsConnected() const
{
   return m_bConnected;
}

CString CSftpProtocol::GetParam(LPCTSTR pstrName) const
{
   CString sName = pstrName;
   if( sName == _T("Path") ) return m_sPath;
   if( sName == _T("SearchPath") ) return m_sSearchPath;
   if( sName == _T("Host") ) return m_sHost;
   if( sName == _T("Username") ) return m_sUsername;
   if( sName == _T("Password") ) return m_sPassword;
   if( sName == _T("Port") ) return ToString(m_lPort);
   if( sName == _T("Proxy") ) return m_sProxy;
   if( sName == _T("Separator") ) return _T("/");
   if( sName == _T("Passive") ) return m_bPassive ? _T("true") : _T("false");
   if( sName == _T("Type") ) return _T("SFTP");
   if( sName == _T("ConnectTimeout") ) return ToString(m_lConnectTimeout);
   if( sName == _T("CompatibilityMode") ) return m_bCompatibilityMode ? _T("true") : _T("false");
   return _T("");
}

void CSftpProtocol::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   if( sName == _T("Path") ) m_sPath = pstrValue;
   if( sName == _T("SearchPath") ) m_sSearchPath = pstrValue;
   if( sName == _T("Host") ) m_sHost = pstrValue;
   if( sName == _T("Username") ) m_sUsername = pstrValue;
   if( sName == _T("Password") ) m_sPassword = pstrValue;
   if( sName == _T("Port") ) m_lPort = _ttol(pstrValue);
   if( sName == _T("Passive") ) m_bPassive = _tcscmp(pstrValue, _T("true")) == 0;
   if( sName == _T("CompatibilityMode") ) m_bCompatibilityMode = _tcscmp(pstrValue, _T("true")) == 0;
   if( sName == _T("Proxy") ) m_sProxy = pstrValue;
   if( sName == _T("ConnectTimeout") ) m_lConnectTimeout = _ttol(pstrValue);
   m_sPath.TrimRight(_T("/"));
   if( m_lConnectTimeout <= 0 ) m_lConnectTimeout = 10;
}

bool CSftpProtocol::LoadFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE* ppOut, DWORD* pdwSize /* = NULL*/)
{
   ATLASSERT(pstrFilename);
   ATLASSERT(ppOut);

   USES_CONVERSION;

   *ppOut = NULL;
   if( pdwSize != NULL ) *pdwSize = 0;

   // Connected?
   if( !WaitForConnection() ) return false;
   // Need a valid session
   if( m_cryptSession == 0 ) return false;

   // Construct remote filename
   CString sFilename = pstrFilename; 
   if( pstrFilename[0] != '/' && m_sCurDir.GetLength() > 1 ) sFilename.Format(_T("%s/%s"), m_sCurDir, pstrFilename);
   sFilename.Replace(_T('\\'), _T('/'));

   // Open file
   BYTE buffer[600];
   LPBYTE p = buffer;
   SSH_WRITE_LONG(p, 1 + 4 + 4 + sFilename.GetLength() + 4 + 4);
   SSH_WRITE_BYTE(p, SSH_FXP_OPEN);
   SSH_WRITE_LONG(p, ++m_dwMsgId);
   SSH_WRITE_LONG(p, sFilename.GetLength());
   SSH_WRITE_STR(p, sFilename);
   SSH_WRITE_LONG(p, SSH_FXF_READ);
   SSH_WRITE_LONG(p, 0);
   if( !_WriteData(m_cryptSession, buffer, p - buffer) ) return false;

   BYTE bHandle[100];
   if( _ReadData(m_cryptSession, bHandle, sizeof(bHandle)) == 0 ) return false;
   p = bHandle;
   DWORD cbSize    = SSH_READ_LONG(p);
   BYTE id         = SSH_READ_BYTE(p);
   DWORD msgid     = SSH_READ_LONG(p);
   DWORD cchHandle = SSH_READ_LONG(p);
   ATLASSERT(msgid==m_dwMsgId);

   LPBYTE pHandle = p;

   if( cchHandle > 64 ) {
      ::SetLastError(ERROR_INVALID_TARGET_HANDLE);
      return false;
   }
   if( id != SSH_FXP_HANDLE ) {
      ::SetLastError(ERROR_FILE_NOT_FOUND);
      return false;
   }

   DWORD dwOffset = 0;
   bool bSuccess = true;
   while( true ) 
   {
      // Read remote file
      p = buffer;
      SSH_WRITE_LONG(p, 1 + 4 + 4 + cchHandle + 4 + 4 + 4);
      SSH_WRITE_BYTE(p, SSH_FXP_READ);
      SSH_WRITE_LONG(p, ++m_dwMsgId);
      SSH_WRITE_LONG(p, cchHandle);
      SSH_WRITE_DATA(p, pHandle, cchHandle);
      SSH_WRITE_LONG(p, 0);
      SSH_WRITE_LONG(p, dwOffset);
      SSH_WRITE_LONG(p, sizeof(buffer) - 100);
      if( !_WriteData(m_cryptSession, buffer, p - buffer) ) return false;

      if( _ReadData(m_cryptSession, buffer, 13) == 0 ) return false;
      p = buffer;
      DWORD cbSize   = SSH_READ_LONG(p);
      BYTE id        = SSH_READ_BYTE(p);
      DWORD msgid    = SSH_READ_LONG(p);
      DWORD dwStatus = SSH_READ_LONG(p);
      ATLASSERT(msgid==m_dwMsgId);

      if( id == SSH_FXP_STATUS )
      {
         // Done?
         if( cbSize > 9 ) _ReadData(m_cryptSession, buffer, min(sizeof(buffer), cbSize - 9));
         if( dwStatus == SSH_FX_EOF ) {
            if( pdwSize != NULL ) *pdwSize = dwOffset;
            break;
         }
         bSuccess = false;
         break;
      }
      else if( id == SSH_FXP_DATA )
      {
         // More data to us
         DWORD dwRead = dwStatus;
         ATLASSERT(dwRead<=cbSize-9);
         *ppOut = (LPBYTE) realloc(*ppOut, dwOffset + dwRead);
         _ReadData(m_cryptSession, *ppOut + dwOffset, dwRead); 
         if( cbSize - dwRead > 9 ) _ReadData(m_cryptSession, buffer, min(sizeof(buffer), cbSize - dwRead - 9));
         dwOffset += dwRead;
      }
      else 
      {
         // Some other error; fail...
         if( cbSize > 9 ) _ReadData(m_cryptSession, buffer, min(sizeof(buffer), cbSize - 9));
         bSuccess = false;
         break;
      }
   }

   // Close handle
   p = buffer;
   SSH_WRITE_LONG(p, 1 + 4 + 4 + cchHandle);
   SSH_WRITE_BYTE(p, SSH_FXP_CLOSE);
   SSH_WRITE_LONG(p, ++m_dwMsgId);
   SSH_WRITE_LONG(p, cchHandle);
   SSH_WRITE_DATA(p, pHandle, cchHandle);
   if( !_WriteData(m_cryptSession, buffer, p - buffer) ) return false;

   if( _ReadData(m_cryptSession, buffer, sizeof(buffer)) == 0 ) return false;
   p = buffer;
   cbSize         = SSH_READ_LONG(p); cbSize;
   id             = SSH_READ_BYTE(p); id;
   msgid          = SSH_READ_LONG(p); msgid;
   DWORD dwStatus = SSH_READ_LONG(p); dwStatus;
   ATLASSERT(msgid==m_dwMsgId);

   if( !bSuccess ) {
      if( *ppOut ) free(*ppOut);
      *ppOut = NULL;
      ::SetLastError(ERROR_READ_FAULT);
   }

   return bSuccess;
}

bool CSftpProtocol::SaveFile(LPCTSTR pstrFilename, bool /*bBinary*/, LPBYTE pData, DWORD dwSize)
{
   ATLASSERT(pstrFilename);
   ATLASSERT(pData);
   
   USES_CONVERSION;

   // Prevent save of an empty file
   if( dwSize == 0 ) {
      ::SetLastError(ERROR_EMPTY);
      return false;
   }

   // Connected?
   if( !WaitForConnection() ) return false;
   // Need a valid session
   if( m_cryptSession == 0 ) return false;

   // Construct remote filename
   CString sFilename = pstrFilename; 
   if( pstrFilename[0] != '/' && m_sCurDir.GetLength() > 1 ) sFilename.Format(_T("%s/%s"), m_sCurDir, pstrFilename);
   sFilename.Replace(_T('\\'), _T('/'));

   // Open file
   enum { _IFREG = 0100000 };  // UNIX regular file
   BYTE buffer[600] = { 0 };
   LPBYTE p = buffer;
   SSH_WRITE_LONG(p, 1 + 4 + 4 + sFilename.GetLength() + 4 + 4 + 4 + 4 + 4);
   SSH_WRITE_BYTE(p, SSH_FXP_OPEN);
   SSH_WRITE_LONG(p, ++m_dwMsgId);
   SSH_WRITE_LONG(p, sFilename.GetLength());
   SSH_WRITE_STR(p, sFilename);
   SSH_WRITE_LONG(p, SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_TRUNC);
   SSH_WRITE_LONG(p, SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_PERMISSIONS);
   SSH_WRITE_LONG(p, 0);
   SSH_WRITE_LONG(p, dwSize);
   SSH_WRITE_LONG(p, 07777);  // BUG: We need to try to preserve the attributes
                              //      since it might cause a security conflict!
   if( !_WriteData(m_cryptSession, buffer, p - buffer) ) return false;

   BYTE bHandle[100];
   if( _ReadData(m_cryptSession, bHandle, sizeof(bHandle)) == 0 ) return false;
   p = bHandle;
   DWORD cbSize         = SSH_READ_LONG(p);
   BYTE id              = SSH_READ_BYTE(p);
   DWORD msgid          = SSH_READ_LONG(p);

   // Didn't return a valid handle?
   if( id != SSH_FXP_HANDLE ) {
      ::SetLastError(ERROR_CANNOT_MAKE);
      if( id == SSH_FXP_STATUS ) {
         DWORD dwStatus = SSH_READ_LONG(p);
         if( dwStatus == SSH_FX_PERMISSION_DENIED ) ::SetLastError(ERROR_ACCESS_DENIED);
      }
      return false;
   }

   DWORD cchHandle = SSH_READ_LONG(p);
   LPBYTE pHandle = p;
   if( cchHandle > 64 ) {
      ::SetLastError(ERROR_INVALID_TARGET_HANDLE);
      return false;
   }

   // Send data to remote host
   const DWORD CHUNK_SIZE = sizeof(buffer) - 100UL;
   bool bSuccess = true;
   DWORD dwOffset = 0;
   DWORD dwStatus = 0;
   while( true ) 
   {
      // Write new data to remote file
      DWORD dwWrite = min(dwSize, CHUNK_SIZE);
      p = buffer;
      SSH_WRITE_LONG(p, 1 + 4 + 4 + cchHandle + 4 + 4 + 4 + dwWrite);
      SSH_WRITE_BYTE(p, SSH_FXP_WRITE);
      SSH_WRITE_LONG(p, ++m_dwMsgId);
      SSH_WRITE_LONG(p, cchHandle);
      SSH_WRITE_DATA(p, pHandle, cchHandle);
      SSH_WRITE_LONG(p, 0);
      SSH_WRITE_LONG(p, dwOffset);
      SSH_WRITE_LONG(p, dwWrite);
      SSH_WRITE_DATA(p, pData + dwOffset, dwWrite);
      if( !_WriteData(m_cryptSession, buffer, p - buffer) ) return false;

      if( _ReadData(m_cryptSession, buffer, sizeof(buffer)) == 0 ) return false;
      p = buffer;
      cbSize   = SSH_READ_LONG(p);
      id       = SSH_READ_BYTE(p);
      msgid    = SSH_READ_LONG(p);
      dwStatus = SSH_READ_LONG(p);
      ATLASSERT(msgid==m_dwMsgId);

      if( id == SSH_FXP_STATUS )
      {
         // Done?
         if( dwStatus != SSH_FX_OK ) {
            bSuccess = false;
            break;
         }
         dwSize -= dwWrite;
         dwOffset += dwWrite;
         if( dwSize == 0 ) break;
      }
      else 
      {
         // Some other error; fail...
         bSuccess = false;
         break;
      }
   }

   // Close handle
   p = buffer;
   SSH_WRITE_LONG(p, 1 + 4 + 4 + cchHandle);
   SSH_WRITE_BYTE(p, SSH_FXP_CLOSE);
   SSH_WRITE_LONG(p, ++m_dwMsgId);
   SSH_WRITE_LONG(p, cchHandle);
   SSH_WRITE_DATA(p, pHandle, cchHandle);
   if( !_WriteData(m_cryptSession, buffer, p - buffer) ) return false;

   if( _ReadData(m_cryptSession, buffer, sizeof(buffer)) == 0 ) return false;
   p = buffer;
   cbSize   = SSH_READ_LONG(p); cbSize;
   id       = SSH_READ_BYTE(p);
   msgid    = SSH_READ_LONG(p); msgid;
   dwStatus = SSH_READ_LONG(p);
   ATLASSERT(msgid==m_dwMsgId);

   if( !bSuccess ) ::SetLastError(ERROR_WRITE_FAULT);

   return bSuccess;
}

bool CSftpProtocol::DeleteFile(LPCTSTR pstrFilename)
{
   ATLASSERT(pstrFilename);
   
   USES_CONVERSION;

   // Connected?
   if( !WaitForConnection() ) return false;
   // Need a valid session
   if( m_cryptSession == 0 ) return false;

   // Construct remote filename
   CString sFilename = pstrFilename; 
   if( pstrFilename[0] != '/' && m_sCurDir.GetLength() > 1 ) sFilename.Format(_T("%s/%s"), m_sCurDir, pstrFilename);
   sFilename.Replace(_T('\\'), _T('/'));

   BYTE buffer[600] = { 0 };
   LPBYTE p = buffer;
   SSH_WRITE_LONG(p, 1 + 4 + 4 + sFilename.GetLength());
   SSH_WRITE_BYTE(p, SSH_FXP_REMOVE);
   SSH_WRITE_LONG(p, ++m_dwMsgId);
   SSH_WRITE_LONG(p, sFilename.GetLength());
   SSH_WRITE_STR(p, sFilename);
   if( !_WriteData(m_cryptSession, buffer, p - buffer) ) return false;

   if( _ReadData(m_cryptSession, buffer, sizeof(buffer)) == 0 ) return false;
   p = buffer;
   DWORD cbSize   = SSH_READ_LONG(p);
   BYTE id        = SSH_READ_BYTE(p);
   DWORD msgid    = SSH_READ_LONG(p);
   DWORD dwStatus = SSH_READ_LONG(p);
   ATLASSERT(msgid==m_dwMsgId);

   return dwStatus == SSH_FX_OK;
}

bool CSftpProtocol::SetCurPath(LPCTSTR pstrPath)
{
   ATLASSERT(pstrPath);

   // Wait for connection
   if( !WaitForConnection() ) return false;
   // Need a valid session
   if( m_cryptSession == 0 ) return false;

   CString sPath = pstrPath;
   if( pstrPath[0] != _T('/') && m_sCurDir.GetLength() > 1 ) sPath.Format(_T("%s/%s"), m_sCurDir, pstrPath);

   sPath = _ResolvePath(sPath);
   if( sPath.IsEmpty() ) return false;
   m_sCurDir = sPath;
   
   return true;
}

CString CSftpProtocol::GetCurPath()
{
   return m_sCurDir;
}

bool CSftpProtocol::EnumFiles(CSimpleArray<WIN32_FIND_DATA>& aFiles, bool /*bUseCache*/)
{
   // Wait for connection
   if( !WaitForConnection() ) return false;
   // Need a valid session
   if( m_cryptSession == 0 ) return false;

   USES_CONVERSION;

   CString sPath = GetCurPath();

   // Open directory
   BYTE buffer[600] = { 0 };
   LPBYTE p = buffer;
   SSH_WRITE_LONG(p, 1 + 4 + 4 + sPath.GetLength());
   SSH_WRITE_BYTE(p, SSH_FXP_OPENDIR);
   SSH_WRITE_LONG(p, ++m_dwMsgId);
   SSH_WRITE_LONG(p, sPath.GetLength());
   SSH_WRITE_STR(p, sPath);
   if( !_WriteData(m_cryptSession, buffer, p - buffer) ) return false;

   BYTE bHandle[100];
   if( _ReadData(m_cryptSession, bHandle, sizeof(bHandle)) == 0 ) return false;
   p = bHandle;
   DWORD cbSize    = SSH_READ_LONG(p);
   BYTE id         = SSH_READ_BYTE(p);
   DWORD msgid     = SSH_READ_LONG(p);
   DWORD cchHandle = SSH_READ_LONG(p);
   ATLASSERT(msgid==m_dwMsgId);

   LPBYTE pHandle = p;

   if( cchHandle > 64 ) {
      ::SetLastError(ERROR_INVALID_TARGET_HANDLE);
      return false;
   }
   if( id != SSH_FXP_HANDLE ) {
      ::SetLastError(ERROR_NOT_CONTAINER);
      return false;
   }

   // Enumerate files in folder
   bool bSuccess = true;
   while( true ) {
      p = buffer;
      SSH_WRITE_LONG(p, 1 + 4 + 4 + cchHandle);
      SSH_WRITE_BYTE(p, SSH_FXP_READDIR);
      SSH_WRITE_LONG(p, ++m_dwMsgId);
      SSH_WRITE_LONG(p, cchHandle);
      SSH_WRITE_DATA(p, pHandle, cchHandle);
      if( !_WriteData(m_cryptSession, buffer, p - buffer) ) return false;

      if( _ReadData(m_cryptSession, buffer, 13) == 0 ) return false;
      p = buffer;
      cbSize        = SSH_READ_LONG(p);
      id            = SSH_READ_BYTE(p);
      msgid         = SSH_READ_LONG(p);
      DWORD dwCount = SSH_READ_LONG(p);
      ATLASSERT(msgid==m_dwMsgId);

      // Got all files?
      if( id == SSH_FXP_STATUS && dwCount == SSH_FX_EOF ) {
         // Read remaining of status record...
         if( cbSize > 9 ) _ReadData(m_cryptSession, buffer, min(sizeof(buffer), cbSize - 9));
         break;
      }

      // Failed?
      if( id != SSH_FXP_NAME ) {
         // Read remaining of status record...
         if( cbSize > 9 ) _ReadData(m_cryptSession, buffer, min(sizeof(buffer), cbSize - 9));
         bSuccess = false;
         break;
      }

      // Get files in buffer...
      for( DWORD i = 0; i < dwCount; i++ ) {
         // Short filename
         DWORD dwSize = 0;
         CHAR szFilename[300] = { 0 };
         if( _ReadData(m_cryptSession, &dwSize, sizeof(dwSize)) == 0 ) return false;
         CONVERT_INT32(dwSize);
         if( _ReadData(m_cryptSession, szFilename, dwSize) == 0 ) return false;

         // Long filename
         BYTE bTemp[400];
         if( _ReadData(m_cryptSession, &dwSize, sizeof(dwSize)) == 0 ) return false;
         CONVERT_INT32(dwSize);
         if( dwSize > sizeof(bTemp) ) return false;
         if( _ReadData(m_cryptSession, bTemp, dwSize) == 0 ) return false;

         // Attribute flags
         DWORD dwFlags = 0;
         if( _ReadData(m_cryptSession, &dwFlags, sizeof(dwFlags)) == 0 ) return false;
         CONVERT_INT32(dwFlags);

         DWORD dwTemp = 0;
         DWORD dwFileSize = 0;
         DWORD dwPermissions = 0;
         if( dwFlags & SSH_FILEXFER_ATTR_SIZE ) {         
            if( _ReadData(m_cryptSession, &dwTemp, sizeof(dwTemp)) == 0 ) return false;
            if( _ReadData(m_cryptSession, &dwFileSize, sizeof(dwSize)) == 0 ) return false;
            CONVERT_INT32(dwFileSize);
         }
         if( dwFlags & SSH_FILEXFER_ATTR_UIDGID ) {         
            if( _ReadData(m_cryptSession, &dwTemp, sizeof(dwTemp)) == 0 ) return false;
            if( _ReadData(m_cryptSession, &dwTemp, sizeof(dwTemp)) == 0 ) return false;
         }
         if( dwFlags & SSH_FILEXFER_ATTR_PERMISSIONS ) {         
            if( _ReadData(m_cryptSession, &dwPermissions, sizeof(dwPermissions)) == 0 ) return false;
            CONVERT_INT32(dwPermissions);
         }
         if( dwFlags & SSH_FILEXFER_ATTR_ACMODTIME ) {         
            if( _ReadData(m_cryptSession, &dwTemp, sizeof(dwTemp)) == 0 ) return false;
            if( _ReadData(m_cryptSession, &dwTemp, sizeof(dwTemp)) == 0 ) return false;
         }
         if( dwFlags & SSH_FILEXFER_ATTR_EXTENDED ) {
            long lCount = 0;
            if( _ReadData(m_cryptSession, &lCount, sizeof(lCount)) == 0 ) return false;
            CONVERT_INT32(lCount);
            for( int j = 0; j < lCount; j++ ) {
               // Read Name
               if( _ReadData(m_cryptSession, &dwSize, sizeof(dwSize)) == 0 ) return false;
               CONVERT_INT32(dwSize);
               if( dwSize > sizeof(bTemp) ) return false;
               if( _ReadData(m_cryptSession, bTemp, dwSize) == 0 ) return false;
               // Read Value
               if( _ReadData(m_cryptSession, &dwSize, sizeof(dwSize)) == 0 ) return false;
               CONVERT_INT32(dwSize);
               if( dwSize > sizeof(bTemp) ) return false;
               if( _ReadData(m_cryptSession, bTemp, dwSize) == 0 ) return false;
            }
         }

         // Ignore meta-paths
         if( strcmp(szFilename, ".") == 0 ) continue;
         if( strcmp(szFilename, "..") == 0 ) continue;

         WIN32_FIND_DATA fd = { 0 };
         _tcscpy(fd.cFileName, A2CT(szFilename));
         fd.nFileSizeLow = dwFileSize;
         // TODO: Translate more file-attributes
         if( (dwPermissions & 0x4000) != 0 ) fd.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
         aFiles.Add(fd);
      }
   }

   // Close handle
   p = buffer;
   SSH_WRITE_LONG(p, 1 + 4 + 4 + cchHandle);
   SSH_WRITE_BYTE(p, SSH_FXP_CLOSE);
   SSH_WRITE_LONG(p, ++m_dwMsgId);
   SSH_WRITE_LONG(p, cchHandle);
   SSH_WRITE_DATA(p, pHandle, cchHandle);
   if( !_WriteData(m_cryptSession, buffer, p - buffer) ) return false;

   if( _ReadData(m_cryptSession, buffer, sizeof(buffer)) == 0 ) return false;
   p = buffer;
   cbSize         = SSH_READ_LONG(p); cbSize;
   id             = SSH_READ_BYTE(p);
   msgid          = SSH_READ_LONG(p); msgid;
   DWORD dwStatus = SSH_READ_LONG(p); dwStatus;
   ATLASSERT(msgid==m_dwMsgId);

   return bSuccess;
}

bool CSftpProtocol::WaitForConnection()
{
   // Wait for the thread to connect to the SFTP host.
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

// Implementation

DWORD CSftpProtocol::_SendInit()
{
   // Send INIT package
   BYTE buffer[64] = { 0 };
   LPBYTE p = buffer;
   SSH_WRITE_LONG(p, 1 + 4);
   SSH_WRITE_BYTE(p, SSH_FXP_INIT);
   SSH_WRITE_LONG(p, 3);
   if( !_WriteData(m_cryptSession, buffer, p - buffer) ) {
      m_dwErrorCode = ::GetLastError();
      return 0;
   }
   // Read reply...
   if( _ReadData(m_cryptSession, buffer, sizeof(buffer)) == 0 ) {
      m_dwErrorCode = ::GetLastError();
      return 0;
   }
   p = buffer;
   DWORD cbSize  = SSH_READ_LONG(p); cbSize;
   BYTE id       = SSH_READ_BYTE(p); id;
   return SSH_READ_LONG(p);
}

CString CSftpProtocol::FindFile(LPCTSTR pstrFilename)
{
   if( pstrFilename[0] == '/' ) {
      LPBYTE pData = NULL;
      bool bFound = LoadFile(pstrFilename, true, &pData);
      free(pData);
      return bFound ? pstrFilename : NULL;
   }
   else {
      CString sPath = m_sSearchPath;
      while( !sPath.IsEmpty() ) {
         CString sSubPath = sPath.SpanExcluding(_T(";"));
         CString sFilename = sSubPath;
         if( sFilename.Right(1) != _T("/") ) sFilename += _T("/");
         sFilename += pstrFilename;
         LPBYTE pData = NULL;
         bool bFound = LoadFile(sFilename, true, &pData);
         free(pData);
         if( bFound ) return sFilename;
         sPath = sPath.Mid(sSubPath.GetLength() + 1);
      }
      return _T("");
   }
}

CString CSftpProtocol::_ResolvePath(LPCTSTR pstrPath)
{
   // Need a valid session
   if( m_cryptSession == 0 ) return _T("");

   USES_CONVERSION;
   CString sPath = pstrPath;
   if( sPath.GetLength() > 3 ) sPath.TrimRight(_T("/\\"));  // BUG: What's with then length=3 ???
   sPath.Replace(_T('\\'), _T('/'));

   // Let the remote system resolve path
   BYTE buffer[300] = { 0 };
   LPBYTE p = buffer;
   SSH_WRITE_LONG(p, 1 + 4 + 4 + sPath.GetLength());
   SSH_WRITE_BYTE(p, SSH_FXP_REALPATH);
   SSH_WRITE_LONG(p, ++m_dwMsgId);
   SSH_WRITE_LONG(p, sPath.GetLength());
   SSH_WRITE_STR(p, sPath);
   if( !_WriteData(m_cryptSession, buffer, p - buffer) ) return _T("");
   // Read reply...
   if( _ReadData(m_cryptSession, buffer, sizeof(buffer)) == 0 ) return _T("");
   p = buffer;
   DWORD cbSize   = SSH_READ_LONG(p); cbSize;
   BYTE id        = SSH_READ_BYTE(p); id;
   DWORD msgid    = SSH_READ_LONG(p); msgid;
   DWORD dwStatus = SSH_READ_LONG(p); dwStatus;
   DWORD cchPath  = SSH_READ_LONG(p);
   ATLASSERT(msgid==m_dwMsgId);

   if( id != SSH_FXP_NAME ) return _T("");

   p[cchPath] = '\0';
   return p;
}

int CSftpProtocol::_ReadData(CRYPT_SESSION cryptSession, LPVOID pData, int iMaxSize)
{
   const DWORD READTIMEOUT = 6UL;
   ::ZeroMemory(pData, iMaxSize);
   int iCopied = 0;
   int status = CRYPT_OK;
   DWORD dwTick = ::GetTickCount();
   while( iCopied == 0 && ::GetTickCount() - dwTick < READTIMEOUT * 1000UL ) {
      status = clib.cryptPopData(cryptSession, pData, iMaxSize, &iCopied);
   }
   if( cryptStatusError(status) ) {
      ::SetLastError(ERROR_READ_FAULT);
      return 0;
   }
   if( iCopied == 0 ) {
      ::SetLastError(WAIT_TIMEOUT);
      return 0;
   }
   return iCopied;
}

bool CSftpProtocol::_WriteData(CRYPT_SESSION cryptSession, LPCVOID pData, int iSize)
{
   ATLASSERT(pData);
   ATLASSERT(iSize>0);
   int iCopied = 0;
   int status = clib.cryptPushData(cryptSession, const_cast<LPVOID>(pData), iSize, &iCopied);
   if( cryptStatusOK(status) ) status = clib.cryptFlushData(cryptSession);
   if( iSize != iCopied || cryptStatusError(status) ) { 
      ::SetLastError(ERROR_WRITE_FAULT);
      return false;
   }
   return true;
}

