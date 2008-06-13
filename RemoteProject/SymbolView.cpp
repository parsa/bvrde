
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
// CSymbolThreadLoader

void CSymbolsLoaderThread::Init(CSymbolView* pOwner, LPCTSTR pstrPattern, DWORD dwCookie)
{
   m_pOwner = pOwner;
   m_dwCookie = dwCookie;
   m_sPattern = pstrPattern;
}

DWORD CSymbolsLoaderThread::Run()
{
   CSimpleValArray<TAGINFO*>* pList = new CSimpleValArray<TAGINFO*>;
   m_pOwner->m_pProject->m_TagManager.GetTypeList(m_sPattern, m_bStopped, *pList);
   if( ShouldStop() ) delete pList;
   else m_pOwner->PostMessage(WM_SYMBOL_GOT_DATA, m_dwCookie, (LPARAM) pList);
   return 0;
}


/////////////////////////////////////////////////////////////////////////
// CSymbolView

CSymbolView::CSymbolView() :
   m_ctrlList(this, 1),
   m_ctrlPattern(this, 2),
   m_pProject(NULL), 
   m_dwCookie(0),
   m_pCurrentHover(NULL),
   m_bMouseTracked(false)
{
}

CSymbolView::~CSymbolView()
{
   if( IsWindow() ) /* scary */
      DestroyWindow();
}

// Operations

void CSymbolView::Init(CRemoteProject* pProject)
{
   Close();

   m_pProject = pProject;
}

void CSymbolView::Close()
{
   if( m_ctrlList.IsWindow() ) m_ctrlList.DeleteAllItems();
   m_pProject = NULL;
}

void CSymbolView::Clear()
{
   m_threadLoader.SignalStop();
   if( m_ctrlList.IsWindow() ) m_ctrlList.DeleteAllItems();
}

// Message handlers

LRESULT CSymbolView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   CCommandBarXPCtrl CmdBar;
   CmdBar.GetSystemSettingsXP();  // Don't ask...

   m_ctrlToolbar.SubclassWindow( CFrameWindowImplBase<>::CreateSimpleToolBarCtrl(m_hWnd, IDR_SYMBOLS, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE) );

   m_ctrlPattern.Create(m_hWnd, rcDefault, _T(""), WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0, ID_SYMBOLS_PATTERN);
   m_ctrlPattern.SetFont(AtlGetDefaultGuiFont());
   CRemoteProject::AddControlToToolBar(m_ctrlToolbar, m_ctrlPattern, 100, ID_SYMBOLS_SEARCH, true, true);

   CImageList Images;
   Images.Create(IDB_CLASSVIEW, 16, 1, RGB(255,0,255));

   DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNTOP | LVS_SINGLESEL | LVS_NOCOLUMNHEADER | LVS_SORTASCENDING;
   m_ctrlList.Create(this, 1, m_hWnd, &rcDefault, NULL, dwStyle);
   m_ctrlList.SetFont(AtlGetDefaultGuiFont());
   m_ctrlList.SetImageList(Images, LVSIL_SMALL);
   m_ctrlList.AddColumn(_T(""), 10);

   return 0;
}

LRESULT CSymbolView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   m_threadLoader.Stop();
   bHandled = FALSE;
   return 0;
}

LRESULT CSymbolView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   if( !m_ctrlToolbar.IsWindow() || !m_ctrlList.IsWindow() ) return 0;
   CClientRect rc = m_hWnd;
   RECT rcTb = { rc.left, rc.top, rc.right, rc.top + 24 };
   m_ctrlToolbar.MoveWindow(&rcTb);
   m_ctrlToolbar.GetWindowRect(&rcTb);
   RECT rcList = { rc.left, rcTb.bottom - rcTb.top, rc.right, rc.bottom };
   m_ctrlList.MoveWindow(&rcList);
   m_ctrlList.SetColumnWidth(0, LVSCW_AUTOSIZE);
   return 0;
}

LRESULT CSymbolView::OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   return TRUE; // Children fills entire client area
}

LRESULT CSymbolView::OnGetSymbolData(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
   CString sText = CWindowText(m_ctrlPattern);
   sText.TrimRight();
   if( sText.IsEmpty() ) {
      m_ctrlList.DeleteAllItems();
      return 0;
   }
   // The cookie allows us to shoot off the thread at any time. When processing the result
   // we'll check that the cookie matches our latest request so we don't display
   // an old result list.
   m_dwCookie = ::GetTickCount();
   // The pattern is a DOS type pattern, so we enclose in wildcards...
   CString sPattern;
   sPattern.Format(_T("*%s*"), sText);
   m_threadLoader.Stop();
   m_threadLoader.Init(this, sPattern, m_dwCookie);
   m_threadLoader.Start();
   return 0;
}

LRESULT CSymbolView::OnGotSymbolData(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
   // The thread delivered data.
   // BUG: Since we deliver futile TAGINFO* structures we need to lock the
   //      lex-manager as long as we process this!
   CSimpleValArray<TAGINFO*>* pList = (CSimpleValArray<TAGINFO*>*) lParam;
   if( wParam != m_dwCookie ) {
      delete pList;
      return 0;
   }
   _PopulateList(*pList);
   delete pList;
   return 0;
}

