#ifndef __DIALOGSHADOWS_H__
#define __DIALOGSHADOWS_H__

/////////////////////////////////////////////////////////////////////////////
// Menu shadows - an experimental menu shadow
//
// To use this class:
//   Derive from CDialogShadows.
//   Then place a MSG_MAP_CHAIN in the top of the dialog/window
//   message map.
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2002 Bjarke Viksoe.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.

#pragma once

#ifndef __cplusplus
   #error WTL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLDIB_H__
  #error This control requires my atldib.h header to be included first
#endif

// Size of the shadow (be carefull when changing)
#ifndef SHADOW_SIZE
   #define SHADOW_SIZE 5
#endif // SHADOW_SIZE


typedef CWinTraits<WS_OVERLAPPED | WS_POPUP | WS_DISABLED, 0> CShadowWinTraits;

class CShadowWindow :
   public CWindowImpl< CShadowWindow, CWindow, CShadowWinTraits >
{
public:
   DECLARE_WND_CLASS_EX(_T("WTL_MenuShadow"), CS_HREDRAW|CS_VREDRAW|CS_SAVEBITS, NULL)

   CDib24 m_dib;

   BEGIN_MSG_MAP(CShadowWindow)
      MESSAGE_HANDLER(WM_PAINT, OnPaint)
   END_MSG_MAP()

   LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      if( wParam != 0 ) return 0; // No printing support
      CPaintDC dc = m_hWnd;
      ATLASSERT(!m_dib.IsEmpty());
      m_dib.Draw(dc, 0,0);
      return 0;
   }

   void CreateShadowBitmap(CDCHandle dc, const RECT& rcWin, bool bVertical)
   {
      if( !m_dib.IsEmpty() ) m_dib.DeleteObject();

      // Recalculate new shadow

      const int cxWin = rcWin.right - rcWin.left;
      const int cyWin = rcWin.bottom - rcWin.top;

      CWindowDC dcWin = HWND_DESKTOP;
      CDC dcMem;
      dcMem.CreateCompatibleDC(dcWin);
      CBitmap bmp;
      bmp.CreateCompatibleBitmap(dcWin, cxWin, cyWin);
      HBITMAP hOldBmp = dcMem.SelectBitmap(bmp);
      dcMem.BitBlt(0, 0, cxWin, cyWin, dcWin, rcWin.left, rcWin.top, SRCCOPY);
      dcMem.SelectBitmap(hOldBmp);

      m_dib.Create(bmp);

      DWORD nLine = m_dib.GetLineWidth();
      DWORD nPixel = m_dib.GetPixelWidth();

      if( bVertical ) {
         int x, y;

         LPBYTE pStream = m_dib.GetBits() + ((cyWin - SHADOW_SIZE) * nLine);
         for( x = 0; x < SHADOW_SIZE; x++ ) {
            LPBYTE p = pStream;
            for( y = 0; y < SHADOW_SIZE; y++ ) {
               int iScale = 16 + (((SHADOW_SIZE - x) * 2) / (y + 1));
               *p++ = (BYTE) ((*p << 4) / iScale);
               *p++ = (BYTE) ((*p << 4) / iScale);
               *p++ = (BYTE) ((*p << 4) / iScale);
               p += nLine - 3;
            }
            pStream += nPixel;
         }

         pStream = m_dib.GetBits();
         for( x = 0; x < SHADOW_SIZE; x++ ) {
            LPBYTE p = pStream;
            int iScale = 16 + ((SHADOW_SIZE - x) * 2);
            for( y = 0; y < cyWin - SHADOW_SIZE; y++ ) {
               *p++ = (BYTE) ((*p << 4) / iScale);
               *p++ = (BYTE) ((*p << 4) / iScale);
               *p++ = (BYTE) ((*p << 4) / iScale);
               p += nLine - 3;
            }
            pStream += nPixel;
         }
      }
      else {
         int x, y;

         LPBYTE pStream = m_dib.GetBits();
         for( y = 0; y < SHADOW_SIZE; y++ ) {
            LPBYTE p = pStream;
            for( int x = 0; x < SHADOW_SIZE; x++ ) {
               int iScale = 16 + (y * 4 / (SHADOW_SIZE - x + 1));
               *p++ = (BYTE) ((*p << 4) / iScale);
               *p++ = (BYTE) ((*p << 4) / iScale);
               *p++ = (BYTE) ((*p << 4) / iScale);
            }
            pStream += nLine;
         }

         pStream = m_dib.GetBits() + (SHADOW_SIZE * nPixel);
         for( y = 0; y < SHADOW_SIZE; y++ ) {
            LPBYTE p = pStream;
            int iScale = 16 + y * 2;
            for( x = 0; x < cxWin - (SHADOW_SIZE * 2) - 1; x++ ) {
               *p++ = (BYTE) ((*p << 4) / iScale);
               *p++ = (BYTE) ((*p << 4) / iScale);
               *p++ = (BYTE) ((*p << 4) / iScale);
            }
            pStream += nLine;
         }

         pStream = m_dib.GetBits() + ((cxWin - SHADOW_SIZE - 1) * nPixel);
         for( y = 0; y < SHADOW_SIZE; y++ ) {
            LPBYTE p = pStream;
            for( x = 0; x < SHADOW_SIZE; x++ ) {
               int iScale = 16 + ((y + 1) * 3 / (x + 3));
               *p++ = (BYTE) ((*p << 4) / iScale);
               *p++ = (BYTE) ((*p << 4) / iScale);
               *p++ = (BYTE) ((*p << 4) / iScale);
            }
            pStream += nLine;
         }
      }
   }
};

