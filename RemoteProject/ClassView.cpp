
#include "StdAfx.h"
#include "resource.h"

#include "ClassView.h"
#include "Project.h"
#include "TagInfo.h"


/////////////////////////////////////////////////////////////////////////
// CTagElement

class CTagElement : public IElement
{
public:
   CTagDetails Info;

   CTagElement(const CTagDetails& Tag) : Info(Tag) 
   {
   }
   BOOL Load(ISerializable* /*pArc*/)
   {
      return FALSE;
   }
   BOOL Save(ISerializable* pArc)
   {
      CString sKind(MAKEINTRESOURCE(IDS_UNKNOWN));
      switch( Info.TagType ) {
      case TAGTYPE_CLASS:          sKind.LoadString(IDS_CLASS); break;
      case TAGTYPE_FUNCTION:       sKind.LoadString(IDS_FUNCTION); break;
      case TAGTYPE_STRUCT:         sKind.LoadString(IDS_STRUCT); break;
      case TAGTYPE_MEMBER:         sKind.LoadString(IDS_MEMBER); break;
      case TAGTYPE_DEFINE:         sKind.LoadString(IDS_DEFINE); break;
      case TAGTYPE_TYPEDEF:        sKind.LoadString(IDS_TYPEDEF); break;
      case TAGTYPE_ENUM:           sKind.LoadString(IDS_ENUM); break;
      case TAGTYPE_IMPLEMENTATION: sKind.LoadString(IDS_FUNCTION); break;
      }
      pArc->Write(_T("name"), Info.sName);
      pArc->Write(_T("type"), _T("Tag"));
      pArc->Write(_T("filename"), Info.sFilename);
      pArc->Write(_T("kind"), sKind);
      pArc->Write(_T("match"), Info.sRegExMatch.IsEmpty() ? Info.sDeclaration : Info.sRegExMatch);
      return TRUE;
   }
   BOOL GetName(LPTSTR pstrName, UINT cchMax) const
   {
      return _tcsncpy(pstrName, Info.sName, cchMax) > 0;
   }
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const
   {
      return _tcsncpy(pstrType, _T("Tag"), cchMax) > 0;
   }
   IElement* GetParent() const
   {
      return NULL;
   }
   IDispatch* GetDispatch()
   {
      return NULL;
   }
};



/////////////////////////////////////////////////////////////////////////
// CClassView

CClassView::CClassView() :
   m_ctrlTree(this, 1),
   m_pProject(NULL), 
   m_pCurrentHover(NULL),
   m_bMouseTracked(false),
   m_bPopulated(false), 
   m_bLocked(false)
{
}

CClassView::~CClassView()
{
   if( IsWindow() ) /* scary */
      DestroyWindow();
}

// Operations

void CClassView::Init(CRemoteProject* pProject)
{
   Close();

   m_bLocked = false;
   m_bPopulated = false;
   m_pProject = pProject;
   m_aExpandedNames.RemoveAll();

   if( IsWindow() 
       && m_pProject != NULL
       && m_pProject->m_TagManager.IsAvailable() ) 
   {
      // Add the Class View window to the tab control.
      // NOTE: The following 'static' stuff works because this window object
      //       is already a static object (of the CRemoteProject class).
      static bool s_bAddedToExplorer = false;
      if( !s_bAddedToExplorer ) {
         s_bAddedToExplorer = true;
         _pDevEnv->AddExplorerView(m_hWnd, CString(MAKEINTRESOURCE(IDS_CAPTION_CLASSVIEW)), 11);
      }
   }
}

void CClassView::Close()
{
   if( m_ctrlTree.IsWindow() ) m_ctrlTree.DeleteAllItems();
   m_aExpandedNames.RemoveAll();
   m_bPopulated = false;
   m_bLocked = false;
   m_pProject = NULL;
}

