#if !defined(AFX_BROWSERVIEW_H__20030706_207B_B23E_DF7E_0080AD509054__INCLUDED_)
#define AFX_BROWSERVIEW_H__20030706_207B_B23E_DF7E_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ShellCtrls.h"

#define IDC_SHELL_FOLDER  1
#define IDC_SHELL_FILES   2


// Show Wait cursor.
// Stripped copy of WTL sources.
class CWaitCursor
{
public:
   HCURSOR m_hOldCursor;
   CWaitCursor()
   {
      m_hOldCursor = ::SetCursor( ::LoadCursor(NULL, IDC_WAIT) );
   }
   ~CWaitCursor()
   {
      ::SetCursor(m_hOldCursor);
   }
};


class CDragDrop : public CRawDropSource
{
public:
   void DragDrop(HWND hWnd, CPidl& pidl)
   {
      CPidl pidlFile;
      CComPtr<IShellFolder> spFolder;
      AtlGetShellPidl(pidl, &spFolder, &pidlFile);
      if( spFolder == NULL ) return;
      CComPtr<IDataObject> spDataObj;
      spFolder->GetUIObjectOf(hWnd, 1, const_cast<LPCITEMIDLIST*>(&pidlFile.m_pidl), IID_IDataObject, NULL, (LPVOID*) &spDataObj);
      if( spDataObj == NULL ) return;
      DWORD dwEffect = 0;
      ::DoDragDrop(spDataObj, this, DROPEFFECT_COPY, &dwEffect);
   }
};


class CFolderView : public CShellTreeCtrl
{
public:
   CExplorerMenu m_ShellMenu;

   BEGIN_MSG_MAP(CFolderView)
      REFLECTED_NOTIFY_CODE_HANDLER(NM_RCLICK, OnRightClick)
      //CHAIN_MSG_MAP_MEMBER( m_ShellMenu )
      CHAIN_MSG_MAP( CShellTreeCtrl )
   END_MSG_MAP()

   LRESULT OnRightClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      CPidl pidl;
      HTREEITEM hItem = GetDropHilightItem();
      if( hItem == NULL ) hItem = GetSelectedItem();
      if( hItem == NULL ) return 0;
      if( !GetItemPidl(hItem, &pidl) ) return 0;
      DWORD dwPos = ::GetMessagePos();
      int x = GET_X_LPARAM(dwPos);
      int y = GET_Y_LPARAM(dwPos); 
      TrackPopupMenu(pidl, x, y, m_hWnd);
      return 0;
   }

   BOOL TrackPopupMenu(LPCITEMIDLIST pidl, int x, int y, HWND hWnd)
   {
      ATLASSERT(pidl);
      ATLASSERT(::IsWindow(hWnd));

      CComPtr<IShellFolder> sspFolder;
      CPidl pidlItem;
      if( !AtlGetShellPidl(pidl, &sspFolder, &pidlItem) ) return FALSE;

      // Get a pointer to the item's IContextMenu interface and call
      // IContextMenu::QueryContextMenu to initialize a context menu.
      BOOL bResult = FALSE;
      CComPtr<IContextMenu> spContextMenu;
      LPCITEMIDLIST lpPidl = pidlItem;
      if( FAILED( sspFolder->GetUIObjectOf(hWnd, 1, &lpPidl, IID_IContextMenu, NULL, (LPVOID*) &spContextMenu))) return FALSE;
      // Do we need to display the popup menu?
      HMENU hMenu = ::CreatePopupMenu();
      int nCmd = 0;
      if( SUCCEEDED( spContextMenu->QueryContextMenu(
           hMenu, 
           0, 
           1, 
           0x7FFF, 
           CMF_EXPLORE)) ) 
      {
         m_ShellMenu.m_spCtxMenu2 = spContextMenu;
         // Display the context menu.
         POINT pt = { x, y };
         nCmd = _pDevEnv->ShowPopupMenu(NULL, hMenu, pt, FALSE);
         m_ShellMenu.m_spCtxMenu2.Release();
      }
      // If a command is available (from the menu, perhaps), execute it.
      if( nCmd ) {
         CMINVOKECOMMANDINFO ici = { 0 };
         ici.cbSize       = sizeof(CMINVOKECOMMANDINFO);
         ici.fMask        = 0;
         ici.hwnd         = hWnd;
         ici.lpVerb       = MAKEINTRESOURCEA(nCmd - 1);
         ici.lpParameters = NULL;
         ici.lpDirectory  = NULL;
         ici.nShow        = SW_SHOWNORMAL;
         ici.dwHotKey     = 0;
         ici.hIcon        = NULL;
         if( SUCCEEDED( spContextMenu->InvokeCommand(&ici)) ) bResult = TRUE;
      }
      if( hMenu ) ::DestroyMenu(hMenu);
      return bResult;
   }
};


