
#include "stdafx.h"
#include "resource.h"

#include "WizardSheet.h"

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


////////////////////////////////////////////////////////////////////////
//

LRESULT CAdvancedEditOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   ATLASSERT(!m_sLanguage.IsEmpty());

   CString sKey;
   sKey.Format(_T("editors.%s."), m_sLanguage);
   SET_CHECK(IDC_AUTOCOMPLETE, sKey + _T("autoComplete"));
   SET_CHECK(IDC_CLOSETAGS, sKey + _T("autoClose"));

   return 0;
}

int CAdvancedEditOptionsPage::OnApply()
{
   CString sKey;
   sKey.Format(_T("editors.%s."), m_sLanguage);
   GET_CHECK(IDC_AUTOCOMPLETE, sKey + _T("autoComplete"));
   GET_CHECK(IDC_CLOSETAGS, sKey + _T("autoClose"));

   if( m_pArc->ReadGroupBegin(_T("Editors")) ) 
   {
      while( m_pArc->ReadGroupBegin(_T("Editor")) ) {
         TCHAR szLanguage[32] = { 0 };
         m_pArc->Read(_T("name"), szLanguage, 31);
         if( m_sLanguage == szLanguage ) {
            m_pArc->Delete(_T("Advanced"));
            m_pArc->WriteItem(_T("Advanced"));
            TRANSFER_PROP(_T("autoComplete"), sKey + _T("autoComplete"));
            TRANSFER_PROP(_T("autoClose"), sKey + _T("autoClose"));
         }
         m_pArc->ReadGroupEnd();
      }
      m_pArc->ReadGroupEnd();
   }
   
   // HACK: To clear the iterator cache
   m_pArc->ReadGroupEnd();

   return PSNRET_NOERROR;
}
