#if !defined(AFX_SHELLMANAGER_H__20030715_7A7D_7035_419A_0080AD509054__INCLUDED_)
#define AFX_SHELLMANAGER_H__20030715_7A7D_7035_419A_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define TERMINAL_BREAK  1
#define TERMINAL_QUIT   2


/**
 * @class CShellManager
 * A proxy class for Command Prompt protocols (telnet, RLogin, SSH, etc).
 */
class CShellManager :  
   public IShellCallback,
   public IRemoteCommandProtocol
{
public:
   CShellManager();
   virtual ~CShellManager();

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

   bool AddLineListener(IOutputLineListener* pListener);
   bool RemoveLineListener(IOutputLineListener* pListener);

   bool WriteData(LPCTSTR pstrData);
   bool WriteSignal(BYTE bCmd);
   bool WriteScreenSize(int w, int h);
   void BroadcastLine(VT100COLOR nColor, LPCTSTR pstrText);
   void PreAuthenticatedLine(LPCTSTR pstrText);
   bool WaitForConnection();

// Attributes
protected:
   CRemoteProject* m_pProject;
   CString m_sType;                /// Prompt type requested
   CString m_sServerType;          /// Server type detected
   IRemoteCommandProtocol* m_pProtocol;
   CSimpleValArray<IOutputLineListener*> m_LineListeners;
};


#endif // !defined(AFX_SHELLMANAGER_H__20030715_7A7D_7035_419A_0080AD509054__INCLUDED_)

