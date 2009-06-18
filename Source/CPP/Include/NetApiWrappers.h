#if !defined(AFX_PROTOCOLWRAPPERS_H__20010923_BB2C_8F54_F0B7_0080AD509054__INCLUDED_)
#define AFX_PROTOCOLWRAPPERS_H__20010923_BB2C_8F54_F0B7_0080AD509054__INCLUDED_

#pragma once

/////////////////////////////////////////////////////////////////////////////
// Network Protocol API Wrappers
//
// Wrappers for the following Windows protocols:
//   MailSlot
//   Named Pipes
//   TCP/IP (Winsock)
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//


/////////////////////////////////////////////////////////////////////////////
// Mail Slots

class CMailSlot
{
public:
   HANDLE m_hMailSlot;
   bool m_fReadSlot;

   CMailSlot(HANDLE hMailSlot = INVALID_HANDLE_VALUE) : m_hMailSlot(hMailSlot)
   {
   }

   ~CMailSlot()
   {
      Close();
   }

   BOOL Create(LPCTSTR pstrName, DWORD dwMaxMsgSize=420, DWORD dwTimeOut=MAILSLOT_WAIT_FOREVER, LPSECURITY_ATTRIBUTES pSec = NULL)
   {
      _ASSERTE(m_hMailSlot==INVALID_HANDLE_VALUE);
      _ASSERTE(!::IsBadStringPtr(pstrName,-1));
      // NOTE: Mailslot creation is only supported on Windows NT.
      // NOTE: Messages sized below 425 bytes are sent as datagrams. 
      //       Various limitations on all Win platforms apply for larger messages.
      if( (m_hMailSlot = ::CreateMailslot(pstrName, dwMaxMsgSize, dwTimeOut, pSec)) == INVALID_HANDLE_VALUE ) return FALSE;
      m_fReadSlot = true;
      return TRUE;
   }

