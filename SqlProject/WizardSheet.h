#if !defined(AFX_WIZARDSHEET_H__20030727_68AF_3774_8663_0080AD509054__INCLUDED_)
#define AFX_WIZARDSHEET_H__20030727_68AF_3774_8663_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


///////////////////////////////////////////////////////////////
// 

class CSqlProject;


///////////////////////////////////////////////////////////////
//

class CProviderPage : public CPropertyPageImpl<CProviderPage>
{
friend CSqlProject;
public:
   enum { IDD = IDD_WIZARD_PROVIDERS };

   CSqlProject* m_pProject;

   CListBox m_ctrlList;
   CSimpleArray<CString> m_aProviders;

   // Overloads

   int OnSetActive();
   int OnWizardNext();
   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CProviderPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_CODE_HANDLER(LBN_DBLCLK, OnListDblClick)
      CHAIN_MSG_MAP( CPropertyPageImpl<CProviderPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnListDblClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};


///////////////////////////////////////////////////////////////
//

class CDefaultConnectionDlg : public CDialogImpl<CDefaultConnectionDlg>
{
friend CSqlProject;
public:
   enum { IDD = IDD_WIZARD_CONNECTION };

   CSqlProject* m_pProject;

   CEdit m_ctrlServer;
   CEdit m_ctrlDatabase;
   CEdit m_ctrlUsername;
   CEdit m_ctrlPassword;
   CEdit m_ctrlCatalog;

   // Operations

   int OnApply();
   int OnWizardNext();
   int OnSetActive();

   // Implementation

   CComVariant _GetProperty(const GUID& guid, DBPROPID propid) const;
   bool _HasProperty(const GUID& guid, DBPROPID propid) const;

   // Message map and handlers

   BEGIN_MSG_MAP(CDefaultConnectionDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


///////////////////////////////////////////////////////////////
//

class CConnectionPage :
   public CPropertyPageImpl<CConnectionPage>,
   public IPropertyPageSite,
   public IPropertyBag
{
friend CSqlProject;
public:
   enum { IDD = IDD_WIZARD_EMPTY };

   CSqlProject* m_pProject;

   CWindow m_wndClient;
   CDefaultConnectionDlg m_wndDefault;
   CComPtr<IPropertyPage> m_spPage;

   // IUnknown methods

   STDMETHOD(QueryInterface)(REFIID, LPVOID*);
   STDMETHOD_(ULONG, AddRef)(void);
   STDMETHOD_(ULONG, Release)(void);

   // IPropertyPageSite

   STDMETHOD(OnStatusChange)(DWORD dwFlags);
   STDMETHOD(GetLocaleID)(LCID* pLocaleID);
   STDMETHOD(GetPageContainer)(IUnknown** ppUnk);
   STDMETHOD(TranslateAccelerator)(LPMSG pMsg);

   // IPropertyBag

   STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog);
   STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT* pVar);

   // Overloads

   int OnSetActive();
   int OnWizardNext();
   int OnApply();
   int OnTranslateAccelerator(LPMSG lpMsg);
 
   // Message map and handlers

   BEGIN_MSG_MAP(CConnectionPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      CHAIN_MSG_MAP( CPropertyPageImpl<CConnectionPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
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
      COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClick)
      CHAIN_MSG_MAP( CPropertyPageImpl<CAdvancedEditOptionsPage> )
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnButtonClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   // Implementation

   void _UpdateButtons();
};


#endif // !defined(AFX_WIZARDSHEET_H__20030727_68AF_3774_8663_0080AD509054__INCLUDED_)

