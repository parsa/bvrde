
#include "StdAfx.h"
#include "resource.h"

#include "SchemaView.h"

#pragma code_seg( "MISC" )


///////////////////////////////////////////////////////////////
//

class CSpeedList : CListViewCtrl
{
public:
   CSpeedList(HWND hWnd) : CListViewCtrl(hWnd)
   {
   }

   void AddItem(UINT nRes, LPCTSTR pstrValue)
   {
      int iItem = InsertItem(GetItemCount(), CString(MAKEINTRESOURCE(nRes)));
      SetItemText(iItem, 1, pstrValue);
   }

   void AddItem(UINT nRes, long lValue)
   {
      int iItem = InsertItem(GetItemCount(), CString(MAKEINTRESOURCE(nRes)));
      CString sValue;
      sValue.Format(_T("%ld"), lValue);
      SetItemText(iItem, 1, sValue);
   }
   
   void AddItem(UINT nRes, bool bValue)
   {
      int iItem = InsertItem(GetItemCount(), CString(MAKEINTRESOURCE(nRes)));
      SetItemText(iItem, 1, CString(MAKEINTRESOURCE(bValue ? IDS_YES : IDS_NO)));
   }
};


///////////////////////////////////////////////////////////////
//

CSchemaView::CSchemaView() :
   m_SplitterHook(this, 1)
{
}

// Operations

BOOL CSchemaView::PreTranslateMessage(MSG* pMsg)
{
   return FALSE;
}

void CSchemaView::OnIdle(IUpdateUI* pUIBase)
{
   pUIBase->UIEnable(ID_FILE_SAVE, TRUE);
   pUIBase->UIEnable(ID_FILE_PRINT, FALSE);
   pUIBase->UIEnable(ID_EDIT_UNDO, FALSE);
   pUIBase->UIEnable(ID_EDIT_REDO, FALSE);
   pUIBase->UIEnable(ID_EDIT_COPY, FALSE);
   pUIBase->UIEnable(ID_EDIT_CUT, FALSE);
   pUIBase->UIEnable(ID_EDIT_PASTE, FALSE);
   pUIBase->UIEnable(ID_EDIT_DELETE, FALSE);
   pUIBase->UIEnable(ID_EDIT_SELECT_ALL, FALSE);
   pUIBase->UIEnable(ID_EDIT_GOTO, FALSE);
   pUIBase->UIEnable(ID_EDIT_FIND, FALSE);
   pUIBase->UIEnable(ID_EDIT_REPLACE, FALSE);
   pUIBase->UIEnable(ID_EDIT_CLEAR, FALSE);
   pUIBase->UIEnable(ID_EDIT_CLEAR_ALL, FALSE);
}

// Message map and handlers

