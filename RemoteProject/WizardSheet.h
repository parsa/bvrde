#if !defined(AFX_WIZARDSHEET_H__20030402_84AB_8C67_2CD0_0080AD509054__INCLUDED_)
#define AFX_WIZARDSHEET_H__20030402_84AB_8C67_2CD0_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropertyList.h"


///////////////////////////////////////////////////////////////
//

class CRemoteProject;


///////////////////////////////////////////////////////////////
//

class CFileTransferPage : public CPropertyPageImpl<CFileTransferPage>
{
public:
   enum { IDD = IDD_WIZARD_FILETRANSFER };

   CRemoteProject* m_pProject;

   CComboBox m_ctrlType;
   CEdit m_ctrlHost;
   CEdit m_ctrlPort;
   CEdit m_ctrlUsername;
   CEdit m_ctrlPassword;
   CEdit m_ctrlPath;
   CButton m_ctrlTest;

   // Overloads

   int OnSetActive();
	int OnWizardNext();
   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CFileTransferPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDC_TEST, OnTest)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnTextChange)
      COMMAND_HANDLER(IDC_TYPE, CBN_SELCHANGE, OnTypeChange)
      CHAIN_MSG_MAP( CPropertyPageImpl<CFileTransferPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnTest(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnTypeChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnTextChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};


///////////////////////////////////////////////////////////////
//

class CFileOptionsPage : public CPropertyPageImpl<CFileOptionsPage>
{
public:
   enum { IDD = IDD_WIZARD_COMMANDS };

   CRemoteProject* m_pProject;

   CPropertyListCtrl m_ctrlList;

   // Overloads

   int OnSetActive();
   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CFileOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      CHAIN_MSG_MAP( CPropertyPageImpl<CFileOptionsPage> )
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


///////////////////////////////////////////////////////////////
//

class CCompilerPage : public CPropertyPageImpl<CCompilerPage>
{
public:
   enum { IDD = IDD_WIZARD_APPSTART };

   CRemoteProject* m_pProject;

   CComboBox m_ctrlType;
   CEdit m_ctrlHost;
   CEdit m_ctrlPort;
   CEdit m_ctrlUsername;
   CEdit m_ctrlPassword;
   CEdit m_ctrlPath;
   CEdit m_ctrlExtra;
   CButton m_ctrlTest;

   // Overloads

   int OnSetActive();
   int OnWizardNext();
   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CCompilerPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDC_TEST, OnTest)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnTextChange)
      COMMAND_HANDLER(IDC_TYPE, CBN_SELCHANGE, OnTypeChange)
      CHAIN_MSG_MAP( CPropertyPageImpl<CCompilerPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnTest(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnTypeChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnTextChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};


///////////////////////////////////////////////////////////////
//

class CCompilerCommandsPage : public CPropertyPageImpl<CCompilerCommandsPage>
{
public:
   enum { IDD = IDD_WIZARD_COMMANDS };

   CRemoteProject* m_pProject;

   CPropertyListCtrl m_ctrlList;

   // Overloads

   int OnSetActive();
   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CCompilerCommandsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      CHAIN_MSG_MAP( CPropertyPageImpl<CCompilerCommandsPage> )
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


///////////////////////////////////////////////////////////////
//

class CDebuggerPage : public CPropertyPageImpl<CDebuggerPage>
{
public:
   enum { IDD = IDD_WIZARD_DEBUGGER };

   CRemoteProject* m_pProject;

   CComboBox m_ctrlDebugger;
   CComboBox m_ctrlType;
   CEdit m_ctrlHost;
   CEdit m_ctrlPort;
   CEdit m_ctrlUsername;
   CEdit m_ctrlPassword;
   CEdit m_ctrlPath;
   CEdit m_ctrlExtra;
   CButton m_ctrlTest;

   // Overloads

   int OnSetActive();
   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CDebuggerPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDC_TEST, OnTest)
      COMMAND_HANDLER(IDC_TYPE, CBN_SELCHANGE, OnTypeChange)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnTextChange)
      CHAIN_MSG_MAP( CPropertyPageImpl<CDebuggerPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnTypeChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnTextChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnTest(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};


///////////////////////////////////////////////////////////////
//

class CDebuggerCommandsPage : public CPropertyPageImpl<CDebuggerCommandsPage>
{
public:
   enum { IDD = IDD_WIZARD_COMMANDS };

   CRemoteProject* m_pProject;

   CPropertyListCtrl m_ctrlList;

   // Overloads

   int OnSetActive();
   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CDebuggerCommandsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      CHAIN_MSG_MAP( CPropertyPageImpl<CDebuggerCommandsPage> )
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


////////////////////////////////////////////////////////////////////////
//

class CAdvancedEditOptionsPage : public CPropertyPageImpl<CAdvancedEditOptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_ADVANCED };

   ISerializable* m_pArc;
   CString m_sLanguage;

   // Overloads

   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CAdvancedEditOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      CHAIN_MSG_MAP( CPropertyPageImpl<CAdvancedEditOptionsPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


#endif // !defined(AFX_WIZARDSHEET_H__20030402_84AB_8C67_2CD0_0080AD509054__INCLUDED_)

