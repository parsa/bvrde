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

   CDisasmView();

   CRemoteProject* m_pProject;
   CEdit m_ctrlAddress;
   CRichEditCtrl m_ctrlView;
   TEXTMETRIC m_tm;
   bool m_bIntelStyle;
   bool m_bShowSource;

   // Operations

   void Init(CRemoteProject* pProject);
   bool WantsData();
   void PopulateView(CSimpleArray<CString>& aDbgCmd);
   void SetInfo(LPCTSTR pstrType, CMiInfo& info);

   // Implementation

   static DWORD CALLBACK _EditStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);

   // Message map and handlers

   BEGIN_MSG_MAP(CDisasmView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      COMMAND_ID_HANDLER(ID_DISASM_SHOWSOURCE, OnShowSource)
      COMMAND_ID_HANDLER(ID_DISASM_INTELSTYLE, OnIntelStyle)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnShowSource(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnIntelStyle(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};


#endif // !defined(AFX_DISASMVIEW_H__20040710_1AF0_D6DF_359C_0080AD509054__INCLUDED_)

