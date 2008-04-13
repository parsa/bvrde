#if !defined(AFX_SYMBOLVIEW_H__20030719_9826_FDF5_D423_0080AD509054__INCLUDED_)
#define AFX_SYMBOLVIEW_H__20030719_9826_FDF5_D423_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FunctionRtf.h"


class CSymbolView : 
   public CWindowImpl<CSymbolView>,
   public IIdleListener,
   public CRawDropSource
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_SymbolView"))

   CSymbolView();

   CRemoteProject* m_pProject;                   // Reference to project
   CTagDetails m_SelectedTag;                    // Data about selected tag item during context-menu
   CTagDetails m_SelectedImpl;                   // Data about source implementation
   TAGINFO* m_pCurrentHover;                     // Reference to item during hover
   bool m_bMouseTracked;                         // Mouse is tracked in window?

   CImageListCtrl m_Images;
   CEdit m_ctrlFilter;
   CButton m_ctrlSearch;
   CContainedWindowT<CTreeViewCtrl> m_ctrlTree;
   CFunctionRtfCtrl m_ctrlHoverTip;

   // Operations

   void Init(CRemoteProject* pProject);
   void Close();
   void Clear();
   
   // Implementation

   void _PopulateTree();
   bool _GetImplementationRef(const CTagDetails& Current, CTagDetails& Info);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);

   // Message map and handlers

   BEGIN_MSG_MAP(CSymbolView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)     
      NOTIFY_CODE_HANDLER(NM_DBLCLK, OnTreeDblClick)
      NOTIFY_CODE_HANDLER(NM_RCLICK, OnTreeRightClick)
      NOTIFY_CODE_HANDLER(TVN_BEGINDRAG, OnTreeBeginDrag)
      NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnTreeSelChanged)
   ALT_MSG_MAP(1)
      MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)     
      MESSAGE_HANDLER(WM_MOUSEHOVER, OnMouseHover)     
      MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)     
      NOTIFY_CODE_HANDLER(EN_REQUESTRESIZE, OnRequestResize);
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnMouseHover(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnPopulate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnTreeDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeBeginDrag(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeSelChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeRightClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnRequestResize(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
};


#endif // !defined(AFX_SYMBOLVIEW_H__20030719_9826_FDF5_D423_0080AD509054__INCLUDED_)
