
#include "StdAfx.h"
#include "resource.h"

#include "WatchView.h"
#include "Project.h"


/////////////////////////////////////////////////////////////////////////
// Constructor/destructor

CWatchView::CWatchView() :
   m_wndCatch(this, 1),
   m_pProject(NULL)
{
}


/////////////////////////////////////////////////////////////////////////
// Operations

#pragma code_seg( "VIEW" )

void CWatchView::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
   // Remove previous items; do a fresh start...
   if( m_ctrlGrid.IsWindow() ) {
      while( m_ctrlGrid.GetItemCount() > 0 ) m_ctrlGrid.DeleteItem(0);
   }
}

bool CWatchView::WantsData() 
{
   if( !IsWindow() ) return false;
   if( !IsWindowVisible() ) return false;
   return true;
}

void CWatchView::ActivateWatches()
{
   if( !IsWindow() ) return;

   int nCount = m_ctrlGrid.GetItemCount();
   for( int i = 0; i < nCount; i++ ) {
      CComVariant vName;
      vName.Clear();
      HPROPERTY hProp = m_ctrlGrid.GetProperty(i, 0);
      m_ctrlGrid.GetItemValue(hProp, &vName);
      ATLASSERT(vName.vt==VT_BSTR);
      CString sName = vName.bstrVal;
      LPARAM lKey = m_ctrlGrid.GetItemData(hProp);
      CString sCommand;
      sCommand.Format(_T("-var-create watch%ld * \"%s\""), lKey, sName);
      m_pProject->DelayedDebugCommand(sCommand);
   }
}

void CWatchView::SetInfo(LPCTSTR pstrType, CMiInfo& info)
{
   if( _tcscmp(pstrType, _T("value")) == 0 )
   {
      CString sName = info.GetItem(_T("name"));
      if( sName.IsEmpty() ) return;
      if( sName.Find(_T("watch")) != 0 ) return;
      CString sValue = info.GetItem(_T("value"));
      LPARAM lWatch = (LPARAM) _ttol(sName.Mid(5));  // formatted as "watch1234"
      int nCount = m_ctrlGrid.GetItemCount();
      for( int i = 0; i < nCount; i++ ) {
         HPROPERTY hProp = m_ctrlGrid.GetProperty(i, 0);
         LPARAM lKey = m_ctrlGrid.GetItemData(hProp);
         if( lKey == lWatch ) {
            HPROPERTY hProp = m_ctrlGrid.GetProperty(i, 1);
            CComVariant vValue = sValue;
            m_ctrlGrid.SetItemValue(hProp, &vValue);
            break;
         }
      }
   }
   if( _tcscmp(pstrType, _T("name")) == 0 )
   {
      CString sName = info.GetItem(_T("name"));
      while( !sName.IsEmpty() ) {
         if( _tcsncmp(sName, _T("watch"), 5) == 0 ) {
            LPARAM lWatch = (LPARAM) _ttol(sName.Mid(5));  // formatted as "watch1234"
            int nCount = m_ctrlGrid.GetItemCount();
            bool bFound = false;
            for( int i = 0; !bFound && i < nCount; i++ ) {
               HPROPERTY hProp = m_ctrlGrid.GetProperty(i, 0);
               LPARAM lKey = m_ctrlGrid.GetItemData(hProp);
               if( lKey == lWatch ) bFound = true;
            }
            if( !bFound ) {
               int iItem = m_ctrlGrid.InsertItem(-1, PropCreateSimple(_T(""), sName, lWatch));
               m_ctrlGrid.SetSubItem(iItem, 1, PropCreateSimple(_T(""), _T("")));
               // Let's not be satisfied with the "watchXXX" name. The debugger can name
               // the variable properly.
               CString sCommand;
               sCommand.Format(_T("-var-info-expression %s"), sName);
               m_pProject->DelayedDebugCommand(sCommand);
            }
         }
         sName = info.FindNext(_T("name"));
      }
   }
   if( _tcscmp(pstrType, _T("lang")) == 0 )
   {
      CString sName = info.GetItem(_T("name"));
      CString sRealName = info.GetItem(_T("exp"));
      if( sName.Left(5) == _T("watch") && !sRealName.IsEmpty() ) {
         LPARAM lWatch = (LPARAM) _ttol(sName.Mid(5));  // formatted as "watch1234"
         int nCount = m_ctrlGrid.GetItemCount();
         for( int i = 0; i < nCount; i++ ) {
            HPROPERTY hProp = m_ctrlGrid.GetProperty(i, 0);
            LPARAM lKey = m_ctrlGrid.GetItemData(hProp);
            if( lKey != lWatch ) continue;
            CComVariant vName = sRealName;
            m_ctrlGrid.SetItemValue(hProp, &vName);
            // Update the value while we're at it...
            CString sCommand;
            sCommand.Format(_T("-var-evaluate-expression watch%ld"), lWatch);
            m_pProject->DelayedDebugCommand(sCommand);
            break;
         }
      }
   }
   if( _tcscmp(pstrType, _T("changelist")) == 0 )
   {
      CSimpleArray<LPARAM> aChangedWatches;
      CString sName = info.GetItem(_T("name"));
      while( !sName.IsEmpty() ) {
         if( _tcsncmp(sName, _T("watch"), 5) == 0 ) {
            LPARAM lWatch = (LPARAM) _ttol(sName.Mid(5));  // formatted as "watch1234"
            aChangedWatches.Add(lWatch);
         }
         sName = info.FindNext(_T("name"));
      }
      COLORREF clrChanged = ::GetSysColor(COLOR_HIGHLIGHT);
      int nCount = m_ctrlGrid.GetItemCount();
      for( int i = 0; i < nCount; i++ ) {
         HPROPERTY hProp = m_ctrlGrid.GetProperty(i, 0);
         LPARAM lKey = m_ctrlGrid.GetItemData(hProp);
         bool bFound = false;
         for( int j = 0; !bFound && j < aChangedWatches.GetSize(); j++ ) if( aChangedWatches[j] == lKey ) bFound = true;
         static_cast<CPropertyItem*>(hProp)->SetTextColor(bFound ? clrChanged : CLR_INVALID);
      }
      // Intelligently try to refresh items...
      static int s_iLastChangeCount = -1;
      if( s_iLastChangeCount > 0 || aChangedWatches.GetSize() > 0 ) m_ctrlGrid.Invalidate(FALSE);
      s_iLastChangeCount = aChangedWatches.GetSize();
   }
}

