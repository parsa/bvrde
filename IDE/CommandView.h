#if !defined(AFX_COMMANDVIEW_H__20030330_2F74_AC0F_6DD8_0080AD509054__INCLUDED_)
#define AFX_COMMANDVIEW_H__20030330_2F74_AC0F_6DD8_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CCommandView : 
   public CWindowImpl<CCommandView, CRichEditCtrl>,
   public CRichEditCommands<CCommandView>,
   public ICommandListener,
   public IIdleListener
{
public:
   DECLARE_WND_SUPERCLASS(_T("BVRDE_OutputEditView"), CRichEditCtrl::GetWndClassName())

   CCommandView();

   // Attributes

   CMainFrame* m_pMainFrame;

   // Operations
      
   void Clear();

   // Implementation

   CString _ParseLine() const;
   CString _GetSystemErrorText(DWORD dwErr) const;
   void _ExecAndCapture(LPCTSTR pstrCommandLine);

   // Message map and handlers

   BEGIN_MSG_MAP(CCommandView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      MESSAGE_HANDLER(WM_PRINTCLIENT, OnPrintClient)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
      CHAIN_MSG_MAP_ALT( CRichEditCommands<CCommandView>, 1 )
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnPrintClient(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEditCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);

   // ICommandListener

   void OnUserCommand(LPCTSTR pstrCommand, BOOL& bHandled);
};


#endif // !defined(AFX_COMMANDVIEW_H__20030330_2F74_AC0F_6DD8_0080AD509054__INCLUDED_)