LRESULT CSchemaView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_Splitter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_CLIENTEDGE);
   m_SplitterHook.SubclassWindow(m_Splitter);

   m_ctrlTree.Create(m_Splitter, rcDefault, NULL, WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS | TVS_TRACKSELECT, 0, IDC_DBTREE);
   m_ctrlList.Create(m_Splitter, rcDefault, NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHAREIMAGELISTS, 0, IDC_DBLIST);
   m_wndInfo.Create(m_Splitter, rcDefault, _T("{8856F961-340A-11D0-A96B-00C04FD705A2}"), WS_CHILD | WS_VISIBLE, 0);

   m_ImagesSmall.Create(IDB_LISTSMALL, 16, 0, RGB(255,0,255));
   m_ImagesLarge.Create(IDB_LISTLARGE, 32, 0, RGB(255,0,255));
   m_ctrlList.SetExtendedListViewStyle(LVS_EX_GRIDLINES);
   m_ctrlList.AddColumn(CString(MAKEINTRESOURCE(IDS_COL_NAME)), 0);
   m_ctrlList.AddColumn(CString(MAKEINTRESOURCE(IDS_COL_VALUE)), 1);

   m_ctrlTree.SetImageList(m_ImagesSmall, TVSIL_NORMAL);
   CString s;
   s.LoadString(IDS_TREE_DATABASE);
   HTREEITEM hRoot = m_ctrlTree.InsertItem(s, 2, 2, TVI_ROOT, TVI_ROOT);
   m_ctrlTree.SetItemData(hRoot, 1L);
   //
   s.LoadString(IDS_TREE_INFO);
   TV_INSERTSTRUCT tvis = { 0 };
   tvis.hParent = hRoot;
   tvis.hInsertAfter = TVI_LAST;
   tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
   tvis.item.iImage = 8;
   tvis.item.iSelectedImage = 8;
   tvis.item.cChildren = 0;
   tvis.item.pszText = (LPTSTR) (LPCTSTR) s;
   tvis.item.lParam = (LPARAM) IDS_TREE_INFO;
   HTREEITEM hItem = m_ctrlTree.InsertItem(&tvis);
   //
   s.LoadString(IDS_TREE_TABLES);
   tvis.item.cChildren = 1;
   tvis.item.iImage = 1;
   tvis.item.iSelectedImage = 1;
   tvis.item.pszText = (LPTSTR) (LPCTSTR) s;
   tvis.item.lParam = (LPARAM) IDS_TREE_TABLES;
   hItem = m_ctrlTree.InsertItem(&tvis);
   //
   s.LoadString(IDS_TREE_VIEWS);
   tvis.item.pszText = (LPTSTR) (LPCTSTR) s;
   tvis.item.lParam = (LPARAM) IDS_TREE_VIEWS;
   hItem = m_ctrlTree.InsertItem(&tvis);
   //
   s.LoadString(IDS_TREE_SP);
   tvis.item.pszText = (LPTSTR) (LPCTSTR) s;
   tvis.item.lParam = (LPARAM) IDS_TREE_SP;
   hItem = m_ctrlTree.InsertItem(&tvis);
   //
   s.LoadString(IDS_TREE_SYSTEMTABLES);
   tvis.item.pszText = (LPTSTR) (LPCTSTR) s;
   tvis.item.lParam = (LPARAM) IDS_TREE_SYSTEMTABLES;
   hItem = m_ctrlTree.InsertItem(&tvis);

   _InitHtml();

   m_Splitter.SetSplitterPanes(m_ctrlTree, m_ctrlList);
   RECT rcSplit = { 0, 0, 200, 200 };
   m_Splitter.SetSplitterExtendedStyle(0);
   m_Splitter.SetSplitterRect(&rcSplit);
   m_Splitter.SetSplitterPos(170);

   m_ctrlTree.Expand(hRoot);

   return 0;
}

LRESULT CSchemaView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   m_ImagesSmall.Destroy();
   m_ImagesLarge.Destroy();
   m_spBrowser.Release();
   bHandled = FALSE;
   return 0;
}

LRESULT CSchemaView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   RECT rcClient = { 0 };
   GetClientRect(&rcClient);
   m_Splitter.MoveWindow(&rcClient);
   // HACK: Select first item when window first gets visible.
   //       Allows us to delay loading of database schema information.
   if( rcClient.right > 0 && m_ctrlTree.GetSelectedItem() == NULL ) m_ctrlTree.SelectItem(m_ctrlTree.GetChildItem(m_ctrlTree.GetRootItem()));
   return 0;
}

LRESULT CSchemaView::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_ctrlTree.SetFocus();
   return 0;
}

LRESULT CSchemaView::OnListActivate(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   int iIndex = m_ctrlList.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
   if( iIndex < 0 ) return 0;
   LPARAM lParam = m_ctrlList.GetItemData(iIndex);
   if( IS_INTRESOURCE(lParam) ) return 0;
   HTREEITEM hItem = _FindTreeItem(m_ctrlTree.GetRootItem(), lParam);
   if( hItem == NULL ) return 0;
   m_ctrlTree.SelectItem(hItem);
   m_ctrlTree.SelectItem(hItem); // FIX: First select might only expand and select parent!
   return 0;
}

