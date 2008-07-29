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
   ~CStackView();

   CRemoteProject* m_pProject;
   DWORD m_dwCurThread;
   
   CListBox m_ctrlStack;
   CComboBox m_ctrlThreads;

   // Operations

   void Init(CRemoteProject* pProject);
   bool WantsData();
   void SetInfo(LPCTSTR pstrType, CMiInfo& info);
   void EvaluateView(CSimpleArray<CString>& aDbgCmd);

   // Implementation

   void _SelectThreadId(long lThreadId);

   // Message map and handlers

   BEGIN_MSG_MAP(CStackView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      COMMAND_HANDLER(IDC_TYPE, CBN_SELCHANGE, OnThreadSelChange)
      COMMAND_HANDLER(IDC_LIST, LBN_DBLCLK, OnListDblClick)
      CHAIN_MSG_MAP( COwnerDraw<CStackView> )
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnListDblClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnThreadSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
};


#endif // !defined(AFX_STACKVIEW_H__20030517_B1CC_C904_2AF9_0080AD509054__INCLUDED_)
