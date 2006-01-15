
#include "StdAfx.h"
#include "resource.h"

#include "QuickWatchDlg.h"

#include "Project.h"

#pragma code_seg( "DIALOGS" )


// Constructor

CQuickWatchDlg::CQuickWatchDlg(IDevEnv* pDevEnv, CRemoteProject* pProject, LPCTSTR pstrDefault) :
   m_pDevEnv(pDevEnv),
   m_pProject(pProject),
   m_ctrlEdit(this, 1),
   m_ctrlList(this, 2),
   m_sDefault(pstrDefault),
   m_iColumnWidth(0),
   m_iQueryParent(0)
{      
}

// Operations

void CQuickWatchDlg::SetInfo(LPCTSTR pstrType, CMiInfo& info)
{
   if( _tcscmp(pstrType, _T("name")) == 0 ) 
   {
      m_aItems.RemoveAll();
      m_ctrlList.ResetContent();
      m_ctrlList.Invalidate();
      // Add root item
      ITEM item;
      item.sKey = _T("quickwatch");
      item.sName = m_sVariableName;
      item.iParent = 0;
      item.iIndent = 0;
      item.bHasValue = false;
      item.bHasChildren = false;
      m_aItems.Add(item);
      m_ctrlList.AddString((LPCTSTR) 0 );
      // Query item, children and value
      m_iQueryParent = 0;
      m_sQueryVariable = _T("quickwatch");
      m_pProject->m_DebugManager.DoDebugCommand(_T("-var-list-children quickwatch"));
      m_pProject->m_DebugManager.DoDebugCommand(_T("-var-evaluate-expression quickwatch"));
      // Recalc column-width
      _CalcColumnWidth();
   }
   else if( _tcscmp(pstrType, _T("value")) == 0 ) 
   {
      // Add the value to this item
      int i;
      for( i = 0; i < m_aItems.GetSize(); i++ ) {
         if( m_sQueryVariable == m_aItems[i].sKey ) {
            ITEM& item = m_aItems[i];
            item.sValue = info.GetItem(_T("value"));
            item.bHasValue = true;
            // Now that the value has changed, resize the item height again...
            for( int j = 0; j < m_ctrlList.GetCount(); j++ ) {
               if( (int) m_ctrlList.GetItemData(j) == i ) {
                  m_ctrlList.SetItemHeight(j, _GetItemHeight(i));
                  RECT rcClient;
                  m_ctrlList.GetClientRect(&rcClient);
                  RECT rcItem;
                  m_ctrlList.GetItemRect(j, &rcItem);
                  rcItem.bottom = rcClient.bottom;
                  m_ctrlList.InvalidateRect(&rcItem, TRUE);
                  break;
               }
            }
            break;
         }
      }
      // Look for the next item to add a value for...
      m_sQueryVariable.Empty();
      for( i = 0; i < m_aItems.GetSize(); i++ ) {
         if( !m_aItems[i].bHasValue ) {
            m_iQueryParent = m_aItems[i].iParent;
            m_sQueryVariable = m_aItems[i].sKey;
            CString sCommand;
            sCommand.Format(_T("-var-evaluate-expression %s"), m_sQueryVariable);
            m_pProject->m_DebugManager.DoDebugCommand(sCommand);
            // NOTE: We break out now because we can only handle
            //       one variable pr debugger MI round-trip.
            //       The cause is the lack of information in the GDB "value" response
            //       which forces us to use a global 'm_sQueryVariable' variable to
            //       hold the currently queried name information.
            //       See several other complaints about this in DebugManager.cpp.
            break;
         }
      }
   }
   else if( _tcscmp(pstrType, _T("numchild")) == 0 ) 
   {
      if( m_sQueryVariable.IsEmpty() ) return;
      CString sName = info.GetItem(_T("name"));
      while( !sName.IsEmpty() ) {
         CString sExp = info.GetSubItem(_T("exp"));
         CString sType = info.GetSubItem(_T("type"));
         CString sNumChild = info.GetSubItem(_T("numchild"));
         if( sExp.IsEmpty() ) {
            sExp = sName;
            sExp.Replace(_T("quickwatch."), _T(""));
         }
         // Add item to cache
         int iParentPos = m_ctrlList.GetItemData(m_iQueryParent);
         ATLASSERT(iParentPos>=0 && iParentPos<m_aItems.GetSize());
         if( iParentPos < 0 ) return;
         const ITEM& parent = m_aItems[iParentPos];
         ITEM item;
         item.sKey = sName;
         item.sName = sExp;
         item.iParent = m_iQueryParent;
         item.iIndent = parent.iIndent + 1;
         item.bHasChildren = _ttol(sNumChild) > 0;
         item.bExpanded = false;
         item.bHasValue = false;
         m_aItems.Add(item);
         int iPos = m_aItems.GetSize() - 1;
         // Find the last item in the control with this indent
         int iInsertAt = m_iQueryParent + 1;
         while( iInsertAt < m_ctrlList.GetCount() ) {
            if( m_aItems[ m_ctrlList.GetItemData(iInsertAt) ].iIndent != item.iIndent ) break;
            iInsertAt++;
         }
         // Insert item in list
         m_ctrlList.InsertString(iInsertAt, (LPCTSTR) iPos);
         // Look for next item
         sName = info.FindNext(_T("name"));
      }
      // Root always has children...
      m_aItems[0].bHasChildren = true;
      // Recalc list layout
      _CalcColumnWidth();
      m_ctrlList.Invalidate();
   }
}

