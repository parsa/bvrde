#if !defined(AFX_DBXADAPTOR_H__20041227_7B52_0472_B0AF_0080AD509054__INCLUDED_)
#define AFX_DBXADAPTOR_H__20041227_7B52_0472_B0AF_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


////////////////////////////////////////////////////////
//

class CDbxAdaptor : public IDebuggerAdaptor
{
public:
   CDbxAdaptor();
   virtual ~CDbxAdaptor();

// IDebuggerAdaptor
public:
   void Init(CRemoteProject* pProject);
   CString TransformInput(LPCTSTR pstrInput);
   void TransformOutput(LPCTSTR pstrOutput, CSimpleArray<CString>& aOutput);

// Implementation
private:
   typedef struct tagDBXLOCATION
   {
      tagDBXLOCATION() : lLineNum(0) { };
      CString sFunction;
      CString sFile;
      long lLineNum;
      CString sAddress;
   } DBXLOCATION;

   void _SplitCommand(LPCTSTR pstrInput, CSimpleArray<CString>& aArgs, CString& sFullArgs) const;
   void _AdjustAnswerList(LPCTSTR pstrBeginning, LPCTSTR pstrEnding, CString sNewItem);
   bool _SkipArg(const CSimpleArray<CString>& aArgs, int& iIndex, LPCTSTR pstrMatch) const;
   CString _GetArg(const CSimpleArray<CString>& aArgs, int& iIndex) const;
   bool _GetNumericArg(const CSimpleArray<CString>& aArgs, int& iIndex, long& lBreakpointNo) const;
   void _GetLocationArgs(const CSimpleArray<CString>& aArgs, int& iIndex, DBXLOCATION& Location) const;
   void _GetInputFileLineArgs(const CString sArg, CString& sFile, CString& sLineNum) const;
   void _ParseLocationArgs(const CString sCommand, DBXLOCATION& Location) const;
   bool _HasSignalSignature(LPCTSTR pstrText) const;

// Data members
private:
   typedef enum DBXSTATE
   {
      DBX_UNKNOWN = 0,
      DBX_RUNNING,
      DBX_WHERE,
      DBX_DUMP,
      DBX_STACKARGS,
      DBX_PWD,
      DBX_THREADS,
      DBX_LWPS,
      DBX_PRINT,
      DBX_PRINTC,
      DBX_EXAMINE,
      DBX_REGNAMES,
      DBX_REGS,
      DBX_DISASM,
      DBX_FRAME,
      DBX_HANDLER,
      DBX_STOP,
      DBX_DELETE,
      DBX_DISPLAY,
      DBX_UNDISPLAY,
      DBX_STATUS,
      DBX_RUNARGS,
   };
   
   DBXSTATE m_State;                  /// State machine
   CString m_sReturnValue;            /// Value to return upon end of frame 
   long m_lReturnIndex;               /// Number of lines in compound output detected so far
   long m_lLevel;                     /// Last known WHERE level in compound output
   bool m_bSeenPrompt;                /// Seen prompt yet?
   bool m_bThreadSupport;             /// Has thread support (as opposed to LWPs)
   CString m_sWatchName;              /// Name of currently inspected watch
   CSimpleMap<CString, CString> m_aWatches;
};



#endif // !defined(AFX_DBXADAPTOR_H__20041227_7B52_0472_B0AF_0080AD509054__INCLUDED_)