void CClassView::Lock()
{
   // Remember which branches were expanded, so we can
   // expand them when we recreate the tree later...
   // NOTE: The caller is about to restructure the TAG pointers, so
   //       we must do this here while the names are still valid.
   if( m_aExpandedNames.GetSize() == 0 ) 
   {
      HTREEITEM hItem = m_ctrlTree.GetRootItem();
      while( hItem != NULL ) {
         UINT uState = m_ctrlTree.GetItemState(hItem, TVIS_EXPANDED);
         if( (uState & TVIS_EXPANDED) != 0 ) {
            CString sName;
            m_ctrlTree.GetItemText(hItem, sName);
            m_aExpandedNames.Add(sName);
         }
         hItem = m_ctrlTree.GetNextSiblingItem(hItem);
      }
   }
   // It's locked!
   m_bLocked = true;
}

void CClassView::Unlock()
{
   // Unlocking is done in the _PopulateTree() method only
}

void CClassView::Clear()
{
   if( !m_ctrlTree.IsWindow() ) return;
   // Delete all items and reset
   m_bLocked = true;
   m_ctrlTree.SetRedraw(FALSE);
   m_ctrlTree.DeleteAllItems();
   m_ctrlTree.SetRedraw(TRUE);
   m_ctrlTree.Invalidate();
   m_aExpandedNames.RemoveAll();
   m_bPopulated = false;
   m_bLocked = false;
}

void CClassView::Populate()
{
   ATLASSERT(IsWindow());
   // TAGS files already scanned and nothing was found!
   if( m_bPopulated ) return;
   if( m_pProject == NULL ) return;
   // Change status text
   CWaitCursor cursor;
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, CString(MAKEINTRESOURCE(IDS_STATUS_LOADTAG)));
   // Fill up tree...
   Lock();
   Rebuild();
   Unlock();
}

void CClassView::Rebuild()
{
   m_bPopulated = false;             // Signal that we should rebuild the tree   
   if( !IsWindowVisible() ) return;  // View is not showing right now? Delay populating the tree then.   
   if( m_pProject == NULL ) return;  // Not initialized?
   _PopulateTree();                  // Ok, do it then...
}

// Message handlers

LRESULT CClassView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_Images.Create(IDB_CLASSVIEW, 16, 1, RGB(255,0,255));
   DWORD dwStyle = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_TRACKSELECT | TVS_INFOTIP;
   m_ctrlTree.Create(this, 1, m_hWnd, &rcDefault, NULL, dwStyle);
   m_ctrlTree.SetImageList(m_Images, TVSIL_NORMAL);
   ATLASSERT(m_ctrlTree.IsWindow());      
   return 0;
}

LRESULT CClassView::OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   return TRUE; // Children fills entire client area
}

LRESULT CClassView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   CClientRect rc = m_hWnd;
   m_ctrlTree.MoveWindow(&rc);
   return 0;
}

LRESULT CClassView::OnPopulate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   // NOTE: Sadly this is the only way we can detect that the Tab View
   //       has been activated! So Populate() should be able to handle
   //       being called with the tree already populated.
   Populate();
   bHandled = FALSE; // Don't eat!
   return 0;
}

LRESULT CClassView::OnTreeDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   HTREEITEM hItem = m_ctrlTree.GetSelectedItem();
   if( hItem == NULL ) return 0;
   if( m_ctrlTree.GetParentItem(hItem) == NULL ) return 0;
   TAGINFO* pTag = (TAGINFO*) m_ctrlTree.GetItemData(hItem);
   if( pTag == NULL ) return 0;
   CTagDetails Current;
   CTagDetails ImplTag;
   m_pProject->m_TagManager.GetItemInfo(pTag, Current);
   m_pProject->m_TagManager.FindImplementationTag(Current, ImplTag);
   if( !ImplTag.sName.IsEmpty() ) m_pProject->m_TagManager.OpenTagInView(ImplTag);
   else m_pProject->m_TagManager.OpenTagInView(Current);
   return 0;
}

