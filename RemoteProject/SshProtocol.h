#if !defined(AFX_SSHPROTOCOL_H__20030716_3213_E032_1D6F_0080AD509054__INCLUDED_)
#define AFX_SSHPROTOCOL_H__20030716_3213_E032_1D6F_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "cryptlib_wrapper.h"

///////////////////////////////////////////////////////
//

class CSshProtocol;


///////////////////////////////////////////////////////
//

class CSshThread : public CThreadImpl
{
public:
   DWORD Run();
   
protected:
   int _GetPrivateKey(CRYPT_CONTEXT *cryptContext, LPCSTR keysetName, LPCSTR keyName, LPCSTR password);
   CHAR _GetByte(CRYPT_SESSION cryptSession, const LPBYTE pBuffer, int dwRead, int& dwPos) const;
   CString _GetLine(LPBYTE pBuffer, int dwStart, int dwEnd) const;

public:
   CSshProtocol* m_pManager;
   IShellCallback* m_pCallback;
};


///////////////////////////////////////////////////////
//

class CSshProtocol : public IRemoteCommandProtocol
{
friend CSshThread;
public:
   CSshProtocol();
   virtual ~CSshProtocol();

// IRemoteCommandProtocol
public:
   void Init(CRemoteProject* pProject, IShellCallback* pCallback = NULL);
   void Clear();
   bool Load(ISerializable* pArc);
   bool Save(ISerializable* pArc);
   bool Start();
   bool Stop();
   void SignalStop();
   bool IsConnected() const;
   bool IsBusy() const;

   CString GetParam(LPCTSTR pstrName) const;
   void SetParam(LPCTSTR pstrName, LPCTSTR pstrValue);

   bool ReadData(CString& s, DWORD dwTimeout = 0);
   bool WriteData(LPCTSTR pstrData);
   bool WriteSignal(BYTE bCmd);
   bool WriteScreenSize(int w, int h);
   bool WaitForConnection();

// Attributes
protected:
   CRemoteProject* m_pProject;
   IShellCallback* m_pCallback;

   CString m_sHost;
   long m_lPort;
   CString m_sUsername;
   CString m_sPassword;
   CString m_sPath;
   CString m_sExtraCommands;
   //
   CSshThread m_thread;
   CEvent m_event;
   DWORD m_dwErrorCode;
   volatile CRYPT_SESSION m_cryptSession;
   volatile bool m_bConnected;
};


#endif // !defined(AFX_SSHPROTOCOL_H__20030716_3213_E032_1D6F_0080AD509054__INCLUDED_)

