
#include "StdAfx.h"
#include "resource.h"

#include "VariableView.h"
#include "Project.h"


/////////////////////////////////////////////////////////////////////////
// CVariableView

CVariableView::CVariableView() :
   m_pProject(NULL)
{
}

CVariableView::~CVariableView()
{
   if( IsWindow() ) /* scary */
      DestroyWindow();
}

// Operations

#pragma code_seg( "VIEW" )

void CVariableView::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
}

bool CVariableView::WantsData() 
{
   if( !IsWindow() ) return false;
   if( !IsWindowVisible() ) return false;
   return true;
}

void CVariableView::SetInfo(LPCTSTR pstrType, CMiInfo& info)
{
   if( _tcscmp(pstrType, _T("locals")) == 0 
       || _tcscmp(pstrType, _T("stack-args")) == 0 ) 
   {
      int iItem = m_ctrlList.GetCurSel();
      m_ctrlList.SetRedraw(FALSE);
      m_ctrlList.ResetContent();
      CString sName = info.GetItem(_T("name"));
      while( !sName.IsEmpty() ) {
         CString sValue = info.GetSubItem(_T("value"));
         // NOTE: Newer versions of GDB states that it exposes an "editable"
         //       property, but it turns our this doesn't actually mean
         //       that a user will be able to edit the value, hence the
         //       crappy string tests below.
         BOOL bEnable = TRUE;
         if( sValue == _T("ARRAY") ) bEnable = FALSE;
         if( sValue == _T("RECORD") ) bEnable = FALSE;
         if( sValue.Find(_T("...")) >= 0 ) bEnable = FALSE;
         if( sValue.Find(_T(" = {")) >= 0 ) bEnable = FALSE;
         TCHAR chFirst = '\0', chLast = '\0';
         if( bEnable && sValue.GetLength() > 0 ) {
            chFirst = sValue.GetAt(0);
            chLast = sValue.GetAt(sValue.GetLength() - 1);
         }
         if( chFirst == '{' ) bEnable = FALSE;
         if( chFirst == '<' ) bEnable = FALSE;
         if( chLast == '>' ) bEnable = FALSE;
         HPROPERTY hProp = m_ctrlList.AddItem(PropCreateSimple(sName, sValue));
         m_ctrlList.SetItemEnabled(hProp, bEnable);
         sName = info.FindNext(_T("name"));
      }
      m_ctrlList.SetCurSel(iItem);
      m_ctrlList.SetRedraw(TRUE);
      m_ctrlList.Invalidate();
   }
}

void CVariableView::EvaluateView(CSimpleArray<CString>& aDbgCmd)
{
   switch( m_ctrlTab.GetCurSel() ) {
   case 0:  aDbgCmd.Add(CString(_T("-stack-list-locals 1"))); break;
   case 1:  aDbgCmd.Add(CString(_T("-stack-list-arguments 1 0 0"))); break;
   }
}

// Message handlers

LRESULT CVariableView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   m_ctrlTab.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE);
   ATLASSERT(m_ctrlTab.IsWindow());

   m_ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL);
   ATLASSERT(m_ctrlList.IsWindow());
   int cxChar = (int) LOWORD(GetDialogBaseUnits());
   m_ctrlList.SetColumnWidth(cxChar * 15);

   CString sTab;
   TCITEM item = { 0 };
   item.mask = TCIF_TEXT;
   sTab.LoadString(IDS_LOCAL_VARS);
   item.pszText = (LPTSTR) (LPCTSTR) sTab;
   m_ctrlTab.InsertItem(0, &item);
   sTab.LoadString(IDS_LOCAL_ARGS);
   item.pszText = (LPTSTR) (LPCTSTR) sTab;
   m_ctrlTab.InsertItem(1, &item);
   m_ctrlTab.SetCurSel(0);

   return 0;
}

LRESULT CVariableView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   CClientRect rc = m_hWnd;
   RECT rcTop = { rc.left, rc.top, rc.right, rc.bottom - ::GetSystemMetrics(SM_CYVTHUMB) };
   m_ctrlList.MoveWindow(&rcTop);
   RECT rcBottom = { rc.left, rcTop.bottom - rcTop.top, rc.right, rc.bottom };
   m_ctrlTab.MoveWindow(&rcBottom);
   return 0;
}

LRESULT CVariableView::OnTabChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   // Refresh all values
   if( IsWindowVisible() ) {
      m_ctrlList.ResetContent();
      m_pProject->DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);
   }
   return 0;
}

LRESULT CVariableView::OnItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   LPNMPROPERTYITEM lpNMPI = (LPNMPROPERTYITEM) pnmh;
   if( pnmh->hwndFrom != m_ctrlList ) return 0;
   ATLASSERT(lpNMPI->prop);

   TCHAR szName[200] = { 0 };
   m_ctrlList.GetItemName(lpNMPI->prop, szName, (sizeof(szName)/sizeof(TCHAR)) - 1);
   CComVariant vValue;
   vValue.Clear();
   m_ctrlList.GetItemValue(lpNMPI->prop, &vValue);
   vValue.ChangeType(VT_BSTR);

   // Assign the value...
   CString sCommand;
   sCommand.Format(_T("-gdb-set variable %s=%ls"), szName, vValue.bstrVal);
   m_pProject->DelayedDebugCommand(sCommand);
   // Refresh all values
   m_pProject->DelayedDebugEvent(LAZY_DEBUG_BREAK_EVENT);

   return 0;
}