// Message handler

LRESULT CQuickWatchDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   m_pDevEnv->EnableModeless(FALSE);

   m_sDefault.TrimLeft();
   m_sDefault.TrimRight();

   m_ctrlLine = GetDlgItem(IDC_LINE);
   m_ctrlLine.SetWindowText(m_sDefault);
   m_ctrlEdit.SubclassWindow(m_ctrlLine.GetWindow(GW_CHILD));
   m_ctrlList.SubclassWindow(GetDlgItem(IDC_LIST));
   ATLASSERT((m_ctrlList.GetStyle() & LBS_HASSTRINGS)==0);

   m_PlusIcon.LoadIcon(IDI_PLUS);
   m_MinusIcon.LoadIcon(IDI_MINUS);

   _CalcColumnWidth();
   _UpdateButtons();

   // Start with viewing right away
   PostMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0));

   DlgResize_Init(true, true);
   return TRUE;
}

LRESULT CQuickWatchDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   _RemoveVariable();
   bHandled = FALSE;
   return 0;
}

LRESULT CQuickWatchDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( m_ctrlLine.GetWindowTextLength() == 0 ) return 0;
   _CreateVariable();
   return 0;
}

LRESULT CQuickWatchDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   m_pDevEnv->EnableModeless(TRUE);
   DestroyWindow();
   return 0;
}

LRESULT CQuickWatchDlg::OnAddWatch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   BOOL bDummy;
   m_pProject->OnViewWatch(0, 0, NULL, bDummy);
   //
   CString sCommand;
   LPARAM lKey = (LPARAM) ::GetTickCount();
   sCommand.Format(_T("-var-create watch%ld * \"%s\""), lKey, CWindowText(m_ctrlLine));
   m_pProject->DelayedDebugCommand(sCommand);
   sCommand = _T("-var-update *");
   m_pProject->DelayedDebugCommand(sCommand);
   //
   m_pDevEnv->EnableModeless(TRUE);
   DestroyWindow();
   return 0;
}

