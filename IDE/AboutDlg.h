#if !defined(AFX_ABOUTDLG_H__840040B2_B4E9_44EA_BEE6_7BB69F293FEF__INCLUDED_)
#define AFX_ABOUTDLG_H__840040B2_B4E9_44EA_BEE6_7BB69F293FEF__INCLUDED_

#include "atldib.h"
#include "MenuShadows.h"

#include <winver.h>
#pragma comment(lib, "version.lib")


class CAboutDlg : 
   public CDialogImpl<CAboutDlg>,
   public CDialogShadows<CAboutDlg>
{
public:
   enum { IDD = IDD_ABOUTBOX };

   enum { SCROLL_TIMERID = 48 };
   enum { SCROLL_SPEED = 100 };

   CFont m_SmallFont;
   TEXTMETRIC m_tm;
   CSimpleArray<CString> m_aLines;
   int m_iScrollPos;
   RECT m_rcWindow;
   CRgn m_rgn;

   BEGIN_MSG_MAP(CAboutDlg)
      CHAIN_MSG_MAP( CDialogShadows<CAboutDlg> )
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_TIMER, OnTimer)
      MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
      COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
      COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      // Resize dialog to size of bitmap (prevent scaling from Large Font
      // settings or other oddities)
      CWindowRect rcWin = GetDlgItem(IDC_SPLASH);
      ResizeClient(rcWin.right - rcWin.left, rcWin.bottom - rcWin.top, FALSE);
      // Center to parent
      CenterWindow(GetParent());
      // Create a really small font
      CLogFont lf = AtlGetDefaultGuiFont();
      _tcscpy(lf.lfFaceName, _T("Small Fonts"));
      lf.lfHeight = -8;
      m_SmallFont.CreateFontIndirect(&lf);
      CClientDC dc = m_hWnd;
      HFONT hOldFont = dc.SelectFont(m_SmallFont);
      dc.GetTextMetrics(&m_tm);
      dc.SelectFont(hOldFont);
      // Get the product version
      TCHAR szModule[MAX_PATH] = { 0 };
      ::GetModuleFileName(NULL, szModule, MAX_PATH);
      CString sVersion = _GetFileVersion(szModule);
      // Split the text into lines
      CString sText(MAKEINTRESOURCE(IDS_CREDITS));
      sText.Replace(_T("$VER$"), sVersion);
      int iPos;
      while( (iPos = sText.Find(_T("\n"))) >= 0 ) {
         CString s = sText.Left(iPos);
         m_aLines.Add(s);
         sText = sText.Mid(iPos + 1);
      }
      // Get ready to scroll
      m_iScrollPos = 0;
      ::SetRect(&m_rcWindow, 260, 124, 260 + 120, 180);
      m_rgn.CreateRectRgnIndirect(&m_rcWindow);
      // Start the show
      SetTimer(SCROLL_TIMERID, SCROLL_SPEED);
      return TRUE;
   }
   LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      KillTimer(SCROLL_TIMERID);
      bHandled = FALSE;
      return TRUE;
   }
   LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      EndDialog(IDOK);
      return TRUE;
   }
   LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( wParam != SCROLL_TIMERID ) {
         bHandled = FALSE;
         return 0;
      }
      const LONG TEXT_GAP = 2;
      const INT SCROLL_SPEED = 1;
      CClientDC dc = m_hWnd;
      HFONT hOldFont = dc.SelectFont(m_SmallFont);
      dc.SelectClipRgn(m_rgn);
      dc.SetBkMode(OPAQUE);
      dc.SetTextColor(RGB(0,0,0));
      dc.SetBkColor(RGB(255,255,255));
      int cy = m_tm.tmHeight + TEXT_GAP;
      RECT rcText = m_rcWindow;
      rcText.top -= m_iScrollPos;
      rcText.bottom = rcText.top + cy;
      for( int i = 0; i < m_aLines.GetSize(); i++ ) {
         // Speedy erase
         dc.ExtTextOut(0, 0, ETO_OPAQUE, &rcText, NULL, 0, NULL);
         // Print text
         dc.DrawText(m_aLines[i], -1, &rcText, DT_SINGLELINE | DT_CENTER | DT_NOCLIP | DT_NOPREFIX);
         // Next line
         ::OffsetRect(&rcText, 0, cy);
      }
      dc.SelectFont(hOldFont);
      dc.SelectClipRgn(NULL);
      m_iScrollPos += SCROLL_SPEED;
      if( m_iScrollPos / (m_tm.tmHeight + TEXT_GAP) > m_aLines.GetSize() ) m_iScrollPos = 0;
      return 0;
   }
   LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   CString _GetFileVersion(LPCTSTR pstrFilename) const
   {
      DWORD dwHandle = 0;
      DWORD cchVer = ::GetFileVersionInfoSize(pstrFilename, &dwHandle);
      if( cchVer == 0 ) return _T("");
      LPVOID pVer = _alloca(cchVer);
      BOOL bRet = ::GetFileVersionInfo(pstrFilename, dwHandle, cchVer, pVer);
      if( !bRet ) return _T("");
      UINT uLen = 0;
      LPVOID pBuf = NULL;
      bRet = ::VerQueryValue(pVer, _T("\\"), &pBuf, &uLen);
      if( !bRet ) return _T("");
      VS_FIXEDFILEINFO vsfi = { 0 };
      memcpy(&vsfi, pBuf, sizeof(VS_FIXEDFILEINFO));
      CString sResult;
      sResult.Format(_T("%ld.%ld"), HIWORD(vsfi.dwProductVersionMS), LOWORD(vsfi.dwProductVersionMS));
      if( LOWORD(vsfi.dwProductVersionLS) > 0 ) sResult += (TCHAR) (LOWORD(vsfi.dwProductVersionLS) + 'a');
      return sResult;
   }
};


#endif // !defined(AFX_ABOUTDLG_H__840040B2_B4E9_44EA_BEE6_7BB69F293FEF__INCLUDED_)
