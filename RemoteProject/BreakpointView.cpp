
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"
#include "BreakpointView.h"

#include "BreakpointInfoDlg.h"


/////////////////////////////////////////////////////////////////////////
// CBreakpointView

CBreakpointView::CBreakpointView() :
   m_pProject(NULL),
   m_bUpdating(false)
{
}

CBreakpointView::~CBreakpointView()
{
   if( IsWindow() ) /* scary */
      DestroyWindow();
}

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
      int iOldSel = m_ctrlList.GetSelectedIndex();
      m_ctrlList.SetRedraw(FALSE);
      m_ctrlList.DeleteAllItems();
      m_aItems.RemoveAll();
      CString sNumber = info.GetItem(_T("number"), _T("bkpt"));
      while( !sNumber.IsEmpty() ) {
         BREAKINFO Info;
         Info.iBrkNr = _ttol(sNumber);
         Info.sType = info.GetSubItem(_T("type"));
         Info.sFile = info.GetSubItem(_T("file"));
         Info.sFunc = info.GetSubItem(_T("func"));
         Info.iLineNum = _ttol(info.GetSubItem(_T("line")));
         Info.bEnabled = (info.GetSubItem(_T("enabled")) == _T("y"));
         Info.sAddress = info.GetSubItem(_T("addr"));
         Info.iIgnoreCount = _ttoi(info.GetSubItem(_T("ignore")));
         Info.sCondition = info.GetSubItem(_T("cond"));
         Info.iHitCount = _ttoi(info.GetSubItem(_T("times")));
         Info.bTemporary = (info.GetSubItem(_T("disp")) == _T("del"));
         m_aItems.Add(Info);

         int iIndex = m_aItems.GetSize() - 1;

         CString sDisplay = Info.sFunc;
         if( sDisplay.IsEmpty() ) sDisplay = Info.sFile;
         if( sDisplay.IsEmpty() ) sDisplay = Info.sAddress;
         CString sLocation;
         sLocation.Format(_T("%s, %d"), sDisplay, Info.iLineNum);

         int iItem = m_ctrlList.InsertItem(m_ctrlList.GetItemCount(), Info.sType);
         m_ctrlList.SetItemText(iItem, 1, Info.sFile);
         m_ctrlList.SetItemText(iItem, 2, sLocation);
         m_ctrlList.SetItemText(iItem, 3, Info.sAddress);
         m_ctrlList.SetCheckState(iItem, Info.bEnabled);
         m_ctrlList.SetItemData(iItem, iIndex);
         
         sNumber = info.FindNext(_T("number"), _T("bkpt"));
      }
      m_ctrlList.SelectItem(iOldSel);
      m_ctrlList.SetRedraw(TRUE);
      m_ctrlList.Invalidate();
      m_ctrlList.EnsureVisible(iOldSel, FALSE);
      m_bUpdating = false;
   }
}

void CBreakpointView::EvaluateView(CSimpleArray<CString>& aDbgCmd)
{
   aDbgCmd.Add(CString(_T("-break-list")));
}

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
   int cxChar = (int) LOWORD(GetDialogBaseUnits());
   m_ctrlList.SetColumnWidth(0, 12 * cxChar);
   m_ctrlList.SetColumnWidth(1, 13 * cxChar);
   m_ctrlList.SetColumnWidth(2, 14 * cxChar);
   m_ctrlList.SetColumnWidth(3, 12 * cxChar);
   bHandled = FALSE;
   return 0;
}

LRESULT CBreakpointView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   CClientRect rcClient = m_hWnd;
   m_ctrlList.MoveWindow(&rcClient);
   return 0;
}

LRESULT CBreakpointView::OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   int iItem = m_ctrlList.GetSelectedIndex();
   if( iItem < 0 ) return 0;
   // Load and show menu
   DWORD dwPos = ::GetMessagePos();
   POINT ptPos = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
   CMenu menu;
   menu.LoadMenu(IDR_BREAKPOINTS);
   CMenuHandle submenu = menu.GetSubMenu(0);
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, ptPos, FALSE, this);
   PostMessage(WM_COMMAND, MAKEWPARAM(nCmd, 0));
   return 0;
}

LRESULT CBreakpointView::OnItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
   LPNMLISTVIEW lpNMLV = (LPNMLISTVIEW) pnmh;
   // To prevent submitting new debug commands while we're just updating
   // existing items with GDB output, we check this guy...
   if( m_bUpdating ) return 0;
   // The state-image changes when the checkbox is clicked
   if( (lpNMLV->uNewState & LVIS_STATEIMAGEMASK) == (lpNMLV->uOldState & LVIS_STATEIMAGEMASK) ) return 0;
   int iItem = lpNMLV->iItem;
   m_ctrlList.SelectItem(iItem);
   const BREAKINFO& Info = m_aItems[ m_ctrlList.GetItemData(iItem) ];
   PostMessage(WM_COMMAND, MAKEWPARAM(Info.bEnabled ? ID_BREAKPOINTS_DISABLE : ID_BREAKPOINTS_ENABLE, 0));
   return 0;
}