LRESULT CSymbolView::OnPatternChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   m_threadLoader.SignalStop();
   PostMessage(WM_SYMBOL_GET_DATA);
   return 0;
}

LRESULT CSymbolView::OnListDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   int iItem = m_ctrlList.GetSelectedIndex();
   if( iItem <0 ) return 0;
   TAGINFO* pTag = (TAGINFO*) m_ctrlList.GetItemData(iItem);
   if( pTag == NULL ) return 0;
   CTagDetails Current;
   CTagDetails ImplTag;
   m_pProject->m_TagManager.GetItemInfo(pTag, Current);
   _GetImplementationRef(Current, ImplTag);
   if( !ImplTag.sName.IsEmpty() ) m_pProject->m_TagManager.OpenTagInView(ImplTag);
   else m_pProject->m_TagManager.OpenTagInView(Current);
   return 0;
}

LRESULT CSymbolView::OnListItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   LPNMLISTVIEW lpNMLV = (LPNMLISTVIEW) pnmh;
   if( lpNMLV->iItem < 0 ) return 0;
   if( !LISTITEM_SELECTED(lpNMLV) ) return 0;
   TAGINFO* pTag = (TAGINFO*) m_ctrlList.GetItemData(lpNMLV->iItem);
   if( pTag == NULL ) return 0;
   CTagDetails Info;
   m_pProject->m_TagManager.GetItemInfo(pTag, Info);
   CTagElement prop = Info;
   _pDevEnv->ShowProperties(&prop, FALSE);
   return 0;
}

LRESULT CSymbolView::OnListRightClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   // Determine click-point and list-item below it
   DWORD dwPos = ::GetMessagePos();
   POINT ptPos = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
   POINT ptClient = ptPos;
   m_ctrlList.ScreenToClient(&ptClient);
   UINT uFlags = 0;
   int iItem = m_ctrlList.HitTest(ptClient, &uFlags);
   if( (uFlags & LVHT_ONITEM) != 0 ) {
      // In case we clicked on a list-item we'll lookup information
      // about the tag and its implementation status
      TAGINFO* pTag = (TAGINFO*) m_ctrlList.GetItemData(iItem);
      CTagDetails Info;
      m_pProject->m_TagManager.GetItemInfo(pTag, Info);
      m_SelectedTag = Info;
      _GetImplementationRef(m_SelectedTag, m_SelectedImpl);
   };
   // Load and show menu...
   CMenu menu;
   menu.LoadMenu(iItem >= 0 ? IDR_SYMBOLTREE_ITEM : IDR_SYMBOLTREE);
   CMenuHandle submenu = menu.GetSubMenu(0);
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, ptPos, FALSE, this);
   // Handle result locally
   switch( nCmd ) {
   case ID_SYMBOLVIEW_SORT_ALPHA:
      {
         _pDevEnv->SetProperty(_T("window.symbolview.sort"), _T("alpha"));
         PostMessage(WM_SYMBOL_GET_DATA);
      }
      break;
   case ID_SYMBOLVIEW_SORT_TYPE:
      {
         _pDevEnv->SetProperty(_T("window.symbolview.sort"), _T("type"));
         PostMessage(WM_SYMBOL_GET_DATA);
      }
      break;
   case ID_SYMBOLVIEW_SORT_NONE:
      {
         _pDevEnv->SetProperty(_T("window.symbolview.sort"), _T("no"));
         PostMessage(WM_SYMBOL_GET_DATA);
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
         m_ctrlList.GetItemText(iItem, 0, sText);
         AtlSetClipboardText(m_hWnd, sText);
      }
      break;
   case ID_SYMBOLVIEW_MARK:
      {
         CString sText = m_SelectedTag.sName;
         if( sText.Find(_T("::")) >= 0 ) sText = sText.Mid(sText.Find(_T("::")) + 2);
         m_pProject->m_wndMain.SendMessage(WM_COMMAND, MAKEWPARAM(ID_EDIT_MARK, 0), (LPARAM) (LPCTSTR) sText);
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

LRESULT CSymbolView::OnListBeginDrag(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   USES_CONVERSION;
   LPNMLISTVIEW lpNMLV = (LPNMLISTVIEW) pnmh;
   CString sText;
   m_ctrlList.GetItemText(lpNMLV->iItem, 0, sText);
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
      tme.hwndTrack = m_ctrlList;
      tme.dwFlags = TME_QUERY;
      _TrackMouseEvent(&tme);
      tme.hwndTrack = m_ctrlList;
      tme.dwFlags |= TME_HOVER | TME_LEAVE;
      tme.dwHoverTime = HOVER_DEFAULT;
      _TrackMouseEvent(&tme);
      m_bMouseTracked = true;
   }
   if( m_ctrlHoverTip.IsWindow() ) m_ctrlList.SendMessage(WM_MOUSEHOVER);
   bHandled = FALSE;
   return 0;
}

LRESULT CSymbolView::OnMouseHover(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   // Check if the list-item below cursor changed; we may want to
   // display a tooltip below the cursor.
   POINT pt = { 0 };
   ::GetCursorPos(&pt);
   POINT ptClient = pt;
   m_ctrlList.ScreenToClient(&ptClient);
   UINT uFlags = 0;
   int iItem = m_ctrlList.HitTest(ptClient, &uFlags);
   if( (uFlags & LVHT_ONITEM) == 0 ) return m_ctrlList.SendMessage(WM_MOUSELEAVE);
   TAGINFO* pTag = (TAGINFO*) m_ctrlList.GetItemData(iItem);
   if( pTag == NULL ) return m_ctrlList.SendMessage(WM_MOUSELEAVE);
   if( m_pCurrentHover == pTag ) return 0;
   if( !m_ctrlHoverTip.IsWindow() ) m_ctrlHoverTip.Create(m_ctrlList, ::GetSysColor(COLOR_INFOBK));
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

// Edit messages

LRESULT CSymbolView::OnEditKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == VK_RETURN ) return PostMessage(WM_COMMAND, ID_SYMBOLS_SEARCH);
   if( wParam == VK_DOWN ) m_ctrlList.SetFocus();
   bHandled = FALSE;
   return 0;
}