void CWatchView::EvaluateValues(CSimpleArray<CString>& aDbgCmd)
{
   if( !IsWindow() ) return;
   if( !IsWindowVisible() ) return;

   CString sCommand = _T("-var-update *");
   aDbgCmd.Add(sCommand);

   // While the above debugger command gives us a list of chenged items, I still
   // perfer to evaluate *all* wathces - simply because there is slight risk that
   // we didn't catch an update.
   // TODO: Baloney. Fix this.

   int nCount = m_ctrlGrid.GetItemCount();
   for( int i = 0; i < nCount; i++ ) {
      HPROPERTY hProp = m_ctrlGrid.GetProperty(i, 0);
      LPARAM lKey = m_ctrlGrid.GetItemData(hProp);
      sCommand.Format(_T("-var-evaluate-expression watch%ld"), lKey);
      aDbgCmd.Add(sCommand);
   }
}


/////////////////////////////////////////////////////////////////////////
// Implementation

void CWatchView::_CreateWatch(HPROPERTY hProp, LPCTSTR pstrName)
{
   ATLASSERT(hProp);
   LPARAM lKey = m_ctrlGrid.GetItemData(hProp);
   CString sCommand;
   sCommand.Format(_T("-var-delete watch%ld"), lKey);
   m_pProject->DelayedDebugCommand(sCommand);
   sCommand.Format(_T("-var-create watch%ld * \"%s\""), lKey, pstrName);
   m_pProject->DelayedDebugCommand(sCommand);
}

void CWatchView::_DeleteWatch(HPROPERTY hProp)
{
   ATLASSERT(hProp);
   int iItem = -1, iCol = -1;
   m_ctrlGrid.FindProperty(hProp, iItem, iCol);
   LPARAM lKey = m_ctrlGrid.GetItemData(hProp);
   m_ctrlGrid.DeleteItem(iItem);
   CString sCommand;
   sCommand.Format(_T("-var-delete watch%ld"), lKey);
   m_pProject->DelayedDebugCommand(sCommand);
}


/////////////////////////////////////////////////////////////////////////
// Message handlers

LRESULT CWatchView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   m_ctrlGrid.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER);
   ATLASSERT(m_ctrlGrid.IsWindow());
   m_wndCatch.SubclassWindow(m_ctrlGrid);
   CString s;
   s.LoadString(IDS_COL_NAME);
   m_ctrlGrid.InsertColumn(0, s, 0, 120, 0);
   s.LoadString(IDS_COL_VALUE);
   m_ctrlGrid.InsertColumn(1, s, 0, 200, 0);
   m_ctrlGrid.SetExtendedGridStyle(PGS_EX_ADDITEMATEND);
   RegisterDropTarget(m_ctrlGrid);
   FORMATETC fe = { CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
   AddDropFormat(fe);
   return 0;
}

