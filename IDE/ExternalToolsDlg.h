#if !defined(AFX_EXTERNALTOOLSDLG_H__20030601_1EF9_3180_E184_0080AD509054__INCLUDED_)
#define AFX_EXTERNALTOOLSDLG_H__20030601_1EF9_3180_E184_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RegSerializer.h"


class CExternalToolsDlg : public CDialogImpl<CExternalToolsDlg>
{
public:
   enum { IDD = IDD_TOOLS };

   CFont m_fontList;
   CListViewCtrl m_ctrlList;
   CEdit m_ctrlTitle;
   CEdit m_ctrlCommand;
   CEdit m_ctrlArguments;
   CEdit m_ctrlPath;
   CComboBox m_ctrlType;
   CButton m_ctrlNew;
   CButton m_ctrlDelete;
   CButton m_ctrlDown;
   CButton m_ctrlUp;
   CButton m_ctrlBrowseFile;
   CButton m_ctrlBrowseArguments;
   CButton m_ctrlBrowsePath;
   CButton m_ctrlConsoleOutput;
   CButton m_ctrlPromptArgs;

   CIcon m_iconNew;
   CIcon m_iconDelete;
   CIcon m_iconUp;
   CIcon m_iconDown;
   CFont m_fontWingdings;

   enum
   {
      TYPE_LOCAL = 0,
      TYPE_COMPILER,
      TYPE_DEBUGGER,
      TYPE_SQL,
   };

   typedef struct 
   {
      CString sTitle;
      CString sCommand;
      CString sArguments;
      CString sPath;
      CString sType;
      long lFlags;
   } TOOL;

   TOOL m_aTools[30];
   int m_nTools;

   CMainFrame* m_pMainFrame;

   CExternalToolsDlg(CMainFrame* pMainFrame) : 
      m_pMainFrame(pMainFrame),
      m_nTools(0)
   {     
   }

   BEGIN_MSG_MAP(CExternalToolsDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnCreate)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_ID_HANDLER(IDC_NEW, OnNew)
      COMMAND_ID_HANDLER(IDC_DELETE, OnDelete)
      COMMAND_ID_HANDLER(IDC_UP, OnUp)
      COMMAND_ID_HANDLER(IDC_DOWN, OnDown)
      COMMAND_ID_HANDLER(IDC_BROWSE_FILE, OnBrowseFile)      
      COMMAND_ID_HANDLER(IDC_BROWSE_ARGUMENTS, OnBrowseArgs)
      COMMAND_ID_HANDLER(IDC_BROWSE_PATH, OnBrowsePath)
      COMMAND_ID_HANDLER(ID_TOOLTOKEN_PROJECTNAME, OnToolToken)
      COMMAND_ID_HANDLER(ID_TOOLTOKEN_PROJECTPATH, OnToolToken)
      COMMAND_ID_HANDLER(ID_TOOLTOKEN_HOSTNAME, OnToolToken)
      COMMAND_ID_HANDLER(ID_TOOLTOKEN_FILENAME, OnToolToken)
      COMMAND_ID_HANDLER(ID_TOOLTOKEN_SELECTION, OnToolToken)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
      COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnCbChange)
      NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnSelChanged)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CenterWindow();

      CLogFont lf = GetFont();
      lf.MakeLarger(2);
      _tcscpy(lf.lfFaceName, _T("Arial"));
      m_fontList.CreateFontIndirect(&lf);

      m_ctrlList = GetDlgItem(IDC_LIST);
      m_ctrlTitle = GetDlgItem(IDC_TITLE);
      m_ctrlCommand = GetDlgItem(IDC_COMMAND);
      m_ctrlArguments = GetDlgItem(IDC_ARGUMENTS);
      m_ctrlPath = GetDlgItem(IDC_PATH);
      m_ctrlNew = GetDlgItem(IDC_NEW);
      m_ctrlDelete = GetDlgItem(IDC_DELETE);
      m_ctrlUp = GetDlgItem(IDC_UP);
      m_ctrlDown = GetDlgItem(IDC_DOWN);
      m_ctrlBrowseFile = GetDlgItem(IDC_BROWSE_FILE);
      m_ctrlBrowseArguments = GetDlgItem(IDC_BROWSE_ARGUMENTS);
      m_ctrlBrowsePath = GetDlgItem(IDC_BROWSE_PATH);
      m_ctrlType = GetDlgItem(IDC_TYPE);
      m_ctrlPromptArgs = GetDlgItem(IDC_PROMPT_ARGS);
      m_ctrlConsoleOutput = GetDlgItem(IDC_USE_OUTPUT);

      m_iconNew.LoadIcon(IDI_NEW, 16, 16);
      m_iconDelete.LoadIcon(IDI_DELETE, 16, 16);
      m_iconUp.LoadIcon(IDI_UP, 16, 16);
      m_iconDown.LoadIcon(IDI_DOWN, 16, 16);
      
      m_fontWingdings.CreateFont(14, 0, 0, 0, 0, 0, 0, 0, SYMBOL_CHARSET, 0, 0, 0, 0, _T("Wingdings"));

      COLORREF clrBack = BlendRGB(::GetSysColor(COLOR_WINDOW), RGB(0,0,0), 6);

      m_ctrlList.SetBkColor(clrBack);
      m_ctrlList.SetFont(m_fontList);
      m_ctrlList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
      m_ctrlList.AddColumn(_T(""), 0);
      m_ctrlList.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);

      m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_TOOLTYPE_LOCAL)));
      m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_TOOLTYPE_COMPILER)));
      m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_TOOLTYPE_DEBUGGER)));
      m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_TOOLTYPE_SQL)));

      m_ctrlNew.SetIcon(m_iconNew);
      m_ctrlDelete.SetIcon(m_iconDelete);
      m_ctrlUp.SetIcon(m_iconUp);
      m_ctrlDown.SetIcon(m_iconDown);
      m_ctrlBrowseArguments.SetFont(m_fontWingdings);
