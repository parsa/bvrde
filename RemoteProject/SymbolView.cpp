
#include "StdAfx.h"
#include "resource.h"

#include "SymbolView.h"
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
// CSymbolView

CSymbolView::CSymbolView() :
   m_ctrlTree(this, 1),
   m_pProject(NULL), 
   m_pCurrentHover(NULL),
   m_bMouseTracked(false)
{
}

// Operations

void CSymbolView::Init(CRemoteProject* pProject)
{
   Close();

   m_pProject = pProject;

   if( IsWindow() 
       && m_pProject != NULL
       && m_pProject->m_TagManager.IsAvailable() ) 
   {
      // Add the Symbol View window to the tab control.
      // NOTE: The following 'static' stuff works because this window object
      //       is already a static object (of the CRemoteProject class).
      static bool s_bAddedToExplorer = false;
      if( !s_bAddedToExplorer ) {
         s_bAddedToExplorer = true;
         _pDevEnv->AddExplorerView(m_hWnd, CString(MAKEINTRESOURCE(IDS_CAPTION_CLASSVIEW)), 11);
      }
   }
}

void CSymbolView::Close()
{
   if( m_ctrlTree.IsWindow() ) m_ctrlTree.DeleteAllItems();
   m_pProject = NULL;
}

void CSymbolView::Clear()
{
   if( !m_ctrlTree.IsWindow() ) return;
   // Delete all items and reset
   m_ctrlTree.SetRedraw(FALSE);
   m_ctrlTree.DeleteAllItems();
   m_ctrlTree.SetRedraw(TRUE);
}

// Message handlers

LRESULT CSymbolView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   m_Images.Create(IDB_CLASSVIEW, 16, 1, RGB(255,0,255));
   DWORD dwStyle = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_TRACKSELECT | TVS_INFOTIP;
   m_ctrlTree.Create(this, 1, m_hWnd, &rcDefault, NULL, dwStyle);
   m_ctrlTree.SetImageList(m_Images, TVSIL_NORMAL);
   ATLASSERT(m_ctrlTree.IsWindow());      
   return 0;
}

LRESULT CSymbolView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   CClientRect rc = m_hWnd;
   m_ctrlTree.MoveWindow(&rc);
   return 0;
}

LRESULT CSymbolView::OnTreeDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   HTREEITEM hItem = m_ctrlTree.GetSelectedItem();
   if( hItem == NULL ) return 0;
   if( m_ctrlTree.GetParentItem(hItem) == NULL ) return 0;
   TAGINFO* pTag = (TAGINFO*) m_ctrlTree.GetItemData(hItem);
   if( pTag == NULL ) return 0;
   CTagDetails Current;
   CTagDetails ImplTag;
   m_pProject->m_TagManager.GetItemInfo(pTag, Current);
   _GetImplementationRef(Current, ImplTag);
   if( !ImplTag.sName.IsEmpty() ) m_pProject->m_TagManager.OpenTagInView(ImplTag);
   else m_pProject->m_TagManager.OpenTagInView(Current);
   return 0;
}

LRESULT CSymbolView::OnTreeSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) pnmh;
   if( lpNMTV->itemNew.hItem == NULL ) return 0;
   TAGINFO* pTag = (TAGINFO*) m_ctrlTree.GetItemData(lpNMTV->itemNew.hItem);
   if( pTag == NULL ) return 0;
   CTagDetails Info;
   m_pProject->m_TagManager.GetItemInfo(pTag, Info);
   CTagElement prop = Info;
   _pDevEnv->ShowProperties(&prop, FALSE);
   return 0;
}

LRESULT CSymbolView::OnTreeRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
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
   CTagDetails Info;
   m_pProject->m_TagManager.GetItemInfo(pTag, Info);
   m_SelectedTag = Info;
   _GetImplementationRef(m_SelectedTag, m_SelectedImpl);
   // Load and show menu...
   CMenu menu;
   menu.LoadMenu(hItem != NULL ? IDR_CLASSTREE_ITEM : IDR_CLASSTREE);
   CMenuHandle submenu = menu.GetSubMenu(0);
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, ptPos, FALSE, this);
   // Handle result locally
   switch( nCmd ) {
   case ID_SYMBOLVIEW_SORT_ALPHA:
      {
         _pDevEnv->SetProperty(_T("window.symbolview.sort"), _T("alpha"));
      }
      break;
   case ID_SYMBOLVIEW_SORT_TYPE:
      {
         _pDevEnv->SetProperty(_T("window.symbolview.sort"), _T("type"));
      }
      break;
   case ID_SYMBOLVIEW_SORT_NONE:
      {
         _pDevEnv->SetProperty(_T("window.symbolview.sort"), _T("no"));
      }
      break;
   case ID_SYMBOLVIEW_GOTODECL:
      {
         m_pProject->m_TagManager.OpenTagInView(m_SelectedTag);
      }
      break;
   case ID_SYMBOLVIEW_GOTOIMPL:
      {
         m_pProject->m_TagManager.OpenTagInView(m_SelectedImpl);
      }
      break;
   case ID_SYMBOLVIEW_COPY:
      {
         CString sText;
         m_ctrlTree.GetItemText(hItem, sText);
         AtlSetClipboardText(m_hWnd, sText);
      }
      break;
   case ID_SYMBOLVIEW_PROPERTIES:
      {
         CTagElement prop = m_SelectedTag;
         _pDevEnv->ShowProperties(&prop, TRUE);
      }
      break;
   }
   return 0;
}

