#if !defined(AFX_MEMORYVIEW_H__20040710_5EB6_EA46_4F33_0080AD509054__INCLUDED_)
#define AFX_MEMORYVIEW_H__20040710_5EB6_EA46_4F33_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRemoteProject; // Forward declare
class CMemoryView;


class CMemoryInlineEdit : public CWindowImpl< CMemoryInlineEdit, CEdit, CControlWinTraits >
{
public:
   DECLARE_WND_SUPERCLASS(_T("BVRDE_MemoryViewInlineEdit"), CEdit::GetWndClassName())

   CMemoryView* m_pMemoryView;
   CRemoteProject* m_pProject;
   CString m_sAddress;
   LONG m_lByteOffset;
   bool m_bCommit;

   void Init(CRemoteProject* pProject, CMemoryView* pMemoryView, LPCTSTR pstrAddress, LONG lByteOffset);

   BEGIN_MSG_MAP(CMemoryInlineEdit)
      MESSAGE_HANDLER(WM_CLOSE, OnClose)
      MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      MESSAGE_HANDLER(WM_CHAR, OnKeyDown)
   END_MSG_MAP()

   LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


class CMemoryView : 
   public CWindowImpl<CMemoryView>,
   public IIdleListener
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_MemoryView"))

   CMemoryView();
   ~CMemoryView();

   typedef struct tagSTREAMCOOKIE
   {
      LPCTSTR pstr;
      LONG pos;
   } STREAMCOOKIE;

   enum { MAX_NUM_BYTES_ON_LINE = 32 };

   CRemoteProject* m_pProject;                      // Project reference
   CContainedWindowT<CEdit> m_ctrlAddress;          // Edit control with address/expression
   CContainedWindowT<CRichEditCtrl> m_ctrlMemory;   // Edit control with memory content
   CMemoryInlineEdit m_ctrlInlineEdit;              // Inline editor control
   CString m_sExpression;                           // Memory location to evaluate (expression)
   CString m_sAddress;                              // Current start address
   CString m_sFormat;                               // Current value display-format (hex, octal etc)
   short m_iDisplaySize;                            // Current word size (digits; don't ask why...)
   bool m_bAllowUpdate;                             // Temporary allow editor modifications

   // Operations

   void Init(CRemoteProject* pProject);
   bool WantsData();
   void SetInfo(LPCTSTR pstrType, CMiInfo& info);

   // Implementation

   void _UpdateDisplay();
   BOOL _DoInlineEditing(LONG lPos, bool bCreateEdit);

   static DWORD CALLBACK _EditStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);

   // Message map and handlers

   BEGIN_MSG_MAP(CMemoryView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      COMMAND_ID_HANDLER(ID_MEMORY_EDIT, OnMemoryEdit)
      COMMAND_ID_HANDLER(ID_MEMORY_REFRESH, OnMemoryRefresh)
      COMMAND_ID_HANDLER(ID_MEMORY_SIZE_DWORD, OnMemorySize)
      COMMAND_ID_HANDLER(ID_MEMORY_SIZE_WORD, OnMemorySize)
      COMMAND_ID_HANDLER(ID_MEMORY_SIZE_BYTE, OnMemorySize)
      NOTIFY_CODE_HANDLER(EN_PROTECTED, OnProtected)
   ALT_MSG_MAP(1)  // Address
      MESSAGE_HANDLER(WM_CHAR, OnEditChar)
      MESSAGE_HANDLER(WM_KEYDOWN, OnEditKeyDown)
   ALT_MSG_MAP(2)  // Memory values
      MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnEditDblClick)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEditChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEditKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEditDblClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnProtected(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnMemoryEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMemoryRefresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMemorySize(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};


#endif // !defined(AFX_MEMORYVIEW_H__20040710_5EB6_EA46_4F33_0080AD509054__INCLUDED_)

