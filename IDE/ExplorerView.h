#if !defined(AFX_SOLUTIONEXPLORERVIEW_H__20020424_B184_A9DC_E870_0080AD509054__INCLUDED_)
#define AFX_SOLUTIONEXPLORERVIEW_H__20020424_B184_A9DC_E870_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DlgTabCtrl.h"
#include "ProjectView.h"


class CExplorerView : public CWindowImpl<CExplorerView>
{
public:
   DECLARE_WND_CLASS_EX(_T("BVRDE_ExplorerView"), 0, COLOR_3DFACE)

   CDlgContainerCtrl m_ctrlViews;
   CImageListCtrl m_Images;
   CDotNetTabCtrl m_ctrlTab;
   CProjectView m_viewFile;

   BOOL PreTranslateMessage(MSG* /*pMsg*/)
   {
      return FALSE;
   }

   BOOL AddView(HWND hWnd, LPCTSTR pstrName, int iImage)
   {
      ATLASSERT(::IsWindow(hWnd));
      ATLASSERT(!::IsBadStringPtr(pstrName,-1));
      ATLASSERT(m_ctrlTab.IsWindow());
      // Set new parent
      ::SetParent(hWnd, m_hWnd);
      // Add view
      m_ctrlViews.AddItem(hWnd);
      // Add tab control item
      TCITEM tci = { 0 };
      tci.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
      tci.pszText = (LPTSTR) pstrName;
      tci.iImage = iImage;
      tci.lParam = (LPARAM) hWnd;
      BOOL bRes = (int) m_ctrlTab.InsertItem(m_ctrlTab.GetItemCount(), &tci) != -1;
      // Repaint tab control nicely
      m_ctrlTab.Invalidate();
      return bRes;
   }
   BOOL RemoveView(HWND hWnd)
   {
      // Remove interval dialog view
      m_ctrlViews.RemoveItem(hWnd);
      // Remove tab control item
      for( int i = 0; i < m_ctrlTab.GetItemCount(); i++ ) {
         TCITEM tci = { 0 };
         tci.mask = TCIF_PARAM;
         m_ctrlTab.GetItem(i, &tci);
         if( (HWND) tci.lParam == hWnd ) {
            m_ctrlTab.DeleteItem(i);
            m_ctrlTab.Invalidate();
            return TRUE;
         }
      }
      return FALSE;
   }

   BEGIN_MSG_MAP(CExplorerView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      NOTIFY_CODE_HANDLER(TCN_SELCHANGE, OnSelChange)
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlViews.Create(m_hWnd, rcDefault);

      m_Images.Create(IDB_EXPLORER, 16, 0, RGB(255,0,255));
      m_ctrlTab.Create(m_hWnd, rcDefault, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TCS_BOTTOM | TCS_TOOLTIPS);
      m_ctrlTab.SetExtendedStyle(0, TCS_EX_FLATSEPARATORS | TCS_EX_COMPRESSLINE);
      m_ctrlTab.SetImageList(m_Images);

      m_viewFile.Create(m_ctrlViews, rcDefault);
      AddView(m_viewFile, CString(MAKEINTRESOURCE(IDS_SOULTIONEXPLORER)), 0);

      m_ctrlTab.SetCurSel(0);      
      return 0;
   }
   LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      return TRUE; // Children fills entire client area
   }
   LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      if( !m_ctrlViews.IsWindow() || !m_ctrlTab.IsWindow() ) return 0;
      RECT rc = { 0 };
      GetClientRect(&rc);
      RECT rcViews = { rc.left, rc.top, rc.right, rc.bottom - 24 };
      m_ctrlViews.MoveWindow(&rcViews);
      RECT rcTab = { rc.left, rc.bottom - 24, rc.right, rc.bottom };
      m_ctrlTab.MoveWindow(&rcTab);
      return 0;
   }
   LRESULT OnSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      m_ctrlViews.SetCurSel( m_ctrlTab.GetCurSel() );
      return 0;
   }
};


#endif // !defined(AFX_SOLUTIONEXPLORERVIEW_H__20020424_B184_A9DC_E870_0080AD509054__INCLUDED_)

