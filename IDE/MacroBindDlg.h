#if !defined(AFX_MACROBINDDLG_H__20040515_BB37_18C3_F7BB_0080AD509054__INCLUDED_)
#define AFX_MACROBINDDLG_H__20040515_BB37_18C3_F7BB_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MacrosDlg.h"


class CMacroBindDlg : 
   public CPropertyPageImpl<CMacroBindDlg>,
   public COwnerDraw<CMacroBindDlg>
{
public:
   enum { IDD = IDD_MACROBIND };

   // We only reserve 15 resource-identifiers for this!
   // TODO: Remove hard-coded limit in resource.h
   enum { MAX_ACCELS = 15 };

   typedef struct tagMACROACCEL : ACCEL
   {
      TCHAR szFilename[MAX_PATH];
      TCHAR szFunction[200];
   } MACROACCEL;

   typedef CSimpleMap<WORD, MACROACCEL> ACCELMAP;

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


   CMacroBindDlg() :
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
   }

   // Message map and handlers

   BEGIN_MSG_MAP(CMacroBindDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDC_ASSIGN, OnAssign)
      COMMAND_ID_HANDLER(IDC_REMOVE, OnRemove)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnItemChange)
      COMMAND_HANDLER(IDC_LIST, LBN_SELCHANGE, OnSelChange)
      CHAIN_MSG_MAP( COwnerDraw<CMacroBindDlg> )
      CHAIN_MSG_MAP( CPropertyPageImpl<CMacroBindDlg> )
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

      _BuildList();

      _LoadTable(m_mapCurrent, m_ctrlList);

      m_ctrlList.SetCurSel(0);
      _UpdateButtons();

      CenterWindow(GetParent());

      return TRUE;
   }
   int OnApply()
   {
      if( *m_phAccel != NULL ) ::DestroyAcceleratorTable(*m_phAccel);
      *m_phAccel = _BuildAccel(m_mapCurrent);

      CString sFilename;
      sFilename.Format(_T("%sBVRDE.XML"), CModulePath());

      CXmlSerializer arc;
      if( !arc.Open(_T("Settings"), sFilename) ) return PSNRET_INVALID_NOCHANGEPAGE;

      arc.Delete(_T("MacroMappings"));
      if( !arc.WriteGroupBegin(_T("MacroMappings")) ) return PSNRET_INVALID_NOCHANGEPAGE;
      for( int i = 0; i < m_mapCurrent.GetSize(); i++ ) {
         // Write settings to config file
         MACROACCEL accel = m_mapCurrent.GetValueAt(i);
         arc.WriteItem(_T("Macro"));
         arc.Write(_T("cmd"), (long) accel.cmd);
         arc.Write(_T("key"), (long) accel.key);
         arc.Write(_T("flags"), (long) accel.fVirt);
         arc.Write(_T("filename"), accel.szFilename);
         arc.Write(_T("function"), accel.szFunction);
      }
      arc.WriteGroupEnd();

      arc.Save();
      arc.Close();
      return PSNRET_NOERROR;
   }

   LRESULT OnAssign(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      // Find free command-identifier
      int iCurSel = m_ctrlList.GetCurSel();
      MACROACCEL macro = _FindItem(iCurSel);
      if( macro.cmd == 0 ) {
         for( WORD wCmd = ID_MACROS_KEY1; wCmd <= ID_MACROS_KEY15; wCmd++ ) {
            if( m_mapCurrent.FindKey(wCmd) < 0 ) {
               macro.cmd = wCmd;
               m_ctrlList.GetText(iCurSel, macro.szFunction);
               m_ctrlList.GetText(m_ctrlList.GetItemData(iCurSel) - 1, macro.szFilename);
               break;
            }
         }
      }
      if( macro.cmd == 0 ) return 0;
      // 
      WORD wKey = (WORD) m_ctrlNew.GetHotKey();
      (ACCEL&) macro = _GetAccelFromHotKey(macro.cmd, wKey);
      if( !m_mapCurrent.SetAt(macro.cmd, macro) ) m_mapCurrent.Add(macro.cmd, macro);
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
      MACROACCEL& macro = _FindItem(m_ctrlList.GetCurSel());
      m_mapCurrent.Remove(macro.cmd);
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
      int iCurSel = m_ctrlList.GetCurSel();
      LPARAM lParam = m_ctrlList.GetItemData(iCurSel);
      MACROACCEL& macro = _FindItem(iCurSel);
      // Clear hotkeys
      m_ctrlDefault.SetHotKey(0, 0);
      m_ctrlCurrent.SetHotKey(0, 0);
      // Look up currently assigned shortcut
      if( macro.cmd >= 0 ) {
         WORD wKey = _GetHotkeyFromAccel(macro);
         m_ctrlCurrent.SetHotKey(LOBYTE(wKey), HIBYTE(wKey));
      }
      // Look up default shortcut
      int posDefault = m_mapDefault.FindKey((WORD&)macro.cmd);
      if( posDefault >= 0 ) {
         MACROACCEL accel = m_mapDefault.GetValueAt(posDefault);
         WORD wKey = _GetHotkeyFromAccel(accel);
         m_ctrlDefault.SetHotKey(LOBYTE(wKey), HIBYTE(wKey));
      }
      // Enable/disable controls
      m_ctrlDefault.EnableWindow(lParam != 0);
      m_ctrlCurrent.EnableWindow(lParam != 0);
      m_ctrlNew.EnableWindow(lParam != 0);
      ::EnableWindow(GetDlgItem(IDC_ASSIGN), lParam != 0 && m_ctrlNew.GetHotKey() != 0 && m_mapCurrent.GetSize() < MAX_ACCELS);
      ::EnableWindow(GetDlgItem(IDC_REMOVE), lParam != 0 && m_ctrlCurrent.GetHotKey() != 0);
   }

   bool _LoadTable(ACCELMAP& map, CListBox& /*ctrlList*/) const
   {
      map.RemoveAll();
      CString sFilename;
      sFilename.Format(_T("%sBVRDE.XML"), CModulePath());
      CXmlSerializer arc;
      if( !arc.Open(_T("Settings"), sFilename) ) return 0;
      if( !arc.ReadGroupBegin(_T("MacroMappings")) ) return 0;
      while( arc.ReadGroupBegin(_T("Macro")) ) {
         MACROACCEL macro;
         ::ZeroMemory(&macro, sizeof(macro));
         long lVal;
         arc.Read(_T("cmd"), lVal); macro.cmd = (WORD) lVal;
         arc.Read(_T("key"), lVal); macro.key = (WORD) lVal;
         arc.Read(_T("flags"), lVal); macro.fVirt = (BYTE) lVal;
         arc.Read(_T("filename"), macro.szFilename, MAX_PATH);
         arc.Read(_T("function"), macro.szFunction, 199);
         map.Add(macro.cmd, macro);
         arc.ReadGroupEnd();
      }
      arc.ReadGroupEnd();
      arc.Close();
      return true;
   }
   bool _LoadTable(ACCELMAP& map, HACCEL hAccel) const
   {
      map.RemoveAll();
      if( hAccel == NULL ) return false;
      MACROACCEL aAccel[200];
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

   MACROACCEL _FindItem(int iIndex) const
   {
      MACROACCEL macro;
      ::ZeroMemory(&macro, sizeof(macro));
      if( iIndex < 0 ) return macro;
      CString sText;
      m_ctrlList.GetText(iIndex, sText);
      CString sParent;
      m_ctrlList.GetText(m_ctrlList.GetItemData(iIndex) - 1, sParent);
      for( int i = 0; i < m_mapCurrent.GetSize(); i++ ) {
         MACROACCEL& item = m_mapCurrent.GetValueAt(i);
         if( sText == item.szFunction && sParent == item.szFilename ) {
            return item;
         }
      }
      return macro;
   }
   WORD _GetHotkeyFromAccel(const ACCEL& accel) const
   {
      WORD wKey = accel.key;
      WORD wFlags = 0;
      if( accel.fVirt & FALT ) wFlags |= HOTKEYF_ALT;
      if( accel.fVirt & FCONTROL ) wFlags |= HOTKEYF_CONTROL;
      if( accel.fVirt & FSHIFT ) wFlags |= HOTKEYF_SHIFT;
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
      if( HIBYTE(wKey) & HOTKEYF_ALT ) accel.fVirt |= FALT;
      if( HIBYTE(wKey) & HOTKEYF_CONTROL ) accel.fVirt |= FCONTROL;
      if( HIBYTE(wKey) & HOTKEYF_SHIFT ) accel.fVirt |= FSHIFT;      
      return accel;
   }

   void _BuildList()
   {
      CString sPattern;
      sPattern.Format(_T("%s*.vbs"), CModulePath());
      CFindFile ff;
      for( BOOL bRes = ff.FindFile(sPattern); bRes; bRes = ff.FindNextFile() ) {
         if( ff.IsDots() ) continue;
         if( ff.IsDirectory() ) continue;
         CString sDescription;
         CSimpleArray<CString> aNames;
         CSimpleArray<CString> aDescriptions;
         CMacrosDlg::GetFileInfo(ff.GetFilePath(), sDescription, aNames, aDescriptions);
         if( aNames.GetSize() == 0 ) continue;

         int iItem = m_ctrlList.AddString(ff.GetFileTitle());
         m_ctrlList.SetItemData(iItem, 0);
         // Recalc new item-height
         MEASUREITEMSTRUCT mis = { 0 };
         mis.itemData = 0;
         MeasureItem(&mis);
         m_ctrlList.SetItemHeight(iItem, mis.itemHeight);

         for( int i = 0; i < aNames.GetSize(); i++ ) {
            int iFile = m_ctrlList.AddString(aNames[i]);
            m_ctrlList.SetItemData(iFile, (LPARAM) iItem + 1);
            // Recalc new item-height
            MEASUREITEMSTRUCT mis = { 0 };
            mis.itemData = iItem + 1;
            MeasureItem(&mis);
            m_ctrlList.SetItemHeight(iFile, mis.itemHeight);
         }
      }
   }
};


#endif // !defined(AFX_MACROBINDDLG_H__20040515_BB37_18C3_F7BB_0080AD509054__INCLUDED_)