LRESULT CClassView::OnTreeSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) pnmh;
   if( m_bLocked ) return 0;
   if( lpNMTV->itemNew.hItem == NULL ) return 0;
   TAGINFO* pTag = (TAGINFO*) m_ctrlTree.GetItemData(lpNMTV->itemNew.hItem);
   if( pTag == NULL ) return 0;
   CTagDetails Info;
   m_pProject->m_TagManager.GetItemInfo(pTag, Info);
   CTagElement prop = Info;
   _pDevEnv->ShowProperties(&prop, FALSE);
   return 0;
}

LRESULT CClassView::OnTreeRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   // Determine click-point and tree-item below it
   DWORD dwPos = ::GetMessagePos();
   POINT ptPos = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
   POINT ptClient = ptPos;
   ScreenToClient(&ptClient);
   HTREEITEM hItem = m_ctrlTree.HitTest(ptClient, NULL);
   // In case we clicked on a tree-item we'll lookup information
   // about the tag and its implementation status
   TAGINFO* pTag = (TAGINFO*) m_ctrlTree.GetItemData(hItem);
   m_pProject->m_TagManager.GetItemInfo(pTag, m_SelectedTag);
   m_pProject->m_TagManager.FindImplementationTag(m_SelectedTag, m_SelectedImpl);
   // Load and show menu...
   CMenu menu;
   menu.LoadMenu(hItem != NULL ? IDR_CLASSTREE_ITEM : IDR_CLASSTREE);
   CMenuHandle submenu = menu.GetSubMenu(0);
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, ptPos, FALSE, this);
   // Handle result locally
   switch( nCmd ) {
   case ID_CLASSVIEW_SORT_ALPHA:
      {
         _pDevEnv->SetProperty(_T("window.classview.sort"), _T("alpha"));
         Rebuild();
      }
      break;
   case ID_CLASSVIEW_SORT_TYPE:
      {
         _pDevEnv->SetProperty(_T("window.classview.sort"), _T("type"));
         Rebuild();
      }
      break;
   case ID_CLASSVIEW_SORT_NONE:
      {
         _pDevEnv->SetProperty(_T("window.classview.sort"), _T("no"));
         Rebuild();
      }
      break;
   case ID_CLASSVIEW_GOTODECL:
      {
         m_pProject->m_TagManager.OpenTagInView(m_SelectedTag);
      }
      break;
   case ID_CLASSVIEW_GOTOIMPL:
      {
         m_pProject->m_TagManager.OpenTagInView(m_SelectedImpl);
      }
      break;
   case ID_CLASSVIEW_COPY:
      {
         CString sText;
         m_ctrlTree.GetItemText(hItem, sText);
         AtlSetClipboardText(m_hWnd, sText);
      }
      break;
   case ID_CLASSVIEW_MARK:
      {
         CString sText = m_SelectedTag.sName;
         if( sText.Find(_T("::")) >= 0 ) sText = sText.Mid(sText.Find(_T("::")) + 2);
         m_pProject->m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_EDIT_MARK, 0), (LPARAM) (LPCTSTR) sText);
      }
      break;
   case ID_CLASSVIEW_PROPERTIES:
      {
         CTagElement prop = m_SelectedTag;
         _pDevEnv->ShowProperties(&prop, TRUE);
      }
      break;
   }
   return 0;
}

LRESULT CClassView::OnTreeBeginDrag(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   USES_CONVERSION;
   LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) pnmh;
   CString sText;
   m_ctrlTree.GetItemText(lpNMTV->itemNew.hItem, sText);
   CComObject<CSimpleDataObj>* pObj = NULL;
   if( FAILED( CComObject<CSimpleDataObj>::CreateInstance(&pObj) ) ) return 0;
   if( FAILED( pObj->SetTextData(T2CA(sText)) ) ) return 0;
   DWORD dwEffect = 0;
   ::DoDragDrop(pObj, this, DROPEFFECT_COPY, &dwEffect);
   return 0;
}

