
#include "stdafx.h"
#include "resource.h"

#include "WizardSheet.h"
#include "Project.h"
#include "Files.h"

#pragma code_seg( "WIZARDS" )


////////////////////////////////////////////////////////////////////////
//

#define SET_CHECK(id, prop) \
   { TCHAR szBuf[32] = { 0 }; _pDevEnv->GetProperty(prop, szBuf, 31); \
     if( _tcscmp(szBuf, _T("false"))!=0 ) CButton(GetDlgItem(id)).Click(); }

#define GET_CHECK(id, prop) \
   _pDevEnv->SetProperty(prop, CButton(GetDlgItem(id)).GetCheck() == BST_CHECKED ? _T("true") : _T("false"))

#define TRANSFER_PROP(name, prop) \
   { TCHAR szBuf[32] = { 0 }; _pDevEnv->GetProperty(prop, szBuf, 31); \
     m_pArc->Write(name, szBuf); }


///////////////////////////////////////////////////////////////
//

LRESULT CProviderPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   _ASSERTE(m_pProject);

   m_ctrlList = GetDlgItem(IDC_LIST);
   
   CComPtr<ISourcesRowset> spSR;
   HRESULT Hr = spSR.CoCreateInstance(CLSID_OLEDB_ENUMERATOR);
   if( FAILED(Hr) ) return 0;
   CComPtr<IRowset> spRS;
   Hr = spSR->GetSourcesRowset(NULL, IID_IRowset, 0, NULL, (IUnknown**) &spRS);
   if( FAILED(Hr) ) return 0;

   COledbRecordset rec = spRS;
   while( !rec.IsEOF() ) {
      long lType;
      rec.GetField(3, lType);
      if( lType != DBSOURCETYPE_ENUMERATOR ) {
         CString sName;
         CString sParseName;
         CString sDescription;
         rec.GetField(0, sName);
         rec.GetField(1, sParseName);
         rec.GetField(2, sDescription);
         m_aProviders.Add(sParseName);
         int iItem = m_ctrlList.AddString(sDescription);
         m_ctrlList.SetItemData(iItem, m_aProviders.GetSize() - 1);
      }
      rec.MoveNext();
   }
   rec.Close();

   m_ctrlList.SetCurSel(0);
   return 0;
}

int CProviderPage::OnSetActive()
{
   SetWizardButtons(GetPropertySheet().GetActiveIndex() == 0 ? PSWIZB_NEXT : PSWIZB_BACK|PSWIZB_NEXT);
   return 0;
}

int CProviderPage::OnWizardNext()
{  
   if( m_pProject->m_pWizardView ) delete m_pProject->m_pWizardView;
   m_pProject->m_pWizardView = new CView(m_pProject, m_pProject);

   CComBSTR bstrProvider = m_aProviders[ m_ctrlList.GetItemData(m_ctrlList.GetCurSel()) ];
   CLSID clsid = { 0 };
   ::CLSIDFromString(bstrProvider, &clsid);
   HRESULT Hr = m_pProject->m_pWizardView->m_Db.m_spInit.CoCreateInstance(clsid);
   if( FAILED(Hr) ) return -1;

   const nProps = 2;
   DBPROP Prop[nProps];
   DBPROPSET PropSet;
   // Initialize common property options.
   for( ULONG i = 0; i < nProps; i++ ) {
      ::VariantInit(&Prop[i].vValue);
      Prop[i].dwOptions = DBPROPOPTIONS_REQUIRED;
      Prop[i].colid = DB_NULLID;
   }
   // Level of prompting that will be done to complete the connection process
   Prop[0].dwPropertyID = DBPROP_INIT_PROMPT;
   Prop[0].vValue.vt = VT_I2;
   Prop[0].vValue.iVal = DBPROMPT_COMPLETE;
   Prop[1].dwPropertyID = DBPROP_INIT_HWND;
   Prop[1].vValue.vt = VT_I4;
   Prop[1].vValue.lVal = (long) (HWND) m_pProject->m_wndMain; 

   // Prepare properties
   PropSet.guidPropertySet = DBPROPSET_DBINIT;
   PropSet.cProperties = nProps;
   PropSet.rgProperties = Prop;
   CComQIPtr<IDBProperties> spProperties = m_pProject->m_pWizardView->m_Db.m_spInit;
   Hr = spProperties->SetProperties(1, &PropSet);

   // Before we check if it failed, clean up
   for( i = 0; i < nProps; i++ ) ::VariantClear(&Prop[i].vValue);

   return 0;
}

