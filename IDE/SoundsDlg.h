#if !defined(AFX_SOUNDSDLG_H__20050306_DDBB_C137_CE60_0080AD509054__INCLUDED_)
#define AFX_SOUNDSDLG_H__20050306_DDBB_C137_CE60_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CSoundsDlg :
   public CPropertyPageImpl<CSoundsDlg>
{
public:
   enum { IDD = IDD_CONFIG_SOUNDS };

   CMainFrame* m_pMainFrame;
   HWND m_hWndSheet;

   void Init(HWND hWndSheet, CMainFrame* pMainFrame)
   {
      m_hWndSheet = hWndSheet;
      m_pMainFrame = pMainFrame;
   }

   BEGIN_MSG_MAP(CSoundsDlg)
      COMMAND_ID_HANDLER(IDC_LAUNCH, OnLaunch)
      CHAIN_MSG_MAP( CPropertyPageImpl<CSoundsDlg> )
   END_MSG_MAP()

   LRESULT OnLaunch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CWaitCursor cursor;
      DWORD dwMajorVersion = 0;
      DWORD dwMinorVersion = 0;
      AtlGetShellVersion(&dwMajorVersion, &dwMinorVersion);
      TCHAR szCommand[200] = { 0 };
      ::wsprintf(szCommand, _T("shell32.dll,Control_RunDLL mmsys.cpl,,%ld"), dwMajorVersion < 6 ? 0L : 1L);
      ::ShellExecute(m_pMainFrame->m_hWnd, _T("open"), _T("rundll32.exe"), szCommand, NULL, SW_SHOWNORMAL);
      return 0;
   }
};


#endif // !defined(AFX_SOUNDSDLG_H__20050306_DDBB_C137_CE60_0080AD509054__INCLUDED_)

