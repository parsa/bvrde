
#include "StdAfx.h"
#include "resource.h"

#include "Container.h"
#include "Files.h"

#pragma code_seg( "MISC" )

#pragma comment(lib, "../GenEdit/Lib/GenEdit.lib")


void CContainerWindow::Init(IProject* pProject, IView* pView)
{
   m_pProject = pProject;
   m_pView = pView;
}

BOOL CContainerWindow::PreTranslateMessage(MSG* pMsg)
{
   int iCurSel = m_ctrlTab.GetCurSel();
   switch( iCurSel ) {
   case 0:
      if( m_wndPreview.PreTranslateMessage(pMsg) ) return TRUE;
      break;
   case 1:
      break;
   case 2:
      if( m_wndDesign.PreTranslateMessage(pMsg) ) return TRUE;
      break;
   }
   return FALSE;
}

void CContainerWindow::OnIdle(IUpdateUI* pUIBase)
{
   int iCurSel = m_ctrlTab.GetCurSel();
   switch( iCurSel ) {
   case 0:
      m_wndPreview.OnIdle(pUIBase);
      break;
   case 1:
      break;
   case 2:
      m_wndDesign.OnIdle(pUIBase);
      break;
   }
}

void CContainerWindow::SetLanguage(CString& sLanguage)
{
   m_sLanguage = sLanguage;
   m_wndPreview.SetLanguage(m_sLanguage);
   m_wndDesign.SetLanguage(m_sLanguage);
}

void CContainerWindow::SetFilename(CString& sFilename)
{
   //m_wndSource.SetFilename(sFilename);
}

CString CContainerWindow::GetViewText()
{
   BOOL bDummy;
   OnTabChanging(0, NULL, bDummy);
   return m_sText;
}

void CContainerWindow::SetViewText(LPCTSTR pstrText)
{
   // Assign new text to document cache.
   // Will be assigned to view next time we switch tab folder.
   m_sText = pstrText;
   // Switch tab folder
   BOOL bDummy;
   OnTabChange(0, NULL, bDummy);
   int iCurSel = m_ctrlTab.GetCurSel();
   if( iCurSel < 0 ) m_ctrlTab.SetCurSel(0);
   m_wndSource.SetSavePoint();
   m_wndSource.EmptyUndoBuffer();
}

void CContainerWindow::OnFinalMessage(HWND /*hWnd*/)
{
   if( m_pProject == NULL ) delete m_pView;
}

LRESULT CContainerWindow::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pView);

   TCHAR szFilename[MAX_PATH] = { 0 };
   m_pView->GetFileName(szFilename, MAX_PATH);

   m_wndDesign.Create(m_hWnd);
   m_wndSource = Bvrde_CreateScintillaView(m_hWnd, _pDevEnv, szFilename, m_sLanguage);
   m_wndPreview.Create(m_hWnd);

   m_ctrlTab.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TCS_BUTTONS);
   m_ctrlTab.SetExtendedStyle(0, TCS_EX_SELHIGHLIGHT);
   //
   CString s;
   s.LoadString(IDS_PREVIEW);
   m_ctrlTab.InsertItem(0, s);
   //
   s.LoadString(IDS_SOURCE);
   m_ctrlTab.InsertItem(1, s);
   //
   if( m_sLanguage == _T("html") ) {
      s.LoadString(IDS_DESIGN);
      m_ctrlTab.InsertItem(2, s);
   }
   m_ctrlTab.SetCurSel(1);

   return 0;
}

LRESULT CContainerWindow::OnQueryEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   if( m_wndSource.GetModify() ) {
      TCHAR szBuffer[32] = { 0 };
      _pDevEnv->GetProperty(_T("editors.general.savePrompt"), szBuffer, 31);
      if( _tcscmp(szBuffer, _T("true")) == 0 ) {
         if( IDNO == _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_SAVEFILE)), CString(MAKEINTRESOURCE(IDS_CAPTION_QUESTION)), MB_ICONINFORMATION | MB_YESNO) ) return TRUE;
      }
      if( SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE, 0)) != 0 ) return FALSE;
   }
   return TRUE;
}