LRESULT CSchemaView::OnTreeExpanding(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   CWaitCursor cursor;
   LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) pnmh;
   TVITEM& tvi = lpNMTV->itemNew;
   // NOTE: We will make sure the item is shown as selected.
   //       This will cause the right-hand view to get updated.
   //       We should do this only while *not* initializing the tree!
   //       See other hack in OnSize() handler.
   if( IsWindowVisible() ) m_ctrlTree.SelectItem(tvi.hItem);
   // Get information about the item
   tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
   m_ctrlTree.GetItem(&tvi);
   if( (lpNMTV->action & TVE_EXPAND) != 0
       && m_ctrlTree.GetChildItem(tvi.hItem) == NULL ) 
   {
      LPARAM lParam = tvi.lParam;
      switch( lParam ) {
      case 0:
      case 1:
      case IDS_TREE_INFO:
      case IDS_TREE_COLUMNS:
      case IDS_TREE_INDICES:
         return 0;
      case IDS_TREE_TABLES:
      case IDS_TREE_SYSTEMTABLES:
      case IDS_TREE_VIEWS:
         _FillTreeWithTables(lParam, tvi.hItem);
         break;
      case IDS_TREE_SP:
         break;
      default:
         DATABASEOBJECT* pObj = (DATABASEOBJECT*) lParam;
         switch( pObj->ItemType ) {
         case DBTYPE_TABLE:
         case DBTYPE_VIEW:
         case DBTYPE_SYSTEMTABLE:
            _FillTreeWithFields(pObj, tvi.hItem);            
            break;
         }
      }
      // If populated, but no children was found, we'll need to
      // reset the plus/minus symbol.
      if( m_ctrlTree.GetChildItem(tvi.hItem) == NULL ) {
         tvi.cChildren = 0;
         m_ctrlTree.SetItem(&tvi);
         return 0;
      }
   }
   // The folder-items have their icons toggled in open/closed state.
   if( tvi.iImage == 0 || tvi.iImage == 1 ) {
      tvi.iSelectedImage = tvi.iImage = (lpNMTV->action & TVE_EXPAND) != 0 ? 0 : 1;
      m_ctrlTree.SetItem(&tvi);
   }
   bHandled = FALSE;
   return 0;
}