LRESULT CQuickWatchDlg::OnSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int iIndex = m_ctrlList.GetCurSel();
   if( iIndex < 0 ) return 0;
   int iPos = m_ctrlList.GetItemData(iIndex);
   CString sName = m_aItems[iPos].sName;
   CString sText = m_aItems[iPos].sKey;
   // FIX: First we need to strip away the "quickwatch" pseudo-name, and
   //      then hack our way through the GDB peculiarities, such as the
   //      "public", "private" entries...
   sText.Replace(_T("quickwatch"), m_aItems[0].sName);
   sText.Replace(_T("public."), _T(""));
   sText.Replace(_T("private."), _T(""));
   sText.Replace(_T("protected."), _T(""));
   // Remove the types
   for( int i = 0; i < m_aItems.GetSize(); i++ ) {
      if( !m_aItems[i].sType.IsEmpty() ) {
         CString sType;
         sType.Format(_T(".%s"), m_aItems[i].sType);
         sText.Replace(sType, _T(""));
      }
   }
   // Do final replacements
   if( !sName.IsEmpty() ) {
      // Turn expression "szName.32" into "szName+32"
      if( _istdigit(sName[0]) ) {
         int iPos = sText.ReverseFind('.');
         if( iPos > 0 ) sText.SetAt(iPos, '+');
      }
      // Turn expression "szName+32.*szName+32" into just "*szName+32"
      if( sName[0] == '*' ) sText.Replace(sName.Mid(1) + _T("."), _T(""));
   }
   m_ctrlLine.SetWindowText(sText);
   return 0;
}

// Edit messages

LRESULT CQuickWatchDlg::OnEditChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   _UpdateButtons();
   return 0;
}

LRESULT CQuickWatchDlg::OnEditChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == '\r' ) return PostMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0));
   if( wParam == VK_ESCAPE ) return PostMessage(WM_COMMAND, MAKEWPARAM(IDCANCEL, 0));
   if( wParam == VK_TAB ) return (LRESULT) ::SetFocus(GetNextDlgTabItem(m_ctrlLine, FALSE));
   bHandled = FALSE;
   return 0;
}

// List messages

LRESULT CQuickWatchDlg::OnListChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   // NOTE: Handling WM_CHARTOITEM would be the proper thing to do, but Windows will
   //       then not handle arrow-keys and I'm too lazy to implement these now...
   if( wParam == VK_RETURN || wParam == VK_ESCAPE ) return PostMessage(WM_COMMAND, MAKEWPARAM(IDCANCEL, 0));
   if( wParam == VK_TAB ) return (LRESULT) ::SetFocus(GetNextDlgTabItem(m_ctrlList, TRUE));
   bHandled = FALSE;
   return 0;
}

LRESULT CQuickWatchDlg::OnListClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
   bHandled = FALSE;
   POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
   BOOL bOutside = FALSE;
   int iItem = m_ctrlList.ItemFromPoint(pt, bOutside);
   if( iItem < 0 || bOutside ) return 0;
   RECT rcItem = { 0 };
   m_ctrlList.GetItemRect(iItem, &rcItem);
   if( !::PtInRect(&rcItem, pt) ) return 0;
   // Can expand this item?
   int iPos = m_ctrlList.GetItemData(iItem);
   const ITEM& item = m_aItems[iPos];
   if( item.iIndent == 0 || !item.bHasChildren ) return 0;
   // Hit the expand-indicator?
   RECT rcButton;
   rcButton.left = rcItem.left + 3 + ((item.iIndent - 1) * CX_INDENT);
   rcButton.top = rcItem.top + 3;
   rcButton.right = rcButton.left + 16;
   rcButton.bottom = rcButton.top + 16;
   if( !::PtInRect(&rcButton, pt) ) return 0;
   if( !item.bExpanded ) _ExpandItem(iItem); else _CollapseItem(iItem);
   return 0;
}

LRESULT CQuickWatchDlg::OnListDblClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
   bHandled = FALSE;
   POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
   BOOL bOutside = FALSE;
   int iItem = m_ctrlList.ItemFromPoint(pt, bOutside);
   if( iItem < 0 || bOutside ) return 0;
   // Expand or collapse?
   int iPos = m_ctrlList.GetItemData(iItem);
   const ITEM& item = m_aItems[iPos];
   if( item.iIndent == 0 || !item.bHasChildren ) return 0;
   if( !item.bExpanded ) _ExpandItem(iItem); else _CollapseItem(iItem);
   return 0;
}

