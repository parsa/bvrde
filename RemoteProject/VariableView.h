#if !defined(AFX_VARIABLEVIEW_H__20030517_1E4D_CA9C_B0E8_0080AD509054__INCLUDED_)
#define AFX_VARIABLEVIEW_H__20030517_1E4D_CA9C_B0E8_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CoolTabCtrls.h"
#include "PropertyList.h"

class CRemoteProject;


class CVariableView : 
   public CWindowImpl<CVariableView>
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_VariableView"))

   CVariableView();

   CRemoteProject* m_pProject;

   CFolderTabCtrl m_ctrlTab;
   CPropertyListCtrl m_ctrlList;

   // Operations

   void Init(CRemoteProject* pProject);
   bool WantsData();
   void SetInfo(LPCTSTR pstrType, CMiInfo& info);
   int GetCurSel() const;

   // Message map and handlers

   BEGIN_MSG_MAP(CVariableView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      NOTIFY_CODE_HANDLER(TCN_SELCHANGE, OnTabChange)
      NOTIFY_CODE_HANDLER(PIN_ITEMCHANGED, OnItemChanged)
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTabChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
};


#endif // !defined(AFX_VARIABLEVIEW_H__20030517_1E4D_CA9C_B0E8_0080AD509054__INCLUDED_)
