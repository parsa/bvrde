#if !defined(AFX_REBUILDLEXDLG_H__20070815_C3BC_5334_4C69_0080AD509054__INCLUDED_)
#define AFX_REBUILDLEXDLG_H__20070815_C3BC_5334_4C69_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


////////////////////////////////////////////////////////
//

#define WM_APP_REBUILDLEX_POS       WM_USER + 100
#define WM_APP_REBUILDLEX_FILENAME  WM_USER + 101


////////////////////////////////////////////////////////
//

class CRebuildLexThread : public CThreadImpl<CRebuildLexThread>
{
public:
   CRemoteProject* pProject;         // Reference to project
   CWindow wndParent;                // Reference to dialog
   TCHAR szFilename[MAX_PATH + 1];   // These are defined pr instance because the name-part is
   LPCTSTR pstrNamePart;             // ...used in both thread and dialog (points to szFilename)

   DWORD Run()
   {
      ::ZeroMemory(szFilename, sizeof(szFilename));
      for( int i = 0; i < pProject->GetItemCount(); i++ ) {
         IView* pFile = pProject->GetItem(i);
         if( pFile == NULL ) break;
         pFile->GetFileName(szFilename, MAX_PATH);
         pstrNamePart = ::PathFindFileName(szFilename);
         wndParent.PostMessage(WM_APP_REBUILDLEX_FILENAME);
         CString sFileType = GetFileTypeFromFilename(szFilename);
         if( sFileType == _T("cpp") || sFileType == _T("header") ) {
            CComBSTR bstrText;
            pFile->GetText(&bstrText);
            int nLen = bstrText.Length();
            if( nLen == 0 ) continue;
            LPSTR pstrData = (LPSTR) malloc((nLen * 2) + 1);
            ATLASSERT(pstrData);
            if( pstrData == NULL ) return 0;
            AtlW2AHelper(pstrData, bstrText, (nLen * 2) + 1);
            if( !pProject->m_TagManager.m_LexInfo.MergeFile(szFilename, pstrData, 1) ) free(pstrData);
         }
         wndParent.PostMessage(WM_APP_REBUILDLEX_POS, i + 1);
         if( ShouldStop() ) break;
      }
      wndParent.PostMessage(WM_COMMAND, MAKEWPARAM(IDCANCEL, 0));
      return 0;
   }
};


////////////////////////////////////////////////////////
//

class CRebuildLexDlg : 
   public CDialogImpl<CRebuildLexDlg>
{
public:
   enum { IDD = IDD_REBUILDLEX };

   CRemoteProject* m_pProject;
   CRebuildLexThread m_thread;

   CFont m_fontBold;
   CStatic m_ctrlFilename;
   CProgressBarCtrl m_ctrlProgress;

   void Init(CRemoteProject* pProject)
   {
      m_pProject = pProject;
   }

   BEGIN_MSG_MAP(CRebuildLexDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_APP_REBUILDLEX_POS, OnAppMsgPos)
      MESSAGE_HANDLER(WM_APP_REBUILDLEX_FILENAME, OnAppMsgFilename)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ATLASSERT(m_pProject);
      CenterWindow();
      CLogFont lf = AtlGetDefaultGuiFont();
      lf.MakeBolder();
      m_fontBold.CreateFontIndirect(&lf);
      CWindow(GetDlgItem(IDC_TITLE)).SetFont(m_fontBold);
      m_ctrlFilename = GetDlgItem(IDC_FILENAME);
      m_ctrlProgress = GetDlgItem(IDC_PROGRESS);
      m_ctrlFilename.SetWindowText(_T(""));
      m_ctrlProgress.SetRange(0, m_pProject->GetItemCount());
      m_thread.pProject = m_pProject;
      m_thread.wndParent = m_hWnd;
      m_thread.Start();
      return FALSE;
   }

   LRESULT OnAppMsgPos(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlProgress.SetPos(wParam);
      return 0;
   }
   
   LRESULT OnAppMsgFilename(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlFilename.SetWindowText(m_thread.pstrNamePart);
      return 0;
   }
   
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      m_thread.SignalStop();
      m_thread.Stop();
      EndDialog(wID);
      return 0;
   }
};


#endif // !defined(AFX_REBUILDLEXDLG_H__20070815_C3BC_5334_4C69_0080AD509054__INCLUDED_)