LRESULT CSymbolView::OnTreeBeginDrag(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
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

// Hover Tip message handler

LRESULT CSymbolView::OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
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

LRESULT CSymbolView::OnMouseHover(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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
   m_ctrlHoverTip.ShowItem(Info.sName, Info.sDeclaration, Info.sComment);
   m_pCurrentHover = pTag;
   return 0;
}

LRESULT CSymbolView::OnMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   if( m_ctrlHoverTip.IsWindow() ) m_ctrlHoverTip.PostMessage(WM_CLOSE);
   m_pCurrentHover = NULL;
   m_bMouseTracked = false;
   return 0;
}

LRESULT CSymbolView::OnRequestResize(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   REQRESIZE* pRR = (REQRESIZE*) pnmh;
   m_ctrlHoverTip.SetResize(pRR);
   return 0;
}

// IIdleListener

void CSymbolView::OnIdle(IUpdateUI* pUIBase)
{   
   BOOL bTagIsSelected = !m_SelectedTag.sName.IsEmpty();  // Real Tag is selected in tree (not "Globals" folder, etc)

   TCHAR szSortValue[32] = { 0 };
   _pDevEnv->GetProperty(_T("window.classview.sort"), szSortValue, 31);

   pUIBase->UIEnable(ID_SYMBOLVIEW_GOTODECL, bTagIsSelected && m_pProject->FindView(m_SelectedTag.sFilename, false) != NULL);
   pUIBase->UIEnable(ID_SYMBOLVIEW_GOTOIMPL, bTagIsSelected && m_pProject->FindView(m_SelectedImpl.sFilename, false) != NULL);
   pUIBase->UIEnable(ID_SYMBOLVIEW_COPY, bTagIsSelected);
   pUIBase->UIEnable(ID_SYMBOLVIEW_PROPERTIES,bTagIsSelected);
   pUIBase->UIEnable(ID_SYMBOLVIEW_SORT_ALPHA, TRUE);
   pUIBase->UIEnable(ID_SYMBOLVIEW_SORT_TYPE, TRUE);
   pUIBase->UIEnable(ID_SYMBOLVIEW_SORT_NONE, TRUE);
   pUIBase->UISetCheck(ID_SYMBOLVIEW_SORT_ALPHA, _tcscmp(szSortValue, _T("alpha")) == 0);
   pUIBase->UISetCheck(ID_SYMBOLVIEW_SORT_TYPE, _tcscmp(szSortValue, _T("type")) == 0);
   pUIBase->UISetCheck(ID_SYMBOLVIEW_SORT_NONE, _tcscmp(szSortValue, _T("no")) == 0);
}

void CSymbolView::OnGetMenuText(UINT /*wID*/, LPTSTR /*pstrText*/, int /*cchMax*/)
{
}

// Implementation

void CSymbolView::_PopulateTree()
{
   // Not ready?
   if( m_pProject == NULL ) return;

   CSimpleValArray<TAGINFO*> aList;
   //m_pProject->m_TagManager.FindSymbol(sFilter, aList);

   m_ctrlTree.SetRedraw(FALSE);

   // Clear tree
   m_ctrlTree.DeleteAllItems();  

   m_ctrlTree.SetRedraw(TRUE);
}

bool CSymbolView::_GetImplementationRef(const CTagDetails& Current, CTagDetails& Info)
{
   Info.sName.Empty();
   Info.sFilename.Empty();
   if( Current.sName.IsEmpty() ) return false;
   CString sLookupName;
   sLookupName.Format(_T("%s%s%s"), Current.sBase, Current.sBase.IsEmpty() ? _T("") : _T("::"), Current.sName);
   CSimpleValArray<TAGINFO*> aList;
   m_pProject->m_TagManager.FindItem(sLookupName, NULL, 0, ::GetTickCount() + 500, aList);
   for( int i = 0; i < aList.GetSize(); i++ ) {
      if( aList[i]->Type == TAGTYPE_IMPLEMENTATION ) {
         m_pProject->m_TagManager.GetItemInfo(aList[i], Info);
         return true;
      }
   }
   return false;
}

