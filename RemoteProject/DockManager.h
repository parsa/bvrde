#if !defined(AFX_DOCKMANAGER_H__20041214_EE48_0390_AB55_0080AD509054__INCLUDED_)
#define AFX_DOCKMANAGER_H__20041214_EE48_0390_AB55_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CDockManager
{
public:
   typedef struct
   {
      IDE_DOCK_TYPE DockType;
      RECT rcWindow;
      RECT rcDocked;
   } TDockInfo;

   CSimpleMap<HWND, TDockInfo> m_panels;

   bool SetInfo(HWND hWnd, LPCTSTR pstrProperty)
   {
      if( !::IsWindow(hWnd) ) return false;
      // Get dock state from window
      TDockInfo info;
      bool bAutoShow = ::IsWindowVisible(hWnd) == TRUE;
      // Set global value
      TCHAR szProperty[64];
      ::wsprintf(szProperty, _T("gui.debugViews.%s"), pstrProperty);
      _pDevEnv->SetProperty(szProperty, bAutoShow ? _T("true") : _T("false"));
      // Record the internal settings
      RECT rcWin = { 0 };
      _pDevEnv->GetDockState(hWnd, (int&) info.DockType, rcWin);
      if( info.DockType == IDE_DOCK_HIDE ) return true;
      if( ::IsRectEmpty(&rcWin) ) return true;
      if( info.DockType == IDE_DOCK_FLOAT ) info.rcWindow = rcWin; else info.rcDocked = rcWin;
      if( !m_panels.SetAt(hWnd, info) ) return false;
      return true;
   }

   bool SetInfo(HWND hWnd, IDE_DOCK_TYPE DockType, RECT rcWindow)
   {
      if( !::IsWindow(hWnd) ) return false;
      TDockInfo info;
      info.DockType = DockType;
      info.rcWindow = rcWindow;
      ::ZeroMemory(&info.rcDocked, sizeof(info.rcDocked));
      return m_panels.Add(hWnd, info) == TRUE;
   }

   bool GetInfo(HWND hWnd, IDE_DOCK_TYPE& DockType, RECT& rcWindow)
   {
      int iIndex =  m_panels.FindKey(hWnd);
      if( iIndex < 0 ) return false;
      TDockInfo info = m_panels.GetValueAt(iIndex);
      DockType = info.DockType;
      if( DockType == IDE_DOCK_FLOAT ) rcWindow = info.rcWindow; else rcWindow = info.rcDocked;
      return true;
   }

   bool IsAutoShown(HWND hWnd, LPCTSTR pstrProperty) const
   {
      TCHAR szProperty[64];
      ::wsprintf(szProperty, _T("gui.debugViews.%s"), pstrProperty);
      TCHAR szValue[32] = { 0 };
      _pDevEnv->GetProperty(szProperty, szValue, 31);
      return _tcscmp(szValue, _T("true")) == 0;
   }

   bool OpenView(HWND hWndMain, HWND hWndView, LPCTSTR pstrProperty, UINT nCmd)
   {
      ATLASSERT(::IsWindow(hWndMain));
      if( !IsAutoShown(hWndView, pstrProperty) ) return false;
      return ::SendMessage(hWndMain, WM_COMMAND, MAKEWPARAM(nCmd, 0), 0L) == TRUE;
   }

   bool CloseView(HWND hWndMain, HWND hWndView, LPCTSTR pstrProperty, UINT nCmd)
   {
      ATLASSERT(::IsWindow(hWndMain));
      SetInfo(hWndView, pstrProperty);
      if( !::IsWindow(hWndView) ) return false;
      if( !::IsWindowVisible(hWndView) )  return false;
      return ::SendMessage(hWndMain, WM_COMMAND, MAKEWPARAM(nCmd, 0), 0L) == TRUE;
   }
};


#endif // !defined(AFX_DOCKMANAGER_H__20041214_EE48_0390_AB55_0080AD509054__INCLUDED_)

