#if !defined(AFX_OUTPUTVIEW_H__20030330_1F18_6A67_9B04_0080AD509054__INCLUDED_)
#define AFX_OUTPUTVIEW_H__20030330_1F18_6A67_9B04_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class COutputView : 
   public CWindowImpl<COutputView, CRichEditCtrl>,
   public IIdleListener
{
public:
   DECLARE_WND_SUPERCLASS(_T("BVRDE_OutputEditView"), CRichEditCtrl::GetWndClassName())

   COutputView();

   // Operations

   void Clear();

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);

   // Message map and handlers

   BEGIN_MSG_MAP(COutputView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_PRINTCLIENT, OnPrintClient)
      MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
      COMMAND_ID_HANDLER(ID_EDIT_CLEARVIEW, OnEditClearView)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnPrintClient(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnLButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEditCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditClearView(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   // Implementation

   bool _OpenView(IProject* pProject, LPCTSTR pstrFilename, long lLineNum);
};


#endif // !defined(AFX_OUTPUTVIEW_H__20030330_1F18_6A67_9B04_0080AD509054__INCLUDED_)