class CFileView : public CShellListCtrl
{
public:
   CExplorerMenu m_ShellMenu;

   BEGIN_MSG_MAP(CShellListCtrl)
      MESSAGE_HANDLER(WM_INITMENUPOPUP, m_ShellMenu.OnShellMenuMsg)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      //CHAIN_MSG_MAP_MEMBER( m_ShellMenu )
      CHAIN_MSG_MAP( CShellListCtrl )
   END_MSG_MAP()

   LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      CPidl pidl;
      if( !GetItemPidl(GetSelectedIndex(), &pidl) ) return 0;
      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam); 
      TrackPopupMenu(pidl, x, y, m_hWnd);
      return 0;
   }
   LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
      return 0;
   }

   BOOL TrackPopupMenu(LPCITEMIDLIST pidl, int x, int y, HWND hWnd)
   {
      ATLASSERT(pidl);
      ATLASSERT(::IsWindow(hWnd));

      CComPtr<IShellFolder> sspFolder;
      CPidl pidlItem;
      if( !AtlGetShellPidl(pidl, &sspFolder, &pidlItem) ) return FALSE;

      // Get a pointer to the item's IContextMenu interface and call
      // IContextMenu::QueryContextMenu to initialize a context menu.
      BOOL bResult = FALSE;
      CComPtr<IContextMenu> spContextMenu;
      LPCITEMIDLIST lpPidl = pidlItem;
      if( FAILED( sspFolder->GetUIObjectOf(hWnd, 1, &lpPidl, IID_IContextMenu, NULL, (LPVOID*) &spContextMenu))) return FALSE;
      // Do we need to display the popup menu?
      HMENU hMenu = ::CreatePopupMenu();
      int nCmd = 0;
      if( SUCCEEDED( spContextMenu->QueryContextMenu(
           hMenu, 
           0, 
           1, 
           0x7FFF, 
           CMF_EXPLORE)) ) 
      {
         m_ShellMenu.m_spCtxMenu2 = spContextMenu;
         // Display the context menu.
         POINT pt = { x, y };
         nCmd = _pDevEnv->ShowPopupMenu(NULL, hMenu, pt, FALSE);
         m_ShellMenu.m_spCtxMenu2.Release();
      }
      // If a command is available (from the menu, perhaps), execute it.
      if( nCmd ) {
         CMINVOKECOMMANDINFO ici = { 0 };
         ici.cbSize       = sizeof(CMINVOKECOMMANDINFO);
         ici.fMask        = 0;
         ici.hwnd         = hWnd;
         ici.lpVerb       = MAKEINTRESOURCEA(nCmd - 1);
         ici.lpParameters = NULL;
         ici.lpDirectory  = NULL;
         ici.nShow        = SW_SHOWNORMAL;
         ici.dwHotKey     = 0;
         ici.hIcon        = NULL;
         if( SUCCEEDED( spContextMenu->InvokeCommand(&ici)) ) bResult = TRUE;
      }
      if( hMenu ) ::DestroyMenu(hMenu);
      return bResult;
   }
};


class CBrowserView : public CWindowImpl<CBrowserView>
{
public:
   CFolderView m_ctrlFolders;
   CFileView m_ctrlFiles;

