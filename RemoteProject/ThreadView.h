#if !defined(AFX_THREADVIEW_H__20030517_1FB5_EEBD_3B70_0080AD509054__INCLUDED_)
#define AFX_THREADVIEW_H__20030517_1FB5_EEBD_3B70_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRemoteProject; // Forward declare


class CThreadView : 
   public CWindowImpl<CThreadView, CListViewCtrl, CWinTraitsOR<LVS_SINGLESEL|LVS_SHOWSELALWAYS|LVS_SORTASCENDING|LVS_LIST> >
{
public:
   DECLARE_WND_SUPERCLASS(_T("BVRDE_ThreadView"), CListViewCtrl::GetWndClassName())

   CThreadView();
   ~CThreadView();

   CRemoteProject* m_pProject;
   DWORD m_dwCurThread;
   double m_dblDbgVersion;

   // Operations

   void Init(CRemoteProject* pProject);
   bool WantsData();
   void SetInfo(LPCTSTR pstrType, CMiInfo& info);
   void EvaluateView(CSimpleArray<CString>& aDbgCmd);

   // Implementation

   void _SelectThreadId(long iThreadId);

   // Message map and handlers

   BEGIN_MSG_MAP(CThreadView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnDblClick)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDblClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


#endif // !defined(AFX_THREADVIEW_H__20030517_1FB5_EEBD_3B70_0080AD509054__INCLUDED_)