LRESULT CSchemaView::OnTreeSelection(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
   CWaitCursor cursor;
   HTREEITEM hItem = m_ctrlTree.GetSelectedItem();
   if( hItem == NULL ) return 0;
   LPARAM lParam = m_ctrlTree.GetItemData(hItem);
   m_ctrlTree.Expand(hItem);
   switch( lParam ) {
   case 0:
      break;
   case 1:
   case IDS_TREE_INFO:
      _SetHtml();
      m_ctrlList.ShowWindow(SW_HIDE);
      m_wndInfo.ShowWindow(SW_SHOWNOACTIVATE);
      m_Splitter.SetSplitterPane(SPLIT_PANE_RIGHT, m_wndInfo);
      break;
   case IDS_TREE_TABLES:
   case IDS_TREE_SYSTEMTABLES:
   case IDS_TREE_VIEWS:
   case IDS_TREE_SP:
   case IDS_TREE_COLUMNS:
   case IDS_TREE_INDICES:
      {
         CWindowRedraw redraw = m_ctrlList;
         m_ctrlList.DeleteAllItems();
         m_ctrlList.SetViewType(LVS_ICON);
         m_ctrlList.SetImageList(m_ImagesSmall, LVSIL_SMALL);
         m_ctrlList.SetImageList(m_ImagesLarge, LVSIL_NORMAL);
         hItem = m_ctrlTree.GetChildItem(hItem);
         while( hItem ) {
            TCHAR szName[128] = { 0 };
            TVITEM tvi = { 0 };
            tvi.hItem = hItem;
            tvi.mask = TVIF_IMAGE | TVIF_PARAM | TVIF_TEXT;
            tvi.pszText = szName;
            tvi.cchTextMax = 127;
            m_ctrlTree.GetItem(&tvi);
            m_ctrlList.InsertItem(LVIF_IMAGE | LVIF_PARAM | LVIF_TEXT, m_ctrlList.GetItemCount(), szName, 1, 0, tvi.iImage, tvi.lParam);
            hItem = m_ctrlTree.GetNextSiblingItem(hItem);
         }
         m_ctrlList.ShowWindow(SW_SHOWNOACTIVATE);
         m_wndInfo.ShowWindow(SW_HIDE);
         m_Splitter.SetSplitterPane(SPLIT_PANE_RIGHT, m_ctrlList);
      }
      break;
   default:
      {
         CWindowRedraw redraw = m_ctrlList;
         m_ctrlList.DeleteAllItems();
         m_ctrlList.SetViewType(LVS_REPORT);
         m_ctrlList.SetImageList(NULL, LVSIL_SMALL);
         m_ctrlList.SetImageList(NULL, LVSIL_NORMAL);
         DATABASEOBJECT* pObj = (DATABASEOBJECT*) lParam;
         switch( pObj->ItemType ) {
         case DBTYPE_TABLE:
         case DBTYPE_VIEW:
         case DBTYPE_SYSTEMTABLE:
            {
               TABLEINFO* pTable = static_cast<TABLEINFO*>(pObj);
               CSpeedList list = m_ctrlList;
               list.AddItem(IDS_INFO_NAME, pTable->sName);
               list.AddItem(IDS_INFO_SCHEMA, pTable->sSchema);
               list.AddItem(IDS_INFO_CATALOG, pTable->sCatalog);
               list.AddItem(IDS_INFO_TYPE, pTable->sType);
            }
            break;
         case DBTYPE_FIELD:
            {
               FIELDINFO* pField = static_cast<FIELDINFO*>(pObj);
               CSpeedList list = m_ctrlList;
               CString sType;
               switch( pField->lType ) {
               case DBTYPE_I2:
               case DBTYPE_I4:
               case DBTYPE_UI1:
                  sType.LoadString(IDS_TYPE_INT);
                  break;
               case  DBTYPE_STR:
               case  DBTYPE_WSTR:
               case  DBTYPE_BSTR:
                  sType.LoadString(IDS_TYPE_STRING);
                  break;
               case DBTYPE_R4:
                  sType.LoadString(IDS_TYPE_FLOAT);
                  break;
               case DBTYPE_R8:
               case DBTYPE_DECIMAL:
               case DBTYPE_VARNUMERIC:
                  sType.LoadString(IDS_TYPE_DOUBLE);
                  break;
               case DBTYPE_BOOL:
                  sType.LoadString(IDS_TYPE_BOOL);
                  break;
               case DBTYPE_DATE:
               case DBTYPE_DBDATE:
                  sType.LoadString(IDS_TYPE_DATE);
                  break;
               case DBTYPE_DBTIME:
                  sType.LoadString(IDS_TYPE_TIME);
                  break;
               case DBTYPE_FILETIME:
               case DBTYPE_DBTIMESTAMP:
                  sType.LoadString(IDS_TYPE_TIMESTAMP);
                  break;
               case DBTYPE_CY:
                  sType.LoadString(IDS_TYPE_CURRENCY);
                  break;
               case DBTYPE_BYTES:
                  sType.LoadString(IDS_TYPE_BLOB);
                  break;
               }
               if( (pField->lType & (DBTYPE_BYREF|DBTYPE_VECTOR)) != 0 ) sType.LoadString(IDS_TYPE_BLOB);
               list.AddItem(IDS_INFO_NAME, pField->sName);
               list.AddItem(IDS_INFO_TYPE, sType);
               list.AddItem(IDS_INFO_POSITION, pField->lPosition);
               list.AddItem(IDS_INFO_LENGTH, pField->lLength);
               list.AddItem(IDS_INFO_PRECISION, pField->lPrecision);
               list.AddItem(IDS_INFO_DIGITS, pField->lDigits);
               list.AddItem(IDS_INFO_HASDEFAULT, pField->bHasDefault);
               list.AddItem(IDS_INFO_DEFAULT, pField->sDefault);
            }
            break;
         case DBTYPE_INDEX:
            {
               INDEXINFO* pIndex = static_cast<INDEXINFO*>(pObj);
               CSpeedList list = m_ctrlList;
               list.AddItem(IDS_INFO_NAME, pIndex->sName);
               list.AddItem(IDS_INFO_TYPE, pIndex->sType);
               list.AddItem(IDS_INFO_PRIMARY, pIndex->bPrimary);
               list.AddItem(IDS_INFO_UNIQUE, pIndex->bUnique);
               list.AddItem(IDS_INFO_CLUSTERED, pIndex->bClustered);
               list.AddItem(IDS_INFO_NULLS, pIndex->bNulls);
               list.AddItem(IDS_INFO_POSITION, pIndex->lPosition);
               CString sColumns;
               for( int i = 0; i < pIndex->nFields; i++ ) {
                  if( i > 0 ) sColumns += ", ";
                  sColumns += pIndex->sFields[i];
               }
               list.AddItem(IDS_INFO_FIELDS, sColumns);
            }
            break;
         }
         m_ctrlList.SetColumnWidth(0, LVSCW_AUTOSIZE);
         if( m_ctrlList.GetColumnWidth(0) < 100 ) m_ctrlList.SetColumnWidth(0, 100);
         m_ctrlList.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
         m_ctrlList.ShowWindow(SW_SHOWNOACTIVATE);
         m_wndInfo.ShowWindow(SW_HIDE);
         m_Splitter.SetSplitterPane(SPLIT_PANE_RIGHT, m_ctrlList);
      }
      break;
   }
   return 0;
}

