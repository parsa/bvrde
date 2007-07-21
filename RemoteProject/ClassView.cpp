
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
   TAGINFO* m_pTag;

   CTagElement(TAGINFO* pTag) : m_pTag(pTag) 
   {
   }
   BOOL Load(ISerializable* /*pArc*/)
   {
      return FALSE;
   }
   BOOL Save(ISerializable* pArc)
   {
      ATLASSERT(!::IsBadReadPtr(m_pTag,sizeof(TAGINFO)));
      CString sKind(MAKEINTRESOURCE(IDS_UNKNOWN));
      if( m_pTag->Type == TAGTYPE_CLASS ) sKind.LoadString(IDS_CLASS);
      else if( m_pTag->Type == TAGTYPE_FUNCTION ) sKind.LoadString(IDS_FUNCTION);
      else if( m_pTag->Type == TAGTYPE_STRUCT ) sKind.LoadString(IDS_STRUCT);
      else if( m_pTag->Type == TAGTYPE_MEMBER ) sKind.LoadString(IDS_MEMBER);
      else if( m_pTag->Type == TAGTYPE_DEFINE ) sKind.LoadString(IDS_DEFINE);
      else if( m_pTag->Type == TAGTYPE_TYPEDEF ) sKind.LoadString(IDS_TYPEDEF);
      else if( m_pTag->Type == TAGTYPE_ENUM ) sKind.LoadString(IDS_ENUM);
      pArc->Write(_T("name"), m_pTag->pstrName);
      pArc->Write(_T("type"), _T("Tag"));
      pArc->Write(_T("filename"), m_pTag->pstrFile);
      pArc->Write(_T("kind"), sKind);
      pArc->Write(_T("match"), m_pTag->pstrToken);
      return TRUE;
   }
   BOOL GetName(LPTSTR pstrName, UINT cchMax) const
   {
      if( m_pTag == NULL ) return FALSE;
      return _tcsncpy(pstrName, m_pTag->pstrName, cchMax) > 0;
   }
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const
   {
      return _tcsncpy(pstrType, _T("Tag"), cchMax) > 0;
   }
   IDispatch* GetDispatch()
   {
      return NULL;
   }
};



/////////////////////////////////////////////////////////////////////////
// CClassView

CClassView::CClassView() :
   m_pProject(NULL), 
   m_pCurrentTag(NULL),
   m_bPopulated(false), 
   m_bLocked(false)
{
}

// Operations

void CClassView::Init(CRemoteProject* pProject)
{
   Close();

   m_bPopulated = false;
   m_bLocked = false;
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
   m_pProject = NULL;
   m_bPopulated = false;
   m_bLocked = false;
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
   m_aExpandedNames.RemoveAll();
   m_bPopulated = false;
   m_bLocked = false;
}

void CClassView::Populate()
{
   ATLASSERT(IsWindow());
   // TAGS files already scanned and nothing was found!
   if( m_bPopulated ) return;
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
   // Signal that we should rebuild the tree
   m_bPopulated = false;
   // View is not showing right now? Delay populating the tree then.
   if( !IsWindowVisible() ) return;
   // Ok, do it then...
   _PopulateTree();
}

// Message handlers

LRESULT CClassView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   m_Images.Create(IDB_CLASSVIEW, 16, 1, RGB(255,0,255));
   DWORD dwStyle;
   dwStyle = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_TRACKSELECT | TVS_INFOTIP;
   m_ctrlTree.Create(m_hWnd, rcDefault, NULL, dwStyle);
   m_ctrlTree.SetImageList(m_Images, TVSIL_NORMAL);
   ATLASSERT(m_ctrlTree.IsWindow());      
   return 0;
}

LRESULT CClassView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
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
   CString sImplRef = _GetImplementationRef(pTag);
   if( !sImplRef.IsEmpty() ) m_pProject->m_TagManager.OpenTagInView(sImplRef, pTag->pstrName);
   else m_pProject->m_TagManager.OpenTagInView(pTag);
   return 0;
}

