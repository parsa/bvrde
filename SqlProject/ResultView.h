#if !defined(AFX_ResultVIEW_H__20030608_61E7_9670_1982_0080AD509054__INCLUDED_)
#define AFX_ResultVIEW_H__20030608_61E7_9670_1982_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "QueryThread.h"

class CDbOperations;


///////////////////////////////////////////////////////7
//

class CResultListCtrl : public CWindowImpl<CResultListCtrl, CListViewCtrl>
{
public:
   DECLARE_WND_SUPERCLASS(_T("BVRDE_SqlResultList"), GetWndClassName())

   DECLARE_EMPTY_MSG_MAP()

   void OnFinalMessage(HWND hWnd);
};


///////////////////////////////////////////////////////7
//

class CResultView : 
   public CWindowImpl<CResultView>
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_SqlResultView"))

   enum { IDC_TEXT = 1040 };
   enum { BACKGROUND_TIMERID = 62 };
   enum { BACKGROUND_TRIGGER_TIME = 20L * 1000L }; 

   CResultView();

   CFont m_font;
   CTabCtrl m_ctrlTab;
   CContainedWindow m_wndParent;
   CRichEditCtrl m_ctrlEdit;
   CSimpleArray<CResultListCtrl*> m_aLists;
   CWindow m_wndClient;

   int m_nErrors;
   CSimpleMap<int, int> m_aLinks;
   CResultListCtrl* m_pCurProcessingList;
   CSimpleMap<CString, int> m_aColumnInfo;

   // Operations

   BOOL PreTranslateMessage(MSG* pMsg);
   void OnIdle(IUpdateUI* pUIBase);

   // Message map and handlers

   BEGIN_MSG_MAP(CResultView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
      MESSAGE_HANDLER(WM_TIMER, OnTimer)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      MESSAGE_HANDLER(WM_USER_DATA_AVAILABLE, OnDataArrived)
      NOTIFY_CODE_HANDLER(TCN_SELCHANGE, OnTabChange)
   ALT_MSG_MAP(1) // Tab control
      NOTIFY_HANDLER(IDC_TEXT, EN_LINK, OnLink)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDataArrived(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnTabChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnLink(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

   // Implementation

   void _ResetLists();
   void _RememberColumnSizes();
};


#endif // !defined(AFX_ResultVIEW_H__20030608_61E7_9670_1982_0080AD509054__INCLUDED_)

