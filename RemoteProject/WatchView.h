#if !defined(AFX_WATCHVIEW_H__20030517_6FE8_FBF3_5A5D_0080AD509054__INCLUDED_)
#define AFX_WATCHVIEW_H__20030517_6FE8_FBF3_5A5D_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRemoteProject;

#include "PropertyGrid.h"


class CWatchView : 
   public CWindowImpl<CWatchView>,
   public CRawDropTarget<CWatchView>
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_WatchView"))

   CWatchView();

   CRemoteProject* m_pProject;
   CPropertyGridCtrl m_ctrlGrid;
   CContainedWindow m_wndCatch;

   // Operations

   void Init(CRemoteProject* pProject);
   bool WantsData();
   void ActivateWatches();
   void SetInfo(LPCTSTR pstrType, CMiInfo& info);
   void EvaluateView(CSimpleArray<CString>& aDbgCmd);

   bool DoDrop(LPDATAOBJECT pDataObj);

   // Implementation

   void _CreateWatch(HPROPERTY hProp, LPCTSTR pstrName);
   void _DeleteWatch(HPROPERTY hProp);

   // Message map and handlers

   BEGIN_MSG_MAP(CWatchView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      NOTIFY_CODE_HANDLER(PIN_ADDITEM, OnAddItem);
      NOTIFY_CODE_HANDLER(PIN_ITEMCHANGED, OnItemChanged);
      REFLECT_NOTIFICATIONS()
   ALT_MSG_MAP(1)
      MESSAGE_HANDLER(WM_KEYDOWN, OnGridKeyDown)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnAddItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   //
   LRESULT OnGridKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


#endif // !defined(AFX_WATCHVIEW_H__20030517_6FE8_FBF3_5A5D_0080AD509054__INCLUDED_)

