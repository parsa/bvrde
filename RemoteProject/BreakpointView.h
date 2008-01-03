#if !defined(AFX_BREAKPOINTVIEW_H__20030517_01D2_DEB1_C8BB_0080AD509054__INCLUDED_)
#define AFX_BREAKPOINTVIEW_H__20030517_01D2_DEB1_C8BB_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRemoteProject;  // Forward declare


class CBreakpointView : 
   public CWindowImpl<CBreakpointView>,
   public IIdleListener
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_BreakpointView"))

   typedef struct tagBREAKINFO {
      int iBrkNr;
      CString sType;
      CString sFile;
      CString sFunc;
      int iLineNum;
      BOOL bEnabled;
      CString sAddress;
      int iIgnoreCount;
      CString sCondition;
      int iHitCount;
      BOOL bTemporary;
   } BREAKINFO;

   CBreakpointView();

   CRemoteProject* m_pProject;
   CListViewCtrl m_ctrlList;
   bool m_bUpdating;
   CSimpleArray<BREAKINFO> m_aItems;

   // Operations

   void Init(CRemoteProject* pProject);
   bool WantsData();
   void SetInfo(LPCTSTR pstrType, CMiInfo& info);
   void EvaluateView(CSimpleArray<CString>& aDbgCmd);

   // Message map and handlers

   BEGIN_MSG_MAP(CBreakpointView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      COMMAND_ID_HANDLER(ID_BREAKPOINTS_OPEN, OnItemOpenSource)
      COMMAND_ID_HANDLER(ID_BREAKPOINTS_DELETE, OnItemDelete)
      COMMAND_ID_HANDLER(ID_BREAKPOINTS_ENABLE, OnItemEnable)
      COMMAND_ID_HANDLER(ID_BREAKPOINTS_DISABLE, OnItemDisable)
      COMMAND_ID_HANDLER(ID_BREAKPOINTS_PROPERTIES, OnItemProperties)
      COMMAND_ID_HANDLER(ID_BREAKPOINTS_REFRESH, OnItemRefresh)
      NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnKeyDown)
      NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
      NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClick)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnItemOpenSource(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnItemDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnItemEnable(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnItemDisable(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnItemRefresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnItemProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);
};


#endif // !defined(AFX_BREAKPOINTVIEW_H__20030517_01D2_DEB1_C8BB_0080AD509054__INCLUDED_)
