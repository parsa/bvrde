#if !defined(AFX_BREAKPOINTVIEW_H__20030517_01D2_DEB1_C8BB_0080AD509054__INCLUDED_)
#define AFX_BREAKPOINTVIEW_H__20030517_01D2_DEB1_C8BB_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRemoteProject;  // Forward declare


class CBreakpointView : 
   public CWindowImpl<CBreakpointView>
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_BreakpointView"))

   CBreakpointView();

   CRemoteProject* m_pProject;
   CListViewCtrl m_ctrlList;
   bool m_bUpdating;

   // Operations

   void Init(CRemoteProject* pProject);
   bool WantsData();
   void SetInfo(LPCTSTR pstrType, CMiInfo& info);

   // Message map and handlers

   BEGIN_MSG_MAP(CBreakpointView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnKeyDown)
      NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
      NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClick)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
};


#endif // !defined(AFX_BREAKPOINTVIEW_H__20030517_01D2_DEB1_C8BB_0080AD509054__INCLUDED_)