LRESULT CClassView::OnTreeSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) pnmh;
   if( m_bLocked ) return 0;
   if( lpNMTV->itemNew.hItem == NULL ) return 0;
   TAGINFO* pTag = (TAGINFO*) m_ctrlTree.GetItemData(lpNMTV->itemNew.hItem);
   if( pTag == NULL ) return 0;
   CTagElement prop = pTag;
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
   m_pCurrentTag = hItem == NULL ? NULL : (TAGINFO*) m_ctrlTree.GetItemData(hItem);
   m_sImplementationEntry = _GetImplementationRef(m_pCurrentTag);
   // Load and show menu
   CMenu menu;
   menu.LoadMenu(hItem != NULL ? IDR_CLASSTREE_ITEM : IDR_CLASSTREE);
   CMenuHandle submenu = menu.GetSubMenu(0);
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, ptPos, FALSE, this);
   // Handle result locally
   switch( nCmd ) {
   case ID_CLASSVIEW_SORT:
      {
         _pDevEnv->SetProperty(_T("window.classview.sort"), _T("alpha"));
         Rebuild();
      }
      break;
   case ID_CLASSVIEW_NOSORT:
      {
         _pDevEnv->SetProperty(_T("window.classview.sort"), _T("no"));
         Rebuild();
      }
      break;
   case ID_CLASSVIEW_GOTODECL:
      {
         m_pProject->m_TagManager.OpenTagInView(m_pCurrentTag);
      }
      break;
   case ID_CLASSVIEW_GOTOIMPL:
      {
         m_pProject->m_TagManager.OpenTagInView(m_sImplementationEntry, m_pCurrentTag->pstrName);
      }
      break;
   case ID_CLASSVIEW_COPY:
      {
         CString sText;
         m_ctrlTree.GetItemText(hItem, sText);
         AtlSetClipboardText(m_hWnd, sText);
      }
      break;
   case ID_CLASSVIEW_PROPERTIES:
      {
         CTagElement prop = m_pCurrentTag;
         _pDevEnv->ShowProperties(&prop, TRUE);
      }
      break;
   }
   return 0;
}

LRESULT CClassView::OnTreeBeginDrag(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) pnmh;
   CString sText;
   m_ctrlTree.GetItemText(lpNMTV->itemNew.hItem, sText);
   USES_CONVERSION;
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
         m_pProject->m_TagManager.GetMemberList(pParent->pstrName, aList, false);
      }
      for( int i = 0; i < aList.GetSize(); i++ ) {
         const TAGINFO* pTag = aList[i];
         int iImage = pTag->Type == TAGTYPE_MEMBER ? 4 : 3;
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
         m_ctrlTree.SortChildren(tvi.hItem);
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
   lpNMTVDI->item.pszText = (LPTSTR) pTag->pstrName;
   return 0;
}

// IIdleListener

void CClassView::OnIdle(IUpdateUI* pUIBase)
{   
   TCHAR szValue[32] = { 0 };
   _pDevEnv->GetProperty(_T("window.classview.sort"), szValue, 31);

   pUIBase->UIEnable(ID_CLASSVIEW_GOTODECL, m_pCurrentTag != NULL && m_pProject->FindView(m_pCurrentTag->pstrFile) != NULL);
   pUIBase->UIEnable(ID_CLASSVIEW_GOTOIMPL, m_pCurrentTag != NULL && !m_sImplementationEntry.IsEmpty());
   pUIBase->UIEnable(ID_CLASSVIEW_COPY, m_pCurrentTag != NULL);
   pUIBase->UIEnable(ID_CLASSVIEW_PROPERTIES, m_pCurrentTag != NULL);
   pUIBase->UIEnable(ID_CLASSVIEW_SORT, TRUE);
   pUIBase->UIEnable(ID_CLASSVIEW_NOSORT, TRUE);
   pUIBase->UISetCheck(ID_CLASSVIEW_SORT, _tcscmp(szValue, _T("alpha")) == 0);
   pUIBase->UISetCheck(ID_CLASSVIEW_NOSORT, _tcscmp(szValue, _T("no")) == 0);
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

   // Sort classes
   TCHAR szValue[32] = { 0 };
   _pDevEnv->GetProperty(_T("window.classview.sort"), szValue, 31);
   if( _tcscmp(szValue, _T("no")) != 0 ) m_ctrlTree.SortChildren(TVI_ROOT);

   // Insert "Globals" item
   CString s(MAKEINTRESOURCE(IDS_GLOBALS));
   tvis.item.pszText = (LPTSTR) (LPCTSTR) s;
   tvis.item.iImage = 1;
   tvis.item.iSelectedImage = 1;
   tvis.item.lParam = 0;
   m_ctrlTree.InsertItem(&tvis);

   m_ctrlTree.SetRedraw(TRUE);

   // FIX: Scrolling must be done outside WM_SETREDRAW section
   m_ctrlTree.SetScrollPos(SB_VERT, iScrollPos, TRUE);

   m_bPopulated = true;
   m_aExpandedNames.RemoveAll();
}

CString CClassView::_GetImplementationRef(TAGINFO* pTag)
{
   if( pTag == NULL ) return _T("");
   CString sFilename;
   long lLineNum = 0;
   if( !m_pProject->m_TagManager.m_LexInfo.FindImplementation(pTag->pstrName, pTag->pstrFields[0], sFilename, lLineNum) ) return _T("");
   CString sRef;
   sRef.Format(_T("%s|%ld"), sFilename, lLineNum);
   return sRef;
}