// IIdleListener

void CSymbolView::OnIdle(IUpdateUI* pUIBase)
{   
   BOOL bTagIsSelected = !m_SelectedTag.sName.IsEmpty();

   TCHAR szSortValue[32] = { 0 };
   _pDevEnv->GetProperty(_T("window.symbolview.sort"), szSortValue, 31);

   pUIBase->UIEnable(ID_SYMBOLVIEW_GOTODECL, bTagIsSelected && m_pProject->FindView(m_SelectedTag.sFilename, false) != NULL);
   pUIBase->UIEnable(ID_SYMBOLVIEW_GOTOIMPL, bTagIsSelected && m_pProject->FindView(m_SelectedImpl.sFilename, false) != NULL);
   pUIBase->UIEnable(ID_SYMBOLVIEW_COPY, bTagIsSelected);
   pUIBase->UIEnable(ID_SYMBOLVIEW_MARK, bTagIsSelected);
   pUIBase->UIEnable(ID_SYMBOLVIEW_PROPERTIES, bTagIsSelected);
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

void CSymbolView::_PopulateList(CSimpleValArray<TAGINFO*>& aList)
{
   // Not ready?
   if( m_pProject == NULL ) return;

   // Clear list
   m_ctrlList.SetRedraw(FALSE);
   m_ctrlList.DeleteAllItems();

   int iImage;
   CString sName, sLastName;
   CTagDetails Info;
   for( int i = 0; i < aList.GetSize(); i++ ) {
      const TAGINFO* pTag = aList[i];
      m_pProject->m_TagManager.GetItemInfo(pTag, Info);
      sName.Format(_T("%s%s%s"), Info.sBase, Info.sBase.IsEmpty() ? _T("") : _T("::"), Info.sName);
      if( sName == sLastName ) continue;
      iImage = pTag->Type == TAGTYPE_MEMBER ? 4 : 3;
      if( iImage == 3 && (pTag->Protection == TAGPROTECTION_PROTECTED || pTag->Protection == TAGPROTECTION_PRIVATE) ) iImage = 5;
      if( iImage == 4 && (pTag->Protection == TAGPROTECTION_PROTECTED || pTag->Protection == TAGPROTECTION_PRIVATE) ) iImage = 6;
      int iItem = m_ctrlList.InsertItem(i, sName, iImage);
      m_ctrlList.SetItemData(iItem, (DWORD_PTR) pTag);
      sLastName = sName;
   }

   TCHAR szSortValue[32] = { 0 };
   _pDevEnv->GetProperty(_T("window.symbolview.sort"), szSortValue, 31);
   m_sSortValue = szSortValue;
   
   m_ctrlList.SortItemsEx(_ListSortProc, (LPARAM) this);

   m_ctrlList.SetColumnWidth(0, LVSCW_AUTOSIZE);
   m_ctrlList.SetRedraw(TRUE);
   m_ctrlList.Invalidate();
}

int CALLBACK CSymbolView::_ListSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   CSymbolView* pThis = (CSymbolView*) lParamSort;
   TCHAR szName1[MAX_PATH];
   TCHAR szName2[MAX_PATH];
   LVITEM lvi1 = { 0 };
   LVITEM lvi2 = { 0 };
   lvi1.iItem = lParam1;
   lvi2.iItem = lParam2;
   lvi1.pszText = szName1;
   lvi2.pszText = szName2;
   lvi1.mask = lvi2.mask = LVIF_TEXT | LVIF_IMAGE;
   lvi1.cchTextMax = lvi2.cchTextMax = MAX_PATH;
   pThis->m_ctrlList.GetItem(&lvi1);
   pThis->m_ctrlList.GetItem(&lvi2);
   int iRes = 0;
   if( pThis->m_sSortValue == _T("none") ) return 0;
   if( pThis->m_sSortValue == _T("type") ) iRes = lvi1.iImage - lvi2.iImage;
   if( iRes == 0 ) iRes = ::lstrcmp(lvi1.pszText, lvi2.pszText);
   return iRes;
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