   BEGIN_MSG_MAP(CBrowserView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      NOTIFY_HANDLER(IDC_SHELL_FOLDER, TVN_SELCHANGED, OnSelChanged)
      NOTIFY_HANDLER(IDC_SHELL_FILES, NM_DBLCLK, OnDblClick)
      NOTIFY_HANDLER(IDC_SHELL_FILES, LVN_BEGINDRAG, OnBeginDrag)
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      DWORD dwStyle;
      dwStyle = TVS_HASBUTTONS | TVS_INFOTIP | TVS_HASLINES | TVS_SHOWSELALWAYS | WS_CHILD | WS_VISIBLE | WS_TABSTOP;
      m_ctrlFolders.Create(m_hWnd, rcDefault, NULL, dwStyle, WS_EX_CLIENTEDGE, IDC_SHELL_FOLDER);
      dwStyle = LVS_SHAREIMAGELISTS | LVS_SINGLESEL | LVS_REPORT | LVS_AUTOARRANGE | LVS_NOSORTHEADER | WS_CHILD | WS_VISIBLE | WS_TABSTOP;
      m_ctrlFiles.Create(m_hWnd, rcDefault, NULL, dwStyle, WS_EX_CLIENTEDGE, IDC_SHELL_FILES);

      m_ctrlFolders.SetShellStyle(SCT_EX_FILESYSTEMONLY);
      m_ctrlFiles.SetShellStyle(SCT_EX_FILESYSTEMONLY);

      TCHAR szTitle[128] = { 0 };
      ::LoadString(_Module.GetResourceInstance(), IDS_FILES, szTitle, 127);
      m_ctrlFiles.InsertColumn(0, szTitle, LVCFMT_LEFT, 200, 0);
      m_ctrlFolders.Populate();
      return 0;
   }
   LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      RECT rcClient;
      GetClientRect(&rcClient);
      int cy = (rcClient.bottom - rcClient.top) / 2;
      m_ctrlFolders.MoveWindow(rcClient.left, rcClient.top, rcClient.right, rcClient.top + cy);
      m_ctrlFiles.MoveWindow(rcClient.left, rcClient.top + cy, rcClient.right, rcClient.bottom - cy);
      return 0;
   }

   LRESULT OnSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      if( pnmh->hwndFrom != m_ctrlFolders ) return 0;
      CWaitCursor cursor;
      CWindowRedraw redraw = m_ctrlFiles;
      LPNMTREEVIEW pnmtv = (LPNMTREEVIEW) pnmh;
      CPidl pidl;
      m_ctrlFolders.GetItemPidl(pnmtv->itemNew.hItem, &pidl);
      m_ctrlFiles.Populate(pidl);
      return 0;
   }
   LRESULT OnDblClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      LPNMLISTVIEW pnmlv = (LPNMLISTVIEW) pnmh;
      CWaitCursor cursor;
      CWindowRedraw redraw = m_ctrlFolders;
      CPidl pidl;
      TCHAR szPath[MAX_PATH] = { 0 };
      m_ctrlFiles.GetItemPath(pnmlv->iItem, szPath);
      m_ctrlFiles.GetItemPidl(pnmlv->iItem, &pidl);
      m_ctrlFolders.SelectPidl(pidl);
      DWORD dwFlags = ::GetFileAttributes(szPath);
      if( (dwFlags & FILE_ATTRIBUTE_DIRECTORY) == 0 && _tcslen(szPath) > 0 ) 
      {
         IView* pView = _pDevEnv->CreateView(szPath, NULL, NULL);
         if( pView ) pView->OpenView(0);
      }
      return 0;
   }
   LRESULT OnBeginDrag(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      LPNMLISTVIEW pnmlv = (LPNMLISTVIEW) pnmh;
      CPidl pidl;
      m_ctrlFiles.SetFocus();
      m_ctrlFiles.SelectItem(pnmlv->iItem);
      m_ctrlFiles.UpdateWindow();
      m_ctrlFiles.GetItemPidl(pnmlv->iItem, &pidl);
      CDragDrop dd;
      dd.DragDrop(m_hWnd, pidl);
      return 0;
   }
};


#endif // !defined(AFX_BROWSERVIEW_H__20030706_207B_B23E_DF7E_0080AD509054__INCLUDED_)

