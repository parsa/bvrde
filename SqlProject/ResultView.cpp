
#include "StdAfx.h"
#include "resource.h"

#include "ResultView.h"

#pragma code_seg( "MISC" )

#include "../GenEdit/GenEdit.h"

#include "atldataobj.h"


////////////////////////////////////////////////////////
//

void CResultListCtrl::OnFinalMessage(HWND hWnd)
{
   delete this;
}


////////////////////////////////////////////////////////
//

CResultView::CResultView() :
   m_pCurProcessingList(NULL),
   m_wndParent(this, 1)
{
}

// Operations

BOOL CResultView::PreTranslateMessage(MSG* pMsg)
{
   return FALSE;
}

void CResultView::OnIdle(IUpdateUI* pUIBase)
{
   int nCount = 0;
   if( m_pCurProcessingList != NULL ) nCount = m_pCurProcessingList->GetSelectedCount();
   pUIBase->UIEnable(ID_FILE_SAVE, TRUE);
   pUIBase->UIEnable(ID_FILE_PRINT, FALSE);
   pUIBase->UIEnable(ID_EDIT_UNDO, FALSE);
   pUIBase->UIEnable(ID_EDIT_REDO, FALSE);
   pUIBase->UIEnable(ID_EDIT_COPY, nCount > 0);
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

LRESULT CResultView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{   
   CLogFont lf = AtlGetDefaultGuiFont();
   _tcscpy(lf.lfFaceName, _T("Arial"));
   lf.lfHeight = -12;
   if( !m_font.IsNull() ) m_font.DeleteObject();
   m_font.CreateFontIndirect(&lf);

   m_ctrlTab.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
   m_ctrlTab.SetFont(AtlGetDefaultGuiFont());
   m_wndParent.SubclassWindow(m_ctrlTab);

   m_ctrlEdit.Create(m_ctrlTab, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, 0, IDC_TEXT);
   m_ctrlEdit.ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
   m_ctrlEdit.SetFont(AtlGetStockFont(ANSI_FIXED_FONT));
   m_ctrlEdit.SetReadOnly(TRUE);
   m_ctrlEdit.SetEventMask(ENM_LINK);
 
   CString sText(MAKEINTRESOURCE(IDS_TAB_SUMMERY));
   TCITEM tci = { 0 };
   tci.mask = TCIF_TEXT;
   tci.pszText = (LPTSTR) (LPCTSTR) sText;
   m_ctrlTab.InsertItem(0, &tci);
   m_ctrlTab.SetCurSel(0);

   m_wndClient = m_ctrlEdit;

   SetTimer(BACKGROUND_TIMERID, BACKGROUND_TRIGGER_TIME);

   return 0;
}

LRESULT CResultView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   _ResetLists();
   bHandled = FALSE;
   return 0;
}

LRESULT CResultView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   RECT rc;
   GetClientRect(&rc);
   m_ctrlTab.MoveWindow(&rc);
   if( !m_wndClient.IsWindow() ) return 0;
   m_ctrlTab.AdjustRect(FALSE, &rc);
   m_wndClient.MoveWindow(&rc);
   return 0;
}

LRESULT CResultView::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_wndClient.SetFocus();
   return 0;
}

LRESULT CResultView::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   bHandled = FALSE;
   if( wParam != BACKGROUND_TIMERID ) return 0;
   return 0;
}

LRESULT CResultView::OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   bHandled = FALSE;
   int iSel = m_ctrlTab.GetCurSel();
   if( iSel == m_ctrlTab.GetItemCount() - 1 ) return 0;
   CListViewCtrl ctrlList = m_aLists[iSel]->m_hWnd;
   if( ctrlList.GetSelectedCount() == 0 ) return 0;

   CMenu menu;
   menu.LoadMenu(IDR_EDIT_LIST);
   CMenuHandle submenu = menu.GetSubMenu(0);
   POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
   UINT nCmd = _pDevEnv->ShowPopupMenu(NULL, submenu, pt, FALSE);
   if( nCmd == 0 ) return 0;

   CHeaderCtrl ctrlHeader = ctrlList.GetHeader();
   int nColumns = ctrlHeader.GetItemCount();
   CString sText, sItem;
   int iIndex = ctrlList.GetNextItem(-1, LVNI_SELECTED);
   while( iIndex != -1 ) {
      for( int i = 0; i < nColumns; i++ ) {
         sItem = _T("");
         ctrlList.GetItemText(iIndex, i, sItem);
         sText += sItem + (i != nColumns - 1 ? _T("\t") : _T("\r\n"));
      }      
      iIndex = ctrlList.GetNextItem(iIndex, LVNI_SELECTED);
   }
   AtlSetClipboardText(ctrlList, sText);

   bHandled = TRUE;
   return 0;
}

