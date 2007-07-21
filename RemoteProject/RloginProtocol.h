#if !defined(AFX_RloginPROTOCOL_H__20030715_7A9F_C1A3_A7D3_0080AD509054__INCLUDED_)
#define AFX_RloginPROTOCOL_H__20030715_7A9F_C1A3_A7D3_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


///////////////////////////////////////////////////////
//

class CRloginProtocol;


class CRloginThread : public CThreadImpl<CRloginThread>
{
public:
   DWORD Run();
   
protected:
   CHAR _GetByte(CSocket& socket, const LPBYTE pBuffer, DWORD dwRead, DWORD& dwPos) const;
   CString _GetLine(LPBYTE pBuffer, DWORD dwStart, DWORD dwEnd) const;

public:
   CRloginProtocol* m_pManager;
   IShellCallback* m_pCallback;
};


///////////////////////////////////////////////////////
//

class CRloginProtocol : public IRemoteCommandProtocol
{
friend CRloginThread;
public:
   CRloginProtocol();
   virtual ~CRloginProtocol();

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
   CString m_sLoginPrompt;
   CString m_sPasswordPrompt;
   long m_lSpeed;
   long m_lConnectTimeout;
   //
   CRloginThread m_thread;
   CSocket m_socket;
   CEvent m_event;
   volatile DWORD m_dwErrorCode;
   volatile bool m_bConnected;
   volatile bool m_bCanWindowSize;
};


#endif // !defined(AFX_RloginPROTOCOL_H__20030715_7A9F_C1A3_A7D3_0080AD509054__INCLUDED_)

