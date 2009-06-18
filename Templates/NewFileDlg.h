#if !defined(AFX_NEWFILEDLG_H__20040730_ED8A_8DCA_74A7_0080AD509054__INCLUDED_)
#define AFX_NEWFILEDLG_H__20040730_ED8A_8DCA_74A7_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"


class CNewFileDlg : public CDialogImpl<CNewFileDlg>
{
public:
   enum { IDD = IDD_NEWFILE };

   typedef struct 
   {
      TCHAR szGroup[100];
      TCHAR szName[100];
   } FILEITEM;

   TCHAR m_szIniFile[MAX_PATH];
   CSimpleArray<FILEITEM> m_aFiles;
   CSimpleMap<CComBSTR, int> m_mapIcons;

   CTreeViewCtrl m_ctrlTree;
   CListViewCtrl m_ctrlList;
   CImageList m_TreeImages;
   CImageList m_ListImages;
   CStatic m_ctrlDescription;
   CButton m_ctrlOK;

   IDevEnv* m_pDevEnv;
   ISolution* m_pSolution;
   IProject* m_pProject;
   IView* m_pView;

   // Operations

   void Init(IDevEnv* pDevEnv, ISolution* pSolution, IProject* pProject, IView* pView)
   {
      m_pDevEnv = pDevEnv;
      m_pSolution = pSolution;
      m_pProject = pProject;
      m_pView = pView;
   }

   // Message map and handlers