void CQuickWatchDlg::_ExpandItem(int iItem)
{
   int iPos = m_ctrlList.GetItemData(iItem);
   ITEM& item = m_aItems[iPos];
   if( item.bExpanded ) return;
   if( !item.bHasChildren ) return;
   // Query value and children so we expand it
   m_iQueryParent = iItem;
   m_sQueryVariable = item.sKey;
   CString sCommand;
   sCommand.Format(_T("-var-list-children %s"), item.sKey);
   m_pProject->m_DebugManager.DoDebugCommand(sCommand);
   // Set off value evaluation run...
   sCommand.Format(_T("-var-evaluate-expression %s"), item.sKey);
   m_pProject->m_DebugManager.DoDebugCommand(sCommand);
   // Mark branch as expanded
   item.bExpanded = true;
}

void CQuickWatchDlg::_CollapseItem(int iItem)
{
   int iPos = m_ctrlList.GetItemData(iItem);
   int nCount = m_ctrlList.GetCount();
   ITEM& item = m_aItems[iPos];
   if( !item.bExpanded ) return;
   // Collapse branch; stop the interation first
   // then remove items
   item.bExpanded = false;
   CSimpleArray<int> aDeletes;
   for( int i = iItem + 1; i < nCount; i++ ) {
      const ITEM& child = m_aItems[ m_ctrlList.GetItemData(i) ];
      if( child.iIndent <= item.iIndent ) break;
      aDeletes.Add(i);
   }
   // HACK: We cannot remove the items because the list-control
   //       relies on it ordering. We'll just mark it invalid.
   for( int x = aDeletes.GetSize() - 1; x >= 0; --x ) {
      int iIndex = aDeletes[x];
      ITEM& child = m_aItems[ m_ctrlList.GetItemData(iIndex) ];
      child.sKey.Empty();
      child.bHasValue = true;
      m_ctrlList.DeleteString(iIndex);
   }
   m_sQueryVariable.Empty();
}


// Ownerdraw

void CQuickWatchDlg::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
   CDCHandle dc = lpDIS->hDC;

   int iIndex = lpDIS->itemID;
   if( iIndex == -1 ) return;

   const ITEM& item = m_aItems[lpDIS->itemData];
   RECT rc = lpDIS->rcItem;
   int iMiddle = rc.left + m_iColumnWidth;
   bool bSelected = (lpDIS->itemState & ODS_SELECTED) != 0;

   dc.FillSolidRect(&rc, ::GetSysColor(bSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW));

   if( item.iIndent > 0 && item.bHasChildren ) {
      POINT pt = { rc.left + 3 + ((item.iIndent - 1) * CX_INDENT), rc.top + 3 };
      if( item.bExpanded ) m_MinusIcon.DrawIcon(dc, pt); else m_PlusIcon.DrawIcon(dc, pt);
   }

   COLORREF clrText = ::GetSysColor(COLOR_WINDOWTEXT);
   dc.SetBkMode(TRANSPARENT);
   dc.SetTextColor(bSelected ? ::GetSysColor(COLOR_HIGHLIGHTTEXT) : clrText);
   RECT rcName = { rc.left + 3, rc.top + 1, iMiddle - 2, rc.bottom };
   rcName.left += item.iIndent * CX_INDENT;
   dc.DrawText(item.sName, -1, &rcName, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

   if( item.sValue.FindOneOf(_T("[{@")) == 0 ) clrText = ::GetSysColor(COLOR_HIGHLIGHT);
   if( item.sValue.Find(_T(" <")) >= 0 ) clrText = ::GetSysColor(COLOR_GRAYTEXT);
   dc.SetTextColor(bSelected ? ::GetSysColor(COLOR_HIGHLIGHTTEXT) : clrText);
   RECT rcValue = { iMiddle + 3, rc.top + 1, rc.right - 2, rc.bottom };
   dc.DrawText(item.sValue.Left(MAX_VALUE_DISPLAY_LENGTH), -1, &rcValue, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);

   if( !bSelected ) 
   {
      CPen pen;
      pen.CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DLIGHT));
      HPEN hOldPen = dc.SelectPen(pen);
      dc.MoveTo(iMiddle, rc.top);
      dc.LineTo(iMiddle, rc.bottom);
      if( iIndex == 0 ) {
         dc.MoveTo(rc.left + 30, rc.bottom - 2);
         dc.LineTo(rc.right - 30, rc.bottom - 2);
      }
      if( iIndex == m_aItems.GetSize() - 1 ) {
         dc.MoveTo(rc.left, rc.bottom - 1);
         dc.LineTo(rc.right, rc.bottom - 1);
      }
      dc.SelectPen(hOldPen);
   }
}

