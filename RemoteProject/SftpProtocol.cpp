
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
   SSH_FXP_INIT = 1,
   SSH_FXP_VERSION =   2,
   SSH_FXP_OPEN = 3,
   SSH_FXP_CLOSE = 4,
   SSH_FXP_READ = 5,
   SSH_FXP_WRITE = 6,
   SSH_FXP_LSTAT = 7,
   SSH_FXP_FSTAT = 8,
   SSH_FXP_SETSTAT = 9,
   SSH_FXP_FSETSTAT = 10,
   SSH_FXP_OPENDIR = 11,
   SSH_FXP_READDIR = 12,
   SSH_FXP_REMOVE = 13,
   SSH_FXP_MKDIR = 14,
   SSH_FXP_RMDIR = 15,
   SSH_FXP_REALPATH = 16,
   SSH_FXP_STAT = 17,
   SSH_FXP_RENAME = 18,
   SSH_FXP_READLINK = 19,
   SSH_FXP_SYMLINK = 20,
   SSH_FXP_STATUS = 101,
   SSH_FXP_HANDLE = 102,
   SSH_FXP_DATA = 103,
   SSH_FXP_NAME = 104,
   SSH_FXP_ATTRS = 105,
};

enum
{
   SSH_FX_OK = 0,
   SSH_FX_EOF = 1,
};

enum
{
   SSH_FXF_READ = 0x00000001,
   SSH_FXF_WRITE = 0x00000002,
   SSH_FXF_APPEND = 0x00000004,
   SSH_FXF_CREAT = 0x00000008,
   SSH_FXF_TRUNC = 0x00000010,
   SSH_FXF_EXCL = 0x00000020,
};

enum
{
   SSH_FILEXFER_ATTR_SIZE = 0x00000001,
   SSH_FILEXFER_ATTR_UIDGID = 0x00000002,
   SSH_FILEXFER_ATTR_PERMISSIONS = 0x00000004,
   SSH_FILEXFER_ATTR_ACMODTIME = 0x00000008,
   SSH_FILEXFER_ATTR_EXTENDED = 0x80000000 ,
};



////////////////////////////////////////////////////////
//

