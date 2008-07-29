#if !defined(AFX_SYMBOLVIEW_H__20030719_9826_FDF5_D423_0080AD509054__INCLUDED_)
#define AFX_SYMBOLVIEW_H__20030719_9826_FDF5_D423_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FunctionRtf.h"


/////////////////////////////////////////////////////////////////////////
//

enum 
{ 
   WM_SYMBOL_GET_DATA = WM_USER + 100,
   WM_SYMBOL_GOT_DATA 
};

class CSymbolView;  // Forward declare


/////////////////////////////////////////////////////////////////////////
// CSymbolThreadLoader

class CSymbolsLoaderThread : public CThreadImpl<CSymbolsLoaderThread>
{
public:
   CSymbolView* m_pOwner;
   CString m_sPattern;
   WPARAM m_dwCookie;

   void Init(CSymbolView* pOwner, LPCTSTR pstrPattern, WPARAM dwCookie);
   
   DWORD Run();
};


/////////////////////////////////////////////////////////////////////////
// CSymbolView

class CSymbolView : 
   public CWindowImpl<CSymbolView>,
   public IIdleListener,
   public CRawDropSource
{
public:
   DECLARE_WND_CLASS_EX(_T("BVRDE_SymbolView"), 0, COLOR_3DFACE)

   CSymbolView();
   ~CSymbolView();

   CRemoteProject* m_pProject;                   // Reference to project
   CSymbolsLoaderThread m_threadLoader;          // Thread that collects result
   CTagDetails m_SelectedTag;                    // Data about selected tag item during context-menu
   CTagDetails m_SelectedImpl;                   // Data about source implementation
   TAGINFO* m_pCurrentHover;                     // Reference to item during hover
   bool m_bMouseTracked;                         // Mouse is tracked in window?
   WPARAM m_dwCookie;                            // Cookie to keep thread in sync with pattern entered
   CString m_sSortValue;                         // List sorting direction

   CToolBarXPCtrl m_ctrlToolbar;
   CContainedWindowT<CEdit> m_ctrlPattern;
   CButton m_ctrlSearch;
   CContainedWindowT<CListViewCtrl> m_ctrlList;
   CFunctionRtfCtrl m_ctrlHoverTip;

   // Operations

   void Init(CRemoteProject* pProject);
   void Close();
   void Clear();
   
   // Implementation

   void _PopulateList(CSimpleValArray<TAGINFO*>& aList);

   static int CALLBACK CSymbolView::_ListSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);

   // Message map and handlers

   BEGIN_MSG_MAP(CSymbolView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_SIZE, OnSize)     
      MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
      MESSAGE_HANDLER(WM_SYMBOL_GET_DATA, OnGetSymbolData)
      MESSAGE_HANDLER(WM_SYMBOL_GOT_DATA, OnGotSymbolData)
      COMMAND_ID_HANDLER(ID_SYMBOLS_SEARCH, OnPatternChange)      
      COMMAND_HANDLER(ID_SYMBOLS_PATTERN, EN_CHANGE, OnPatternChange)
      NOTIFY_CODE_HANDLER(NM_RCLICK, OnListRightClick)
      NOTIFY_CODE_HANDLER(LVN_BEGINDRAG, OnListBeginDrag)
      NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnListItemChanged)
      NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnListDblClick)
      // FIX: Not entirely sure why this filter is needed...
      if( uMsg == WM_NOTIFY && ((LPNMHDR)lParam)->hwndFrom == m_ctrlToolbar ) REFLECT_NOTIFICATIONS()
      //REFLECT_NOTIFICATIONS()
   ALT_MSG_MAP(1)
      MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)     
      MESSAGE_HANDLER(WM_MOUSEHOVER, OnMouseHover)     
      MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)     
      NOTIFY_CODE_HANDLER(EN_REQUESTRESIZE, OnRequestResize);
   ALT_MSG_MAP(2)
      MESSAGE_HANDLER(WM_KEYDOWN, OnEditKeyDown)     
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnGetSymbolData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnGotSymbolData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnPatternChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnMouseHover(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnEditKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnPopulate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnListDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnListBeginDrag(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnListItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnListRightClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnRequestResize(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
};


#endif // !defined(AFX_SYMBOLVIEW_H__20030719_9826_FDF5_D423_0080AD509054__INCLUDED_)