void CQuickWatchDlg::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{   
   lpMIS->itemHeight = _GetItemHeight(lpMIS->itemData);
}

// Implementation

int CQuickWatchDlg::_GetItemHeight(int iIndex) const
{
   // Calculate height of item.
   // NOTE: This may be a multi-line text so we need to determine the
   //       correct DrawText height.
   CString sValue = m_aItems[iIndex].sValue.Left(MAX_VALUE_DISPLAY_LENGTH);
   sValue.TrimRight();
   if( sValue.IsEmpty() ) sValue = _T("X");
   RECT rc;
   m_ctrlList.GetClientRect(&rc);
   rc.left += m_iColumnWidth;
   CClientDC dc = m_ctrlList;
   int iHeight = dc.DrawText(sValue, -1, &rc, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
   if( iIndex == 0 ) {
      iHeight += 3;
   }
   else if( iIndex == m_aItems.GetSize() - 1 ) {
      iHeight++;
   }
   return iHeight;
}

void CQuickWatchDlg::_CreateVariable()
{
   CWaitCursor cursor;
   // Remove previous line
   _RemoveVariable();
   // Add new watch in GDB
   m_sVariableName = CWindowText(m_ctrlEdit);
   m_sVariableName.Remove(' ');
   CString sCommand;
   sCommand.Format(_T("-var-create quickwatch * %s"), m_sVariableName);
   m_pProject->m_DebugManager.DoDebugCommand(sCommand);
   // Add to history
   if( m_ctrlLine.FindStringExact(-1, m_sVariableName) == CB_ERR ) m_ctrlLine.AddString(m_sVariableName);
}

void CQuickWatchDlg::_RemoveVariable()
{
   if( m_sVariableName.IsEmpty() ) return;
   m_pProject->m_DebugManager.DoDebugCommand(_T("-var-delete quickwatch"));
}

void CQuickWatchDlg::_CalcColumnWidth()
{
   ATLASSERT(m_ctrlList.IsWindow());
   RECT rcClient;
   m_ctrlList.GetClientRect(&rcClient);
   int iWidth = 0;
   CClientDC dc = m_ctrlList;
   HFONT hOldFont = dc.SelectFont(m_ctrlList.GetFont());
   for( int i = 0; i < m_aItems.GetSize(); i++ ) {
      RECT rcText = rcClient;
      dc.DrawText(m_aItems[i].sName, -1, &rcText, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
      int cx = (rcText.right - rcText.left) + 10 + (m_aItems[i].iIndent * CX_INDENT);
      if( cx > iWidth ) iWidth = cx;
   }
   dc.SelectFont(hOldFont);
   // Repaint when changed
   if( iWidth < 100 ) iWidth = 100;
   if( m_iColumnWidth != iWidth ) {
      m_iColumnWidth = iWidth;
      m_ctrlList.Invalidate();
   }
}

void CQuickWatchDlg::_UpdateButtons()
{
   CWindow(GetDlgItem(IDOK)).EnableWindow(m_ctrlLine.GetWindowTextLength() > 0);
   CWindow(GetDlgItem(IDC_ADD_WATCH)).EnableWindow(m_ctrlLine.GetWindowTextLength() > 0);
   CButton(GetDlgItem(IDOK)).SetButtonStyle(m_ctrlLine.GetWindowTextLength() > 0 ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON);
}