int CProviderPage::OnApply()
{
   ATLASSERT(m_pProject);

   // Attempt to connect
   if( !m_pProject->m_pWizardView->m_Db.Connect() ) {
      _pDevEnv->ShowMessageBox(m_hWnd, CString(MAKEINTRESOURCE(IDS_ERR_DBOPEN)), CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION);
      return PSNRET_INVALID_NOCHANGEPAGE;
   }

   // Extract Connection string
   CComPtr<IDataInitialize> spData;
   if( SUCCEEDED( spData.CoCreateInstance(CLSID_MSDAINITIALIZE) ) ) {
      LPOLESTR pwstr = NULL;
      spData->GetInitializationString(m_pProject->m_pWizardView->m_Db.m_spInit, VARIANT_TRUE, &pwstr);
      if( pwstr ) {
         m_pProject->m_pWizardView->m_sConnectString = pwstr;
         ::CoTaskMemFree(pwstr);
      }
   }

   m_pProject->m_aViews.Add(m_pProject->m_pWizardView);
   m_pProject->m_bIsDirty = true;
   m_pProject->m_pWizardView = NULL;

   return PSNRET_NOERROR;
}

LRESULT CProviderPage::OnListDblClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   if( m_ctrlList.GetCurSel() < 0 ) return 0;
   GetPropertySheet().PressButton(PSBTN_NEXT);
   return 0;
}


///////////////////////////////////////////////////////////////
//

LRESULT CDefaultConnectionDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   _ASSERTE(m_pProject);
   return 0;
}

int CDefaultConnectionDlg::OnWizardNext()
{
   CComBSTR bstrServer = CWindowText(GetDlgItem(IDC_SERVER));
   CComBSTR bstrDatabase = CWindowText(GetDlgItem(IDC_DATABASE));
   CComBSTR bstrUser = CWindowText(GetDlgItem(IDC_USERNAME));
   CComBSTR bstrPassword = CWindowText(GetDlgItem(IDC_PASSWORD));
   CComBSTR bstrCatalog = CWindowText(GetDlgItem(IDC_CATALOG));
   VARIANT_BOOL vbSavePassword = CButton(GetDlgItem(IDC_SAVEPASSWORD)).GetCheck() == BST_CHECKED ? VARIANT_TRUE : VARIANT_FALSE;

   const nProps = 6;
   DBPROP Prop[nProps];
   DBPROPSET PropSet;
   // Initialize common property options.
   for( ULONG i = 0; i < nProps; i++ ) {
      ::VariantInit(&Prop[i].vValue);
      Prop[i].dwOptions = DBPROPOPTIONS_REQUIRED;
      Prop[i].colid = DB_NULLID;
   }
   Prop[0].dwPropertyID = DBPROP_INIT_DATASOURCE;
   Prop[0].vValue.vt = VT_BSTR;
   Prop[0].vValue.bstrVal = bstrServer;
   Prop[1].dwPropertyID = DBPROP_AUTH_USERID;
   Prop[1].vValue.vt = VT_BSTR;
   Prop[1].vValue.bstrVal = bstrUser;
   Prop[2].dwPropertyID = DBPROP_AUTH_PASSWORD;
   Prop[2].vValue.vt = VT_BSTR;
   Prop[2].vValue.bstrVal = bstrPassword;
   Prop[3].dwPropertyID = DBPROP_INIT_LOCATION;
   Prop[3].vValue.vt = VT_BSTR;
   Prop[3].vValue.bstrVal = bstrDatabase;
   Prop[4].dwPropertyID = DBPROP_INIT_CATALOG;
   Prop[4].vValue.vt = VT_BSTR;
   Prop[4].vValue.bstrVal = bstrCatalog;
   Prop[5].dwPropertyID = DBPROP_AUTH_PERSIST_SENSITIVE_AUTHINFO;
   Prop[5].dwOptions = DBPROPOPTIONS_OPTIONAL;
   Prop[5].vValue.vt = VT_BOOL;
   Prop[5].vValue.boolVal = vbSavePassword;

   // Prepare properties
   PropSet.guidPropertySet = DBPROPSET_DBINIT;
   PropSet.cProperties = nProps;
   PropSet.rgProperties = Prop;
   CComQIPtr<IDBProperties> spProperties = m_pProject->m_pWizardView->m_Db.m_spInit;
   spProperties->SetProperties(1, &PropSet);

   // Before we check if it failed, clean up
   for( i = 0; i < nProps; i++ ) ::VariantClear(&Prop[i].vValue);

   CComVariant v = _GetProperty(DBPROPSET_DATASOURCEINFO, DBPROP_DATASOURCENAME);
   if( v.vt != VT_BSTR ) v = _GetProperty(DBPROPSET_DBINIT, DBPROP_INIT_DATASOURCE);
   if( v.vt != VT_BSTR ) v = CString(MAKEINTRESOURCE(IDS_UNKNOWN));
   m_pProject->m_pWizardView->SetName(CString(v.bstrVal));

   return 0;
}