template< class T, class TWin = CShadowWindow >
class CDialogShadows
{
public: 
   BEGIN_MSG_MAP(CDialogShadows)
      MESSAGE_HANDLER(WM_PAINT, OnPaint)
   END_MSG_MAP()

   LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      _CreateShadows();
      bHandled = FALSE;
      return 0;
   }

   // Implementation

   RECT _GetShadowRect(bool bVertical) const
   {
      const T* pT = static_cast<const T*>(this);
      RECT rc = { 0 };
      RECT rcWin = { 0 };
      pT->GetWindowRect(&rc);
      if( bVertical ) {
         ::SetRect(&rcWin, rc.right, rc.top + SHADOW_SIZE, rc.right + SHADOW_SIZE, rc.bottom);
      }
      else {
         ::SetRect(&rcWin, rc.left + SHADOW_SIZE, rc.bottom, rc.right + SHADOW_SIZE, rc.bottom + SHADOW_SIZE);
      }
      return rcWin;
   }

   void _CreateShadows()
   {
      T* pT = static_cast<T*>(this);

      // Test if it already has shadows enabled
#ifndef CS_DROPSHADOW
      const DWORD CS_DROPSHADOW = 0x00020000;  // From Platform SDK (Win2000 and above only)
#endif // CS_DROPSHADOW
      DWORD dwStyle = ::GetClassLong(pT->m_hWnd, GCL_STYLE);
      if( (dwStyle & CS_DROPSHADOW) != 0 ) return;

      RECT rc = { 0 };
      pT->GetWindowRect(&rc);

      if( !m_wndRight.IsWindow() && !m_wndBottom.IsWindow() ) {
         // Create the shadow bitmaps
         CWindowDC dcDesktop = HWND_DESKTOP;
         UpdateWindow(HWND_DESKTOP);
         // Create the 2 shadow windows (they are created as seperate windows).
         // We don't create them visible since this forces them to take focus...
         RECT rcWin = _GetShadowRect(true);
         m_wndRight.CreateShadowBitmap( (HDC) dcDesktop, rcWin, true );
         m_wndRight.Create(pT->m_hWnd, CWindow::rcDefault);
         m_wndRight.SetWindowPos(pT->m_hWnd, &rcWin, SWP_NOACTIVATE | SWP_SHOWWINDOW);
         rcWin = _GetShadowRect(false);
         m_wndBottom.CreateShadowBitmap( (HDC) dcDesktop, rcWin, false );
         m_wndBottom.Create(pT->m_hWnd, CWindow::rcDefault);
         m_wndBottom.SetWindowPos(pT->m_hWnd, &rcWin, SWP_NOACTIVATE | SWP_SHOWWINDOW);
      }
   }

   TWin m_wndRight;
   TWin m_wndBottom;
};


#endif // !defined(__DIALOGSHADOWS_H__)
