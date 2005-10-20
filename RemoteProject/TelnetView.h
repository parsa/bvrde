#if !defined(AFX_TELNETVIEW_H__20030318_5B05_F159_45FA_0080AD509054__INCLUDED_)
#define AFX_TELNETVIEW_H__20030318_5B05_F159_45FA_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ShellProxy.h"

#define TNV_EDITABLE          0x00000001
#define TNV_TERMINATEONCLOSE  0x00000002


class CTelnetView : 
   public CScrollWindowImpl<CTelnetView>,
   public IOutputLineListener,
   public IIdleListener
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_TelnetView"))

   CTelnetView();
   ~CTelnetView();

   enum
   {
      MAX_CHARS = 80,
      MAX_LINES = 25,
   };

   typedef struct tagLINE
   {
      VT100COLOR nColor;
      TCHAR szText[MAX_CHARS + 1];
   } LINE;

   CShellManager* m_pShell;
   CSimpleArray<LINE> m_aLines;
   CFontHandle m_font;
   COLORREF m_clrText;
   COLORREF m_clrBack;
   COLORREF m_clrDefBack;
   TEXTMETRIC m_tm;
   int m_iStart;
   DWORD m_dwFlags;

   // Operations

   void Init(CShellManager* pTelnet, DWORD dwFlags = 0);
   void Close();
   void Clear();
   void SetFlags(DWORD dwFlags);
   void SetColors(COLORREF clrText, COLORREF clrBack);

   // Message map and handlers

   BEGIN_MSG_MAP(CTelnetView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
      MESSAGE_HANDLER(WM_COMPACTING, OnCompacting)      
      MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
      MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      COMMAND_ID_HANDLER(ID_VIEW_CLOSE, OnViewClose)
      COMMAND_ID_HANDLER(ID_EDIT_CLEAR, OnEditClear)
      COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
      CHAIN_MSG_MAP( CScrollWindowImpl<CTelnetView> )
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnCompacting(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnViewClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditClear(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   // IOutputListener

   void OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);

   // Implementation
   
   void DoPaint(CDCHandle dc);
};


#endif // !defined(AFX_TELNETVIEW_H__20030318_5B05_F159_45FA_0080AD509054__INCLUDED_)
