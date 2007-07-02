#if !defined(AFX_MACROSDLG_H__20030713_E0EE_40E2_C393_0080AD509054__INCLUDED_)
#define AFX_MACROSDLG_H__20030713_E0EE_40E2_C393_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define SCRIPT_COMMENT '\''


class CMacrosDlg :
   public CDialogImpl<CMacrosDlg>
{
public:
   enum { IDD = IDD_CONFIG_MACROS };

   CListBox m_ctrlList;
   CComboBox m_ctrlFiles;
   CEdit m_ctrlDescription;
   CBrush m_brBack;
   COLORREF m_clrBack;

   CSimpleArray<CString> m_aFiles;
   CSimpleArray<CString> m_aNames;
   CSimpleArray<CString> m_aDescriptions;
   CString m_sFileDescription;
   CString m_sSelFile;
   CString m_sSelName;

   void GetSelection(CString& sFile, CString& sName)
   {
      sFile = m_sSelFile;
      sName = m_sSelName;
   }

   BEGIN_MSG_MAP(CMacrosDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnCreate)
      MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, OnColorListBox)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_HANDLER(IDC_LIST, LBN_SELCHANGE, OnListChange)
      COMMAND_HANDLER(IDC_LIST, LBN_DBLCLK, OnDblClick)
      COMMAND_HANDLER(IDC_FILES, CBN_SELCHANGE, OnFileChange)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_clrBack = BlendRGB(::GetSysColor(COLOR_WINDOW), RGB(0,0,0), 6);
      m_brBack.CreateSolidBrush(m_clrBack);

      m_ctrlList = GetDlgItem(IDC_LIST);
      m_ctrlFiles = GetDlgItem(IDC_FILES);
      m_ctrlDescription = GetDlgItem(IDC_DESCRIPTION);

      CString sPattern;
      sPattern.Format(_T("%s*.VBS"), CModulePath());
      CFindFile ff;
      for( BOOL bRes = ff.FindFile(sPattern); bRes; bRes = ff.FindNextFile() ) {
         CString s = ff.GetFilePath();
         m_aFiles.Add(s);
         int iItem = m_ctrlFiles.AddString(ff.GetFileName());
         m_ctrlFiles.SetItemData(iItem, m_ctrlFiles.GetCount() - 1);
      }
      m_ctrlFiles.SetCurSel(0);

      BOOL bDummy;
      OnFileChange(0, 0, NULL, bDummy);

      return TRUE;
   }
   LRESULT OnColorListBox(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CDCHandle dc = (HDC) wParam;
      dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
      dc.SetBkColor(m_clrBack);
      return (LRESULT) (HBRUSH) m_brBack;
   }
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnDblClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      PostMessage(WM_COMMAND, MAKELPARAM(IDOK, 0));
      return 0;
   }
   LRESULT OnListChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      int iSel = m_ctrlFiles.GetCurSel();
      if( iSel < 0 ) return 0;
      iSel = m_ctrlFiles.GetItemData(iSel);
      m_sSelFile = m_aFiles[iSel];
      iSel = m_ctrlList.GetCurSel();
      if( iSel < 0 ) return 0;
      m_ctrlList.GetText(iSel, m_sSelName);
      iSel = m_ctrlList.GetItemData(iSel);
      if( iSel < 0 ) return 0;
      m_ctrlDescription.SetWindowText(m_aDescriptions[iSel]);

      ::EnableWindow(GetDlgItem(IDOK), TRUE);
      return 0;
   }
   LRESULT OnFileChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CWaitCursor cursor;

      int iSel = m_ctrlFiles.GetCurSel();
      if( iSel < 0 ) return 0;
      iSel = m_ctrlFiles.GetItemData(iSel);     

      GetFileInfo(m_aFiles[iSel], m_sFileDescription, m_aNames, m_aDescriptions);

      m_ctrlList.ResetContent();
      for( int i = 0; i < m_aNames.GetSize(); i++ ) {
         int iItem = m_ctrlList.AddString(m_aNames[i]);
         m_ctrlList.SetItemData(iItem, i);
      }

      m_ctrlDescription.SetWindowText(m_sFileDescription);
      return 0;
   }

   // Operations

   static bool GetFileInfo(LPCTSTR pstrFilename,
                           CString& sFileDescription, 
                           CSimpleArray<CString>& aNames, 
                           CSimpleArray<CString>& aDescriptions)
   {
      CFile f;
      if( !f.Open(pstrFilename) ) {
         HWND hWnd = g_pDevEnv->GetHwnd(IDE_HWND_MAIN);
         g_pDevEnv->ShowMessageBox(hWnd, CString(MAKEINTRESOURCE(IDS_ERR_FILELOCKED)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
         return false;
      }
      DWORD dwSize = f.GetSize();
      LPSTR pData = (LPSTR) malloc(dwSize + 1);
      if( pData == NULL ) return 0;
      f.Read(pData, dwSize);
      pData[dwSize] = '\0';
      CString sText = pData;
      free(pData);

      sFileDescription.Empty();
      aNames.RemoveAll();
      aDescriptions.RemoveAll();

      CString sDescription;
      bool bSeenCode = false;

      while( !sText.IsEmpty() ) {
         CString sLine = sText;
         int iPos = sText.Find('\n');
         if( iPos >= 0 ) sLine = sText.Left(iPos);

         if( sLine.GetLength() > 0 && sLine[0] == _T(SCRIPT_COMMENT) ) {
            CString sPart = sLine.Mid(1);
            sPart.TrimLeft();
            sPart.TrimRight(_T("\t \r\n"));
            sDescription += sPart;
            sDescription += _T("\r\n");
         }
         else if( _tcsnicmp(sLine, _T("SUB "), 4) == 0 ) {
            CString sName = sLine.Mid(4);
            sName.TrimLeft();
            iPos = sName.FindOneOf(_T("('\r\n"));
            if( iPos >= 0 ) sName = sName.Left(iPos);
            sDescription.TrimRight(_T("\r\n\t "));
            aNames.Add(sName);
            aDescriptions.Add(sDescription);
         }
         else {
            if( !bSeenCode ) sFileDescription = sDescription;
            sDescription.Empty();
            bSeenCode = true;
         }
         
         sText = sText.Mid(sLine.GetLength() + 1);
      }

      return true;
   }
};


#endif // !defined(AFX_MACROSDLG_H__20030713_E0EE_40E2_C393_0080AD509054__INCLUDED_)