int CDefaultConnectionDlg::OnApply()
{
   return PSNRET_NOERROR;
}

int CDefaultConnectionDlg::OnSetActive()
{
   ::EnableWindow(GetDlgItem(IDC_SERVER), _HasProperty(DBPROPSET_DBINIT, DBPROP_INIT_DATASOURCE));
   ::EnableWindow(GetDlgItem(IDC_DATABASE), _HasProperty(DBPROPSET_DBINIT, DBPROP_INIT_LOCATION));
   ::EnableWindow(GetDlgItem(IDC_USERNAME), _HasProperty(DBPROPSET_DBINIT, DBPROP_AUTH_USERID));
   ::EnableWindow(GetDlgItem(IDC_PASSWORD), _HasProperty(DBPROPSET_DBINIT, DBPROP_AUTH_PASSWORD));
   ::EnableWindow(GetDlgItem(IDC_CATALOG), _HasProperty(DBPROPSET_DBINIT, DBPROP_INIT_CATALOG));
   ::EnableWindow(GetDlgItem(IDC_SAVEPASSWORD), _HasProperty(DBPROPSET_DBINIT, DBPROP_AUTH_PERSIST_SENSITIVE_AUTHINFO));

   CComVariant v;
   v.Clear();
   v = _GetProperty(DBPROPSET_DBINIT, DBPROP_INIT_DATASOURCE);
   SetDlgItemText(IDC_SERVER, v.vt != VT_BSTR ? _T("") : CString(v.bstrVal));
   v.Clear();
   v = _GetProperty(DBPROPSET_DBINIT, DBPROP_INIT_LOCATION);
   SetDlgItemText(IDC_DATABASE, v.vt != VT_BSTR ? _T("") : CString(v.bstrVal));
   v.Clear();
   v = _GetProperty(DBPROPSET_DBINIT, DBPROP_AUTH_USERID);
   SetDlgItemText(IDC_USERNAME, v.vt != VT_BSTR ? _T("") : CString(v.bstrVal));
   v.Clear();
   v = _GetProperty(DBPROPSET_DBINIT, DBPROP_AUTH_PASSWORD);
   SetDlgItemText(IDC_PASSWORD, v.vt != VT_BSTR ? _T("") : CString(v.bstrVal));
   v.Clear();
   v = _GetProperty(DBPROPSET_DBINIT, DBPROP_INIT_CATALOG);
   SetDlgItemText(IDC_CATALOG, v.vt != VT_BSTR ? _T("") : CString(v.bstrVal));
   CheckDlgButton(IDC_SAVEPASSWORD, BST_CHECKED);

   return 0;
}

// Implementation

