#if !defined(AFX_SPLASHDLG_H__20020220_461C_AE84_5328_0080AD509054__INCLUDED_)
#define AFX_SPLASHDLG_H__20020220_461C_AE84_5328_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CSplashWindow : public CWindowImpl<CSplashWindow>
{
public:
   CBitmap m_bm;
   HANDLE m_hThread;
   bool m_bCreated;

   enum { TIMERID = 12 };

   enum { WM_USER_REMOVESPLASH = WM_USER + 123 };

   CSplashWindow()
   {
      m_hThread = NULL;
      m_bCreated = false;
      m_bm.LoadBitmap(IDB_SPLASH);
      ATLASSERT(!m_bm.IsNull());
   }
   ~CSplashWindow()
   {
      // Safety!
      while( IsWindow() ) ::Sleep(500L);
   }

   // Operations
   
   BOOL ShowSplash()
   {
      ATLTRACE("CSplash::ShowSplash()\n");
      DWORD dwThreadID;
      m_hThread = ::CreateThread(NULL, 0, RunThread, this, 0, &dwThreadID);
      if( m_hThread == NULL ) {
         AtlMessageBox(NULL, _T("ERROR: Cannot creating thread!!!"));
         return FALSE;
      }
      ::WaitForInputIdle(::GetCurrentProcess(), 300L);
      return TRUE;
   }
   void RemoveSplash(UINT dwDelay = 1000L)
   {
      ATLTRACE("CSplash::RemoveSplash()\n");
      if( m_hThread == NULL ) return;
      DWORD dwCode = 0;
      ::GetExitCodeThread(m_hThread, &dwCode);
      if( dwCode == STILL_ACTIVE ) {
         // Hvis vinduet ikke er oprettet endnu, så er vi
         // nødt til at vente lidt på den...
         while( !m_bCreated ) ::Sleep(500L);
         // Ok, nu kan vi lukke vinduet.
         PostMessage(WM_USER_REMOVESPLASH, 0, dwDelay);
      }
      ::CloseHandle(m_hThread);
      m_hThread = NULL;
   }

   // Thread proc

   static DWORD WINAPI RunThread(LPVOID lpData)
   {
      CSplashWindow* pThis = (CSplashWindow*) lpData;
      ATLASSERT(pThis);
      HWND hWnd = pThis->m_hWnd;

      CMessageLoop theLoop;
      _Module.AddMessageLoop(&theLoop);

      CWindow wndSplash = pThis->Create(::GetDesktopWindow(), CWindow::rcDefault, NULL, WS_POPUP, WS_EX_TOOLWINDOW);
      ATLASSERT(wndSplash);
      if( !wndSplash.IsWindow() ) return 0;
      ::SetForegroundWindow(wndSplash);   // Win95 needs this

      int nRet = theLoop.Run();

      _Module.RemoveMessageLoop();
      return nRet;
   }

   BEGIN_MSG_MAP(CSplashWindow)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_PAINT, OnPaint)
      MESSAGE_HANDLER(WM_PRINTCLIENT, OnPaint)
      MESSAGE_HANDLER(WM_TIMER, OnTimer)
      MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
      MESSAGE_HANDLER(WM_USER_REMOVESPLASH, OnRemoveSplash)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ATLTRACE("CSplash::OnCreate()\n");
      SIZE size;
      m_bm.GetSize(size);
      SetWindowPos(HWND_TOPMOST, 0, 0, size.cx, size.cy, SWP_NOMOVE);
      CenterWindow();
      ::SetForegroundWindow(m_hWnd);
      
      CAnimateWindow wndAnimate = m_hWnd;
      if( !wndAnimate.AnimateWindow(200) ) ::ShowWindow(m_hWnd, SW_SHOW);

      // Marker at dialogen endelig er oprettet!
      m_bCreated = true;
      return 0;
   }
   LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ATLTRACE("CSplash::OnPaint()\n");
      if( wParam != NULL ) {
         DoPaint( (HDC) wParam );
      }
      else {
         PAINTSTRUCT ps;
         HDC hDC = ::BeginPaint(m_hWnd, &ps);
         DoPaint(hDC);
         ::EndPaint(m_hWnd, &ps);
      }
      return 0;
   }
   LRESULT OnSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ::SetCursor(::LoadCursor(NULL, IDC_APPSTARTING));
      return TRUE;
   }
   LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      KillTimer(TIMERID);
      PostQuitMessage(0);
      DestroyWindow();
      return 0;
   }
   LRESULT OnRemoveSplash(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      SetTimer(TIMERID, lParam);
      return 0;
   }

   void DoPaint(CDCHandle dc)
   {
      RECT rc;
      GetClientRect(&rc);   
      CDC dcBmp;
      dcBmp.CreateCompatibleDC(dc);
      HBITMAP hOldBitmap = dcBmp.SelectBitmap(m_bm);
      dc.BitBlt(rc.left, rc.top, rc.right, rc.bottom, dcBmp, 0,0, SRCCOPY);
      dc.SelectBitmap(hOldBitmap);
   }
};


#endif // !defined(AFX_SPLASHDLG_H__20020220_461C_AE84_5328_0080AD509054__INCLUDED_)