LRESULT CClassView::OnTreeExpanding(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) pnmh;
   TVITEM& tvi = lpNMTV->itemNew;
   if( (lpNMTV->action & TVE_EXPAND) != 0 ) {
      CWaitCursor cursor;
      CSimpleValArray<TAGINFO*> aList;
      TAGINFO* pParent = (TAGINFO*) m_ctrlTree.GetItemData(tvi.hItem);
      if( pParent == NULL ) {
         m_pProject->m_TagManager.GetGlobalList(aList);
      }
      else {
         // NOTE: We are only collection members attached directly to the
         //       base so we will not list inherited members here.
         m_pProject->m_TagManager.GetMemberList(pParent->pstrName, 0, ::GetTickCount() + 1000, aList);
      }
      for( int i = 0; i < aList.GetSize(); i++ ) {
         const TAGINFO* pTag = aList[i];
         int iImage = pTag->Type == TAGTYPE_MEMBER ? 4 : 3;
         if( iImage == 3 && (pTag->Protection == TAGPROTECTION_PROTECTED || pTag->Protection == TAGPROTECTION_PRIVATE) ) iImage = 5;
         if( iImage == 4 && (pTag->Protection == TAGPROTECTION_PROTECTED || pTag->Protection == TAGPROTECTION_PRIVATE) ) iImage = 6;
         m_ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM,
            LPSTR_TEXTCALLBACK, 
            iImage, iImage, 
            0, 0,
            (LPARAM) pTag,
            tvi.hItem, TVI_LAST);
      }
      if( aList.GetSize() == 0 ) {
         // There were no children of this item
         tvi.mask = TVIF_CHILDREN;
         tvi.cChildren = 0;
         m_ctrlTree.SetItem(&tvi);
      }
      else {
         // Sort the children
         TCHAR szSortValue[32] = { 0 };
         _pDevEnv->GetProperty(_T("window.classview.sort"), szSortValue, 31);   
         if( _tcscmp(szSortValue, _T("alpha")) == 0 ) {
            m_ctrlTree.SortChildren(tvi.hItem, FALSE);
         }
         if( _tcscmp(szSortValue, _T("type")) == 0 ) {
            TVSORTCB tvscb = { 0 };
            tvscb.hParent = tvi.hItem;
            tvscb.lParam = 0;
            tvscb.lpfnCompare = _TreeSortTypeCB;
            m_ctrlTree.SortChildrenCB(&tvscb, FALSE);
         }
      }
   }
   bHandled = FALSE;
   return 0;
}

LRESULT CClassView::OnTreeExpanded(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) pnmh;
   TVITEM& tvi = lpNMTV->itemNew;
   if( lpNMTV->action == TVE_COLLAPSE ) {
      m_ctrlTree.Expand(tvi.hItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
   }
   bHandled = FALSE;
   return 0;
}

LRESULT CClassView::OnGetDisplayInfo(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   LPNMTVDISPINFO lpNMTVDI = (LPNMTVDISPINFO) pnmh;
   if( m_bLocked ) return 0;
   if( (lpNMTVDI->item.mask & TVIF_TEXT) == 0 ) return 0;
   // NOTE: We use a callback for populating the tree text. This saves
   //       memory when keeping a large parse tree, but certainly makes it
   //       more difficult to maintain the tree. Since we modify classes
   //       on the fly, we need to re-populate/clear the tree often.
   TAGINFO* pTag = (TAGINFO*) m_ctrlTree.GetItemData(lpNMTVDI->item.hItem);
   ATLASSERT(!::IsBadReadPtr(pTag, sizeof(TAGINFO)));
   lpNMTVDI->item.pszText = const_cast<LPTSTR>(pTag->pstrName);
   return 0;
}

// Hover Tip message handler