DWORD CSftpThread::Run()
{
   ATLASSERT(m_pManager);

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
   CString sHost = m_pManager->m_sHost;
   CString sUsername = m_pManager->m_sUsername;
   CString sPassword = m_pManager->m_sPassword;
   long lPort = m_pManager->m_lPort;
   CString sCertificate = m_pManager->GetParam(_T("Certificate"));
   CString sPath = m_pManager->m_sPath;
   CString sProxy = m_pManager->m_sProxy;
   bool bPassive = m_pManager->m_bPassive;

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
   int status = clib.cryptCreateSession(&cryptSession, CRYPT_UNUSED, CRYPT_SESSION_SSH);
   if( cryptStatusError(status) ) {
      m_pManager->m_dwErrorCode = NTE_BAD_VER;
      if( status == CRYPT_ERROR_PARAM3 ) m_pManager->m_dwErrorCode = ERROR_MEDIA_NOT_AVAILABLE;
      return 0;
   }

   status = clib.cryptSetAttributeString(cryptSession, CRYPT_SESSINFO_SERVER_NAME, pstrHost, sHost.GetLength());
   status = clib.cryptSetAttributeString(cryptSession, CRYPT_SESSINFO_USERNAME, pstrUsername, sUsername.GetLength());
   if( cryptStatusOK(status) ) {
      if( !sCertificate.IsEmpty() ) {
         CRYPT_CONTEXT privateKey;
         status = SshGetPrivateKey(clib, &privateKey, pstrCertificate, "BVRDE Certificate", pstrPassword);
         if( cryptStatusOK(status) ) {
            status = clib.cryptSetAttribute(cryptSession, CRYPT_SESSINFO_PRIVATEKEY, privateKey);
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
   if( cryptStatusOK(status) ) {
      status = clib.cryptSetAttributeString(cryptSession, CRYPT_SESSINFO_SSH_SUBSYSTEM, "sftp", 4);
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
   clib.cryptSetAttribute(cryptSession, CRYPT_OPTION_NET_CONNECTTIMEOUT, 10 - 1);
   clib.cryptSetAttribute(cryptSession, CRYPT_OPTION_NET_TIMEOUT, 10);

   // Start connection
   status = clib.cryptSetAttribute(cryptSession, CRYPT_SESSINFO_ACTIVE, TRUE);
   if( cryptStatusError(status) ) {
      m_pManager->m_dwErrorCode = WSASERVICE_NOT_FOUND;
      if( status == CRYPT_ERROR_TIMEOUT ) m_pManager->m_dwErrorCode = ERROR_TIMEOUT;
      if( status == CRYPT_ERROR_WRONGKEY ) m_pManager->m_dwErrorCode = ERROR_NOT_AUTHENTICATED;
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
   m_bPassive = false;
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
   if( m_sPath.IsEmpty() ) m_sPath = _T("/");

   return true;
}

bool CSftpProtocol::Save(ISerializable* pArc)
{
   if( !pArc->WriteItem(_T("Sftp")) ) return false;
   pArc->Write(_T("host"), m_sHost);
   pArc->Write(_T("port"), m_lPort);
   pArc->Write(_T("user"), m_sUsername);
   pArc->Write(_T("password"), m_sPassword);
   pArc->Write(_T("path"), m_sPath);
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
   if( sName == _T("Host") ) return m_sHost;
   if( sName == _T("Username") ) return m_sUsername;
   if( sName == _T("Password") ) return m_sPassword;
   if( sName == _T("Port") ) return ToString(m_lPort);
   if( sName == _T("Proxy") ) return m_sProxy;
   if( sName == _T("Separator") ) return _T("/");
   if( sName == _T("Type") ) return _T("SFTP");
   return "";
}

void CSftpProtocol::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   if( sName == _T("Path") ) m_sPath = pstrValue;
   if( sName == _T("Host") ) m_sHost = pstrValue;
   if( sName == _T("Username") ) m_sUsername = pstrValue;
   if( sName == _T("Password") ) m_sPassword = pstrValue;
   if( sName == _T("Port") ) m_lPort = _ttol(pstrValue);
   if( sName == _T("Passive") ) m_bPassive = _tcscmp(pstrValue, _T("true")) == 0;
   if( sName == _T("Proxy") ) m_sProxy = pstrValue;
}

bool CSftpProtocol::LoadFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE* ppOut, DWORD* pdwSize /* = NULL*/)
{
   ATLASSERT(pstrFilename);
   ATLASSERT(ppOut);

   USES_CONVERSION;

   *ppOut = NULL;
   if( pdwSize ) *pdwSize = 0;

   // Connected?
   if( !_WaitForConnection() ) return false;
   // Need a valid session
   if( m_cryptSession == 0 ) return false;

   // Construct remote filename
   CString sFilename;
   sFilename.Format(_T("%s/%s"), m_sCurDir, pstrFilename);
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

      if( _ReadData(m_cryptSession, buffer, sizeof(buffer)) == 0 ) return false;
      p = buffer;
      DWORD cbSize   = SSH_READ_LONG(p);
      BYTE id        = SSH_READ_BYTE(p);
      DWORD msgid    = SSH_READ_LONG(p);
      DWORD dwStatus = SSH_READ_LONG(p);
      ATLASSERT(msgid==m_dwMsgId);

      if( id == SSH_FXP_STATUS )
      {
         // Done?
         if( dwStatus == SSH_FX_EOF ) {
            *pdwSize = dwOffset;
            break;
         }
         bSuccess = false;
         break;
      }
      else if( id == SSH_FXP_DATA )
      {
         // More data to us
         DWORD dwRead = dwStatus;
         LPBYTE pData = p;
         if( *ppOut == NULL ) *ppOut = (LPBYTE) malloc(dwRead); else *ppOut = (LPBYTE) realloc(*ppOut, dwOffset + dwRead);
         memcpy(*ppOut + dwOffset, pData, dwRead);
         dwOffset += dwRead;
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
      ::SetLastError(ERROR_SOURCE_ELEMENT_EMPTY);
      return false;
   }

   // Connected?
   if( !_WaitForConnection() ) return false;
   // Need a valid session
   if( m_cryptSession == 0 ) return false;

   // Construct remote filename
   CString sFilename;
   sFilename.Format(_T("%s/%s"), m_sCurDir, pstrFilename);
   sFilename.Replace(_T('\\'), _T('/'));

   // Let's remove the file first
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

   // Open file
   enum { _IFREG = 0100000 };  // UNIX regular file
   p = buffer;
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
   cbSize          = SSH_READ_LONG(p);
   id              = SSH_READ_BYTE(p);
   msgid           = SSH_READ_LONG(p);
   DWORD cchHandle = SSH_READ_LONG(p);
   LPBYTE pHandle = p;

   if( cchHandle > 64 ) {
      ::SetLastError(ERROR_INVALID_TARGET_HANDLE);
      return false;
   }
   if( id != SSH_FXP_HANDLE ) {
      ::SetLastError(ERROR_CANNOT_MAKE);
      return false;
   }

   // Send data to remote host
   const DWORD CHUNK_SIZE = sizeof(buffer) - 100UL;
   bool bSuccess = true;
   DWORD dwOffset = 0;
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

bool CSftpProtocol::SetCurPath(LPCTSTR pstrPath)
{
   ATLASSERT(pstrPath);

   // Wait for connection
   if( !_WaitForConnection() ) return false;
   // Need a valid session
   if( m_cryptSession == 0 ) return false;

   CString sPath = pstrPath;
   if( pstrPath[0] != _T('/') ) sPath.Format(_T("%s/%s"), m_sCurDir, pstrPath);

   sPath = _ResolvePath(sPath);
   if( sPath.IsEmpty() ) return false;
   m_sCurDir = sPath;
   
   return true;
}

CString CSftpProtocol::GetCurPath()
{
   return m_sCurDir;
}

bool CSftpProtocol::EnumFiles(CSimpleArray<WIN32_FIND_DATA>& aFiles)
{
   // Wait for connection
   if( !_WaitForConnection() ) return false;
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
         if( cbSize > 13 ) _ReadData(m_cryptSession, buffer, sizeof(buffer));
         break;
      }

      // Failed?
      if( id != SSH_FXP_NAME ) {
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

bool CSftpProtocol::_WaitForConnection()
{
   // Wait for the thread to connect to the SFTP host.
   const DWORD TIMEOUT = 10UL;
   const DWORD SPAWNTIMEOUT = 2UL;
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

      // Timeout after 10 sec.
      if( ::GetTickCount() - dwTickStart > TIMEOUT * 1000L ) {
         ::SetLastError(ERROR_TIMEOUT);
         return false;
      }

      bHasRun |= m_thread.IsRunning();
      
      // If the connect thread hasn't run after 2 secs, we might
      // as well assume it failed or didn't run at all...
      if( !bHasRun          
          && ::GetTickCount() - dwTickStart > SPAWNTIMEOUT * 1000L ) 
      {
         // Attempt to reconnect to the remote host.
         Start();
         ::SetLastError(m_dwErrorCode == 0 ? ERROR_NOT_CONNECTED : m_dwErrorCode);
         return false;
      }

      PumpIdleMessages();
   }
   ATLASSERT(m_cryptSession!=0);
   return true;
}

