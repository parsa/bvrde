
#include "StdAfx.h"
#include "resource.h"

#include "ScintillaView.h"
#include "SciLexer.h"

#include "Project.h"
#include "Files.h"

#pragma code_seg( "EDITOR" )

#pragma comment(lib, "../GenEdit/Lib/GenEdit.lib")


// Constructor

CScintillaView::CScintillaView() :
   m_ctrlEdit(this, 1),
   m_pProject(NULL),
   m_pView(NULL)
{
}

// Operations

void CScintillaView::OnFinalMessage(HWND /*hWnd*/)
{
   // If it's a file that was dragged into the app, then
   // the view was probably created by CreateViewFromFilename()
   // and thus we own the memory.
   if( m_pProject == NULL ) delete m_pView;
}

BOOL CScintillaView::Init(IProject* pProject, IView* pView, LPCTSTR pstrFilename, LPCTSTR pstrLanguage)
{
   m_pProject = pProject;
   m_pView = pView;
   m_sLanguage = pstrLanguage;
   m_sFilename = pstrFilename;
   return TRUE;
}

BOOL CScintillaView::GetText(LPSTR& pstrText)
{
   TCHAR szBuffer[32] = { 0 };
   _pDevEnv->GetProperty(_T("editors.general.eolMode"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("cr")) == 0 ) m_ctrlEdit.ConvertEOLs(SC_EOL_CR);
   else if( _tcscmp(szBuffer, _T("lf")) == 0 ) m_ctrlEdit.ConvertEOLs(SC_EOL_LF);
   else if( _tcscmp(szBuffer, _T("crlf")) == 0 ) m_ctrlEdit.ConvertEOLs(SC_EOL_CRLF);

   int nLength = m_ctrlEdit.GetTextLength() + 1;
   pstrText = (LPSTR) malloc(nLength);
   if( pstrText == NULL ) {
      ::SetLastError(ERROR_OUTOFMEMORY);
      return FALSE;
   }
   m_ctrlEdit.GetText(nLength, pstrText);
   return TRUE;
}

BOOL CScintillaView::SetText(LPCSTR pstrText)
{
   m_ctrlEdit.SetText(pstrText);
   m_ctrlEdit.EmptyUndoBuffer();
   m_ctrlEdit.SetSavePoint();

   // Adjust the line-number margin width.
   int iWidth = m_ctrlEdit.GetMarginWidthN(0);
   if( iWidth > 0 && m_ctrlEdit.GetLineCount() > 9999 ) m_ctrlEdit.SetMarginWidthN(0, m_ctrlEdit.TextWidth(STYLE_LINENUMBER, "_99999"));  

   ::ZeroMemory(&m_ftCurrent, sizeof(FILETIME));
   SendMessage(WM_SETTINGCHANGE);
   return TRUE;
}

// Message handlers

LRESULT CScintillaView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   // Create Scintilla editor
   m_ctrlEdit.SubclassWindow( Bvrde_CreateScintillaView(m_hWnd, _pDevEnv, m_sFilename, m_sLanguage) );
   ATLASSERT(m_ctrlEdit.IsWindow());
   m_ctrlEdit.ShowWindow(SW_SHOW);
   ::ZeroMemory(&m_ftCurrent, sizeof(FILETIME));
   return 0;
}

LRESULT CScintillaView::OnQueryEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   if( !m_ctrlEdit.GetModify() ) return TRUE;

   TCHAR szBuffer[32] = { 0 };
   _pDevEnv->GetProperty(_T("editors.general.savePrompt"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("true")) == 0 ) {
      if( IDNO == _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_SAVEFILE)), CString(MAKEINTRESOURCE(IDS_CAPTION_QUESTION)), MB_ICONINFORMATION | MB_YESNO) ) return TRUE;
   }
   if( SendMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE, 0)) != 0 ) return FALSE;

   return TRUE;
}

LRESULT CScintillaView::OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   SendMessageToDescendants(WM_SETTINGCHANGE);
   return 0;
}

LRESULT CScintillaView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{   
   CClientRect rcClient = m_hWnd;
   m_ctrlEdit.MoveWindow(&rcClient);
   return 0;
}

LRESULT CScintillaView::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{   
   ::SetFocus(m_ctrlEdit);
   // Check if file changed outside view
   TCHAR szBuffer[32] = { 0 };;
   _pDevEnv->GetProperty(_T("gui.document.detectChange"), szBuffer, 31);
   if( _tcscmp(szBuffer, _T("true")) == 0 ) {
      FILETIME ft = { 0 };
      CFile f;
      if( f.Open(m_sFilename) ) {
         ::GetFileTime(f, NULL, NULL, &ft);
         if( m_ftCurrent.dwLowDateTime == 0 ) m_ftCurrent = ft;
         if( ::CompareFileTime(&m_ftCurrent, &ft) != 0 ) {
            _pDevEnv->GetProperty(_T("gui.document.autoLoad"), szBuffer, 31);           
            if( _tcscmp(szBuffer, _T("true")) == 0 || IDYES == _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_FILECHANGES)), CString(MAKEINTRESOURCE(IDS_CAPTION_QUESTION)), MB_YESNO | MB_ICONQUESTION) ) {
               DWORD dwSize = f.GetSize();
               LPSTR pstr = (LPSTR) malloc(dwSize + 1);
               f.Read(pstr, dwSize);
               pstr[dwSize] = '\0';
               SetText(pstr);
               free(pstr);
            }
         }
         m_ftCurrent = ft;
         f.Close();
      }
   }
   return 0;
}

LRESULT CScintillaView::OnSetEditFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{   
   _pDevEnv->AddIdleListener(this);
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnKillEditFocus(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   _pDevEnv->RemoveIdleListener(this);
   bHandled = FALSE;
   return 0;
}

LRESULT CScintillaView::OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   ATLASSERT(m_pView);
   if( m_pView == NULL ) return 0;

   if( !m_ctrlEdit.GetModify() ) return 0;

   if( !m_pView->Save() ) {
      CString sMsg;
      TCHAR szName[128] = { 0 };
      m_pView->GetName(szName, 127);
      sMsg.Format(IDS_ERR_FILESAVE, szName);
      _pDevEnv->ShowMessageBox(m_hWnd, sMsg, CString(MAKEINTRESOURCE(IDS_CAPTION_WARNING)), MB_ICONEXCLAMATION);
      return 1; // Return ERROR indication
   }

   ::ZeroMemory(&m_ftCurrent, sizeof(FILETIME));

   m_ctrlEdit.EmptyUndoBuffer();
   return 0;
}

// IIdleListener

void CScintillaView::OnIdle(IUpdateUI* pUIBase)
{
   pUIBase->UIEnable(ID_FILE_SAVE, m_ctrlEdit.GetModify());
}

void CScintillaView::OnGetMenuText(UINT /*wID*/, LPTSTR /*pstrText*/, int /*cchMax*/)
{
}