CComVariant CDefaultConnectionDlg::_GetProperty(const GUID& guid, DBPROPID propid) const
{
   ATLASSERT(m_pProject);
   // Prepare properties
   DBPROPIDSET PropIdSet;
   PropIdSet.guidPropertySet = guid;
   PropIdSet.cPropertyIDs = 1;
   PropIdSet.rgPropertyIDs = &propid;
   // Retrieve property
   CComQIPtr<IDBProperties> spProperties = m_pProject->m_pWizardView->m_Db.m_spInit;
   ULONG lCount = 0;
   DBPROPSET* pPropSet = NULL;
   spProperties->GetProperties(1, &PropIdSet, &lCount, &pPropSet);
   CComVariant vRes = pPropSet->rgProperties->vValue;
   // Free memory
   if( pPropSet ) {
      ::VariantClear(&pPropSet->rgProperties[0].vValue);
      ::CoTaskMemFree(pPropSet->rgProperties);
      ::CoTaskMemFree(pPropSet);
   }
   return vRes;
}

bool CDefaultConnectionDlg::_HasProperty(const GUID& guid, DBPROPID propid) const
{
   // Prepare properties
   DBPROPIDSET PropIdSet;
   PropIdSet.guidPropertySet = guid;
   PropIdSet.cPropertyIDs = 1;
   PropIdSet.rgPropertyIDs = &propid;
   // Retrieve property
   CComQIPtr<IDBProperties> spProperties = m_pProject->m_pWizardView->m_Db.m_spInit;
   ULONG lCount = 0;
   DBPROPSET* pPropSet = NULL;
   spProperties->GetProperties(1, &PropIdSet, &lCount, &pPropSet);
   bool bValid = pPropSet->rgProperties->dwStatus == DBPROPSTATUS_OK;
   // Free memory
   if( pPropSet ) {
      ::VariantClear(&pPropSet->rgProperties[0].vValue);
      ::CoTaskMemFree(pPropSet->rgProperties);
      ::CoTaskMemFree(pPropSet);
   }
   return bValid;
}


///////////////////////////////////////////////////////////////
//

LRESULT CConnectionPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   _ASSERTE(m_pProject);
   ModifyStyleEx(0, WS_EX_CONTROLPARENT);

   m_wndDefault.m_pProject = m_pProject;
   m_wndDefault.Create(m_hWnd);
   return 0;
}

LRESULT CConnectionPage::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   if( !m_wndClient.IsWindow() ) return 0;
   RECT rcClient;
   GetClientRect(&rcClient);
   m_wndClient.MoveWindow(&rcClient);
   return 0;
}

int CConnectionPage::OnTranslateAccelerator(LPMSG lpMsg)
{
   if( m_wndClient.IsDialogMessage(lpMsg) ) return PSNRET_MESSAGEHANDLED;
   return PSNRET_NOERROR;
}

int CConnectionPage::OnSetActive()
{
   bool bCustomPage = false;
#if 0
   CComQIPtr<IServiceProvider> spSP = m_pProject->m_pWizardView->m_Db.m_spInit;
   if( spSP ) {
      CComPtr<ISpecifyPropertyPages> spSPP;
      spSP->QueryService(OLEDB_SVC_DSLPropertyPages, IID_ISpecifyPropertyPages, (VOID**) &spSPP);
      if( spSPP ) {
         struct
         {
            ULONG cElems;
            GUID Elems[10];
         } cauuid = { 0 };
         cauuid.cElems = 10;
         spSPP->GetPages( (CAUUID*) &cauuid );
         if( cauuid.Elems[0] != CLSID_NULL ) {
            m_spPage.CoCreateInstance(cauuid.Elems[0]);
            if( m_spPage != NULL ) {
               RECT rcClient;
               GetClientRect(&rcClient);
               m_spPage->SetPageSite(this);
               m_spPage->Activate(m_hWnd, &rcClient, TRUE);
               m_spPage->Show(SW_SHOW);
               bCustomPage = true;
            }
         }
      }
   }
#endif
   if( !bCustomPage ) {
      m_wndDefault.OnSetActive();
      m_wndClient = m_wndDefault;
      m_wndDefault.ShowWindow(SW_SHOW);
      SendMessage(WM_SIZE);
      m_wndClient.SetFocus();
   }  

   SetWizardButtons(PSWIZB_BACK|PSWIZB_NEXT);
   return 0;
}

