#if !defined(AFX_OPTIONSDLG_H__20030409_14FF_E76E_F7BA_0080AD509054__INCLUDED_)
#define AFX_OPTIONSDLG_H__20030409_14FF_E76E_F7BA_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MainFrm.h"
#include "DlgTabCtrl.h"


/////////////////////////////////////////////////////////////////////
// 
//

class CPropertyDialog : public CWindow
{
public:
   PROPSHEETPAGE m_psp;

   HWND Create(HWND hWndParent, PROPSHEETPAGE psp)
   {
      m_psp = psp;
      ATLASSERT(m_hWnd==NULL);
      if( m_psp.dwFlags & PSP_USECALLBACK ) m_psp.pfnCallback(m_hWnd, PSPCB_CREATE, &m_psp);
      CWindow wnd = ::CreateDialogParam(m_psp.hInstance, 
         m_psp.pszTemplate, 
         hWndParent, 
         m_psp.pfnDlgProc, 
         NULL);
      ATLASSERT(wnd.IsWindow());
      ATLASSERT(wnd.GetStyle() & WS_CHILD);
      m_hWnd = wnd;
      return m_hWnd;
   }
   BOOL EndDialog(int nRetCode)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      return ::EndDialog(m_hWnd, nRetCode);
   }
   BOOL DestroyWindow()
   {
      ATLASSERT(::IsWindow(m_hWnd));
      return ::DestroyWindow(m_hWnd);
   }

   // Message map and handlers

   DECLARE_EMPTY_MSG_MAP()
};


/////////////////////////////////////////////////////////////////////
//
//

