#if !defined(AFX_LOGINCVSDLG_H__20040612_0083_34E2_DF5E_0080AD509054__INCLUDED_)
#define AFX_LOGINCVSDLG_H__20040612_0083_34E2_DF5E_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Commands.h"


class CLoginCvsDlg : public CDialogImpl<CLoginCvsDlg>
{
public:
   enum { IDD = IDD_CVS_LOGIN };

   // Data Members

   CEdit m_ctrlUsername;
   CEdit m_ctrlPassword;
   CEdit m_ctrlPath;
   CEdit m_ctrlServer;
   CComboBox m_ctrlType;
   //
   CScCommands* m_pOwner;
   //
   CString m_sOptions;
   CString m_sPassword;

   // Operations

   // Message map and handlers

   BEGIN_MSG_MAP(CLoginCvsDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
      COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      m_ctrlUsername = GetDlgItem(IDC_USERNAME);
      m_ctrlPassword = GetDlgItem(IDC_PASSWORD);
      m_ctrlServer = GetDlgItem(IDC_SERVER);
      m_ctrlType = GetDlgItem(IDC_CTYPE);
      m_ctrlPath = GetDlgItem(IDC_PATH);

      CString sUsername;
      CString sPassword;
      CString sServer;
      CString sType;
      CString sPath;

      IProject* pProject = _pDevEnv->GetSolution()->GetActiveProject();
      if( pProject ) {
         CComDispatchDriver dd = pProject->GetDispatch();
         if( dd != NULL ) {
            CComVariant vUsername;
            CComVariant vPassword;
            CComVariant vServer;
            CComVariant vPath;
            dd.GetPropertyByName(L"Username", &vUsername);
            dd.GetPropertyByName(L"Password", &vPassword);
            dd.GetPropertyByName(L"Server", &vServer);
            dd.GetPropertyByName(L"CurDir", &vPath);
            sUsername = vUsername.bstrVal;
            sPassword = vPassword.bstrVal;
            sServer = vServer.bstrVal;
            sPath = vPath.bstrVal;
         }
      }

      m_ctrlUsername.SetWindowText(sUsername);
      m_ctrlPassword.SetWindowText(sPassword);
      m_ctrlServer.SetWindowText(sServer);
      m_ctrlPath.SetWindowText(sPath);

      m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_TYPE_LOCAL)));
      m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_TYPE_PSERVER)));
      m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_TYPE_EXT)));
      m_ctrlType.AddString(CString(MAKEINTRESOURCE(IDS_TYPE_SERVER)));    
      m_ctrlType.SetCurSel(0);

      _UpdateButtons();
      return 0;
   }
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      m_sOptions.Empty();
      CString sTemp;
      sTemp.Format(_T(" -d %s"), _GetFullCvsRoot());
      m_sOptions += sTemp;
      m_sPassword = CWindowText(m_ctrlPassword);

      EndDialog(wID);
      return 0;
   }
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      if( wID != IDC_CONNECT ) _UpdateButtons();
      return 0;
   }

   // Implementation

   void _UpdateButtons()
   {
      CWindow(GetDlgItem(IDC_CONNECT)).SetWindowText(_GetFullCvsRoot());
   }
   CString _GetFullCvsRoot() const
   {
      CString sUsername = CWindowText(m_ctrlUsername);
      CString sPassword = CWindowText(m_ctrlPassword);
      CString sServer = CWindowText(m_ctrlServer);
      CString sPath = CWindowText(m_ctrlPath);
      CString sType = CWindowText(m_ctrlType);

      CString sCvsRoot;
      if( sType == _T("local") ) {
         sCvsRoot.Format(_T("%s:%s"), sType, sPath);
      }
      else {
         sCvsRoot.Format(_T("%s:%s@%s:%s"), sType, sUsername, sServer, sPath);
      }
      return sCvsRoot;
   }
};


#endif // !defined(AFX_LOGINCVSDLG_H__20040612_0083_34E2_DF5E_0080AD509054__INCLUDED_)

