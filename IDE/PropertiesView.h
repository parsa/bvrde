#if !defined(AFX_PROPERTIESVIEW_H__20030315_D78F_23BB_B6D4_0080AD509054__INCLUDED_)
#define AFX_PROPERTIESVIEW_H__20030315_D78F_23BB_B6D4_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropertyList.h"
#include "Solution.h"


class CPropertiesView : 
   public CWindowImpl<CPropertiesView>,
   public ISerializable
{
public:
   DECLARE_WND_CLASS_EX(_T("BVRDE_PropertiesView"), 0, COLOR_BTNFACE)

   CComboBox m_ctrlName;
   CPropertyListCtrl m_ctrlList;
   CFont m_BoldFont;
   bool m_bIgnoreRest;

   BEGIN_MSG_MAP(CPropertiesView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CLogFont lf = AtlGetDefaultGuiFont();
      lf.MakeBolder();
      m_BoldFont.CreateFontIndirect(&lf);

      m_ctrlName.Create(m_hWnd, rcDefault, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_TABSTOP|CBS_DROPDOWNLIST|CBS_HASSTRINGS|CBS_OWNERDRAWFIXED, 0, IDC_NAME);
      m_ctrlName.SetFont(m_BoldFont);
      m_ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_VSCROLL|WS_TABSTOP|LBS_HASSTRINGS|LBS_OWNERDRAWVARIABLE|LBS_NOTIFY, WS_EX_CLIENTEDGE, IDC_LIST);
      m_ctrlList.SetFont(AtlGetDefaultGuiFont());
      m_ctrlList.SetExtendedListStyle(PLS_EX_CATEGORIZED|PLS_EX_XPLOOK);
      return 0;
   }
   LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      if( !m_ctrlName.IsWindow() || !m_ctrlList.IsWindow() ) return 0;
      RECT rcClient;
      GetClientRect(&rcClient);
      ::InflateRect(&rcClient, -2, -2);
      RECT rc = { rcClient.left, rcClient.top, rcClient.right, rcClient.top + 140 };
      m_ctrlName.MoveWindow(&rc);
      RECT rcList = { rc.left, rc.top + 26, rc.right, rcClient.bottom };
      m_ctrlList.MoveWindow(&rcList);
      return 0;
   }
   LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
   {
      if( wParam != IDC_NAME ) {
         bHandled = FALSE;
         return 0;
      }
      DRAWITEMSTRUCT* pDIS = (DRAWITEMSTRUCT*) lParam;
      CDCHandle dc = pDIS->hDC;
      
      // Get combobox/list text
      CString sText;
      if( pDIS->itemState & ODS_COMBOBOXEDIT ) {
         sText = CWindowText(m_ctrlName);
      }
      else {
         if( pDIS->itemID == -1 ) return 0;
         m_ctrlName.GetLBText(pDIS->itemID, sText);
      }
      int pos = sText.ReverseFind(_T('-'));
      if( pos < 0 ) return 0;
      CString sLeft = sText.Left(pos);
      CString sRight = sText.Mid(pos);

      // Fill background
      COLORREF clrBack = ::GetSysColor(COLOR_WINDOW);
      COLORREF clrText = ::GetSysColor(COLOR_WINDOWTEXT);
      if( pDIS->itemState & ODS_SELECTED ) {
         clrBack = ::GetSysColor(COLOR_HIGHLIGHT);
         clrText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
      }
      dc.FillSolidRect(&pDIS->rcItem, clrBack);

      // Draw text (first part in bold, second in normal font)
      const UINT uStyle = DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_EDITCONTROL;
      RECT rc = pDIS->rcItem;
      RECT rcText = rc;
      HFONT hOldFont = dc.SelectFont(m_BoldFont);
      dc.SetBkMode(TRANSPARENT);
      dc.SetTextColor(clrText);
      dc.DrawText(sLeft, -1, &rcText, uStyle | DT_CALCRECT);
      dc.DrawText(sLeft, -1, &rcText, uStyle);
      rc.left += rcText.right - rcText.left;
      dc.SelectFont(AtlGetDefaultGuiFont());
      dc.DrawText(sRight, -1, &rc, uStyle | DT_END_ELLIPSIS);
      dc.SelectFont(hOldFont);

      return 0;
   }

   // Operations

   void Clear()
   {
      ATLASSERT(::IsWindow(m_hWnd));
      m_ctrlName.ResetContent();
      m_ctrlList.ResetContent();
   }
   void SetActiveElement(IElement* pElement)
   {
      Clear();
      if( pElement == NULL ) return;
      // Add name to combobox
      CString sElementName;
      pElement->GetName(sElementName.GetBufferSetLength(128), 128);
      sElementName.ReleaseBuffer();
      CString sElementType;
      pElement->GetType(sElementType.GetBufferSetLength(64), 64);
      sElementType.ReleaseBuffer();
      CString sName;
      sName.Format(IDS_PROPERTIES_NAME, sElementName, sElementType);
      m_ctrlName.AddString(sName);
      m_ctrlName.SetCurSel(0);
      // Write header and items
      if( pElement == g_pSolution ) return;
      m_bIgnoreRest = false;
      WriteItem(CString(MAKEINTRESOURCE(IDS_MISC)));
      pElement->Save(this);
   }

   // ISerializable

   BOOL ReadGroupBegin(LPCTSTR pstrName) { return FALSE; };
   BOOL ReadGroupEnd() { return FALSE; };
   BOOL ReadItem(LPCTSTR pstrName) { return FALSE; };
   BOOL Read(LPCTSTR pstrName, LPTSTR szValue, UINT cchMax) { return FALSE; };
   BOOL Read(LPCTSTR pstrName, SYSTEMTIME& stValue) { return FALSE; };
   BOOL Read(LPCTSTR pstrName, long& lValue) { return FALSE; };
   BOOL Read(LPCTSTR pstrName, BOOL& bValue) { return FALSE; };
   BOOL WriteGroupBegin(LPCTSTR pstrName) 
   { 
      // Denies serialization of nested objects.
      // We only want the main properties.
      m_bIgnoreRest = true;
      return FALSE;
   }
   BOOL WriteGroupEnd() 
   { 
      return FALSE; 
   }
   BOOL WriteItem(LPCTSTR pstrName)
   { 
      if( m_bIgnoreRest ) return TRUE;
      CString sName = pstrName;
      if( sName[0] == _T('_') ) {
         m_bIgnoreRest = true;
         return TRUE;
      }
      ::CharUpperBuff( (LPTSTR) (LPCTSTR) sName, 1 );
      m_ctrlList.AddItem( PropCreateCategory(sName) );
      return TRUE; 
   }
   BOOL Write(LPCTSTR pstrName, LPCTSTR pstrValue)
   { 
      if( m_bIgnoreRest ) return TRUE;
      CString sName = pstrName;
      if( sName[0] == _T('_') ) return TRUE;
      ::CharUpperBuff( (LPTSTR) (LPCTSTR) sName, 1 );
      m_ctrlList.AddItem( PropCreateReadOnlyItem(sName, pstrValue) );
      return TRUE;
   }
   BOOL Write(LPCTSTR pstrName, SYSTEMTIME stValue)
   { 
      return TRUE; 
   }
   BOOL Write(LPCTSTR pstrName, long lValue)
   { 
      TCHAR szValue[20];
      ::wsprintf(szValue, _T("%ld"), lValue);
      return Write(pstrName, szValue);
   }
   BOOL Write(LPCTSTR pstrName, BOOL bValue)
   {
      CString sValue(MAKEINTRESOURCE(bValue ? IDS_TRUE : IDS_FALSE));
      return Write(pstrName, sValue);
   }
   BOOL WriteExternal(LPCTSTR /*pstrName*/)
   {      
      // Denies serialization of nested objects.
      // We only want the main properties.
      m_bIgnoreRest = true;
      return FALSE;
   }
   BOOL Delete(LPCTSTR /*pstrName*/)
   {      
      return FALSE;
   }
};


#endif // !defined(AFX_PROPERTIESVIEW_H__20030315_D78F_23BB_B6D4_0080AD509054__INCLUDED_)