   BOOL Open(LPCTSTR pstrName)
   {
      _ASSERTE(m_hMailSlot==INVALID_HANDLE_VALUE);
      _ASSERTE(!::IsBadStringPtr(pstrName,-1));
      if( (m_hMailSlot = ::CreateFile(pstrName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE ) return FALSE;
      m_fReadSlot = false;
      return TRUE;
   }

   void Close()
   {
      if( m_hMailSlot == INVALID_HANDLE_VALUE ) return;
      ::CloseHandle(m_hMailSlot); 
      m_hMailSlot = INVALID_HANDLE_VALUE;
   }

   BOOL IsNull() const
   {
      return m_hMailSlot == INVALID_HANDLE_VALUE;
   }

   void Attach(HANDLE hPipe)
   {
      _ASSERTE(m_hMailSlot==INVALID_HANDLE_VALUE);
      m_hMailSlot = hPipe;
   }

   HANDLE Detach()
   {
      HANDLE hPipe = m_hMailSlot;
      m_hMailSlot = INVALID_HANDLE_VALUE;
      return hPipe;
   }

   BOOL Read(LPVOID pData, DWORD dwSize, LPDWORD pdwRead = NULL)
   {
      _ASSERTE(m_hMailSlot!=INVALID_HANDLE_VALUE);
      _ASSERTE(m_fReadSlot);
      _ASSERTE(!::IsBadWritePtr(pData, dwSize));
      if( pdwRead != NULL ) *pdwRead = 0;
      if( dwSize == 0 ) return TRUE;
      if( !m_fReadSlot ) return FALSE;
      DWORD dwDummy;
      if( pdwRead == NULL ) pdwRead = &dwDummy;
      return ::ReadFile(m_hMailSlot, pData, dwSize, pdwRead, NULL);
   }

   BOOL Write(LPCVOID pData, DWORD dwSize, LPDWORD pdwWritten = NULL)
   {
      _ASSERTE(m_hMailSlot!=INVALID_HANDLE_VALUE);
      _ASSERTE(!m_fReadSlot);
      _ASSERTE(!::IsBadReadPtr(pData,dwSize));
      if( pdwWritten != NULL ) *pdwWritten = 0;
      if( dwSize == 0 ) return TRUE;
      if( m_fReadSlot ) return FALSE;
      DWORD dwDummy;
      if( pdwWritten == NULL ) pdwWritten = &dwDummy;
      return ::WriteFile(m_hMailSlot, pData, dwSize, pdwWritten, NULL);
   }

   BOOL GetInfo(LPDWORD lpMessageCount = NULL, LPDWORD lpNextSize = NULL, LPDWORD lpMaxMessageSize = NULL, LPDWORD lpReadTimeout = NULL) const
   {
      _ASSERTE(m_hMailSlot!=INVALID_HANDLE_VALUE);
      _ASSERTE(m_fReadSlot);
      // NOTE: Problems with lpNextSize; see Q192276
      return ::GetMailslotInfo(m_hMailSlot, lpMaxMessageSize, lpNextSize, lpMessageCount, lpReadTimeout);
   }

   BOOL SetInfo(DWORD dwTimeOut) const
   {
      _ASSERTE(m_hMailSlot!=INVALID_HANDLE_VALUE);
      _ASSERTE(m_fReadSlot);
      return ::SetMailslotInfo(m_hMailSlot, dwTimeOut);
   }

   DWORD GetPendingMessageCount() const
   {
      _ASSERTE(m_hMailSlot!=INVALID_HANDLE_VALUE);
      _ASSERTE(m_fReadSlot);
      DWORD dwCount;
      if( ::GetMailslotInfo(m_hMailSlot, NULL, NULL, &dwCount, NULL) == FALSE ) return 0;
      return dwCount;
   }

   operator HANDLE() const 
   { 
      return m_hMailSlot; 
   }
};


/////////////////////////////////////////////////////////////////////////////
// Named Pipes

class CNamedPipe
{
public:
   HANDLE m_hPipe;
   HANDLE m_hEvent;
   bool m_bConnected;

   CNamedPipe(HANDLE hPipe = INVALID_HANDLE_VALUE) : 
      m_hPipe(hPipe), 
      m_bConnected(false), 
      m_hEvent(NULL)
   {
   }

   ~CNamedPipe()
   {
      Close();
   }

   BOOL Create(LPCTSTR pstrName, 
      DWORD dwOpenMode,
      DWORD dwPipeMode,
      DWORD dwBufferSize, 
      DWORD dwTimeOut, 
      DWORD dwInstances = PIPE_UNLIMITED_INSTANCES,
      LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL)
   {
      _ASSERTE(m_hPipe==INVALID_HANDLE_VALUE);
      _ASSERTE(!::IsBadStringPtr(pstrName,-1));
      if( (m_hPipe = ::CreateNamedPipe( 
          pstrName,                 // pipe name 
          dwPipeMode,               // pipe mode
          dwOpenMode,               // open mode
          dwInstances,              // max. instances  
          dwBufferSize,             // output buffer size 
          dwBufferSize,             // input buffer size 
          dwTimeOut,                // client time-out 
          lpSecurityAttributes)     // no security attribute 
         ) == INVALID_HANDLE_VALUE ) return FALSE;
      return TRUE;
   }

   BOOL Accept()
   {
      _ASSERTE(m_hPipe!=INVALID_HANDLE_VALUE);
      BOOL fConnected = ::ConnectNamedPipe(m_hPipe, NULL) ? TRUE : (::GetLastError() == ERROR_PIPE_CONNECTED); 
      if( !fConnected ) {
         // The client could not connect, so close the pipe. 
         ::CloseHandle(m_hPipe); 
         m_hPipe = INVALID_HANDLE_VALUE;
      }
      else {
         m_bConnected = true;
      }
      return fConnected;
   }

   BOOL Open(LPCTSTR pstrName, DWORD dwAccess = GENERIC_READ|GENERIC_WRITE, LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL)
   {
      _ASSERTE(m_hPipe==INVALID_HANDLE_VALUE);
      _ASSERTE(!::IsBadStringPtr(pstrName,-1));
      if( (m_hPipe = ::CreateFile(pstrName, dwAccess, 0, lpSecurityAttributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE ) return FALSE;
      return TRUE;
   }

   void Close()
   {
      if( m_hEvent != NULL ) {
         ::CloseHandle(m_hEvent);
         m_hEvent = NULL;
      }
      if( m_bConnected ) {
         ::FlushFileBuffers(m_hPipe); 
         ::DisconnectNamedPipe(m_hPipe); 
         m_bConnected = false;
      }
      if( m_hPipe != INVALID_HANDLE_VALUE ) {
         ::CloseHandle(m_hPipe); 
         m_hPipe = INVALID_HANDLE_VALUE;
      }
   }

   BOOL IsNull() const
   {
      return m_hPipe == INVALID_HANDLE_VALUE;
   }

   void Attach(HANDLE hPipe)
   {
      _ASSERTE(m_hPipe==INVALID_HANDLE_VALUE);
      m_hPipe = hPipe;
   }

   HANDLE Detach()
   {
      HANDLE hPipe = m_hPipe;
      m_hPipe = INVALID_HANDLE_VALUE;
      return hPipe;
   }

   BOOL Read(LPVOID pData, DWORD dwSize, LPDWORD pdwRead = NULL, LPOVERLAPPED lpOverlapped = NULL)
   {
      _ASSERTE(m_hPipe!=INVALID_HANDLE_VALUE);
      _ASSERTE(!::IsBadWritePtr(pData, dwSize));
      if( pdwRead != NULL ) *pdwRead = 0;
      if( dwSize == 0 ) return TRUE;
      DWORD dwDummy;
      if( pdwRead == NULL ) pdwRead = &dwDummy;
      return ::ReadFile(m_hPipe, pData, dwSize, pdwRead, lpOverlapped);
   }

   BOOL Write(LPCVOID pData, DWORD dwSize, LPDWORD pdwWritten = NULL, LPOVERLAPPED lpOverlapped = NULL)
   {
      _ASSERTE(m_hPipe!=INVALID_HANDLE_VALUE);
      _ASSERTE(!::IsBadReadPtr(pData, dwSize));   
      if( pdwWritten != NULL ) *pdwWritten = 0;
      if( dwSize == 0 ) return TRUE;
      DWORD dwDummy;
      if( pdwWritten == NULL ) pdwWritten = &dwDummy;
      return ::WriteFile(m_hPipe, pData, dwSize, pdwWritten, lpOverlapped);
   }

   BOOL GetInfo(LPDWORD lpFlags, LPDWORD lpOutBufferSize = NULL, LPDWORD lpInBufferSize = NULL, LPDWORD lpMaxInstances = NULL) const
   {
      _ASSERTE(m_hPipe!=INVALID_HANDLE_VALUE);
      return ::GetNamedPipeInfo(m_hPipe, lpFlags, lpOutBufferSize, lpInBufferSize, lpMaxInstances);
   }

   BOOL GetHandleState(LPDWORD lpState, LPDWORD lpCurInstances = NULL, LPDWORD lpMaxCollectionCount = NULL, LPDWORD lpCollectDataTimeout = NULL, LPTSTR lpUserName = NULL, DWORD nMaxUserNameSize = 0) const
   {
      _ASSERTE(m_hPipe!=INVALID_HANDLE_VALUE);
      return ::GetNamedPipeHandleState(m_hPipe, lpState, lpCurInstances, lpMaxCollectionCount, lpCollectDataTimeout, lpUserName, nMaxUserNameSize);
   }

   BOOL SetHandleState(LPDWORD lpMode, LPDWORD lpMaxCollectionCount = NULL, LPDWORD lpCollectDataTimeout = NULL) const
   {
      _ASSERTE(m_hPipe!=INVALID_HANDLE_VALUE);
      return ::SetNamedPipeHandleState(m_hPipe, lpMode, lpMaxCollectionCount, lpCollectDataTimeout);
   }

   BOOL Peek(LPDWORD lpTotalBytesAvail, LPDWORD lpBytesLeftThisMessage = NULL, LPVOID lpBuffer = NULL, DWORD nBufferSize = 0, LPDWORD lpBytesRead = NULL) const
   {
      _ASSERTE(m_hPipe!=INVALID_HANDLE_VALUE);
      return ::PeekNamedPipe(m_hPipe, lpBuffer, nBufferSize, lpBytesRead, lpTotalBytesAvail, lpBytesLeftThisMessage);
   }

   BOOL WaitForData(DWORD dwTimeOut=INFINITE)
   {
      _ASSERTE(m_hPipe!=INVALID_HANDLE_VALUE);
      // NOTE: Remember to create pipe with FILE_FLAG_OVERLAPPED flag.
      //       Overlapped results are only supported on Windows NT.
      if( m_hEvent == NULL ) m_hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
      _ASSERTE(m_hEvent!=NULL);
      ::ResetEvent(m_hEvent);
      OVERLAPPED Ovlap = { 0 };
      Ovlap.hEvent = m_hEvent;
      WORD w;
      // Read 0 bytes to get overlapped result
      if( ::ReadFile(m_hPipe, &w, 0, NULL, &Ovlap) == FALSE ) {
         if( ::GetLastError() != ERROR_IO_PENDING ) return TRUE; // error?
      }
      DWORD ret = ::WaitForMultipleObjects(1, &m_hEvent, FALSE, dwTimeOut);
      if( ret == WAIT_FAILED ) return TRUE;
      if( ret == WAIT_TIMEOUT ) return FALSE;
      // Check result
      DWORD dwRead;
      if( ::GetOverlappedResult(m_hPipe, &Ovlap, &dwRead, FALSE) == 0 ) return TRUE;
      return TRUE;
   }

   DWORD GetAvailableDataCount() const
   {
      _ASSERTE(m_hPipe!=INVALID_HANDLE_VALUE);
      DWORD dwAvailable;
      if( ::PeekNamedPipe(m_hPipe, NULL, 0, NULL, &dwAvailable, NULL) == FALSE ) return 0;
      return dwAvailable;
   }

   static BOOL WaitNamedPipe(LPCTSTR pstrName, DWORD dwTimeOut = NMPWAIT_WAIT_FOREVER)
   {
      _ASSERTE(!::IsBadStringPtr(pstrName,-1));
      return ::WaitNamedPipe(pstrName, dwTimeOut);
   }

   operator HANDLE() const 
   { 
      return m_hPipe; 
   }
};


/////////////////////////////////////////////////////////////////////////////
// TCP/IP

#ifndef _NO_WINSOCK

#ifndef _WINSOCK2API_
   #include <winsock.h>
   #pragma comment(lib, "wsock32.lib")
#endif

// defined in winsock2.h
#ifndef SD_BOTH
   #define SD_BOTH 0x02
#endif

struct WSAInit
{
   WSAInit(BYTE bMajor = 1, BYTE bMinor = 1, WSADATA *pData = NULL)
   {
      WSADATA dummy;
      if( pData == NULL ) pData = &dummy;
      ::WSAStartup(MAKEWORD(bMajor,bMinor), pData);
   }

   ~WSAInit()
   {
      ::WSACleanup();
   }
};

class CSocket
{
public:
   SOCKET m_hSocket;
   SOCKADDR_IN m_sockaddr;

   CSocket(SOCKET hSocket = INVALID_SOCKET) : m_hSocket(hSocket)
   {
   }

   ~CSocket()
   {
      Close();
   }

   BOOL Create(u_short iPort, LPCTSTR pstrName = NULL, int iBacklog = 8)
   {
      // Pass NULL as 'pstrName' to bind to default IP interface
      if( OpenSocket(iPort, pstrName) == FALSE ) return FALSE;
      if( ::bind(m_hSocket, (PSOCKADDR) &m_sockaddr, sizeof(m_sockaddr)) == SOCKET_ERROR ) {
         return FALSE;
      }
      return ::listen(m_hSocket, iBacklog) == 0;
   }

   BOOL Open(u_short iPort, LPCTSTR pstrName)
   {
      _ASSERTE(!::IsBadStringPtr(pstrName,-1));
      if( OpenSocket(iPort, pstrName) == FALSE ) return FALSE;
      if( ::connect(m_hSocket, (PSOCKADDR) &m_sockaddr, sizeof(m_sockaddr)) == SOCKET_ERROR ) {
         return FALSE;
      }
      return TRUE;
   }

   BOOL OpenSocket(u_short iPort, LPCTSTR pstrName)
   {
      _ASSERTE(m_hSocket==INVALID_SOCKET);
      _ASSERTE(iPort>0 && iPort<=49152); // NOTE: Port 1024 to 49152 are reseved for you, the rest if for web/ftp/nntp servers      
      m_hSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
      if( m_hSocket == INVALID_SOCKET ) return FALSE;
      if( pstrName == NULL ) {
         m_sockaddr.sin_addr.s_addr = INADDR_ANY;
      }
      else {
         USES_CONVERSION;
         LPCSTR pstrAddress = T2CA(pstrName);
         m_sockaddr.sin_addr.s_addr = ::inet_addr(pstrAddress);
         if( m_sockaddr.sin_addr.s_addr == INADDR_NONE ) {
            PHOSTENT pHost = ::gethostbyname(pstrAddress);
            if( pHost == NULL ) return FALSE;
            ::CopyMemory(&m_sockaddr.sin_addr, pHost->h_addr_list[0], pHost->h_length);
         }
      }
      m_sockaddr.sin_family = AF_INET;
      m_sockaddr.sin_port = ::htons(iPort);
      return TRUE;
   }

   void Close()
   {
      if( m_hSocket == INVALID_SOCKET ) return;
      ::shutdown(m_hSocket, SD_BOTH);
      ::closesocket(m_hSocket);
      m_hSocket = INVALID_SOCKET;
      return;
   }

   SOCKET Accept(PSOCKADDR pClient, int *pAddrSize)
   {
      _ASSERTE(pClient);
      _ASSERTE(pAddrSize);
      return ::accept(m_hSocket, pClient, pAddrSize);
   }

   BOOL IsNull() const
   {
      return m_hSocket == INVALID_SOCKET;
   }

   void Attach(SOCKET hSocket)
   {
      _ASSERTE(m_hSocket==INVALID_SOCKET);
      m_hSocket = hSocket;
   }

   SOCKET Detach()
   {
      SOCKET hSocket = m_hSocket;
      m_hSocket = INVALID_SOCKET;
      return hSocket;
   }

   BOOL Read(LPVOID pData, DWORD dwSize, LPDWORD pdwRead = NULL, int iFlags = 0)
   {
      _ASSERTE(m_hSocket!=INVALID_SOCKET);
      _ASSERTE(!::IsBadWritePtr(pData,dwSize));
      // NOTE: You should always check the 'pdwRead' argument since
      //       large data may be split up in frames.
      if( pdwRead != NULL ) *pdwRead = 0;
      int ret = ::recv(m_hSocket, (char*) pData, dwSize, iFlags);
      if( ret == 0 ) return FALSE; // Closed
      if( ret == SOCKET_ERROR ) return FALSE;
      if( pdwRead != NULL ) *pdwRead = ret;
      return TRUE;
   }

   BOOL Write(LPCVOID pData, DWORD dwSize, LPDWORD pdwWritten = NULL, int iFlags = 0)
   {
      _ASSERTE(m_hSocket!=INVALID_SOCKET);
      _ASSERTE(!::IsBadReadPtr(pData,dwSize));
      if( pdwWritten != NULL ) *pdwWritten = 0;
      // Streaming TCP/IP may not send all data at once
      int iLeft = (int) dwSize;
      int iPos = 0;
      while( iLeft > 0 ) {
         int ret = ::send(m_hSocket, (char*) pData + iPos, iLeft, iFlags);
         if( ret == SOCKET_ERROR ) {
            return FALSE;
         }
         iLeft -= ret;
         iPos += ret;
      }
      if( pdwWritten != NULL ) *pdwWritten = dwSize;
      return TRUE;
   }

   BOOL WaitForData(DWORD dwTimeOut = 0) const
   {
      _ASSERTE(m_hSocket!=INVALID_SOCKET);
      TIMEVAL tm = { 0, dwTimeOut };
      while( true ) {
         fd_set fdread;
         FD_ZERO(&fdread);
         FD_SET(m_hSocket, &fdread);
         int ret = ::select(0, &fdread, NULL, NULL, dwTimeOut == 0 ? NULL : &tm);
         switch( ret ) {
         case 0:
            return FALSE; // timeout
         case SOCKET_ERROR:
            if( ::WSAGetLastError() == WSAEINPROGRESS ) return FALSE; // busy
            // BUG: Hmm, this causes the code to continue and the next Read() to fail
            return TRUE;
         case 1:
            if( FD_ISSET(m_hSocket, &fdread) ) return TRUE;
            // FALL THROUGH...
         default:
            _ASSERTE(false);
         }
      }
   }

   BOOL WaitForSendReady(DWORD dwTimeOut = 0) const
   {
      _ASSERTE(m_hSocket!=INVALID_SOCKET);
      TIMEVAL tm = { 0, dwTimeOut };
      while( true ) {
         fd_set fdsend;
         FD_ZERO(&fdsend);
         FD_SET(m_hSocket, &fdsend);
         int ret = ::select(0, NULL, &fdsend, NULL, dwTimeOut == 0 ? NULL : &tm);
         switch( ret ) {
         case 0:
            return FALSE; // timeout
         case SOCKET_ERROR:
            if( ::WSAGetLastError() == WSAEINPROGRESS ) return FALSE; // busy
            // BUG: Hmm, this causes the code to continue and the next Send() to fail
            return TRUE;
         case 1:
            if( FD_ISSET(m_hSocket, &fdsend) ) return TRUE;
            // FALL THROUGH...
         default:
            _ASSERTE(false);
         }
      }
   }

   DWORD GetAvailableDataCount() const
   {
      _ASSERTE(m_hSocket!=INVALID_SOCKET);
      // NOTE: See Q192599 why this is a bad idea!
      u_long ulRead;
      if( ::ioctlsocket(m_hSocket, FIONREAD, &ulRead) != 0 ) return 0;
      return (DWORD) ulRead;
   }

   operator SOCKET() const 
   { 
      return m_hSocket; 
   }
};

#endif // _NO_WINSOCK


#endif // !defined(AFX_PROTOCOLWRAPPERS_H__20010923_BB2C_8F54_F0B7_0080AD509054__INCLUDED_)

