#if !defined(AFX_ATTACHCOREFILEDLG_H__20080302_83F8_0280_6E85_0080AD509054__INCLUDED_)
#define AFX_ATTACHCOREFILEDLG_H__20080302_83F8_0280_6E85_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RemoteFileDlg.h"


class CAttachCoreFileDlg : 
   public CDialogImpl<CAttachCoreFileDlg>,
   public COwnerDraw<CAttachCoreFileDlg>
{
public:
   enum { IDD = IDD_ATTACHCORE };

   CEdit m_ctrlProcess;
   CEdit m_ctrlCoreFile;

   CRemoteProject* m_pProject;
   CString m_sProcess;
   CString m_sCoreFile;

   void Init(CRemoteProject* pProject, LPCTSTR pstrProcess, LPCTSTR pstrCoreFile)
   {
      m_pProject = pProject;
      m_sProcess = pstrProcess;
      m_sCoreFile = pstrCoreFile;

      // Create a default process name if none is given...
      if( m_sProcess.IsEmpty() ) {
         TCHAR szProjectName[128] = { 0 };
         m_pProject->GetName(szProjectName, 127);
         m_sProcess.Format(_T("./%s"), szProjectName);
      }
   }
   CString GetSelectedProcess() const
   {
      return m_sProcess;
   }
   CString GetSelectedCoreFile() const
   {
      return m_sCoreFile;
   }

   BEGIN_MSG_MAP(CAttachProcessDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnOk)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_ID_HANDLER(IDC_BROWSE_PROCESS, OnBrowseProcess);
      COMMAND_ID_HANDLER(IDC_BROWSE_COREFILE, OnBrowseCoreFile);
      COMMAND_CODE_HANDLER(EN_CHANGE, OnChange);
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlProcess = GetDlgItem(IDC_PROCESS);
      m_ctrlCoreFile = GetDlgItem(IDC_COREFILE);

      m_ctrlProcess.SetWindowText(m_sProcess);
      m_ctrlCoreFile.SetWindowText(m_sCoreFile);

      BOOL bDummy = FALSE;
      OnChange(0, 0, NULL, bDummy);
      return 0;
   }
   LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      m_sProcess = CWindowText(m_ctrlProcess);
      m_sCoreFile = CWindowText(m_ctrlCoreFile);
      EndDialog(wID);
      return 0;
   }
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {      
      EndDialog(wID);
      return 0;
   }
   LRESULT OnBrowseProcess(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CString sFilter(MAKEINTRESOURCE(IDS_FILTER_FILES));
      for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == '|' ) sFilter.SetAt(i, '\0');
      CString sRemotePath = m_pProject->m_FileManager.GetCurPath();
      CRemoteFileDlg dlg(TRUE, &m_pProject->m_FileManager, _T(""), OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST, sFilter);
      dlg.m_ofn.lpstrInitialDir = sRemotePath;
      if( dlg.DoModal(m_hWnd) != IDOK ) return 0;
      CString sSeparator = m_pProject->m_FileManager.GetParam(_T("Separator"));
      CString sFilename = dlg.m_ofn.lpstrFile;
      if( sFilename.Right(1) != sSeparator ) sFilename += sSeparator;
      sFilename += dlg.m_ofn.lpstrFile + _tcslen(dlg.m_ofn.lpstrFile) + 1;
      m_ctrlProcess.SetWindowText(sFilename);
      return 0;
   }
   LRESULT OnBrowseCoreFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CString sFilter(MAKEINTRESOURCE(IDS_FILTER_FILES));
      for( int i = 0; i < sFilter.GetLength(); i++ ) if( sFilter[i] == '|' ) sFilter.SetAt(i, '\0');
      CString sRemotePath = m_pProject->m_FileManager.GetCurPath();
      CRemoteFileDlg dlg(TRUE, &m_pProject->m_FileManager, _T(""), OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST, sFilter);
      dlg.m_ofn.lpstrInitialDir = sRemotePath;
      if( dlg.DoModal(m_hWnd) != IDOK ) return 0;
      CString sSeparator = m_pProject->m_FileManager.GetParam(_T("Separator"));
      CString sFilename = dlg.m_ofn.lpstrFile;
      if( sFilename.Right(1) != sSeparator ) sFilename += sSeparator;
      sFilename += dlg.m_ofn.lpstrFile + _tcslen(dlg.m_ofn.lpstrFile) + 1;
      m_ctrlCoreFile.SetWindowText(sFilename);
      return 0;
   }
   LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {      
      BOOL bEnabled = TRUE;
      if( m_ctrlProcess.GetWindowTextLength() == 0 ) bEnabled = FALSE;
      if( m_ctrlCoreFile.GetWindowTextLength() == 0 ) bEnabled = FALSE;
      ::EnableWindow(GetDlgItem(IDOK), bEnabled);
      return 0;
   }
};


#endif // !defined(AFX_ATTACHCOREFILEDLG_H__20080302_83F8_0280_6E85_0080AD509054__INCLUDED_)