LRESULT CBreakpointView::OnKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
   LPNMLVKEYDOWN lpNMLVKD = (LPNMLVKEYDOWN) pnmh;
   if( lpNMLVKD->wVKey == VK_RETURN ) return PostMessage(WM_COMMAND, MAKEWPARAM(ID_BREAKPOINTS_OPEN, 0));
   if( lpNMLVKD->wVKey == VK_DELETE ) return PostMessage(WM_COMMAND, MAKEWPARAM(ID_BREAKPOINTS_DELETE, 0));
   bHandled = FALSE;
   return 0;
}

LRESULT CBreakpointView::OnDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   PostMessage(WM_COMMAND, MAKEWPARAM(ID_BREAKPOINTS_OPEN, 0));
   return 0;
}

LRESULT CBreakpointView::OnItemOpenSource(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iItem = m_ctrlList.GetSelectedIndex();
   if( iItem < 0 ) return 0;
   const BREAKINFO& Info = m_aItems[ m_ctrlList.GetItemData(iItem) ];
   m_pProject->OpenView(Info.sFile, Info.iLineNum, FINDVIEW_ALL, true);
   return 0;
}

LRESULT CBreakpointView::OnItemDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iItem = m_ctrlList.GetSelectedIndex();
   if( iItem < 0 ) return 0;
   const BREAKINFO& Info = m_aItems[ m_ctrlList.GetItemData(iItem) ];
   m_pProject->m_DebugManager.RemoveBreakpoint(Info.sFile, Info.iLineNum);
   BOOL bDummy = FALSE;
   OnItemRefresh(0, 0, NULL, bDummy);
   return 0;
}

LRESULT CBreakpointView::OnItemEnable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iItem = m_ctrlList.GetSelectedIndex();
   if( iItem < 0 ) return 0;
   const BREAKINFO& Info = m_aItems[ m_ctrlList.GetItemData(iItem) ];
   m_pProject->m_DebugManager.EnableBreakpoint(Info.sFile, Info.iLineNum, TRUE);
   BOOL bDummy = FALSE;
   OnItemRefresh(0, 0, NULL, bDummy);
   return 0;
}

LRESULT CBreakpointView::OnItemDisable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iItem = m_ctrlList.GetSelectedIndex();
   if( iItem < 0 ) return 0;
   const BREAKINFO& Info = m_aItems[ m_ctrlList.GetItemData(iItem) ];
   m_pProject->m_DebugManager.EnableBreakpoint(Info.sFile, Info.iLineNum, FALSE);
   BOOL bDummy = FALSE;
   OnItemRefresh(0, 0, NULL, bDummy);
   return 0;
}

LRESULT CBreakpointView::OnItemRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   // Asking to refresh will also update our internal list.
   // We'll have to ask the views to update too I guess.
   m_pProject->DelayedDebugCommand(_T("-break-list"));
   m_pProject->DelayedGlobalViewMessage(DEBUG_CMD_SET_BREAKPOINTS);
   return 0;
}

LRESULT CBreakpointView::OnItemProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iItem = m_ctrlList.GetSelectedIndex();
   if( iItem < 0 ) return 0;
   const BREAKINFO& Info = m_aItems[ m_ctrlList.GetItemData(iItem) ];
   CBreakpointInfoDlg dlg;
   dlg.Init(m_pProject, Info);
   dlg.DoModal();
   return 0;
}

// IIdleListener

void CBreakpointView::OnIdle(IUpdateUI* pUIBase)
{   
   const BREAKINFO& Info = m_aItems[ m_ctrlList.GetSelectedIndex() ];
   pUIBase->UIEnable(ID_BREAKPOINTS_OPEN, TRUE);
   pUIBase->UIEnable(ID_BREAKPOINTS_DELETE, TRUE);
   pUIBase->UIEnable(ID_BREAKPOINTS_ENABLE, TRUE);
   pUIBase->UIEnable(ID_BREAKPOINTS_DISABLE, TRUE);
   pUIBase->UIEnable(ID_BREAKPOINTS_PROPERTIES, TRUE);
   pUIBase->UIEnable(ID_BREAKPOINTS_REFRESH, TRUE);
   pUIBase->UISetCheck(ID_BREAKPOINTS_ENABLE, Info.bEnabled);
   pUIBase->UISetCheck(ID_BREAKPOINTS_DISABLE, !Info.bEnabled);
}

void CBreakpointView::OnGetMenuText(UINT /*wID*/, LPTSTR /*pstrText*/, int /*cchMax*/)
{
}