BOOL CSchemaView::_InitHtml()
{
   CComPtr<IUnknown> spUnk;
   HRESULT Hr = m_wndInfo.QueryControl(IID_IUnknown, (LPVOID*) &spUnk);
   ATLASSERT(SUCCEEDED(Hr));
   if( FAILED(Hr) ) return FALSE;

   CComQIPtr<IServiceProvider> spSP = spUnk;
   ATLASSERT(spSP);
   if( spSP == NULL ) return FALSE;

   // Get to the IE Web Browser
   CComPtr<IWebBrowserApp> spWebApp;
   Hr = spSP->QueryService(IID_IWebBrowserApp, &spWebApp);
   ATLASSERT(SUCCEEDED(Hr));
   if( FAILED(Hr) ) return FALSE;
   m_spBrowser = spWebApp;
   if( m_spBrowser == NULL ) return FALSE;

    // Turn off text selection and right-click menu
   CComPtr<IAxWinAmbientDispatch> spHost;
   Hr = m_wndInfo.QueryHost(IID_IAxWinAmbientDispatch, (LPVOID*) &spHost);
   if( SUCCEEDED(Hr) ) {
      spHost->put_AllowContextMenu(VARIANT_FALSE);
      spHost->put_DocHostFlags(DOCHOSTUIFLAG_SCROLL_NO | DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_DISABLE_HELP_MENU);
   }

   // Display HTML page
   CComVariant vURL = L"about:<html></html>";
   CComVariant vEmpty;
   Hr = m_spBrowser->Navigate2(&vURL, &vEmpty, &vEmpty, &vEmpty, &vEmpty);
   return SUCCEEDED(Hr);
}

