
#include "StdAfx.h"
#include "resource.h"

#include "ClassView.h"
#include "Project.h"
#include "TagInfo.h"


/////////////////////////////////////////////////////////////////////////
// CTagProperties

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
// Constructor/destructor

CClassView::CClassView() :
   m_pProject(NULL), m_bLoaded(false), m_bLocked(false)
{
}


/////////////////////////////////////////////////////////////////////////
// Operations

void CClassView::Init(CRemoteProject* pProject)
{
   Close();

   m_bLoaded = false;
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
   m_bLoaded = false;
   m_bLocked = false;
}

void CClassView::Lock()
{
   // Remember which branches was expanded, so we can
   // expand them when we recreate the tree later...
   // NOTE: The caller is about the restructure the TAG pointers, so
   //       we must do this process here while the names are still valid.
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
   m_ctrlTree.DeleteAllItems();
   m_aExpandedNames.RemoveAll();
   m_bLoaded = false;
   m_bLocked = false;
}

void CClassView::Populate()
{
   ATLASSERT(IsWindow());
   // TAGS files already scanned and nothing was found!
   if( m_bLoaded ) return;
   // Show status text
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
   m_bLoaded = false;
   // View is not showing right now? Delay populating the tree then.
   if( !IsWindowVisible() ) return;
   // Ok, do it then...
   _PopulateTree();
}

void CClassView::_PopulateTree()
{
   // Not ready?
   if( m_pProject == NULL ) return;

   CSimpleValArray<TAGINFO*> aList;
   m_pProject->m_TagManager.GetOuterList(aList);
   if( aList.GetSize() > 0 ) 
   {
      m_ctrlTree.SetRedraw(FALSE);

      // Remember the scroll position
      int iScrollPos = m_ctrlTree.GetScrollPos(SB_VERT);

      // Clear tree
      m_ctrlTree.DeleteAllItems();  

      // Not locked anymore; tree is safe!
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
         if( pTag->Type == TAGTYPE_CLASS || pTag->Type == TAGTYPE_STRUCT ) {
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
   }

   m_bLoaded = true;
   m_aExpandedNames.RemoveAll();
}

void CClassView::_GoToDefinition(TAGINFO* pTag)
{
   ATLASSERT(m_pProject);
   ATLASSERT(pTag);
   if( pTag->iLineNo >= 0 ) {
      // Line-numbers have first priority. We don't parse lineno. from
      // CTAGS files because they are too unreliable, but we will get
      // them from our own realtime C++ lexer.
      m_pProject->OpenView(pTag->pstrFile, pTag->iLineNo);
   }
   else if( m_pProject->OpenView(pTag->pstrFile, 0) ) {
      // FIX: CTAGS doesn't actually produce sensible REGEX
      //      so we need to strip tokens and prepare a standard search.
      CString sToken = pTag->pstrToken;
      sToken.Replace(_T("\\/"), _T("/"));
      sToken.TrimLeft(_T("/^"));
      sToken.TrimRight(_T("$/;\""));
      int iFlags = SCFIND_MATCHCASE; //|SCFIND_REGEXP;
      m_pProject->DelayedViewMessage(DEBUG_CMD_FINDTEXT, sToken, 0, iFlags);
      m_pProject->DelayedViewMessage(DEBUG_CMD_FOLDCURSOR);
   }
   else {
      ::MessageBeep((UINT)-1);
   }
}


/////////////////////////////////////////////////////////////////////////
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
   _GoToDefinition(pTag);
   return 0;
}

LRESULT CClassView::OnTreeRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   USES_CONVERSION;
   HTREEITEM hItem = m_ctrlTree.GetDropHilightItem();
   if( hItem == NULL ) hItem = m_ctrlTree.GetSelectedItem();
   if( hItem == NULL ) return 0;
   TAGINFO* pTag = (TAGINFO*) m_ctrlTree.GetItemData(hItem);
   // Load menu and enable/disable a few items...
   CMenu menu;
   menu.LoadMenu(IDR_CLASSTREE);
   CMenuHandle submenu = menu.GetSubMenu(0);
   submenu.EnableMenuItem(ID_CLASSVIEW_GOTO, pTag != NULL && m_pProject->FindView(pTag->pstrFile) != NULL ? MF_ENABLED : MF_GRAYED);
   submenu.EnableMenuItem(ID_CLASSVIEW_COPY, pTag != NULL ? MF_ENABLED : MF_GRAYED);
   // Ok show it
   DWORD dwPos = ::GetMessagePos();
   POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, pt, FALSE);
   // Handle result locally
   switch( nCmd ) {
   case ID_CLASSVIEW_GOTO:
      {
         _GoToDefinition(pTag);
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
         CTagElement prop(pTag);
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
   if( lpNMTV->action == TVE_EXPAND ) {
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
         tvi.mask = TVIF_CHILDREN;
         tvi.cChildren = 0;
         m_ctrlTree.SetItem(&tvi);
      }
      else {
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
   if( (lpNMTVDI->item.mask & TVIF_TEXT) == 0 ) return 0;
   if( m_bLocked ) return 0;
   // NOTE: We use a callback for populating the tree text. This saves
   //       memory when keeping a large parse tree, but certainly makes it
   //       more difficult to maintain the tree. Since we modify classes
   //       on the fly, we need to repopulate/clear the tree often.
   TAGINFO* pTag = (TAGINFO*) m_ctrlTree.GetItemData(lpNMTVDI->item.hItem);
   ATLASSERT(!::IsBadReadPtr(pTag, sizeof(TAGINFO)));
   lpNMTVDI->item.pszText = (LPTSTR) pTag->pstrName;
   return 0;
}
