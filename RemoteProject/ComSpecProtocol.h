#if !defined(AFX_COMSPECPROTOCOL_H__20070627_BD81_B9E5_1C56_0080AD509054__INCLUDED_)
#define AFX_COMSPECPROTOCOL_H__20070627_BD81_B9E5_1C56_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


////////////////////////////////////////////////////////
//

class CComSpecProtocol;


class CComSpecThread : public CThreadImpl<CComSpecThread>
{
public:
   CComSpecThread();

   DWORD Run();
   
protected:
   CString _GetLine(LPBYTE pBuffer, DWORD dwStart, DWORD dwEnd) const;

public:
   CComSpecProtocol* m_pManager;
   IShellCallback* m_pCallback;
   HANDLE m_hPipeStdOut;
   HANDLE m_hPipeStdIn;
   HANDLE m_hPipeRead;
   HANDLE m_hPipeWrite;
   DWORD m_dwProcessId;
};


///////////////////////////////////////////////////////
//

class CComSpecProtocol : public IRemoteCommandProtocol
{
friend CComSpecThread;
public:
   CComSpecProtocol();
   virtual ~CComSpecProtocol();

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

   bool WriteData(LPCTSTR pstrData);
   bool WriteSignal(BYTE bCmd);
   bool WriteScreenSize(int w, int h);
   bool WaitForConnection();

// Attributes
protected:
   CRemoteProject* m_pProject;
   IShellCallback* m_pCallback;

   CString m_sPath;
   CString m_sExtraCommands;
   //
   CComSpecThread m_thread;
   volatile DWORD m_dwErrorCode;
   volatile bool m_bConnected;
   DWORD m_dwProcessId;
};


#endif // !defined(AFX_COMSPECPROTOCOL_H__20070627_BD81_B9E5_1C56_0080AD509054__INCLUDED_)