LRESULT CResultView::OnDataArrived(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
   DATAPACKET* pPacket = (DATAPACKET*) lParam;
   ATLASSERT(pPacket);
   if( pPacket == NULL ) return 0;

   switch( pPacket->Type ) {
   case PACKET_START:
      {
         KillTimer(BACKGROUND_TIMERID);

         _pDevEnv->PlayAnimation(TRUE, ANIM_TRANSFER);
         _pDevEnv->ShowStatusText(0, CString(MAKEINTRESOURCE(IDS_SB_EXECUTING)), TRUE);

         _RememberColumnSizes();
         _ResetLists();
      }
      break;
   case PACKET_COLUMNINFO:
      {
         m_pCurProcessingList = NULL;

         if( pPacket->iCols == 0 ) break;

         // Prepare list control
         CResultListCtrl* pList = new CResultListCtrl();
         pList->Create(m_ctrlTab, rcDefault, NULL, WS_CHILD | WS_BORDER | LVS_REPORT | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS);
         if( !pList->IsWindow() ) break;

         m_pCurProcessingList = pList;
         m_aLists.Add(pList);

         CListViewCtrl ctrlList = *pList;
         ctrlList.SetFont(m_font);
         ctrlList.SetExtendedListViewStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_INFOTIP);

         // Add tab for new recordset
         CString sTitle;
         sTitle.Format(IDS_TAB_RESULT, m_ctrlTab.GetItemCount());
         TCITEM tci = { 0 };
         tci.mask = TCIF_TEXT;
         tci.pszText = (LPTSTR) (LPCTSTR) sTitle;
         m_ctrlTab.InsertItem(m_ctrlTab.GetItemCount() - 1, &tci);

         CClientDC dc = ctrlList;
         HFONT hOldFont = dc.SelectFont(ctrlList.GetFont());
         TEXTMETRIC tm = { 0 };
         dc.GetTextMetrics(&tm);

         // Adjust column widths to last-known size or
         // approximated size of field width.
         for( int i = 0; i < pPacket->iCols; i++ ) {
            CString sName = pPacket->pstrData[i];
            ctrlList.AddColumn(sName, i);

            // Same columns appear in new recordset? 
            // Apply old widths.
            bool bAdjusted = false;
            if( m_aColumnInfo.GetSize() > i ) {
               int iIndex = m_aColumnInfo.FindKey(sName);
               if( iIndex >= 0 ) {
                  ctrlList.SetColumnWidth(i, m_aColumnInfo.GetValueAt(iIndex));
                  bAdjusted = true;
               }
            }
            if( !bAdjusted ) {
               // What is the column length
               // FIX: Using tmMaxCharWidth would have seemed reasonable here but
               //      creates much too large columns.
               int cx = (pPacket->plData[i] * (tm.tmAveCharWidth + 4)) + 10;
               // We'll also consider the width of the header
               SIZE szText = { 0 };
               dc.GetTextExtent(sName, sName.GetLength(), &szText);
               // NOTE: We won't allow really large columns, so here
               //       is a hard-coded limit in pixels.
               const int MAX_COLUMN_WIDTH = 150;
               cx = max(szText.cx + 25, cx);
               cx = min(MAX_COLUMN_WIDTH, cx);
               ctrlList.SetColumnWidth(i, cx);
            }
         }

         dc.SelectFont(hOldFont);
         if( m_ctrlTab.GetItemCount() == 2 ) m_ctrlTab.SetCurFocus(0);
      }
      break;
   case PACKET_ROWINFO:
      {
         if( pPacket->iCols == 0 ) break;
         if( m_pCurProcessingList == NULL ) break;

         // Add more items...
         m_pCurProcessingList->SetRedraw(FALSE);
         int nCount = m_pCurProcessingList->GetItemCount();
         int iPos = 0;
         int iIndex = nCount;
         for( int i = 0; i < pPacket->iRows; i++ ) {
            int iItem = m_pCurProcessingList->InsertItem(iIndex++, pPacket->pstrData[iPos++]);
            for( int j = 1; j < pPacket->iCols; j++ ) {
               m_pCurProcessingList->SetItemText(iItem, j, pPacket->pstrData[iPos++]);
            }
         }
         m_pCurProcessingList->SetRedraw(TRUE);
         if( nCount <= m_pCurProcessingList->GetTopIndex() ) {
            m_pCurProcessingList->Invalidate();
            m_pCurProcessingList->UpdateWindow();
         }
      }
      break;
   case PACKET_ERROR:
      {
         m_nErrors++;

         CString sText = pPacket->pstrData[0];
         sText += _T("\r\n\r\n");
         m_ctrlEdit.SetSel(-1, -1);
         CHARRANGE crStart;
         m_ctrlEdit.GetSel(crStart);
         m_ctrlEdit.ReplaceSel(sText);
         CHARRANGE crEnd;
         m_ctrlEdit.GetSel(crEnd);
         int iPos = sText.Find('\r');
         CString sPrefix(MAKEINTRESOURCE(IDS_DBERROR_FAILED));         
         CHARFORMAT cf;
         cf.cbSize = sizeof(CHARFORMAT);
         cf.dwMask = CFM_LINK;;
         cf.dwEffects = CFE_LINK;
         int iStart = crStart.cpMin + sPrefix.GetLength();
         int iEnd = crStart.cpMin + iPos;
         m_ctrlEdit.SetSel(iStart, iEnd);
         m_ctrlEdit.SetSelectionCharFormat(cf);
         m_ctrlEdit.SetSel(crEnd.cpMax, crEnd.cpMax);
         m_aLinks.Add(iStart, pPacket->iCols);

         TCHAR szBuffer[32] = { 0 };
         _pDevEnv->GetProperty(_T("editors.sql.maxErrors"), szBuffer, 31);
         int iMaxErrors = _ttoi(szBuffer);
         if( iMaxErrors > 0 && m_nErrors >= iMaxErrors ) ::PostMessage(GetParent(), WM_COMMAND, MAKEWPARAM(ID_QUERY_STOP, 0), 0L);
      }
      break;
   case PACKET_FINISH:
      {
         CString sText;
         if( pPacket->iCols > 0 && m_pCurProcessingList != NULL ) {
            sText.Format(pPacket->iRows == 1 ? IDS_ROW_SELECTED : IDS_ROWS_SELECTED, m_pCurProcessingList->GetItemCount());
         }
         else {
            sText.Format(pPacket->iRows == 1 ? IDS_ROW_AFFECTED : IDS_ROWS_AFFECTED, pPacket->iRows);
            if( pPacket->iRows <= 0 ) sText.LoadString(IDS_NO_ROWS_AFFECTED);
         }
         sText += _T("\r\n\r\n");
         m_ctrlEdit.SetSel(-1, -1);
         m_ctrlEdit.ReplaceSel(sText);
         m_ctrlEdit.SetSel(-1, -1);
      }
      break;
   case PACKET_DONE:
      {
         _pDevEnv->PlayAnimation(FALSE, ANIM_TRANSFER);
         _pDevEnv->ShowStatusText(0, CString(MAKEINTRESOURCE(IDS_SB_DONE)), TRUE);
         SetTimer(BACKGROUND_TIMERID, BACKGROUND_TRIGGER_TIME);
      }
      break;
   }

   // Clean up
   if( pPacket->plData ) delete [] pPacket->plData;
   if( pPacket->pstrData ) delete [] pPacket->pstrData;
   delete pPacket;

   return 0;
}

