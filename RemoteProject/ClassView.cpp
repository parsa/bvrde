
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
   CTagInfo::TAGINFO* m_pTag;
   
   CTagElement(CTagInfo::TAGINFO* pTag) : m_pTag(pTag) 
   {
   }
   BOOL Load(ISerializable* /*pArc*/)
   {
      return FALSE;
   }
   BOOL Save(ISerializable* pArc)
   {
      ATLASSERT(!::IsBadReadPtr(m_pTag,sizeof(*m_pTag)));
      CString sKind(MAKEINTRESOURCE(IDS_UNKNOWN));
      if( m_pTag->Type == CTagInfo::TAGTYPE_CLASS ) sKind.LoadString(IDS_CLASS);
      else if( m_pTag->Type == CTagInfo::TAGTYPE_FUNCTION ) sKind.LoadString(IDS_FUNCTION);
      else if( m_pTag->Type == CTagInfo::TAGTYPE_STRUCT ) sKind.LoadString(IDS_STRUCT);
      else if( m_pTag->Type == CTagInfo::TAGTYPE_MEMBER ) sKind.LoadString(IDS_MEMBER);
      else if( m_pTag->Type == CTagInfo::TAGTYPE_DEFINE ) sKind.LoadString(IDS_DEFINE);
      else if( m_pTag->Type == CTagInfo::TAGTYPE_TYPEDEF ) sKind.LoadString(IDS_TYPEDEF);
      else if( m_pTag->Type == CTagInfo::TAGTYPE_ENUM ) sKind.LoadString(IDS_ENUM);
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
   m_pProject(NULL)
{
}


/////////////////////////////////////////////////////////////////////////
// Operations

void CClassView::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
   
   if( IsWindow() && m_pProject && m_pProject->m_TagInfo.IsTagsAvailable() ) {
      // NOTE: The following 'static' stuff works because this window object
      //       is already a static object (of the CRemoteProject class).
      static bool s_bAddedToExplorer = false;
      if( !s_bAddedToExplorer ) {
         s_bAddedToExplorer = true;
         _pDevEnv->AddExplorerView(m_hWnd, CString(MAKEINTRESOURCE(IDS_CAPTION_CLASSVIEW)), 11);
      }
   }
}

void CClassView::Clear()
{
   if( !m_ctrlTree.IsWindow() ) return;
   m_ctrlTree.DeleteAllItems();
}

void CClassView::Populate()
{
   ATLASSERT(IsWindow());
   // Not ready?
   if( m_pProject == NULL ) return;
   // View not showing at all!
   if( !IsWindowVisible() ) return;
   // Tree already populated (must use Clear() to remove tree first)
   if( m_ctrlTree.GetRootItem() != NULL ) return;
   // TAGS files already scanned and nothing was found!
   if( m_pProject->m_TagInfo.IsLoaded() && m_pProject->m_TagInfo.GetItemCount() == 0 ) return;

   CWaitCursor cursor;
   CString sStatus(MAKEINTRESOURCE(IDS_STATUS_LOADTAG));
   _pDevEnv->ShowStatusText(ID_DEFAULT_PANE, sStatus);

   CSimpleValArray<CTagInfo::TAGINFO*> aList;
   m_pProject->m_TagInfo.GetOuterList(aList);
   if( aList.GetSize() == 0 ) return;

   CWindowRedraw redraw = m_ctrlTree;
   m_ctrlTree.DeleteAllItems();  
   // Insert classes
   TV_INSERTSTRUCT tvis = { 0 };
   tvis.hParent = TVI_ROOT;
   tvis.hInsertAfter = TVI_LAST;
   tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
   tvis.item.iImage = 0;
   tvis.item.iSelectedImage = 0;
   tvis.item.cChildren = 1;
   for( int i = 0; i < aList.GetSize(); i++ ) {
      CTagInfo::TAGINFO* pTag = aList[i];
      if( pTag->Type == CTagInfo::TAGTYPE_CLASS ) {
         tvis.item.pszText = LPSTR_TEXTCALLBACK;
         tvis.item.lParam = (LPARAM) pTag;
         m_ctrlTree.InsertItem(&tvis);
      }
   }
   // Insert "Globals" item
   CString s(MAKEINTRESOURCE(IDS_GLOBALS));
   tvis.item.pszText = (LPTSTR) (LPCTSTR) s;
   tvis.item.iImage = 1;
   tvis.item.iSelectedImage = 1;
   tvis.item.lParam = 0;
   m_ctrlTree.InsertItem(&tvis);
}