LRESULT CClassView::OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( !m_bMouseTracked ) {
      TRACKMOUSEEVENT tme = { 0 };
      tme.cbSize = sizeof(tme);
      tme.hwndTrack = m_ctrlTree;
      tme.dwFlags = TME_QUERY;
      _TrackMouseEvent(&tme);
      tme.hwndTrack = m_ctrlTree;
      tme.dwFlags |= TME_HOVER | TME_LEAVE;
      tme.dwHoverTime = HOVER_DEFAULT;
      _TrackMouseEvent(&tme);
      m_bMouseTracked = true;
   }
   if( m_ctrlHoverTip.IsWindow() ) m_ctrlTree.SendMessage(WM_MOUSEHOVER);
   bHandled = FALSE;
   return 0;
}

LRESULT CClassView::OnMouseHover(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   // Check if the tree-item below cursor changed; we may want to
   // display a tooltip below the cursor.
   POINT pt = { 0 };
   ::GetCursorPos(&pt);
   POINT ptLocal = pt;
   m_ctrlTree.ScreenToClient(&ptLocal);
   TVHITTESTINFO tvhti = { 0 };
   tvhti.pt = ptLocal;
   m_ctrlTree.HitTest(&tvhti);
   if( (tvhti.flags & TVHT_ONITEM) == 0 ) return m_ctrlTree.SendMessage(WM_MOUSELEAVE);
   TAGINFO* pTag = (TAGINFO*) m_ctrlTree.GetItemData(tvhti.hItem);
   if( pTag == NULL ) return m_ctrlTree.SendMessage(WM_MOUSELEAVE);
   if( m_pCurrentHover == pTag ) return 0;
   if( !m_ctrlHoverTip.IsWindow() ) m_ctrlHoverTip.Create(m_ctrlTree, ::GetSysColor(COLOR_INFOBK));
   // Position, resize and show the tooltip
   m_ctrlHoverTip.ShowWindow(SW_HIDE);
   const INT TOOLTIP_WIDTH = 260;
   while( pt.x + TOOLTIP_WIDTH > ::GetSystemMetrics(SM_CXSCREEN) ) pt.x -= 30;
   m_ctrlHoverTip.MoveWindow(pt.x, pt.y + 40, TOOLTIP_WIDTH, 40);
   CTagDetails Info;
   m_pProject->m_TagManager.GetItemInfo(pTag, Info);
   m_ctrlHoverTip.ShowItem(Info.sName, Info);
   m_pCurrentHover = pTag;
   return 0;
}

LRESULT CClassView::OnMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   if( m_ctrlHoverTip.IsWindow() ) m_ctrlHoverTip.PostMessage(WM_CLOSE);
   m_pCurrentHover = NULL;
   m_bMouseTracked = false;
   return 0;
}

LRESULT CClassView::OnRequestResize(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   REQRESIZE* pRR = (REQRESIZE*) pnmh;
   m_ctrlHoverTip.SetResize(pRR);
   return 0;
}

// IIdleListener

void CClassView::OnIdle(IUpdateUI* pUIBase)
{   
   BOOL bTagIsSelected = !m_SelectedTag.sName.IsEmpty();  // Real Tag is selected in tree (not "Globals" folder, etc)

   TCHAR szSortValue[32] = { 0 };
   _pDevEnv->GetProperty(_T("window.classview.sort"), szSortValue, 31);

   pUIBase->UIEnable(ID_CLASSVIEW_GOTODECL, bTagIsSelected && m_pProject->FindView(m_SelectedTag.sFilename, false) != NULL);
   pUIBase->UIEnable(ID_CLASSVIEW_GOTOIMPL, bTagIsSelected && m_pProject->FindView(m_SelectedImpl.sFilename, false) != NULL);
   pUIBase->UIEnable(ID_CLASSVIEW_COPY, bTagIsSelected);
   pUIBase->UIEnable(ID_CLASSVIEW_MARK, bTagIsSelected);
   pUIBase->UIEnable(ID_CLASSVIEW_PROPERTIES, bTagIsSelected);
   pUIBase->UIEnable(ID_CLASSVIEW_SORT_ALPHA, TRUE);
   pUIBase->UIEnable(ID_CLASSVIEW_SORT_TYPE, TRUE);
   pUIBase->UIEnable(ID_CLASSVIEW_SORT_NONE, TRUE);
   pUIBase->UISetCheck(ID_CLASSVIEW_SORT_ALPHA, _tcscmp(szSortValue, _T("alpha")) == 0);
   pUIBase->UISetCheck(ID_CLASSVIEW_SORT_TYPE, _tcscmp(szSortValue, _T("type")) == 0);
   pUIBase->UISetCheck(ID_CLASSVIEW_SORT_NONE, _tcscmp(szSortValue, _T("no")) == 0);
}