LRESULT CContainerWindow::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   if( !m_hWndClient.IsWindow() ) return 0;
   m_hWndClient.SetFocus();
   return 0;
}

LRESULT CContainerWindow::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   if( !m_ctrlTab.IsWindow() ) return 0;
   if( !m_hWndClient.IsWindow() ) return 0;
   // Place the tab control and client window
   RECT rc;
   GetClientRect(&rc);
   int cy = ::GetSystemMetrics(SM_CYMENU) + 6;
   m_hWndClient.MoveWindow(rc.left, rc.top, rc.right, rc.bottom - cy);
   m_ctrlTab.MoveWindow(rc.left, rc.bottom - cy, rc.right - rc.left, cy);
   return 0;
}

LRESULT CContainerWindow::OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   return 1; // No need
}

LRESULT CContainerWindow::OnSendMsgToAll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
   if( m_wndSource.IsWindow() ) m_wndSource.SendMessage(uMsg, wParam, lParam);
   if( m_wndDesign.IsWindow() ) m_wndDesign.SendMessage(uMsg, wParam, lParam);
   if( m_wndPreview.IsWindow() ) m_wndPreview.SendMessage(uMsg, wParam, lParam);
   return 1; // No need
}

LRESULT CContainerWindow::OnSendMsgToClient(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
   if( ::IsWindow(m_hWndClient) ) m_hWndClient.SendMessage(uMsg, wParam, lParam);
   return 1; // No need
}

LRESULT CContainerWindow::OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   CWaitCursor cursor;  
   return m_pView->Save() ? 0 : 1;
}

LRESULT CContainerWindow::OnTabChanging(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   int iCurSel = m_ctrlTab.GetCurSel();
   switch( iCurSel ) {
   case 0:
      // Does not change the text!
      break;
   case 1:
      {
         int nLen = m_wndSource.GetTextLength();
         PCHAR pstrText = (PCHAR) malloc(nLen + 1);
         if( pstrText == NULL ) return 0;
         m_wndSource.GetText(nLen + 1, pstrText);
         pstrText[nLen] = '\0';
         m_sText = pstrText;
         free(pstrText);
         //m_sText = m_wndSource.GetViewText();
      }
      break;
   case 2:
      m_sText = m_wndDesign.GetViewText();
      break;
   }
   return 0;
}

LRESULT CContainerWindow::OnTabChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
   if( m_hWndClient.IsWindow() ) m_hWndClient.ShowWindow(SW_HIDE);
   int iCurSel = m_ctrlTab.GetCurSel();
   switch( iCurSel ) {
   case -1:
      {
         m_hWndClient = NULL;
         return 0;
      }
   case 0:
      {
         m_hWndClient = m_wndPreview;
         m_wndPreview.SetViewText(m_sText);
      }
      break;
   case 1:
      {
         m_hWndClient = m_wndSource;

         m_wndSource.BeginUndoAction();

         int nLen = m_sText.GetLength();
         LPSTR pstrData = (LPSTR) malloc(nLen + 1);
         if( pstrData == NULL ) return 0;
         AtlW2AHelper(pstrData, m_sText, nLen + 1);
         pstrData[nLen] = '\0';
         m_wndSource.SetText(pstrData);
         m_wndSource.SendMessage(WM_SETTINGCHANGE);
  
         // TODO: Compare previous contents with new contents (might have
         //       been modified in design-editor) and determine if we should
         //       reset/repaint the text-editor!

         m_wndSource.EndUndoAction();
      }
      break;
   case 2:
      {
         m_hWndClient = m_wndDesign;
         m_wndDesign.SetViewText(m_sText);
      }
      break;
   }
   SendMessage(WM_SIZE);
   m_hWndClient.ShowWindow(SW_SHOW);
   m_hWndClient.SetFocus();
   return 0;
}