void CClassView::_GoToDefinition(LPCTSTR pstrFilename, LPCTSTR pstrPattern)
{
   ATLASSERT(m_pProject);
   ATLASSERT(!::IsBadStringPtr(pstrFilename,-1));
   ATLASSERT(!::IsBadStringPtr(pstrPattern,-1));
   if( m_pProject->OpenView(pstrFilename, 0) ) {
      // HACK: CTAGS doesn't actually produce sensible REGEX
      //       so we need to strip tokens and prepare a standard search.
      CString sToken = pstrPattern;
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
   dwStyle = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_DISABLEDRAGDROP | TVS_TRACKSELECT | TVS_INFOTIP;
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
   bHandled = FALSE; // Don't eat
   return 0;
}

LRESULT CClassView::OnTreeDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   HTREEITEM hItem = m_ctrlTree.GetSelectedItem();
   if( hItem == NULL ) return 0;
   if( m_ctrlTree.GetParentItem(hItem) == NULL ) return 0;
   CTagInfo::TAGINFO* pTag = (CTagInfo::TAGINFO*) m_ctrlTree.GetItemData(hItem);
   if( pTag == NULL ) return 0;
   CString sFilename = pTag->pstrFile;
   CString sToken = pTag->pstrToken;
   _GoToDefinition(sFilename, sToken);
   return 0;
}

LRESULT CClassView::OnTreeRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   HTREEITEM hItem = m_ctrlTree.GetDropHilightItem();
   if( hItem == NULL ) hItem = m_ctrlTree.GetSelectedItem();
   if( hItem == NULL ) return 0;
   CTagInfo::TAGINFO* pTag = (CTagInfo::TAGINFO*) m_ctrlTree.GetItemData(hItem);
   CMenu menu;
   menu.LoadMenu(IDR_CLASSTREE);
   // Before we show the popup menu, let's enable/disable some items
   CMenuHandle submenu = menu.GetSubMenu(0);
   submenu.EnableMenuItem(ID_CLASSVIEW_GOTO, pTag != NULL && m_pProject->FindView(pTag->pstrFile) != NULL ? MF_ENABLED : MF_GRAYED);
   // Ok show it
   DWORD dwPos = ::GetMessagePos();
   POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, pt, FALSE);
   // Handle result locally
   switch( nCmd ) {
   case ID_CLASSVIEW_GOTO:
      {
         _GoToDefinition(pTag->pstrFile, pTag->pstrToken);
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

LRESULT CClassView::OnTreeExpanding(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) pnmh;
   TVITEM& tvi = lpNMTV->itemNew;
   if( lpNMTV->action == TVE_EXPAND ) {
      CTagInfo::TAGINFO* pParent = (CTagInfo::TAGINFO*) m_ctrlTree.GetItemData(tvi.hItem);
      if( pParent == NULL ) {
         // List globals
         if( m_ctrlTree.GetParentItem(tvi.hItem) == NULL ) {
            // TODO: Add code here...
         }
      }
      else {
         CString sParentName = pParent->pstrName;
         CSimpleValArray<CTagInfo::TAGINFO*> aList;
         m_pProject->m_TagInfo.GetMemberList(sParentName, aList, false);
         for( int i = 0; i < aList.GetSize(); i++ ) {
            const CTagInfo::TAGINFO* pTag = aList[i];
            int iImage = 3;
            if( pTag->Type == CTagInfo::TAGTYPE_MEMBER ) iImage = 4;
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
   CTagInfo::TAGINFO* pTag = (CTagInfo::TAGINFO*) m_ctrlTree.GetItemData(lpNMTVDI->item.hItem);
   lpNMTVDI->item.pszText = (LPTSTR) pTag->pstrName;
   return 0;
}