#ifdef _UNICODE
      USHORT pstrArrow[] = { 0xE8, '\0' };
      m_ctrlBrowseArguments.SetWindowText((LPCWSTR)pstrArrow);
#else
      m_ctrlBrowseArguments.SetWindowText(_T("\xE8"));
#endif // _UNICODE

      _LoadItems();
      m_ctrlList.SelectItem(0);

      _UpdateButtons();

      return TRUE;
   }

   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      _SaveItems();
      EndDialog(wID);
      return 0;
   }

   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   
   LRESULT OnNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {    
      CString sNewTool(MAKEINTRESOURCE(IDS_NEWTOOL));
      TOOL& tool = m_aTools[m_nTools];
      tool.sTitle = sNewTool;
      tool.sType = _T("local");
      tool.lFlags = 0;
      int iItem = m_ctrlList.InsertItem(m_ctrlList.GetItemCount(), sNewTool);
      m_ctrlList.SetItemData(iItem, (LPARAM) &m_aTools[m_nTools]);
      m_ctrlList.SelectItem(iItem);
      m_ctrlTitle.SetFocus();
      m_ctrlTitle.SetSel(0, -1);
      m_ctrlNew.SetButtonStyle(BS_PUSHBUTTON);
      m_nTools++;
      return 0;
   }
   
   LRESULT OnDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      m_ctrlList.DeleteItem(m_ctrlList.GetSelectedIndex());
      m_ctrlList.SelectItem(0);
      m_ctrlList.SetFocus();
      m_ctrlDelete.SetButtonStyle(BS_PUSHBUTTON);
      return 0;
   }
   
   LRESULT OnUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      int iIndex = m_ctrlList.GetSelectedIndex();
      CString sItem;
      LPARAM lData;
      m_ctrlList.GetItemText(iIndex, 0, sItem);
      lData = m_ctrlList.GetItemData(iIndex);
      m_ctrlList.DeleteItem(iIndex);
      int iNewIndex = m_ctrlList.InsertItem(iIndex - 1, sItem);
      m_ctrlList.SetItemData(iNewIndex, lData);
      m_ctrlList.SelectItem(iNewIndex);
      m_ctrlList.SetFocus();
      m_ctrlUp.SetButtonStyle(BS_PUSHBUTTON);
      return 0;
   }
   
   LRESULT OnDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      int iIndex = m_ctrlList.GetSelectedIndex();
      CString sItem;
      LPARAM lData;
      m_ctrlList.GetItemText(iIndex, 0, sItem);
      lData = m_ctrlList.GetItemData(iIndex);
      m_ctrlList.DeleteItem(iIndex);
      int iNewIndex = m_ctrlList.InsertItem(iIndex + 1, sItem);
      m_ctrlList.SetItemData(iNewIndex, lData);
      m_ctrlList.SelectItem(iNewIndex);
      m_ctrlList.SetFocus();
      m_ctrlDown.SetButtonStyle(BS_PUSHBUTTON);
      return 0;
   }
   
   LRESULT OnBrowseFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CString sFilter(MAKEINTRESOURCE(IDS_FILTER_EXE));
      for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == '|' ) sFilter.SetAt(i, '\0');
      CWindowText sOriginal = CWindowText(m_ctrlCommand);
      CFileDialog dlg(TRUE, _T("exe"), sOriginal, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, sFilter);
      int nRet = dlg.DoModal();
      if(nRet == IDOK) {
         m_ctrlCommand.SetWindowText(dlg.m_ofn.lpstrFile);
         m_ctrlCommand.SetFocus();
      }
      m_ctrlBrowseFile.SetButtonStyle(BS_PUSHBUTTON);
      return 0;
   }
   
   LRESULT OnBrowseArgs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CMenu menu;
      menu.LoadMenu(IDM_TOOL_ARGS);
      CMenuHandle submenu = menu.GetSubMenu(0);
      RECT rcWindow;
      m_ctrlBrowseArguments.GetWindowRect(&rcWindow);
      POINT pt = { rcWindow.right, rcWindow.top };
      UINT nCmd = g_pDevEnv->ShowPopupMenu(NULL, submenu, pt, FALSE);
      if( nCmd != 0 ) PostMessage(WM_COMMAND, MAKEWPARAM(nCmd, 0));
      m_ctrlArguments.SetFocus();
      return 0;
   }
   
   LRESULT OnBrowsePath(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CFolderDialog dlg;
      if( dlg.DoModal() != IDOK ) return 0;
      m_ctrlPath.SetWindowText(dlg.m_szFolderPath);
      return 0;
   }
   
   LRESULT OnToolToken(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CEdit ctrlEdit = ::GetFocus();
      CString sText;
      switch( wID ) {
      case ID_TOOLTOKEN_HOSTNAME: sText = _T("$HOSTNAME$"); break;
      case ID_TOOLTOKEN_FILENAME: sText = _T("$FILENAME$"); break;
      case ID_TOOLTOKEN_SELECTION: sText = _T("$SELECTION$"); break;
      case ID_TOOLTOKEN_PROJECTNAME: sText = _T("$PROJECTNAME$"); break;
      case ID_TOOLTOKEN_PROJECTPATH: sText = _T("$PROJECTPATH$"); break;
      }
      ctrlEdit.ReplaceSel(sText);
      return 0;
   }

   LRESULT OnChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      int iIndex = m_ctrlList.GetSelectedIndex();
      if( iIndex < 0 ) return 0;
      if( wID == IDC_TITLE ) m_ctrlList.SetItemText(iIndex, 0, CWindowText(m_ctrlTitle));
      return 0;
   }

   LRESULT OnCbChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      _UpdateButtons();
      return 0;
   }

   LRESULT OnSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {      
      LPNMLISTVIEW pnmlv = (LPNMLISTVIEW) pnmh;
      if( pnmlv->iItem < 0 ) {
         m_ctrlTitle.SetWindowText(_T(""));
         m_ctrlCommand.SetWindowText(_T(""));
         m_ctrlArguments.SetWindowText(_T(""));
         m_ctrlPath.SetWindowText(_T(""));
         m_ctrlType.SetCurSel(0);
         m_ctrlPromptArgs.SetCheck(BST_UNCHECKED);
         m_ctrlConsoleOutput.SetCheck(BST_UNCHECKED);
      }
      else if( LISTITEM_SELECTED(pnmlv) ) {
         TOOL* pTool = (TOOL*) pnmlv->lParam;
         m_ctrlTitle.SetWindowText(pTool->sTitle);
         m_ctrlCommand.SetWindowText(pTool->sCommand);
         m_ctrlArguments.SetWindowText(pTool->sArguments);
         m_ctrlPath.SetWindowText(pTool->sPath);
         if( pTool->sType == _T("local") ) m_ctrlType.SetCurSel(TYPE_LOCAL);
         if( pTool->sType == _T("compiler") ) m_ctrlType.SetCurSel(TYPE_COMPILER);
         if( pTool->sType == _T("debugger") ) m_ctrlType.SetCurSel(TYPE_DEBUGGER);
         if( pTool->sType == _T("sql") ) m_ctrlType.SetCurSel(TYPE_SQL);
         m_ctrlPromptArgs.SetCheck( (pTool->lFlags & TOOLFLAGS_PROMPTARGS) != 0 ? BST_CHECKED : BST_UNCHECKED);
         m_ctrlConsoleOutput.SetCheck( (pTool->lFlags & TOOLFLAGS_CONSOLEOUTPUT) != 0 ? BST_CHECKED : BST_UNCHECKED);
      }
      else if( LISTITEM_UNSELECTED(pnmlv) ) {
         TOOL* pTool = (TOOL*) pnmlv->lParam;
         _BuildToolStructure(pTool);
      }
      _UpdateButtons();
      return 0;
   }

   // Implementation

   void _LoadItems()
   {
      CRegSerializer arc;
      int i = 1;
      if( arc.Open(REG_BVRDE _T("\\Tools")) ) {
         while( true ) {
            CString sKey;
            sKey.Format(_T("Tool%ld"), i++);
            if( !arc.ReadGroupBegin(sKey) ) break;

            TOOL& tool = m_aTools[m_nTools];
            TCHAR szBuffer[300] = { 0 };
            arc.Read(_T("title"), szBuffer, 299);
            tool.sTitle = szBuffer;
            arc.Read(_T("command"), szBuffer, 299);
            tool.sCommand = szBuffer;
            arc.Read(_T("arguments"), szBuffer, 299);
            tool.sArguments = szBuffer;
            arc.Read(_T("path"), szBuffer, 299);
            tool.sPath = szBuffer;
            arc.Read(_T("type"), szBuffer, 299);
            tool.sType = szBuffer;
            arc.Read(_T("flags"), tool.lFlags);
         
            int iItem = m_ctrlList.InsertItem(m_ctrlList.GetItemCount(), tool.sTitle);
            m_ctrlList.SetItemData(iItem, (LPARAM) &tool);
         
            m_nTools++;
            arc.ReadGroupEnd();
         }
         arc.Close();
      }
   }
   
   void _SaveItems()
   {
      // HACK: To commit latest changes!
      int iIndex = m_ctrlList.GetSelectedIndex();
      if( iIndex >= 0 ) {
         TOOL* pTool = (TOOL*) m_ctrlList.GetItemData(iIndex);
         _BuildToolStructure(pTool);
      }

      CRegSerializer arc;
      if( arc.Create(REG_BVRDE _T("\\Tools")) ) {
         int i;
         for( i = 0; i < m_ctrlList.GetItemCount(); i++ ) {
            TOOL* pTool = (TOOL*) m_ctrlList.GetItemData(i);
            CString sKey;
            sKey.Format(_T("Tool%ld"), i + 1);
            if( arc.WriteGroupBegin(sKey) ) {
               arc.Write(_T("title"), pTool->sTitle);
               arc.Write(_T("command"), pTool->sCommand);
               arc.Write(_T("arguments"), pTool->sArguments);
               arc.Write(_T("path"), pTool->sPath);
               arc.Write(_T("type"), pTool->sType);
               arc.Write(_T("flags"), pTool->lFlags);
               arc.WriteGroupEnd();
            }
         }
         for( ; i < 20; i++ ) {
            CString sKey;
            sKey.Format(_T("Tool%ld"), i + 1);
            arc.Delete(sKey);
         }
         arc.Close();
      }
   }

   void _BuildToolStructure(TOOL* pTool)
   {
      ATLASSERT(pTool);
      long lFlags = 0;
      if( m_ctrlPromptArgs.GetCheck() == BST_CHECKED ) lFlags |= TOOLFLAGS_PROMPTARGS;
      if( m_ctrlConsoleOutput.GetCheck() == BST_CHECKED ) lFlags |= TOOLFLAGS_CONSOLEOUTPUT;
      CString sType;
      switch( m_ctrlType.GetCurSel() ) {
      case TYPE_LOCAL:    sType = _T("local"); break;
      case TYPE_COMPILER: sType = _T("compiler"); break;
      case TYPE_DEBUGGER: sType = _T("debugger"); break;
      case TYPE_SQL:      sType = _T("sql"); break;
      }
      pTool->sTitle = CWindowText(m_ctrlTitle);
      pTool->sCommand = CWindowText(m_ctrlCommand);
      pTool->sArguments = CWindowText(m_ctrlArguments);
      pTool->sPath = CWindowText(m_ctrlPath);
      pTool->lFlags = lFlags;
      pTool->sType = sType;
   }

   void _UpdateButtons()
   {
      int nCount = m_ctrlList.GetItemCount();
      int iSelIndex = m_ctrlList.GetSelectedIndex();
      int nSelCount = m_ctrlList.GetSelectedCount();
      int iType = m_ctrlType.GetCurSel();
      m_ctrlNew.EnableWindow(nCount < 8 && m_nTools < sizeof(m_aTools)/sizeof(TOOL));
      m_ctrlDelete.EnableWindow(nSelCount > 0);
      m_ctrlUp.EnableWindow(nCount > 1 && nSelCount > 0 && iSelIndex > 0);
      m_ctrlDown.EnableWindow(nCount > 1 && nSelCount > 0 && iSelIndex < nCount - 1);
      m_ctrlTitle.EnableWindow(nSelCount > 0);
      m_ctrlCommand.EnableWindow(nSelCount > 0);
      m_ctrlArguments.EnableWindow(nSelCount > 0);
      m_ctrlPath.EnableWindow(nSelCount > 0);
      m_ctrlBrowseArguments.EnableWindow(nSelCount > 0);
      m_ctrlBrowseFile.EnableWindow(iType == TYPE_LOCAL && nSelCount > 0);
      m_ctrlBrowsePath.EnableWindow(iType == TYPE_LOCAL && nSelCount > 0);
      m_ctrlType.EnableWindow(nSelCount > 0);
      m_ctrlConsoleOutput.EnableWindow(nSelCount > 0);
      m_ctrlPromptArgs.EnableWindow(nSelCount > 0);
   }
};


#endif // !defined(AFX_EXTERNALTOOLSDLG_H__20030601_1EF9_3180_E184_0080AD509054__INCLUDED_)