BOOL CSchemaView::_SetHtml()
{
   ATLASSERT(m_spBrowser);
   int i;
   CString sHTML = AtlLoadHtmlResource(IDR_INFO);
   static struct {
      LPCTSTR pstrToken;         LPCTSTR pstrLabel;
   } aTranslations[] = {
      { _T("$DATABASE"),         MAKEINTRESOURCE(IDS_DATABASE) },
      { _T("$DBNAME_LABEL"),     MAKEINTRESOURCE(IDS_DBNAME_LABEL) },
      { _T("$DBTYPE_LABEL"),     MAKEINTRESOURCE(IDS_DBTYPE_LABEL) },
      { _T("$DBVERSION_LABEL"),  MAKEINTRESOURCE(IDS_DBVERSION_LABEL) },
      { _T("$DBLOCATION_LABEL"), MAKEINTRESOURCE(IDS_DBLOCATION_LABEL) },
      { _T("$DBCATALOG_LABEL"),  MAKEINTRESOURCE(IDS_DBCATALOG_LABEL) },
      { _T("$DBSERVER_LABEL"),   MAKEINTRESOURCE(IDS_DBSERVER_LABEL) },
   };
   for( i = 0; i < sizeof(aTranslations) / sizeof(aTranslations[0]); i++ ) {
      if( sHTML.Find(aTranslations[i].pstrToken) >= 0 ) sHTML.Replace(aTranslations[i].pstrToken, CString(aTranslations[i].pstrLabel));
   }
   sHTML.Replace(_T("$DBNAME"),     m_pDb->GetPropertyStr(DBPROPSET_DBINIT, DBPROP_INIT_DATASOURCE));
   sHTML.Replace(_T("$DBLOCATION"), m_pDb->GetPropertyStr(DBPROPSET_DBINIT, DBPROP_INIT_LOCATION));
   sHTML.Replace(_T("$DBCATALOG"),  m_pDb->GetPropertyStr(DBPROPSET_DBINIT, DBPROP_INIT_CATALOG));
   sHTML.Replace(_T("$DBSERVER"),   m_pDb->GetPropertyStr(DBPROPSET_DATASOURCEINFO, DBPROP_SERVERNAME));
   sHTML.Replace(_T("$DBVERSION"),  m_pDb->GetPropertyStr(DBPROPSET_DATASOURCEINFO, DBPROP_DBMSVER));      
   sHTML.Replace(_T("$DBTYPE"),     m_pDb->GetPropertyStr(DBPROPSET_DATASOURCEINFO, DBPROP_DBMSNAME));
   CString sEmptyLine;
   for( i = 0; i < sizeof(aTranslations) / sizeof(aTranslations[0]); i++ ) {
      // <b>$DBSERVER_LABEL:</b> $DBSERVER<br>
      sEmptyLine.Format(_T("<b>%s:</b> <br>"), CString(aTranslations[i].pstrLabel));
      sHTML.Replace(sEmptyLine, _T(""));
   }
   _LoadHtml(m_spBrowser, sHTML);
   return TRUE;
}

BOOL CSchemaView::_FillTreeWithTables(LPARAM lParam, HTREEITEM hParent)
{
   ATLASSERT(m_pDb);
   CLockStaticDataInit lock;
   if( !m_pDb->EnumTables() ) return FALSE;
   bool bFound = false;
   for( int i = 0; i < m_pDb->m_aTables.GetSize(); i++ ) {
      TABLEINFO& ti = m_pDb->m_aTables[i];
      if( lParam == IDS_TREE_TABLES && ti.ItemType != DBTYPE_TABLE ) continue;
      if( lParam == IDS_TREE_VIEWS && ti.ItemType != DBTYPE_VIEW ) continue;
      if( lParam == IDS_TREE_SYSTEMTABLES && ti.ItemType != DBTYPE_SYSTEMTABLE ) continue;
      CString sName;
      if( !ti.sSchema.IsEmpty() ) sName = ti.sSchema + _T(".");
      if( !ti.sCatalog.IsEmpty() ) sName = ti.sCatalog + _T(".");
      sName += ti.sName;
      int iImage = 4;
      switch( ti.ItemType ) {
      case DBTYPE_VIEW:
         iImage = 7;
         if( ti.sType == "SYNONYM" ) iImage = 9;
         break;
      case DBTYPE_SYSTEMTABLE:
         iImage = 8;
         break;
      }
      TV_INSERTSTRUCT tvis = { 0 };
      tvis.hParent = hParent;
      tvis.hInsertAfter = TVI_LAST;
      tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
      tvis.item.iImage = iImage;
      tvis.item.iSelectedImage = iImage;
      tvis.item.cChildren = 1;
      tvis.item.pszText = (LPTSTR) (LPCTSTR) sName;
      tvis.item.lParam = (LPARAM) &ti;
      m_ctrlTree.InsertItem(&tvis);
      bFound = true;
   }
   m_ctrlTree.SortChildren(hParent);
   return TRUE;
}

