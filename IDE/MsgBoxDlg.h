#if !defined(AFX_MSGBOXDLG_H__20020324_B925_543D_B582_0080AD509054__INCLUDED_)
#define AFX_MSGBOXDLG_H__20020324_B925_543D_B582_0080AD509054__INCLUDED_

#pragma once


///////////////////////////////////////////////////////////////////////////////
// The MsgBox label 

class CMsgBoxLabelCtrl : public CWindowImpl<CMsgBoxLabelCtrl>
{
public:
   CString m_sTitle;
   CString m_sText;
   HICON m_hIcon;

   CMsgBoxLabelCtrl() : m_hIcon(NULL)
   {
   }

   // Operations

   BOOL SubclassWindow(HWND hWnd)
   {
      ATLASSERT(m_hWnd==NULL);
      ATLASSERT(::IsWindow(hWnd));
      BOOL bRet = CWindowImpl<CMsgBoxLabelCtrl>::SubclassWindow(hWnd);
      if( bRet ) _Init();
      return bRet;
   }

   void SetTitle(LPCTSTR pstr)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      m_sTitle = pstr;
      Invalidate();
   }
   void SetMessage(LPCTSTR pstr)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      m_sText = pstr;
      Invalidate();
   }
   BOOL SetIcon(HICON hIcon)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      m_hIcon = hIcon;
      Invalidate();
      return TRUE;
   }

   // Message map and handlers

   BEGIN_MSG_MAP(CMsgBoxLabelCtrl)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_PAINT, OnPaint)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      _Init();
      return 0;
   }

   LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CPaintDC dc( m_hWnd );
      RECT rc;
      GetClientRect(&rc);

      HFONT hFont = GetFont();
      if( hFont == NULL ) hFont = AtlGetDefaultGuiFont();

      dc.FillRect(&rc, ::GetSysColorBrush(COLOR_WINDOW));
      
      POINT ptIcon = { rc.left + 16, rc.top + 16 };
      dc.DrawIcon(ptIcon, m_hIcon);

      CLogFont lf = hFont;
      lf.SetBold();
      CFont m_fontTitle;
      m_fontTitle.CreateFontIndirect(&lf);

      dc.SetBkMode(TRANSPARENT);
      dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

      HFONT hOldFont = dc.SelectFont( m_fontTitle );
      RECT rcTitle = { rc.left + 60, rc.top + 18, rc.right - 20, rc.bottom };
      dc.DrawText(m_sTitle, -1, &rcTitle, DT_SINGLELINE | DT_LEFT | DT_NOPREFIX);
      dc.SelectFont( hFont );
      RECT rcText = { rc.left + 60, rc.top + 34, rc.right - 20, rc.bottom };
      dc.DrawText(m_sText, -1, &rcText, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS);
      dc.SelectFont( hOldFont );

      RECT rcBorder = { rc.left, rc.bottom-1, rc.right, rc.bottom };
      dc.FillRect(&rcBorder, ::GetSysColorBrush(COLOR_3DSHADOW));

      return 0;
   }

   // Implementation

   void _Init()
   {
   }
};


///////////////////////////////////////////////////////////////////////////////
// The MsgBox dialog

class CMsgBoxDlg : public CDialogImpl<CMsgBoxDlg>
{
public:
   enum { IDD = IDD_MSGBOX };

   IDevEnv* m_pDevEnv;
   CMsgBoxLabelCtrl m_ctrlLabel;
   CContainedWindowT<CButton> m_ctrlOK;
   CContainedWindowT<CButton> m_ctrlCancel;
   CIcon m_Icon;
   CString m_sMessage;
   CString m_sCaption;
   CButton m_ctrlAskAgain;
   UINT m_uType;
   long m_lHash;

   // Message map and handlers

   CMsgBoxDlg(IDevEnv* pDevEnv) :
      m_pDevEnv(pDevEnv),
      m_ctrlOK(this, 1),
      m_ctrlCancel(this, 1)
   {
   }
   ~CMsgBoxDlg()
   {
      if( IsWindow() ) DestroyWindow();
   }

