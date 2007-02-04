
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"
#include "BreakpointView.h"


/////////////////////////////////////////////////////////////////////////
// Constructor/destructor

CBreakpointView::CBreakpointView() :
   m_pProject(NULL),
   m_bUpdating(false)
{
}


/////////////////////////////////////////////////////////////////////////
// Operations

#pragma code_seg( "VIEW" )

void CBreakpointView::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
}

bool CBreakpointView::WantsData()
{
   if( !IsWindow() ) return false;
   if( !IsWindowVisible() ) return false;
   return true;
}

void CBreakpointView::SetInfo(LPCTSTR pstrType, CMiInfo& info)
{
   if( _tcscmp(pstrType, _T("BreakpointTable")) == 0 ) 
   {
      m_bUpdating = true;
      int iSel = m_ctrlList.GetSelectedIndex();
      m_ctrlList.DeleteAllItems();
      CString sNumber = info.GetItem(_T("number"), _T("bkpt"));
      while( !sNumber.IsEmpty() ) {
         CString sType = info.GetSubItem(_T("type"));
         CString sFile = info.GetSubItem(_T("file"));
         CString sFunc = info.GetSubItem(_T("func"));
         CString sLine = info.GetSubItem(_T("line"));
         CString sEnabled = info.GetSubItem(_T("enabled"));
         CString sAddress = info.GetSubItem(_T("addr"));

         int iItem = m_ctrlList.InsertItem(m_ctrlList.GetItemCount(), sType);
         CString sLocation;
         sLocation.Format(_T("%s, %ld"), sFunc, _ttol(sLine));
         m_ctrlList.SetItemText(iItem, 1, sFile);
         m_ctrlList.SetItemText(iItem, 2, sLocation);
         m_ctrlList.SetItemText(iItem, 3, sAddress);
         m_ctrlList.SetItemData(iItem, (LPARAM) _ttol(sNumber));
         m_ctrlList.SetCheckState(iItem, sEnabled == _T("y"));
         sNumber = info.FindNext(_T("number"), _T("bkpt"));
      }
      if( iSel == -1 ) {
         // No selection yet? Let's take the oppotunity to
         // resize the column headers...
         m_ctrlList.SetColumnWidth(0, 90);
         m_ctrlList.SetColumnWidth(1, 100);
         m_ctrlList.SetColumnWidth(2, 120);
         m_ctrlList.SetColumnWidth(3, 100);
      }
      m_ctrlList.SelectItem(iSel);
      m_bUpdating = false;
   }
}


/////////////////////////////////////////////////////////////////////////
// Message handlers

LRESULT CBreakpointView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   m_ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_SORTASCENDING | LVS_NOSORTHEADER | LVS_REPORT);
   m_ctrlList.SetFont(AtlGetDefaultGuiFont());
   m_ctrlList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
   m_ctrlList.AddColumn(CString(MAKEINTRESOURCE(IDS_BRKCOL1)), 0);
   m_ctrlList.AddColumn(CString(MAKEINTRESOURCE(IDS_BRKCOL2)), 1);
   m_ctrlList.AddColumn(CString(MAKEINTRESOURCE(IDS_BRKCOL3)), 2);
   m_ctrlList.AddColumn(CString(MAKEINTRESOURCE(IDS_BRKCOL4)), 3);
   bHandled = FALSE;
   return 0;
}

LRESULT CBreakpointView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   CClientRect rcClient = m_hWnd;
   m_ctrlList.MoveWindow(&rcClient);
   return 0;
}

LRESULT CBreakpointView::OnItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   LPNMLISTVIEW lpNMLV = (LPNMLISTVIEW) pnmh;
   // To prevent submitting new debug commands while we're just updating
   // existing items with GDB output, we check this guy...
   if( m_bUpdating ) return 0;
   if( (lpNMLV->uNewState & LVIS_STATEIMAGEMASK) == (lpNMLV->uOldState & LVIS_STATEIMAGEMASK) ) return 0;
   // User changed item? Let's update the debugger...
   int iItem = lpNMLV->iItem;
   long lName = (long) m_ctrlList.GetItemData(iItem);
   if( m_ctrlList.GetCheckState(iItem) ) {
      CString sCommand;
      sCommand.Format(_T("-break-enable %ld"), lName);
      m_pProject->DelayedDebugCommand(sCommand);
   }
   else {
      CString sCommand;
      sCommand.Format(_T("-break-disable %ld"), lName);
      m_pProject->DelayedDebugCommand(sCommand);
   }
   // Get a fresh view of things
   m_pProject->DelayedDebugCommand(_T("-break-list"));
   return 0;
}

LRESULT CBreakpointView::OnKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   LPNMLVKEYDOWN lpNMLVKD = (LPNMLVKEYDOWN) pnmh;
   if( lpNMLVKD->wVKey == VK_DELETE ) {
      int iItem = m_ctrlList.GetSelectedIndex();
      long lName = (long) m_ctrlList.GetItemData(iItem);
      CString sCommand;
      sCommand.Format(_T("-break-delete %ld"), lName);
      m_pProject->DelayedDebugCommand(sCommand);
      m_pProject->DelayedDebugCommand(_T("-break-list"));
      // BUG: Argh, no function to refresh views!!
      m_pProject->DelayedViewMessage(DEBUG_CMD_CLEAR_BREAKPOINTS);
   }
   return 0;
}

LRESULT CBreakpointView::OnDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   int iItem = m_ctrlList.GetSelectedIndex();
   if( iItem < 0 ) return 0;
   CString sFilename;
   CString sLocation;
   m_ctrlList.GetItemText(iItem, 1, sFilename);
   m_ctrlList.GetItemText(iItem, 2, sLocation);
   LPCTSTR p = _tcschr(sLocation, ',');
   if( p ) m_pProject->OpenView(sFilename, _ttol(p + 1));
   return 0;
}