int CConnectionPage::OnWizardNext()
{
   if( m_wndClient == m_wndDefault ) return m_wndDefault.OnWizardNext();
   if( m_spPage ) m_spPage->Deactivate();
   m_spPage.Release();
   return 0;
}

int CConnectionPage::OnApply()
{
   if( m_wndClient == m_wndDefault ) return m_wndDefault.OnApply();
   return PSNRET_NOERROR;
}

// IUnknown

STDMETHODIMP CConnectionPage::QueryInterface(REFIID iid, void** ppvObject)
{
   *ppvObject = NULL;
   if( iid == IID_IPropertyPageSite ) *ppvObject = this;
   else if( iid == IID_IPropertyBag ) *ppvObject = this;
   else if( iid == IID_IUnknown ) *ppvObject = this;
   else return E_NOINTERFACE;
   return S_OK;
}

STDMETHODIMP_(DWORD) CConnectionPage::AddRef(VOID)
{
   return 0;
}

STDMETHODIMP_(DWORD) CConnectionPage::Release(VOID)
{
   return 0;
}

// IPropertyPageSite

STDMETHODIMP CConnectionPage::OnStatusChange(DWORD dwFlags)
{
   ATLTRACENOTIMPL(_T("CConnectionPage::OnStatusChange"));
}

STDMETHODIMP CConnectionPage::GetLocaleID(LCID *pLocaleID)
{
   ATLTRACENOTIMPL(_T("CConnectionPage::GetLocaleID"));
}

STDMETHODIMP CConnectionPage::GetPageContainer(IUnknown **ppUnk)
{
   ATLTRACENOTIMPL(_T("CConnectionPage::GetPageContainer"));
}

STDMETHODIMP CConnectionPage::TranslateAccelerator(LPMSG pMsg)
{
   ATLTRACENOTIMPL(_T("CConnectionPage::TranslateAccelerator"));
}

// IPropertyBag

STDMETHODIMP CConnectionPage::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
   ATLTRACENOTIMPL(_T("CConnectionPage::Read"));
}

STDMETHODIMP CConnectionPage::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
   ATLTRACENOTIMPL(_T("CConnectionPage::Write"));
}


////////////////////////////////////////////////////////////////////////
//

LRESULT CAdvancedEditOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   ATLASSERT(!m_sLanguage.IsEmpty());

   CString sKey;
   sKey.Format(_T("editors.%s."), m_sLanguage);
   SET_CHECK(IDC_AUTOCOMPLETE, sKey + _T("autoComplete"));
   SET_CHECK(IDC_IGNOREVIEWS, sKey + _T("ignoreViews"));
   SET_CHECK(IDC_AUTOCOMMIT, sKey + _T("autoCommit"));
   SET_CHECK(IDC_STRIPCOMMENTS, sKey + _T("stripComments"));
   SET_CHECK(IDC_MATCHBRACES, sKey + _T("matchBraces"));
   SET_CHECK(IDC_BACKGROUNDLOAD, sKey + _T("backgroundLoad"));

   CEdit ctrlEdit = GetDlgItem(IDC_TERMINATOR);
   ctrlEdit.SetLimitText(4);

   TCHAR szBuffer[64] = { 0 };
   _pDevEnv->GetProperty(sKey + _T("terminator"), szBuffer, 63);
   SetDlgItemText(IDC_TERMINATOR, szBuffer);

   _pDevEnv->GetProperty(sKey + _T("maxRecords"), szBuffer, 63);
   SetDlgItemInt(IDC_MAXROWS, _ttol(szBuffer));
   if( _ttol(szBuffer) > 0 ) CheckDlgButton(IDC_LIMITRECORDS, BST_CHECKED); else SetDlgItemInt(IDC_MAXROWS, 500);

   _pDevEnv->GetProperty(sKey + _T("maxErrors"), szBuffer, 63);
   SetDlgItemInt(IDC_MAXERRORS, _ttol(szBuffer));
   if( _ttol(szBuffer) > 0 ) CheckDlgButton(IDC_LIMITERRORS, BST_CHECKED); else SetDlgItemInt(IDC_MAXERRORS, 10);

   _UpdateButtons();

   return 0;
}

