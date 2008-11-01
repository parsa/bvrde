// Dialogs.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainFrm.h"
#include "Macro.h"

#if _MSC_VER < 1300
   #pragma code_seg( "DIALOGS" )
#endif

#include "AboutDlg.h"
#include "MacrosDlg.h"
#include "WindowsDlg.h"
#include "OptionsDlg.h"
#include "CustomizeDlg.h"
#include "AddinManagerDlg.h"
#include "ExternalToolsDlg.h"
#include "ChooseSolutionDlg.h"



LRESULT CMainFrame::OnFilePrintSetup(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   PAGESETUPDLG pdlg = 
   {
      sizeof(PAGESETUPDLG), 0, 0, 0, 0, {0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0, 0, 0, 0
   };
   pdlg.hwndOwner = m_hWnd;
   pdlg.hInstance = _Module.GetModuleInstance();
   pdlg.hDevMode = m_hDevMode;
   pdlg.hDevNames = m_hDevNames;
   pdlg.rtMargin = m_rcPageMargins;
   if( !::PageSetupDlg(&pdlg) ) return 0;
   m_hDevMode = pdlg.hDevMode;
   m_hDevNames = pdlg.hDevNames;
   m_rcPageMargins = pdlg.rtMargin;
   return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CAboutDlg dlg;
   dlg.DoModal();
   return 0;
}

LRESULT CMainFrame::OnToolsOptions(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CString sFilename = GetSettingsFilename();
   CXmlSerializer arc;
   if( !arc.Open(_T("Settings"), sFilename) ) return 0;
   COptionsDlg dlg(this, &arc, NULL);
   UINT nRes = dlg.DoModal();
   CWaitCursor cursor;
   CLockWindowUpdate lock = m_hWnd;
   // Re-open archive and load settings...
   arc.Close();
   if( !arc.Open(_T("Settings"), sFilename) ) return 0;
   if( !_LoadSettings(arc) ) return 0;
   arc.Close();
   // Need to show MDI container/CoolTabs?
   TCHAR szBuffer[64] = { 0 };
   GetProperty(_T("gui.main.client"), szBuffer, 63);
   m_MDIContainer.SetVisible(_tcscmp(szBuffer, _T("mdi")) != 0);
   // Update view layout
   UpdateLayout();
   // Allow all views to repaint/reformat
   CWindow wndMDIClient = m_hWndMDIClient;
   wndMDIClient.SendMessageToDescendants(WM_SETTINGCHANGE);
   arc.Close();
   return 0;
}

LRESULT CMainFrame::OnToolsExternal(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CExternalToolsDlg dlg(this);
   UINT nRes = dlg.DoModal();
   if( nRes == IDOK ) {
      // Fake a project change to reinitialize menus
      PostMessage(WM_APP_VIEWCHANGE);
   }
   return nRes == IDOK;
}

LRESULT CMainFrame::OnToolsAddinManager(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CAddinManagerDlg dlg(this);
   UINT nRes = dlg.DoModal();
   return nRes == IDOK;
}

LRESULT CMainFrame::OnToolsCustomize(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CCustomizeDlg dlg;
   dlg.Init(this);
   UINT nRes = dlg.DoModal();
   if( nRes == IDOK ) UIClear();
   return nRes == IDOK;
}

LRESULT CMainFrame::OnMacroManager(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CMacrosDlg dlg;
   UINT nRes = dlg.DoModal();
   if( nRes != IDOK ) return 0;
   CString sFilename;
   CString sFunction;
   dlg.GetSelection(sFilename, sFunction);
   CComObjectGlobal<CMacro> macro;
   macro.Init(this, &m_Dispatch);
   macro.RunMacroFromFile(sFilename, sFunction);
   return 0;
}

LRESULT CMainFrame::OnWindowView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CWindowsDlg dlg(m_hWnd, m_hWndMDIClient);
   dlg.DoModal();
   return 0;
}