   UINT DoModal(HWND hWnd, LPCTSTR pstrMessage, LPCTSTR pstrCaption, UINT uType)
   {
      ATLASSERT(!::IsBadStringPtr(pstrCaption,-1));
      ATLASSERT(!::IsBadStringPtr(pstrMessage,-1));
      if( IsWindow() ) return IDOK;
      m_sCaption = pstrCaption;
      m_sMessage = pstrMessage;
      m_uType = uType;
      m_lHash = 0L;
      HWND hwndFocus = ::GetFocus();
      if( (m_uType & MB_MODELESS) != 0 ) {
         CDialogImpl<CMsgBoxDlg>::Create(hWnd);
         ShowWindow(SW_SHOW);
         UpdateWindow();
         return IDOK;
      }
      else {
         UINT nRes = CDialogImpl<CMsgBoxDlg>::DoModal(hWnd);
         ::SetFocus(hwndFocus);
         return nRes;
      }
   }

   BEGIN_MSG_MAP(CMsgBoxDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
      COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
   ALT_MSG_MAP(1)
      MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ATLASSERT(m_pDevEnv);

      if( (m_uType & MB_MODELESS) != 0 ) m_pDevEnv->EnableModeless(FALSE);

      SetWindowText(m_sCaption);

      m_ctrlOK.SubclassWindow(GetDlgItem(IDOK));
      m_ctrlCancel.SubclassWindow(GetDlgItem(IDCANCEL));
      m_ctrlAskAgain = GetDlgItem(IDC_DONT_SHOW);

      if( (m_uType & MB_TYPEMASK) == MB_YESNO ) {
         // YES/NO type
         m_ctrlOK.SetWindowText(CString(MAKEINTRESOURCE(IDS_YES)));
         m_ctrlCancel.SetWindowText(CString(MAKEINTRESOURCE(IDS_NO)));
      }
      else if( (m_uType & MB_TYPEMASK) == MB_OKCANCEL ) {
         // OK/CANCEL type
         m_ctrlOK.SetWindowText(CString(MAKEINTRESOURCE(IDS_OK)));
         m_ctrlCancel.SetWindowText(CString(MAKEINTRESOURCE(IDS_CANCEL)));
      }
      else {
         // OK type
         m_ctrlOK.ShowWindow(SW_HIDE);
         m_ctrlCancel.SetWindowText(CString(MAKEINTRESOURCE(IDS_OK)));
      }

      enum { ICON_SIZE = 32 };

      if( !m_Icon.IsNull() ) m_Icon.DestroyIcon();
      switch( m_uType & MB_ICONMASK ) {
      case MB_ICONQUESTION:
         m_Icon.LoadIcon(IDI_MSG_QUESTION, ICON_SIZE, ICON_SIZE);
         break;
      case MB_ICONWARNING:
         m_Icon.LoadIcon(IDI_MSG_WARNING, ICON_SIZE, ICON_SIZE);
         break;
      case MB_ICONERROR:
         m_Icon.LoadIcon(IDI_MSG_ERROR, ICON_SIZE, ICON_SIZE);
         break;
      case MB_ICONINFORMATION:
      default:
         m_Icon.LoadIcon(IDI_MSG_INFORMATION, ICON_SIZE, ICON_SIZE);
         break;
      }

      m_ctrlAskAgain.ShowWindow((m_uType & MB_REMOVABLE) != 0 ? SW_SHOWNOACTIVATE : SW_HIDE);

      // Don't show a message that's already been dismissed
      m_lHash = _CalcTextHash();
      if( _FindTextHash() ) {
         ResizeClient(1, 1); // HACK: Hide window; still flashes dialog
         PostMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0));
         return FALSE;
      }

      // Extract title/description from input text
      CString sTitle;
      CString sDescription;

      CString s = m_sMessage;
      int pos = s.Find('\n');
      if( pos > 0 ) {
         sTitle = s.Left(pos);
         m_sMessage = s.Mid(pos+1);
         pos = m_sMessage.Find('\n');
         if( pos > 0 ) {
            sDescription = m_sMessage.Mid(pos+1);
            m_sMessage = m_sMessage.Left(pos);
         }
      }

      m_ctrlLabel.SubclassWindow(GetDlgItem(IDC_TEXT));
      m_ctrlLabel.SetIcon(m_Icon);
      m_ctrlLabel.SetTitle(sTitle);
      m_ctrlLabel.SetMessage(m_sMessage);
      SetDlgItemText(IDC_DESCRIPTION, sDescription);

      // Make it sound like a Windows MessageBox
      ::MessageBeep(m_uType & MB_ICONMASK);

      CenterWindow(GetParent());

      // Handle default-button change
      if( (m_uType & MB_TYPEMASK) == MB_OK 
          || ((m_ctrlOK.GetStyle() & WS_VISIBLE) != 0 
              && (m_uType & MB_DEFBUTTON2) != 0) ) 
      {
         m_ctrlOK.ModifyStyle(0x0F, BS_PUSHBUTTON);
         m_ctrlCancel.ModifyStyle(0x0F, BS_DEFPUSHBUTTON);
         m_ctrlCancel.SetFocus();
      }
      else 
      {
         m_ctrlOK.SetFocus();
      }

      return FALSE;
   }
   LRESULT OnKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      CWindowText sOK = GetDlgItem(IDOK);
      CWindowText sCancel = GetDlgItem(IDCANCEL);
      if( m_ctrlOK.IsWindowVisible() && wParam == (WPARAM) sOK[0] ) PostMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0), (LPARAM) m_hWnd);
      if( m_ctrlCancel.IsWindowVisible() && wParam == (WPARAM) sCancel[0] ) PostMessage(WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), (LPARAM) m_hWnd);
      if( wParam == (WPARAM) VK_ESCAPE ) PostMessage(WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), (LPARAM) m_hWnd);
      if( wParam == (WPARAM) VK_RETURN ) PostMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0), (LPARAM) m_hWnd);
      bHandled = FALSE;
      return 0;
   }
   LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      // Display once message?
      if( (m_uType & MB_SHOWONCE) != 0 || m_ctrlAskAgain.GetCheck() == BST_CHECKED ) {
         _SaveTextHash();
      }
      // Modeless dialog? Must enable surrounding windows as well.
      if( (m_uType & MB_MODELESS) != 0 ) {
         m_pDevEnv->EnableModeless(TRUE);
         DestroyWindow();
      }
      else {
         // In a YES/NO dialog, we expect a IDYES/IDNO answer...
         if( (m_uType & MB_TYPEMASK) == MB_YESNO ) wID = (wID == IDOK) ? IDYES : IDNO;
         EndDialog(wID);
      }
      return 0;
   }

   // Implementation

   long _CalcTextHash() const
   {
      long lHash = 0;
      for( int i = 0; i < m_sMessage.GetLength(); i++ ) lHash += (lHash << 5) + m_sMessage.GetAt(i);
      return lHash;
   }
   bool _FindTextHash() const
   {
      CRegKey key;
      if( key.Open(HKEY_CURRENT_USER, REG_BVRDE _T("\\Messages"), KEY_READ) != ERROR_SUCCESS ) return false;
      TCHAR szMessages[2048] = { 0 };
      DWORD dwCount = (sizeof(szMessages) / sizeof(TCHAR)) - 1;
      key.QueryValue(szMessages, NULL, &dwCount);
      _tcscat(szMessages, _T(","));
      TCHAR szNumber[32];
      ::wsprintf(szNumber, _T(",%ld,"), m_lHash);
      return _tcsstr(szMessages, szNumber) != NULL;
   }
   bool _SaveTextHash() const
   {
      // Already there?
      if( _FindTextHash() ) return true;
      // Add text hash-key to message list.
      CRegKey key;
      if( key.Create(HKEY_CURRENT_USER, REG_BVRDE _T("\\Messages")) != ERROR_SUCCESS ) return false;
      TCHAR szMessages[2048] = { 0 };
      DWORD dwCount = 2047;
      key.QueryValue(szMessages, NULL, &dwCount);
      TCHAR szNumber[32];
      ::wsprintf(szNumber, _T(",%ld"), m_lHash);
      _tcscat(szMessages, szNumber);
      key.SetValue(szMessages);
      key.Close();
      return true;
   }
};


#endif // !defined(AFX_MSGBOXDLG_H__20020324_B925_543D_B582_0080AD509054__INCLUDED_)