LRESULT CAdvancedEditOptionsPage::OnButtonClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
   _UpdateButtons();
   bHandled = FALSE;
   return 0;
}

int CAdvancedEditOptionsPage::OnApply()
{
   CString sKey;
   sKey.Format(_T("editors.%s."), m_sLanguage);
   GET_CHECK(IDC_AUTOCOMPLETE, sKey + _T("autoComplete"));
   GET_CHECK(IDC_IGNOREVIEWS, sKey + _T("ignoreViews"));
   GET_CHECK(IDC_AUTOCOMMIT, sKey + _T("autoCommit"));
   GET_CHECK(IDC_STRIPCOMMENTS, sKey + _T("stripComments"));
   GET_CHECK(IDC_MATCHBRACES, sKey + _T("matchBraces"));
   GET_CHECK(IDC_BACKGROUNDLOAD, sKey + _T("backgroundLoad"));
  
   TCHAR szTerminator[64] = { 0 };
   GetDlgItemText(IDC_TERMINATOR, szTerminator, 63);
   _pDevEnv->SetProperty(sKey + _T("terminator"), szTerminator);

   long lMaxRecords = 0;
   if( IsDlgButtonChecked(IDC_LIMITRECORDS) ) lMaxRecords = GetDlgItemInt(IDC_MAXROWS);
   _pDevEnv->SetProperty(sKey + _T("maxRecords"), ToString(lMaxRecords));

   long lMaxErrors = 0;
   if( IsDlgButtonChecked(IDC_LIMITERRORS) ) lMaxErrors = GetDlgItemInt(IDC_MAXERRORS);
   _pDevEnv->SetProperty(sKey + _T("maxErrors"), ToString(lMaxErrors));

   if( m_pArc->ReadGroupBegin(_T("Editors")) ) 
   {
      while( m_pArc->ReadGroupBegin(_T("Editor")) ) {
         TCHAR szLanguage[32] = { 0 };
         m_pArc->Read(_T("name"), szLanguage, 31);
         if( m_sLanguage == szLanguage ) {
            m_pArc->Delete(_T("Advanced"));
            m_pArc->WriteItem(_T("Advanced"));
            TRANSFER_PROP(_T("autoComplete"), sKey + _T("autoComplete"));
            TRANSFER_PROP(_T("ignoreViews"), sKey + _T("ignoreViews"));
            TRANSFER_PROP(_T("autoCommit"), sKey + _T("autoCommit"));
            TRANSFER_PROP(_T("stripComments"), sKey + _T("stripComments"));
            TRANSFER_PROP(_T("matchBraces"), sKey + _T("matchBraces"));
            TRANSFER_PROP(_T("backgroundLoad"), sKey + _T("backgroundLoad"));
            m_pArc->Write(_T("terminator"), szTerminator);
            m_pArc->Write(_T("maxRecords"), lMaxRecords);
            m_pArc->Write(_T("maxErrors"), lMaxErrors);
         }
         m_pArc->ReadGroupEnd();
      }
      m_pArc->ReadGroupEnd();
   }
   
   // HACK: To clear the iterator cache
   m_pArc->ReadGroupEnd();

   return PSNRET_NOERROR;
}

void CAdvancedEditOptionsPage::_UpdateButtons()
{
   CWindow(GetDlgItem(IDC_MAXROWS)).EnableWindow(IsDlgButtonChecked(IDC_LIMITRECORDS) == BST_CHECKED);
   CWindow(GetDlgItem(IDC_MAXERRORS)).EnableWindow(IsDlgButtonChecked(IDC_LIMITERRORS) == BST_CHECKED);
}
