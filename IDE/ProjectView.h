#if !defined(AFX_PROJECTVIEW_H__20030227_D7CC_9E1C_3668_0080AD509054__INCLUDED_)
#define AFX_PROJECTVIEW_H__20030227_D7CC_9E1C_3668_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CProjectView : public CWindowImpl<CProjectView>
{
public:
   DECLARE_WND_CLASS_EX(_T("BVRDE_ProjectView"), 0, COLOR_3DFACE)

   typedef BOOL (CALLBACK* TREEENUMPROC)(CTreeViewCtrl&, HTREEITEM, LPARAM);

   CToolBarXPCtrl m_ctrlToolbar;
   CTreeViewCtrl m_ctrlTree;
   CImageListCtrl m_Images;

   BEGIN_MSG_MAP(CProjectView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      NOTIFY_HANDLER(IDC_TREE, TVN_ITEMEXPANDED, OnItemExpanded)      
      NOTIFY_HANDLER(IDC_TREE, NM_RCLICK, OnRClick)
      NOTIFY_ID_HANDLER(IDC_TREE, OnTreeMessage)
      // FIX: Not entirely sure why this filter is needed...
      if( uMsg == WM_NOTIFY && ((LPNMHDR)lParam)->hwndFrom == m_ctrlToolbar ) REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlToolbar.SubclassWindow( CFrameWindowImplBase<>::CreateSimpleToolBarCtrl(m_hWnd, IDR_PROJECT, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE) );

      m_Images.Create(IDB_FOLDERS, 16, 0, RGB(255,0,255));
      DWORD dwStyle = WS_BORDER | WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_EDITLABELS;
      m_ctrlTree.Create(m_hWnd, rcDefault, NULL, dwStyle, 0, IDC_TREE);
      m_ctrlTree.SetImageList(m_Images, TVSIL_NORMAL);

      Clear();

      return 0;
   }
   LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      return TRUE; // Children fills entire client area
   }
   LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      if( !m_ctrlToolbar.IsWindow() || !m_ctrlTree.IsWindow() ) return 0;
      RECT rc = { 0 };
      GetClientRect(&rc);
      RECT rcTb = { rc.left, rc.top, rc.right, rc.top + 24 };
      m_ctrlToolbar.MoveWindow(&rcTb);
      m_ctrlToolbar.GetWindowRect(&rcTb);
      RECT rcTree = { rc.left, rcTb.bottom - rcTb.top, rc.right, rc.bottom };
      m_ctrlTree.MoveWindow(&rcTree);
      return 0;
   }
   LRESULT OnTreeMessage(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      return ::SendMessage(GetTopLevelWindow(), WM_APP_TREEMESSAGE, 0, (LPARAM) pnmh);
   }
   LRESULT OnItemExpanded(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
   {
      bHandled = FALSE;
      // Toggle the icon of the folder items.
      // They have both an open/closed state.
      LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) pnmh;
      TVITEM item = lpNMTV->itemNew;
      item.mask |= TVIF_IMAGE;
      m_ctrlTree.GetItem(&item);
      if( item.iImage == 0 || item.iImage == 1 ) {
         item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
         item.iImage = item.iSelectedImage = (lpNMTV->action & TVE_EXPAND) != 0 ? 1 : 0;
         m_ctrlTree.SetItem(&item);
      }
      return 0;
   }
   LRESULT OnRClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
   {
      bHandled = FALSE;
      // Show popup menu for Solution if this item was clicked...
      LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) pnmh;
      HTREEITEM hItem = m_ctrlTree.GetDropHilightItem();
      if( hItem == NULL ) hItem = m_ctrlTree.GetSelectedItem();
      if( hItem == NULL ) return 0;
      IElement* pElement = (IElement*) m_ctrlTree.GetItemData(hItem);
      CString sType;
      pElement->GetType(sType.GetBufferSetLength(64), 64);
      sType.ReleaseBuffer();
      if( sType != _T("Solution") ) return 0;
      CMenu menu;
      menu.LoadMenu(IDR_SOLUTION);
      CMenuHandle submenu = menu.GetSubMenu(0);
      DWORD dwPos = ::GetMessagePos();
      POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
      g_pDevEnv->ShowPopupMenu(pElement, submenu, pt);
      bHandled = TRUE;
      return 0;
   }

   // Operations

   void Clear()
   {
      ATLASSERT(m_ctrlTree.IsWindow());
      // Remove all items and insert the default "Solution" item...
      m_ctrlTree.DeleteAllItems();
      HTREEITEM hRoot;
      CString sSolution(MAKEINTRESOURCE(IDS_SOLUTION));
      hRoot = m_ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
         sSolution, 
         IDE_TREEIMAGE_SOLUTION, IDE_TREEIMAGE_SOLUTION, 
         0, 0,
         (LPARAM) g_pSolution,
         TVI_ROOT, TVI_LAST);
      m_ctrlTree.Expand(hRoot);
   }
   void SelectRoot()
   {
      m_ctrlTree.SelectItem(m_ctrlTree.GetRootItem());
      m_ctrlTree.EnsureVisible(m_ctrlTree.GetRootItem());
   }
   void SetActiveProject(IProject* pProject)
   {
      _EnumTreeItems(_SetActiveProjectProc, (long) pProject, m_ctrlTree.GetRootItem());
   }
   IView* GetActiveView() const
   {
      HTREEITEM hItem = m_ctrlTree.GetDropHilightItem();
      if( hItem == NULL ) hItem = m_ctrlTree.GetSelectedItem();
      if( hItem == NULL ) return NULL;
      LPARAM lParam = m_ctrlTree.GetItemData(hItem);
      if( lParam == NULL ) return NULL;
      return (IView*) lParam;
   }
   void SetActiveView(IView* pView)
   {
      _EnumTreeItems(_SetActiveViewProc, (long) pView, m_ctrlTree.GetRootItem());
   }

   // Implementation

   static BOOL CALLBACK _SetActiveProjectProc(CTreeViewCtrl& tree, HTREEITEM hItem, LPARAM lParam)
   {
      TVITEM tvi = { 0 };
      tvi.hItem = hItem;
      tvi.mask = TVIF_PARAM;
      tree.GetItem(&tvi);
      tvi.mask = TVIF_STATE;
      tvi.state = lParam == tvi.lParam ? TVIS_BOLD : 0;
      tvi.stateMask = TVIS_BOLD;
      tree.SetItem(&tvi);
      if( lParam == tvi.lParam ) {
         tree.SelectItem(hItem);
         tree.EnsureVisible(hItem);
      }
      return TRUE;
   }
   static BOOL CALLBACK _SetActiveViewProc(CTreeViewCtrl& tree, HTREEITEM hItem, LPARAM lParam)
   {
      if( (LPARAM) tree.GetItemData(hItem) == lParam ) {
         tree.SelectItem(hItem);
         //tree.EnsureVisible(hItem);
         return FALSE;
      }
      return TRUE;
   }
   BOOL _EnumTreeItems(TREEENUMPROC pProc, LPARAM lParam, HTREEITEM hItem)
   {
      if( !(*pProc)(m_ctrlTree, hItem, lParam) ) return FALSE;
      HTREEITEM hSibling = m_ctrlTree.GetNextSiblingItem(hItem);
      while( hSibling ) {
         if( !(*pProc)(m_ctrlTree, hSibling, lParam) ) return FALSE;
         hSibling = m_ctrlTree.GetNextSiblingItem(hSibling);
      }
      HTREEITEM hChild = m_ctrlTree.GetChildItem(hItem);
      if( hChild ) {
         if( !(*pProc)(m_ctrlTree, hChild, lParam) ) return FALSE;
         if( !_EnumTreeItems(pProc, lParam, hChild) ) return FALSE;
      }
      return TRUE;
   }
};


#endif // !defined(AFX_PROJECTVIEW_H__20030227_D7CC_9E1C_3668_0080AD509054__INCLUDED_)

