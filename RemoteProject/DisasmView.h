#if !defined(AFX_DISASMVIEW_H__20040710_1AF0_D6DF_359C_0080AD509054__INCLUDED_)
#define AFX_DISASMVIEW_H__20040710_1AF0_D6DF_359C_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRemoteProject; // Forward declare


class CDisasmView : 
   public CWindowImpl<CDisasmView>,
   public IIdleListener
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_DisasmView"))

   typedef struct STREAMCOOKIE
   {
      LPCTSTR pstr;
      LONG pos;
   };

   enum
   {
      IDC_SCROLLUP = 0x2000,
      IDC_SCROLLDOWN,
   };

   CDisasmView();

   CRemoteProject* m_pProject;
   CEdit m_ctrlAddress;
   CButton m_ctrlUp;
   CButton m_ctrlDown;
   CRichEditCtrl m_ctrlView;
   CFont m_fontWingdings;
   TEXTMETRIC m_tm;
   bool m_bIntelStyle;
   bool m_bShowSource;
   bool m_bDontResetOffset;
   BYTE m_iLastStyle;
   long m_lOffset;

   // Operations

   void Init(CRemoteProject* pProject);
   bool WantsData();
   void SetInfo(LPCTSTR pstrType, CMiInfo& info);
   void EvaluateView(CSimpleArray<CString>& aDbgCmd);

   int _GetPageSize() const;

   // Implementation

   static DWORD CALLBACK _EditStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);

   // Message map and handlers

   BEGIN_MSG_MAP(CDisasmView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      COMMAND_ID_HANDLER(ID_DEBUG_STEP_INTO, OnProjectCommand)
      COMMAND_ID_HANDLER(ID_DEBUG_STEP_INSTRUCTION, OnProjectCommand)
      COMMAND_ID_HANDLER(ID_DISASM_SHOWSOURCE, OnShowSource)
      COMMAND_ID_HANDLER(ID_DISASM_INTELSTYLE, OnIntelStyle)
      COMMAND_ID_HANDLER(ID_DISASM_CURRENT, OnGotoCurrent)
      COMMAND_ID_HANDLER(IDC_SCROLLDOWN, OnScrollDown)
      COMMAND_ID_HANDLER(IDC_SCROLLUP, OnScrollUp)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnProjectCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnShowSource(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnIntelStyle(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnGotoCurrent(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnScrollUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnScrollDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};


#endif // !defined(AFX_DISASMVIEW_H__20040710_1AF0_D6DF_359C_0080AD509054__INCLUDED_)

