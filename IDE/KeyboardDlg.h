#if !defined(AFX_KEYBOARDDLG_H__20030126_01A0_E58B_B164_0080AD509054__INCLUDED_)
#define AFX_KEYBOARDDLG_H__20030126_01A0_E58B_B164_0080AD509054__INCLUDED_

#pragma once

 
class CKeyboardDlg : 
   public CPropertyPageImpl<CKeyboardDlg>,
   public COwnerDraw<CKeyboardDlg>
{
public:
   enum { IDD = IDD_CONFIG_KEYBOARD };

   enum { MAX_ACCELS = 200 };

   typedef CSimpleMap<WORD, ACCEL> ACCELMAP;

   HWND m_hWndSheet;
   ACCELMAP m_mapCurrent;
   ACCELMAP m_mapDefault;
   HACCEL* m_phAccel;
   HMENU m_hMenu;

   CListBox m_ctrlList;
   CContainedWindowT<CHotKeyCtrl> m_ctrlDefault;
   CContainedWindowT<CHotKeyCtrl> m_ctrlCurrent;
   CHotKeyCtrl m_ctrlNew;
   CFont m_fontNormal;
   CFont m_fontBold;


   CKeyboardDlg() :
      m_hWndSheet(NULL),
      m_phAccel(NULL), 
      m_hMenu(NULL),
      m_ctrlDefault(this, 1),
      m_ctrlCurrent(this, 1)
   {
   }

   void Init(HWND hWndSheet, HMENU hMenu, HACCEL hDefault, HACCEL* phAccel)
   {
      m_hWndSheet = hWndSheet;
      m_hMenu = hMenu;
      m_phAccel = phAccel;
      _LoadTable(m_mapDefault, hDefault);
      _LoadTable(m_mapCurrent, *m_phAccel);
   }

   // Message map and handlers

   BEGIN_MSG_MAP(CKeyboardDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDC_ASSIGN, OnAssign)
      COMMAND_ID_HANDLER(IDC_REMOVE, OnRemove)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnItemChange)
      COMMAND_HANDLER(IDC_LIST, LBN_SELCHANGE, OnSelChange)
      CHAIN_MSG_MAP( COwnerDraw<CKeyboardDlg> )
      CHAIN_MSG_MAP( CPropertyPageImpl<CKeyboardDlg> )
   ALT_MSG_MAP(1)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ATLASSERT(m_phAccel);
      ATLASSERT(::IsMenu(m_hMenu));

      m_ctrlList = GetDlgItem(IDC_LIST);

      CLogFont lf;
      lf.SetMenuFont();
      lf.MakeLarger(2);
      m_fontNormal.CreateFontIndirect(&lf);
      lf.MakeLarger(1);
      lf.MakeBolder();
      m_fontBold.CreateFontIndirect(&lf);

      m_ctrlDefault.SubclassWindow(GetDlgItem(IDC_DEFAULT_KEY));
      m_ctrlCurrent.SubclassWindow(GetDlgItem(IDC_CURRENT_KEY));
      m_ctrlNew = GetDlgItem(IDC_NEW_KEY);

      _BuildMenu(m_hMenu);

      m_ctrlList.SetCurSel(0);
      _UpdateButtons();

      return TRUE;
   }
   int OnApply()
   {
      if( *m_phAccel != NULL ) ::DestroyAcceleratorTable(*m_phAccel);
      *m_phAccel = _BuildAccel(m_mapCurrent);

      CString sFilename = CMainFrame::GetSettingsFilename();
      CXmlSerializer arc;
      if( !arc.Open(_T("Settings"), sFilename) ) return PSNRET_INVALID_NOCHANGEPAGE;

      arc.Delete(_T("KeyMappings"));
      if( !arc.WriteGroupBegin(_T("KeyMappings")) ) return PSNRET_INVALID_NOCHANGEPAGE;
      for( int i = 0; i < m_mapCurrent.GetSize(); i++ ) {
         // Write settings to config file
         ACCEL accel = m_mapCurrent.GetValueAt(i);
         arc.WriteItem(_T("Key"));
         arc.Write(_T("cmd"), (long) accel.cmd);
         arc.Write(_T("key"), (long) accel.key);
         arc.Write(_T("flags"), (long) accel.fVirt);
      }
      arc.WriteGroupEnd();

      arc.Save();
      arc.Close();
      return PSNRET_NOERROR;
   }

   LRESULT OnAssign(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      WORD wKey = (WORD) m_ctrlNew.GetHotKey();
      WORD wCmd = (WORD) m_ctrlList.GetItemData(m_ctrlList.GetCurSel());
      ACCEL accel = _GetAccelFromHotKey(wCmd, wKey);
      if( m_mapDefault.FindKey(accel.cmd) >= 0 ) {
         if( IDNO == g_pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_KEYDUPLICATE)), CString(MAKEINTRESOURCE(IDS_CAPTION_QUESTION)), MB_YESNO | MB_ICONQUESTION) ) return 0;
      }
      if( !m_mapCurrent.SetAt(accel.cmd, accel) ) m_mapCurrent.Add(accel.cmd, accel);
      //
      m_ctrlCurrent.SetHotKey(LOBYTE(wKey), HIBYTE(wKey));      
      m_ctrlNew.SetHotKey(0, 0);
      m_ctrlNew.SetFocus();
      //
      _UpdateButtons();
      SetModified(TRUE);
      return 0;
   }
   LRESULT OnRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      WORD cmd = (WORD) m_ctrlList.GetItemData(m_ctrlList.GetCurSel());
      m_mapCurrent.Remove(cmd);
      //
      m_ctrlCurrent.SetHotKey(0, 0);
      m_ctrlNew.SetHotKey(0, 0);
      m_ctrlNew.SetFocus();
      //
      _UpdateButtons();
      SetModified(TRUE);
      return 0;
   }

   LRESULT OnSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      m_ctrlNew.SetHotKey(0, 0);
      _UpdateButtons();
      return 0;
   }
   LRESULT OnItemChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      _UpdateButtons();
      return 0;
   }

   // Ownerdrawn listbox

   void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
   {
      CClientDC dc(m_ctrlList);
      TEXTMETRIC tm;
      HFONT hOldFont = dc.SelectFont(lpMeasureItemStruct->itemData == 0 ? m_fontBold : m_fontNormal);
      dc.GetTextMetrics(&tm);
      dc.SelectFont(hOldFont);
      if( lpMeasureItemStruct->itemData == 0 ) tm.tmHeight += 4;
      lpMeasureItemStruct->itemHeight = tm.tmHeight;
   }
   void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
   {
      CDCHandle dc = lpDrawItemStruct->hDC;
      RECT rc = lpDrawItemStruct->rcItem;
      TCHAR szText[128] = { 0 };
      m_ctrlList.GetText(lpDrawItemStruct->itemID, szText);
      COLORREF clrMenu = ::GetSysColor(COLOR_MENU);
      COLORREF clrText = ::GetSysColor(COLOR_MENUTEXT);
      COLORREF clrBack;
      int cxOffset = 3;
      if( lpDrawItemStruct->itemData == 0 ) {
         clrBack = BlendRGB(clrMenu, RGB(0,0,0), 6);
      }
      else {
         clrBack = BlendRGB(clrMenu, RGB(255,255,255), 10);
         cxOffset += 10;
      }
      if( lpDrawItemStruct->itemState & ODS_SELECTED ) {
         clrText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
         clrBack = ::GetSysColor(COLOR_HIGHLIGHT);
      }
      dc.FillSolidRect(&rc, clrBack);
      rc.left += cxOffset;
      HFONT hOldFont = dc.SelectFont(lpDrawItemStruct->itemData == 0 ? m_fontBold : m_fontNormal);
      dc.SetBkMode(TRANSPARENT);
      dc.SetTextColor(clrText);
      dc.DrawText(szText, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
      dc.SelectFont(hOldFont);
   }

   // Subclassed control message handlers

   LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlNew.SetFocus();
      return 0;
   }

   // Implementation

   void _UpdateButtons()
   {
      int cmd = m_ctrlList.GetCurSel();
      if( cmd != LB_ERR ) cmd = (int) m_ctrlList.GetItemData(cmd);
      // Clear hotkeys
      m_ctrlDefault.SetHotKey(0, 0);
      m_ctrlCurrent.SetHotKey(0, 0);
      // Look up currently assigned shortcut
      int posCurrent = m_mapCurrent.FindKey((WORD&)cmd);
      if( posCurrent >= 0 ) {
         ACCEL accel = m_mapCurrent.GetValueAt(posCurrent);
         WORD wKey = _GetHotkeyFromAccel(accel);
         m_ctrlCurrent.SetHotKey(LOBYTE(wKey), HIBYTE(wKey));
      }
      // Look up default shortcut
      int posDefault = m_mapDefault.FindKey((WORD&)cmd);
      if( posDefault >= 0 ) {
         ACCEL accel = m_mapDefault.GetValueAt(posDefault);
         WORD wKey = _GetHotkeyFromAccel(accel);
         m_ctrlDefault.SetHotKey(LOBYTE(wKey), HIBYTE(wKey));
      }
      // Enable/disable controls
      m_ctrlDefault.EnableWindow(cmd != 0);
      m_ctrlCurrent.EnableWindow(cmd != 0);
      m_ctrlNew.EnableWindow(cmd != 0);
      ::EnableWindow(GetDlgItem(IDC_ASSIGN), cmd != 0 && m_ctrlNew.GetHotKey() != 0 && m_mapCurrent.GetSize() < MAX_ACCELS);
      ::EnableWindow(GetDlgItem(IDC_REMOVE), cmd != 0 && m_ctrlCurrent.GetHotKey() != 0);
   }

   bool _LoadTable(ACCELMAP& map, HACCEL hAccel) const
   {
      map.RemoveAll();
      if( hAccel == NULL ) return false;
      ACCEL aAccel[MAX_ACCELS];
      int nCount = ::CopyAcceleratorTable(hAccel, aAccel, sizeof(aAccel)/sizeof(ACCEL));
      ATLASSERT(nCount<sizeof(aAccel)/sizeof(ACCEL));
      for( int i = 0; i < nCount; i++ ) map.Add(aAccel[i].cmd, aAccel[i]);
      return true;
   }
   HACCEL _BuildAccel(const ACCELMAP& map) const
   {
      ACCEL aAccel[MAX_ACCELS];
      ATLASSERT(map.GetSize()<sizeof(aAccel)/sizeof(ACCEL));
      for( int i = 0; i < map.GetSize(); i++ ) aAccel[i] = map.GetValueAt(i);
      return ::CreateAcceleratorTable(aAccel, map.GetSize());
   }

   WORD _GetHotkeyFromAccel(const ACCEL& accel) const
   {
      WORD wKey = accel.key;
      WORD wFlags = 0;
      if( (accel.fVirt & FALT) != 0 ) wFlags |= HOTKEYF_ALT;
      if( (accel.fVirt & FCONTROL) != 0 ) wFlags |= HOTKEYF_CONTROL;
      if( (accel.fVirt & FSHIFT) != 0 ) wFlags |= HOTKEYF_SHIFT;
      return MAKEWORD(wKey, wFlags);
   }
   ACCEL _GetAccelFromHotKey(WORD wCmd, WORD wKey) const
   {
      ATLASSERT(wCmd!=0);
      ATLASSERT(wKey!=0);
      ACCEL accel = { 0 };
      accel.cmd = wCmd;
      accel.key = LOBYTE(wKey);
      accel.fVirt = FVIRTKEY | FNOINVERT;
      if( (HIBYTE(wKey) & HOTKEYF_ALT) != 0 ) accel.fVirt |= FALT;
      if( (HIBYTE(wKey) & HOTKEYF_CONTROL) != 0 ) accel.fVirt |= FCONTROL;
      if( (HIBYTE(wKey) & HOTKEYF_SHIFT) != 0 ) accel.fVirt |= FSHIFT;      
      return accel;
   }

   void _BuildMenu(HMENU hMenu)
   {
      ATLASSERT(::IsMenu(hMenu));
      CMenuHandle menu = hMenu;
      int nCount = menu.GetMenuItemCount();
      // We're conducting two scans: one for regular items, another
      // for submenus (which we then parse recursively)...
      for( int i = 0; i < 2; i++ ) {
         for( int j = 0; j < nCount; j++ ) {
            // Get menu information
            TCHAR szText[128] = { 0 };
            menu.GetMenuString(j, szText, (sizeof(szText)/sizeof(TCHAR))-1, MF_BYPOSITION);
            UINT uState = menu.GetMenuState(j, MF_BYPOSITION) & 0xFF;     
            UINT nID = menu.GetMenuItemID(j);

            // Filter items
            if( nID == (UINT) -1 ) nID = 0;
            if( szText[0] == '\0' ) continue;
            if( uState & MF_SEPARATOR ) continue;
            if( uState & MF_MENUBREAK ) continue;
            if( nID >= ID_FILE_MRU_FIRST && nID <= ID_FILE_MRU_LAST ) continue;
            if( nID >= ATL_IDM_FIRST_MDICHILD && nID <= ATL_IDM_FIRST_MDICHILD + 15 ) continue;

            if( i == 0 && (uState & MF_POPUP) != 0 ) continue;
            if( i == 1 && (uState & MF_POPUP) == 0 ) continue;

            // Trim text and remove shortcut part
            LPTSTR ps, pd;         
            for( ps = pd = szText; *ps; ps = ::CharNext(ps) ) {
               if( *ps == '&' ) continue;
               if( *ps == '\t' ) break;
               *pd = *ps;
#ifdef _MBCS
               if( ::IsDBCSLeadByte(*ps) ) *(pd + 1) = *(ps + 1);
#endif
               pd = ::CharNext(pd);
            }
            *pd = '\0';

            // Add to list
            int idx = m_ctrlList.AddString(szText);
            m_ctrlList.SetItemData( idx, (LPARAM) nID );
            // Recalc new item-height
            MEASUREITEMSTRUCT mis = { 0 };
            mis.itemData = nID;
            MeasureItem(&mis);
            m_ctrlList.SetItemHeight(idx, mis.itemHeight);

            // Recursively parse submenus
            if( (uState & MF_POPUP) != 0 ) _BuildMenu(menu.GetSubMenu(j));
         }
      }
   }
};


#endif // !defined(AFX_KEYBOARDDLG_H__20030126_01A0_E58B_B164_0080AD509054__INCLUDED_)

