#if !defined(AFX_COMMANDS_H__20040420_973B_94B7_53E0_0080AD509054__INCLUDED_)
#define AFX_COMMANDS_H__20040420_973B_94B7_53E0_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CCommandThread : 
   public CThreadImpl<CCommandThread>, 
   public ILineCallback
{
public:
   CString m_sCommand;
   CString m_sResult;
   long m_lTimeout;

   CString GetResult() const;
   void SetCommand(UINT nCmd, LPCTSTR pstrCommand, LONG lTimeout = 4000L);

   DWORD Run();

   void _SplitResult(CString sResult, CSimpleArray<CString>& aLines) const;

   // ILineCallback

   HRESULT STDMETHODCALLTYPE OnIncomingLine(BSTR bstr);

   // IUnknown

   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);
};


class CScCommands
{
public:
   CCommandThread m_thread;

   bool bEnabled;
   CString sType;
   CString sProgram;
   CString sOutput;
   CString sCmdCheckIn;
   CString sCmdCheckOut;
   CString sCmdUpdate;
   CString sCmdAddFile;
   CString sCmdRemoveFile;
   CString sCmdStatus;
   CString sCmdDiff;
   CString sCmdLogIn;
   CString sCmdLogOut;
   CString sOptRecursive;
   CString sOptMessage;
   CString sOptStickyTag;
   CString sOptUpdateDirs;
   CString sOptBranch;
   CString sOptCommon;
   CString sBrowseAll;
   CString sBrowseSingle;

   bool Init();
   bool CollectFiles(CSimpleArray<CString>& aFiles);
   bool CheckIn(CSimpleArray<CString>& sFiles);
   bool CheckOut(CSimpleArray<CString>& sFiles);
   bool Update(CSimpleArray<CString>& sFiles);
   bool AddFile(CSimpleArray<CString>& sFiles);
   bool RemoveFile(CSimpleArray<CString>& sFiles);
   bool LogIn(CSimpleArray<CString>& sFiles);
   bool LogOut(CSimpleArray<CString>& sFiles);
   bool DiffFile(CSimpleArray<CString>& sFiles);
   bool StatusFile(CSimpleArray<CString>& sFiles);
};

extern CScCommands _Commands;


#endif // !defined(AFX_COMMANDS_H__20040420_973B_94B7_53E0_0080AD509054__INCLUDED_)