LRESULT CResultView::OnTabChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   // Change view
   if( m_wndClient.IsWindow() ) m_wndClient.ShowWindow(SW_HIDE);
   int iSel = m_ctrlTab.GetCurSel();
   if( iSel == m_ctrlTab.GetItemCount() - 1 ) {
      m_wndClient = m_ctrlEdit;
   }
   else {
      m_wndClient = m_aLists[iSel]->m_hWnd;
   }
   SendMessage(WM_SIZE);
   m_wndClient.ShowWindow(SW_SHOW);
   return 0;
}

LRESULT CResultView::OnLink(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
   ENLINK* pEnlink = (ENLINK*) pnmh;
   if( pEnlink->msg != WM_LBUTTONDOWN ) return 0;
   int iStart = pEnlink->chrg.cpMin;
   int iIndex = m_aLinks.FindKey(iStart);
   if( iIndex < 0 ) return 0;
   int iLine = m_aLinks.GetValueAt(iIndex);
   ::PostMessage(GetParent(), WM_COMMAND, MAKEWPARAM(ID_VIEW_SETLINE, iLine), 0L);
   return 0;
}

void CResultView::_RememberColumnSizes()
{
   // Remember width of current columns
   for( int i = 0; i < m_aLists.GetSize(); i++ ) {
      if( !m_aLists[i]->IsWindow() ) break;
      CHeaderCtrl ctrlHeader = m_aLists[i]->GetHeader();
      for( int j = 0; j < ctrlHeader.GetItemCount(); j++ ) {
         TCHAR szName[128] = { 0 };
         HDITEM hdi = { 0 };
         hdi.mask = HDI_TEXT | HDI_WIDTH;
         hdi.pszText = szName;
         hdi.cchTextMax = (sizeof(szName) / sizeof(TCHAR)) - 1;
         ctrlHeader.GetItem(j, &hdi);
         CString sTitle = szName;
         if( !m_aColumnInfo.SetAt(sTitle, hdi.cxy) ) m_aColumnInfo.Add(sTitle, hdi.cxy);
      }
   }
}

void CResultView::_ResetLists()
{
   m_pCurProcessingList = NULL;

   // Remove lists and reset result text
   while( m_ctrlTab.GetItemCount() > 1 ) m_ctrlTab.DeleteItem(0);
   for( int i = 0; i < m_aLists.GetSize(); i++ ) m_aLists[i]->DestroyWindow();
   m_aLists.RemoveAll();

   m_nErrors = 0;
   m_aLinks.RemoveAll();
   m_ctrlTab.SetCurFocus(0);
   m_ctrlEdit.SetWindowText(_T("\r\n"));
}
