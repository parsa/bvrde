#if !defined(AFX_CLASSVIEW_H__20030719_9826_FDF5_D423_0080AD509054__INCLUDED_)
#define AFX_CLASSVIEW_H__20030719_9826_FDF5_D423_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CClassView : 
   public CWindowImpl<CClassView>,
   public CRawDropSource
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_ClassView"))

   CClassView();

   bool m_bLoaded;
   CRemoteProject* m_pProject;
   int m_iScrollPos;
   CSimpleArray<CString> m_aExpandedNames;

   CImageListCtrl m_Images;
   CTreeViewCtrl m_ctrlTree;

   // Operations

   void Init(CRemoteProject* pProject);
   void Close();
   void Clear();
   void Rebuild();
   void Populate();
   
   // Implementation

   void _PopulateTree();
   void _GoToDefinition(TAGINFO* pTag);

   // Message map and handlers

   BEGIN_MSG_MAP(CClassView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)     
      MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnPopulate)
      NOTIFY_CODE_HANDLER(NM_DBLCLK, OnTreeDblClick)
      NOTIFY_CODE_HANDLER(NM_RCLICK, OnTreeRightClick)
      NOTIFY_CODE_HANDLER(TVN_BEGINDRAG, OnTreeBeginDrag)
      NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING, OnTreeExpanding)
      NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDED, OnTreeExpanded)
      NOTIFY_CODE_HANDLER(TVN_GETDISPINFO, OnGetDisplayInfo)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnPopulate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnTreeDblClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeRightClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeBeginDrag(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeExpanding(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnTreeExpanded(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnGetDisplayInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
};


#endif // !defined(AFX_CLASSVIEW_H__20030719_9826_FDF5_D423_0080AD509054__INCLUDED_)
