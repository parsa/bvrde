#if !defined(AFX_FTPMANAGER_H__20030310_991D_9455_553A_0080AD509054__INCLUDED_)
#define AFX_FTPMANAGER_H__20030310_991D_9455_553A_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

////////////////////////////////////////////////////////
//

class CFtpProtocol;


////////////////////////////////////////////////////////
//

class CFtpThread : public CThreadImpl<CFtpThread>
{
public:
   DWORD Run();
   HINTERNET InternetConnect(HINTERNET hInternet, LPCWSTR lpszServerName, INTERNET_PORT nServerPort, LPCWSTR lpszUserName, LPCWSTR lpszPassword, DWORD dwFlags);

   CFtpProtocol* m_pManager;
};


////////////////////////////////////////////////////////
//

class CFtpProtocol : public IRemoteFileProtocol
{
friend CFtpThread;
public:
   CFtpProtocol();
   virtual ~CFtpProtocol();

// IRemoteFileProtocol
public:
   void Clear();
   bool Load(ISerializable* pArc);
   bool Save(ISerializable* pArc);

   bool Start();
   bool Stop();
   void SignalStop();
   bool IsConnected() const;

   bool LoadFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE* ppOut, DWORD* pdwSize = NULL);
   bool SaveFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE ppOut, DWORD dwSize);
   bool SetCurPath(LPCTSTR pstrPath);
   CString GetCurPath();
   CString FindFile(LPCTSTR pstrFilename);
   bool EnumFiles(CSimpleArray<WIN32_FIND_DATA>& aFiles);

   CString GetParam(LPCTSTR pstrName) const;
   void SetParam(LPCTSTR pstrName, LPCTSTR pstrValue);

protected:
   bool _WaitForConnection();

protected:
   CString m_sHost;
   CString m_sUsername;
   CString m_sPassword;
   long m_lPort;
   CString m_sPath;
   CString m_sProxy;
   CString m_sSearchPath;
   BOOL m_bPassive;
   //
   CFtpThread m_thread;
   HINTERNET m_hInternet;
   HINTERNET m_hFTP;
   DWORD m_dwLastCheck;
   bool m_bCancel;
   volatile DWORD m_dwErrorCode;
   volatile bool m_bConnected;
};


#endif // !defined(AFX_FTPMANAGER_H__20030310_991D_9455_553A_0080AD509054__INCLUDED_)