BOOL CSchemaView::_FillTreeWithFields(DATABASEOBJECT* pObj, HTREEITEM hParent)
{
   ATLASSERT(m_pDb);
   ATLASSERT(pObj);
   TABLEINFO* pTable = static_cast<TABLEINFO*>(pObj);
   if( !m_pDb->LoadTableInfo(pTable) ) return FALSE;
   HTREEITEM hFields = m_ctrlTree.InsertItem(CString(MAKEINTRESOURCE(IDS_TREE_COLUMNS)), 1, 1, hParent, TVI_LAST);
   m_ctrlTree.SetItemData(hFields, IDS_TREE_COLUMNS);
   HTREEITEM hIndices = m_ctrlTree.InsertItem(CString(MAKEINTRESOURCE(IDS_TREE_INDICES)), 1, 1, hParent, TVI_LAST);
   m_ctrlTree.SetItemData(hIndices, IDS_TREE_INDICES);
   int i;
   for( i = 0; i < pTable->aFields.GetSize(); i++ ) {
      FIELDINFO& fi = pTable->aFields[i];
      HTREEITEM hItem = m_ctrlTree.InsertItem(fi.sName, 6, 6, hFields, TVI_LAST);
      m_ctrlTree.SetItemData(hItem, (LPARAM) &fi);
   }
   for( i = 0; i < pTable->aIndices.GetSize(); i++ ) {
      INDEXINFO& ii = pTable->aIndices[i];
      HTREEITEM hItem = m_ctrlTree.InsertItem(ii.sName, 5, 5, hIndices, TVI_LAST);
      m_ctrlTree.SetItemData(hItem, (LPARAM) &ii);
   }
   m_ctrlTree.SortChildren(hFields);
   m_ctrlTree.SortChildren(hIndices);
   return TRUE;
}

BOOL CSchemaView::_LoadHtml(IUnknown* pUnk, LPCTSTR pstrHTML)
{
   CComQIPtr<IWebBrowser> spBrowser = pUnk;
   if( spBrowser ) {
      CComPtr<IDispatch> spDisp;
      spBrowser->get_Document(&spDisp);
      if( spDisp == NULL ) return FALSE;
      pUnk = spDisp;
   }
   HANDLE hHTMLText = ::GlobalAlloc(GPTR, (_tcslen(pstrHTML) + 1) * sizeof(TCHAR));
   if( hHTMLText == NULL ) return FALSE;
   CComPtr<IStream> spStream;
   _tcscpy( (LPTSTR) hHTMLText, pstrHTML );
   HRESULT Hr = ::CreateStreamOnHGlobal(hHTMLText, TRUE, &spStream);
   if( SUCCEEDED(Hr) ) {
      CComPtr<IPersistStreamInit> spPersistStreamInit;
      Hr = pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID*) &spPersistStreamInit);
      if( SUCCEEDED(Hr) ) {
           Hr = spPersistStreamInit->InitNew();
           if( SUCCEEDED(Hr) ) {
              Hr = spPersistStreamInit->Load(spStream);
           }
      }
   }
   return SUCCEEDED(Hr);
}

HTREEITEM CSchemaView::_FindTreeItem(HTREEITEM hItem, LPARAM lParam)
{
   while( hItem != NULL ) {
      if( (LPARAM) m_ctrlTree.GetItemData(hItem) == lParam ) return hItem;
      HTREEITEM hNext;
      if( (hNext = _FindTreeItem(m_ctrlTree.GetChildItem(hItem), lParam)) != NULL ) return hNext;
      hItem = m_ctrlTree.GetNextSiblingItem(hItem);
   }
   return NULL;
}