LRESULT CWatchView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   RevokeDropTarget();
   bHandled = FALSE;
   return 0;
}

LRESULT CWatchView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   CClientRect rcClient = m_hWnd;
   m_ctrlGrid.MoveWindow(&rcClient);
   m_ctrlGrid.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
   return 0;
}

bool CWatchView::DoDrop(LPDATAOBJECT pDataObj)
{
   FORMATETC fe = GetDropFormat(0);
   STGMEDIUM stgme = { 0 };
   if( FAILED( pDataObj->GetData(&fe, &stgme) ) ) return false;
   LPCSTR lpstr = (LPCSTR) ::GlobalLock(stgme.hGlobal); 
   CString sText;
   if( lpstr != NULL ) sText = lpstr;
   ::GlobalUnlock(stgme.hGlobal); 
   ::ReleaseStgMedium(&stgme);

   if( sText.IsEmpty() ) return false;

   // Add new item...
   LPARAM lKey = (LPARAM) ::GetTickCount();
   int iItem = m_ctrlGrid.InsertItem(-1, PropCreateSimple(_T(""), sText, lKey));
   m_ctrlGrid.SetSubItem(iItem, 1, PropCreateSimple(_T(""), _T("")));
   m_ctrlGrid.SelectItem(iItem);
   CString sCommand;
   sCommand.Format(_T("-var-create watch%ld * \"%s\""), lKey, sText);
   m_pProject->DelayedDebugCommand(sCommand);

   CSimpleArray<CString> aDbgCmd;
   EvaluateValues(aDbgCmd);
   for( int i = 0; i < aDbgCmd.GetSize(); i++ ) m_pProject->DelayedDebugCommand(aDbgCmd[i]);

   SetFocus();
   return true;
}

LRESULT CWatchView::OnAddItem(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   LPARAM lKey = ::GetTickCount();
   int iItem = m_ctrlGrid.InsertItem(-1, PropCreateSimple(_T(""), _T(""), lKey));
   m_ctrlGrid.SetSubItem(iItem, 1, PropCreateSimple(_T(""), _T("")));
   m_ctrlGrid.SelectItem(iItem);
   CString sCommand;
   sCommand.Format(_T("-var-create watch%ld * \"%s\""), lKey, _T("0"));
   m_pProject->DelayedDebugCommand(sCommand);
   return 0;
}

LRESULT CWatchView::OnItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   LPNMPROPERTYITEM pnpi = (LPNMPROPERTYITEM) pnmh;

   int iItem = -1;
   int iSubItem = -1;
   m_ctrlGrid.FindProperty(pnpi->prop, iItem, iSubItem);

   if( iSubItem == 0 ) 
   {
      HPROPERTY hProp = pnpi->prop;
      CComVariant vName;
      vName.Clear();
      m_ctrlGrid.GetItemValue(hProp, &vName);
      CString sName = CString(vName.bstrVal);
      if( sName.IsEmpty() ) _DeleteWatch(hProp); else _CreateWatch(hProp, sName);
   }
   else
   {
      CComVariant vValue;
      vValue.Clear();
      HPROPERTY hProp = pnpi->prop;
      m_ctrlGrid.GetItemValue(hProp, &vValue);
      CString sValue = CString(vValue.bstrVal);
      hProp = m_ctrlGrid.GetProperty(iItem, 0);
      LPARAM lKey = m_ctrlGrid.GetItemData(hProp);
      CString sCommand;
      sCommand.Format(_T("-var-assign watch%ld \"%s\""), lKey, sValue);
      m_pProject->DelayedDebugCommand(sCommand);
   }

   CSimpleArray<CString> aDbgCmd;
   EvaluateValues(aDbgCmd);
   for( int i = 0; i < aDbgCmd.GetSize(); i++ ) m_pProject->DelayedDebugCommand(aDbgCmd[i]);

   return 0;
}

LRESULT CWatchView::OnGridKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   switch( wParam ) {
   case VK_DELETE:
      {
         // NOTE: It would be easier to catch the LVN_KEYDOWN notification
         //       but our grid control snags it before us and we need to subclass
         //       and intercept it.
         int iIndex = m_ctrlGrid.GetSelectedIndex();
         if( iIndex < 0 || iIndex >= m_ctrlGrid.GetItemCount() ) return 0;
         _DeleteWatch( m_ctrlGrid.GetProperty(iIndex, 0) );
      }
      return 0;
   }
   bHandled = FALSE;
   return 0;
}

