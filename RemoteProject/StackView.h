#if !defined(AFX_STACKVIEW_H__20030517_B1CC_C904_2AF9_0080AD509054__INCLUDED_)
#define AFX_STACKVIEW_H__20030517_B1CC_C904_2AF9_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Forward declare
class CRemoteProject;


class CStackView : 
   public CWindowImpl<CStackView>,
   public COwnerDraw<CStackView>
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_StackView"))

   CStackView();

   CRemoteProject* m_pProject;
   DWORD m_dwCurThread;
   
   CComboBox m_ctrlThreads;
   CListBox m_ctrlStack;

   // Operations

   void Init(CRemoteProject* pProject);
   bool WantsData();
   void SetInfo(LPCTSTR pstrType, CMiInfo& info);

   // Implementation

   void _SelectThread(long lThreadId);

   // Message map and handlers

   BEGIN_MSG_MAP(CStackView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      COMMAND_HANDLER(IDC_TYPE, CBN_SELCHANGE, OnSelChange)
      COMMAND_HANDLER(IDC_LIST, LBN_DBLCLK, OnDblClick)
      CHAIN_MSG_MAP( COwnerDraw<CStackView> )
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDblClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
};


#endif // !defined(AFX_STACKVIEW_H__20030517_B1CC_C904_2AF9_0080AD509054__INCLUDED_)