class COptionsDlg : 
   public CDialogImpl<COptionsDlg>,
   public IWizardManager
{
public:
   enum { IDD = IDD_OPTIONS };

   COptionsDlg(CMainFrame* pMainFrame, IElement* pElement = NULL, ISerializable* pArc = NULL) :
      m_pMainFrame(pMainFrame),
      m_pElement(pElement),
      m_pArc(pArc)
   {
   }

   CMainFrame* m_pMainFrame;
   IElement* m_pElement;
   ISerializable* m_pArc;

   CTreeViewCtrl m_ctrlTree;
   CImageListCtrl m_Images;
   CDlgContainerCtrl m_ctrlContainer;
   CSimpleValArray<CPropertyDialog*> m_aViews;
   HTREEITEM m_hRoot;
   HTREEITEM m_hGroup;

   // IWizardManager

   BOOL AddWizardGroup(LPCTSTR pstrParent, LPCTSTR pstrName)
   {
      ATLASSERT(!::IsBadStringPtr(pstrName,-1));
      if( !SetWizardGroup(pstrParent) ) return FALSE;
      m_hGroup = m_ctrlTree.InsertItem(pstrName, 0, 0, m_hGroup, TVI_LAST);
      return m_hGroup != NULL;
   }
   BOOL SetWizardGroup(LPCTSTR pstrName)
   {
      m_hGroup = _FindTreeItem(m_ctrlTree.GetRootItem(), pstrName);
      ATLASSERT(m_hGroup);
      return m_hGroup != NULL;
   }

   BOOL AddWizardPage(UINT nID, LPCPROPSHEETPAGE hPage)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(m_ctrlContainer.IsWindow());
      ATLASSERT(hPage);
      ATLASSERT(!::IsBadStringPtr(hPage->pszTitle,-1));
      ATLASSERT(hPage->pfnDlgProc);

      if( hPage == NULL ) return FALSE;
      if( ::IsBadStringPtr(hPage->pszTitle, -1) ) return FALSE;
      if( hPage->pfnDlgProc == NULL ) return FALSE;

      CPropertyDialog* pDlg = new CPropertyDialog();
      ATLASSERT(pDlg);
      if( pDlg == NULL ) return FALSE;
      pDlg->Create(m_ctrlContainer, *hPage);
      ATLASSERT(pDlg->IsWindow());
      if( !pDlg->IsWindow() ) return FALSE;

      HTREEITEM hItem = m_ctrlTree.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, 
                                              hPage->pszTitle, 
                                              64, 2, 
                                              0, 0, 
                                              m_ctrlContainer.GetItemCount(), 
                                              m_hGroup, 
                                              TVI_LAST);

      m_ctrlContainer.AddItem(pDlg->m_hWnd);
      m_aViews.Add(pDlg);
      return hItem != NULL;
   }

   // Message map and handlers

   BEGIN_MSG_MAP(COptionsDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_ID_HANDLER(IDC_APPLY, OnApply)
      NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDED, OnItemExpanded)
      NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnItemSelected)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CenterWindow(GetParent());

      COLORREF clrBack = BlendRGB(::GetSysColor(COLOR_WINDOW), RGB(0,0,0), 6);

      m_Images.Create(IDB_FOLDERS, 16, 1, RGB(255,0,255));

      m_ctrlTree = GetDlgItem(IDC_TREE);
      m_ctrlTree.SetBkColor(clrBack);
      m_ctrlTree.SetImageList(m_Images, TVSIL_NORMAL);

      m_ctrlContainer.SubclassWindow(GetDlgItem(IDC_PLACEHOLDER));

      // Create root item
      CString sTitle;
      sTitle.LoadString(m_pElement == NULL ? IDS_TREE_ENVIRONMENT : IDS_TREE_ROOT);
      m_hRoot = m_hGroup = m_ctrlTree.InsertItem(sTitle, 0, 0, TVI_ROOT, TVI_LAST);
      ATLASSERT(m_hRoot!=NULL);
      
      // Allow listeners to add their own pages
      for( int i = 0; i < m_pMainFrame->m_aWizardListeners.GetSize(); i++ ) {
         if( m_pElement == NULL ) {
            m_pMainFrame->m_aWizardListeners[i]->OnInitOptions(this, m_pArc);
         }
         else {
            m_pMainFrame->m_aWizardListeners[i]->OnInitProperties(this, m_pElement);
         }
      }
      
      // Expand root and first sub-tree
      m_ctrlTree.Expand(m_hRoot);
      HTREEITEM hItem = m_hRoot;
      while( m_ctrlTree.GetChildItem(hItem) != NULL ) hItem = m_ctrlTree.GetChildItem(hItem);
      m_ctrlTree.Expand(hItem);
      m_ctrlTree.SelectItem(hItem);

      return TRUE;
   }
   LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      _SendNotifications(PSN_RESET);
      for( int i = 0; i < m_aViews.GetSize(); i++ ) {
         m_aViews[i]->Detach();
         delete m_aViews[i];
      }
      bHandled = FALSE;
      return 0;
   }
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      BOOL bDummy;
      OnApply(0, 0, NULL, bDummy);
      EndDialog(wID);
      return 0;
   }
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CWaitCursor cursor;
      _SendNotifications(PSN_APPLY);
      if( wID == IDC_APPLY ) CWindow(GetParent()).SendMessage(WM_SETTINGCHANGE);
      return 0;
   }
   LRESULT OnItemExpanded(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
   {
      LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW) pnmh;
      HTREEITEM hChild = m_ctrlTree.GetChildItem(lpnmtv->itemNew.hItem);
      if( hChild ) {
         TVITEM item = { 0 };
         item.hItem = lpnmtv->itemNew.hItem;
         item.mask = TVIF_IMAGE;
         item.iImage = item.iSelectedImage = (lpnmtv->action & TVE_EXPAND) != 0 ? 1 : 0;
         m_ctrlTree.SetItem(&item);
         if( (lpnmtv->action & TVE_EXPAND) != 0 ) m_ctrlTree.SelectItem(hChild);
      }
      bHandled = FALSE;
      return 0;
   }
   LRESULT OnItemSelected(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
   {
      LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW) pnmh;
      HTREEITEM hChild = m_ctrlTree.GetChildItem(lpnmtv->itemNew.hItem);
      if( hChild == NULL ) {
         int iPage = (int) m_ctrlTree.GetItemData( lpnmtv->itemNew.hItem );
         // Send PSN_SETACTIVE notification
         HWND hWnd = m_ctrlContainer.GetItem(iPage);
         PSHNOTIFY pshn = { hWnd, GetDlgCtrlID(), PSN_SETACTIVE, m_aViews[iPage]->m_psp.lParam };
         ::SendMessage(hWnd, WM_NOTIFY, pshn.hdr.idFrom, (LPARAM) &pshn);
         // Activate page
         m_ctrlContainer.SetCurSel(iPage);
      }
      bHandled = FALSE;
      return 0;
   }

   // Implementation

   void _SendNotifications(UINT nCode)
   {
      for( int i = 0; i < m_aViews.GetSize(); i++ ) {
         PSHNOTIFY pshn = { m_aViews[i]->m_hWnd, GetDlgCtrlID(), nCode, m_aViews[i]->m_psp.lParam };
         m_aViews[i]->SendMessage(WM_NOTIFY, pshn.hdr.idFrom, (LPARAM) &pshn);
      }
   }
   HTREEITEM _FindTreeItem(HTREEITEM hItem, LPCTSTR pstrName)
   {
      while( hItem ) {
         CString sName;
         m_ctrlTree.GetItemText(hItem, sName);
         if( sName == pstrName ) return hItem;
         HTREEITEM hNext;
         if( (hNext = _FindTreeItem(m_ctrlTree.GetChildItem(hItem), pstrName)) ) return hNext;
         hItem = m_ctrlTree.GetNextSiblingItem(hItem);
      }
      return NULL;
   }
};


#endif // !defined(AFX_OPTIONSDLG_H__20030409_14FF_E76E_F7BA_0080AD509054__INCLUDED_)