void CClassView::OnGetMenuText(UINT /*wID*/, LPTSTR /*pstrText*/, int /*cchMax*/)
{
}

// Implementation

void CClassView::_PopulateTree()
{
   // Not ready?
   if( m_pProject == NULL ) return;

   CSimpleValArray<TAGINFO*> aList;
   m_pProject->m_TagManager.GetOuterList(aList);

   m_ctrlTree.SetRedraw(FALSE);

   // Remember the scroll position
   int iScrollPos = m_ctrlTree.GetScrollPos(SB_VERT);

   // Clear tree
   m_ctrlTree.DeleteAllItems();  

   // Not locked anymore; tree is safe to access!
   // We need this so we can actually insert items (with text) in the tree.
   m_bLocked = false;

   // Insert classes and expand previously expanded branches...
   HTREEITEM hFirstVisible = NULL;
   TV_INSERTSTRUCT tvis = { 0 };
   tvis.hParent = TVI_ROOT;
   tvis.hInsertAfter = TVI_LAST;
   tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
   tvis.item.iImage = 0;
   tvis.item.iSelectedImage = 0;
   tvis.item.cChildren = 1;
   tvis.item.pszText = LPSTR_TEXTCALLBACK;
   for( int i = 0; i < aList.GetSize(); i++ ) {
      TAGINFO* pTag = aList[i];
      if( pTag->Type == TAGTYPE_CLASS 
          || pTag->Type == TAGTYPE_STRUCT ) 
      {
         tvis.item.lParam = (LPARAM) pTag;
         HTREEITEM hItem = m_ctrlTree.InsertItem(&tvis);
         for( int j = 0; j < m_aExpandedNames.GetSize(); j++ ) {
            if( m_aExpandedNames[j] == pTag->pstrName ) {
               m_ctrlTree.Expand(hItem);
               break;
            }
         }
      }
   }

   // Root items are always sort alphabetically
   m_ctrlTree.SortChildren(TVI_ROOT, FALSE);

   // Insert "Globals" tree-item
   CString sGlobals(MAKEINTRESOURCE(IDS_GLOBALS));
   tvis.item.pszText = (LPTSTR) (LPCTSTR) sGlobals;
   tvis.item.iImage = 1;
   tvis.item.iSelectedImage = 1;
   tvis.item.lParam = 0;
   m_ctrlTree.InsertItem(&tvis);

   m_ctrlTree.SetRedraw(TRUE);

   // FIX: Scrolling must be done outside WM_SETREDRAW section.
   //      See Q130611
   m_ctrlTree.SetScrollPos(SB_VERT, iScrollPos, TRUE);

   m_bPopulated = true;
   m_aExpandedNames.RemoveAll();
}

int CALLBACK CClassView::_TreeSortTypeCB(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   TAGINFO* pTag1 = (TAGINFO*) lParam1;
   TAGINFO* pTag2 = (TAGINFO*) lParam2;
   if( pTag1 == NULL || pTag2 == NULL ) return 0;
   if( pTag1->Type != pTag2->Type ) return (int) pTag1->Type - (int) pTag2->Type;
   return _tcscmp(pTag1->pstrName, pTag2->pstrName);
}