   BEGIN_MSG_MAP(CNewFileDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
      NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnItemActivate)
      NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnTreeSelection)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ::wsprintf(m_szIniFile, _T("%sTemplates\\Files.ini"), CModulePath());
      m_TreeImages.Create(IDB_FOLDERS, 16, 0, RGB(255,0,255));
      CBitmap bmpItems;
      bmpItems.LoadBitmap(IDB_ITEMS);
      m_ListImages.Create(32, 32, ILC_COLOR32 | ILC_MASK, 4, 100);
      m_ListImages.Add(bmpItems, RGB(255,0,255));
      m_ctrlTree = GetDlgItem(IDC_GROUPTREE);
      m_ctrlTree.SetImageList(m_TreeImages, TVSIL_NORMAL);
      m_ctrlList = GetDlgItem(IDC_FILELIST);
      m_ctrlList.SetImageList(m_ListImages, LVSIL_NORMAL);
      m_ctrlDescription = GetDlgItem(IDC_DESCRIPTION);
      m_ctrlOK = GetDlgItem(IDOK);
      _PopulateTree();
      m_ctrlTree.SelectItem(m_ctrlTree.GetRootItem());
      return TRUE;
   }

   LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      m_TreeImages.Destroy();
      m_ListImages.Destroy();
      bHandled = FALSE;
      return 0;
   }
   
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CWaitCursor cursor;
      TCHAR szName[100] = { 0 };
      if( !_GetItemName(szName) ) return 0;
      TCHAR szFilename[MAX_PATH] = { 0 };
      ::GetPrivateProfileString(szName, _T("File"), _T(""), szFilename, MAX_PATH, m_szIniFile);
      if( ::lstrlen(szFilename) == 0 ) return 0;
      TCHAR szActualFile[MAX_PATH] = { 0 };
      ::wsprintf(szActualFile, _T("%sTemplates\\Files\\%s"), CModulePath(), szFilename);
      CFile f;      
      if( !f.Open(szActualFile) ) return 0;
      DWORD dwSize = f.GetSize();
      LPSTR lpstr = (LPSTR) malloc(dwSize + 1);
      if( lpstr == NULL ) return 0;
      f.Read(lpstr, dwSize);
      f.Close();
      lpstr[dwSize] = '\0';
      CComBSTR bstrScript = lpstr;
      free(lpstr);
      CParser parser;
      LPWSTR pstrScript = parser.Process(bstrScript);
      if( pstrScript == NULL ) return 0;
      CComObjectGlobal<CScriptHandler> script;
      script.Init(m_hWnd,
         m_pDevEnv->GetDispatch(), 
         m_pSolution->GetDispatch(), 
         m_pProject->GetDispatch(), 
         m_pView->GetDispatch());
      bool bRes = script.Execute(pstrScript);
      free(pstrScript);
      if( !bRes ) return 0;
      CComBSTR bstrResult = script.GetResult();
      CComDispatchDriver spDisp = m_pView->GetDispatch();
      spDisp.Invoke0(L"Open");
      CComVariant vArg = bstrResult;
      spDisp.Invoke1(L"ReplaceSelection", &vArg);
      EndDialog(wID);
      return 0;
   }
   
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   
   LRESULT OnItemActivate(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      PostMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0));
      return 0;
   }
   
   LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      int iIndex = m_ctrlList.GetSelectedIndex();
      TCHAR szDescription[100] = { 0 };
      if( iIndex >= 0 ) {
         iIndex = m_ctrlList.GetItemData(iIndex);
         ::GetPrivateProfileString(m_aFiles[iIndex].szName, _T("Description"), _T(""), szDescription, 99, m_szIniFile);
      }
      m_ctrlDescription.SetWindowText(szDescription);
      _UpdateButtons();
      return 0;
   }
   
   LRESULT OnTreeSelection(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      _PopulateList();
      _UpdateButtons();
      return 0;
   }

   // Implementation

   bool _GetItemName(LPTSTR pstrName) const
   {
      int iIndex = m_ctrlList.GetSelectedIndex();
      if( iIndex < 0 ) return false;
      return ::lstrcpy(pstrName, m_aFiles[m_ctrlList.GetItemData(iIndex)].szName) > 0;
   }
   
   void _UpdateButtons()
   {
      m_ctrlOK.EnableWindow(m_ctrlList.GetSelectedCount() > 0);
   }
   
   void _PopulateTree()
   {
      int iIndex = 0;
      while( true ) {
         TCHAR szKey[20] = { 0 };
         ::wsprintf(szKey, _T("%ld"), iIndex + 1);
         TCHAR szItem[100] = { 0 };
         ::GetPrivateProfileString(_T("Groups"), szKey, _T(""), szItem, 99, m_szIniFile);
         if( ::lstrlen(szItem) == 0 ) break;
         m_ctrlTree.InsertItem(szItem, 0, 1, TVI_ROOT, TVI_LAST);
         iIndex++;
      }
   }
   
   void _PopulateList()
   {
      CWaitCursor cursor;
      CWindowRedraw redraw = m_ctrlList;
      m_aFiles.RemoveAll();
      m_ctrlList.DeleteAllItems();
      HTREEITEM hItem = m_ctrlTree.GetSelectedItem();
      if( hItem == NULL ) return;
      TCHAR szGroup[100] = { 0 };
      m_ctrlTree.GetItemText(hItem, szGroup, 99);
      int iIndex = 0;
      while( true ) {
         // Template entry format:
         //    [C++ File]
         //    File = Foo.cpp
         //    Icon = Test.ico
         //    Description = Bla bla bla...
         TCHAR szKey[20] = { 0 };
         ::wsprintf(szKey, _T("%ld"), iIndex + 1);
         TCHAR szItem[100] = { 0 };
         ::GetPrivateProfileString(szGroup, szKey, _T(""), szItem, 99, m_szIniFile);
         if( ::lstrlen(szItem) == 0 ) break;

         FILEITEM Item = { 0 };
         ::lstrcpy(Item.szGroup, szGroup);
         ::lstrcpy(Item.szName, szItem);
         m_aFiles.Add(Item);
         
         int iImage = 2;
         TCHAR szIcon[100] = { 0 };
         ::GetPrivateProfileString(szItem, _T("File"), _T(""), szIcon, 99, m_szIniFile);
         if( ::lstrlen(szIcon) > 0 ) {
            CComBSTR bstrKey = szIcon;
            if( m_mapIcons.FindKey(bstrKey) < 0 ) {
               SHFILEINFO shfi = { 0 };
               if( ::SHGetFileInfo(szIcon, FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_SHELLICONSIZE | SHGFI_USEFILEATTRIBUTES) ) {
                  iImage = m_ListImages.AddIcon(shfi.hIcon);
                  ::DestroyIcon(shfi.hIcon);
                  m_mapIcons.Add(bstrKey, iImage);
               }
            }
            else {
               iImage = m_mapIcons.Lookup(bstrKey);
            }
         }
         ::GetPrivateProfileString(szItem, _T("Icon"), _T(""), szIcon, 99, m_szIniFile);
         if( ::lstrlen(szIcon) > 0 ) {
            CComBSTR bstrKey = szIcon;
            if( m_mapIcons.FindKey(bstrKey) < 0 ) {
               TCHAR szFilename[MAX_PATH] = { 0 };
               ::wsprintf(szFilename, _T("%sTemplates\\Files\\%s"), CModulePath(), szIcon);
               HICON hIcon = (HICON) ::LoadImage(_Module.GetResourceInstance(), szFilename, IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
               if( hIcon != NULL ) {
                  iImage = m_ListImages.AddIcon(hIcon);
                  ::DestroyIcon(hIcon);
                  m_mapIcons.Add(bstrKey, iImage);
               }
            }
            else {
               iImage = m_mapIcons.Lookup(bstrKey);
            }
         }

         int iItem = m_ctrlList.InsertItem(iIndex, szItem, iImage);
         m_ctrlList.SetItemData(iItem, iIndex);

         iIndex++;
      }
      m_ctrlList.SelectItem(0);
   }
};


#endif // !defined(AFX_NEWFILEDLG_H__20040730_ED8A_8DCA_74A7_0080AD509054__INCLUDED_)
