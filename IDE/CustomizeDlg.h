#if !defined(AFX_CUSTOMIZEDLG_H__20030525_41C8_C513_32CC_0080AD509054__INCLUDED_)
#define AFX_CUSTOMIZEDLG_H__20030525_41C8_C513_32CC_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MainFrm.h"
#include "XmlSerializer.h"

#include "DlgTabCtrl.h"

#define WM_USER_MODIFIED WM_USER + 434

#include "LayoutDlg.h"
#include "KeyboardDlg.h"
#include "MacroBindDlg.h"


class CCustomizeDlg :
   public CDialogImpl<CCustomizeDlg>
{
public:
   enum { IDD = IDD_CUSTOMIZE };

   CDialogTabCtrl m_ctrlTab;
   CLayoutDlg m_viewLayout;
   CKeyboardDlg m_viewKeyboard;
   CMacroBindDlg m_viewMacros;

   CMainFrame* m_pMainFrame;

   CCustomizeDlg(CMainFrame* pMainFrame) : 
      m_pMainFrame(pMainFrame)
   {     
   }

   BEGIN_MSG_MAP(CCustomizeDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnCreate)
      MESSAGE_HANDLER(WM_USER_MODIFIED, OnModified)      
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CenterWindow(GetParent());

      m_ctrlTab.SubclassWindow(GetDlgItem(IDC_TAB));

      HMENU hMenu = m_pMainFrame->GetMenuHandle(IDE_HWND_MAIN);
      HACCEL hDefaultAccel = m_pMainFrame->m_hAccel;
      HACCEL* phUserAccel = &m_pMainFrame->m_UserAccel.m_hAccel;
      HACCEL* phMacroAccel = &m_pMainFrame->m_MacroAccel.m_hAccel;

      m_viewLayout.Init(m_hWnd, m_pMainFrame);
      m_viewLayout.Create(m_ctrlTab, rcDefault);
      ATLASSERT(::IsWindow(m_viewLayout));

      m_viewKeyboard.Init(m_hWnd, hMenu, hDefaultAccel, phUserAccel);
      m_viewKeyboard.Create(m_ctrlTab, rcDefault);
      ATLASSERT(::IsWindow(m_viewKeyboard));

      m_viewMacros.Init(m_hWnd, hMenu, hDefaultAccel, phMacroAccel);
      m_viewMacros.Create(m_ctrlTab, rcDefault);
      ATLASSERT(::IsWindow(m_viewMacros));

      CString s;
      TCITEM item = { 0 };
      item.mask = TCIF_TEXT;
      s.LoadString(IDS_TOOLBARS);
      item.pszText = (LPTSTR) (LPCTSTR) s;
      m_ctrlTab.InsertItem(0, &item, m_viewLayout);
      s.LoadString(IDS_KEYBOARD);
      item.pszText = (LPTSTR) (LPCTSTR) s;
      m_ctrlTab.InsertItem(1, &item, m_viewKeyboard);
      s.LoadString(IDS_MACROBIND);
      item.pszText = (LPTSTR) (LPCTSTR) s;
      m_ctrlTab.InsertItem(2, &item, m_viewMacros);

      m_ctrlTab.SetCurSel(0);

      return TRUE;
   }
   LRESULT OnModified(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ::EnableWindow(GetDlgItem(IDOK), TRUE);
      return 0;
   }

   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      BOOL bDummy;
      OnApply(0, 0, NULL, bDummy);
      EndDialog(wID);
      return 0;
   }
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnApply(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      // Simulate Apply by calling it directly on the view
      CWaitCursor cursor;
      BOOL bDummy;
      m_viewLayout.OnApply(0, 0, NULL, bDummy);
      m_viewKeyboard.OnApply(0, 0, NULL, bDummy);
      m_viewMacros.OnApply(0, 0, NULL, bDummy);
      return 0;
   }
};


#endif // !defined(AFX_CUSTOMIZEDLG_H__20030525_41C8_C513_32CC_0080AD509054__INCLUDED_)

