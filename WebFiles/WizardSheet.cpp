
#include "stdafx.h"
#include "resource.h"

#include "WizardSheet.h"

#pragma code_seg( "WIZARDS" )


////////////////////////////////////////////////////////////////////////
// 

#define SET_CHECK(id, prop) \
   { TCHAR szBuf[32] = { 0 }; _pDevEnv->GetProperty(prop, szBuf, 31); \
     if( _tcscmp(szBuf, _T("false"))!=0 ) CButton(GetDlgItem(id)).Click(); }

#define WRITE_CHECKBOX(id, name) \
     m_pArc->Write(name, CButton(GetDlgItem(id)).GetCheck() == BST_CHECKED ? _T("true") : _T("false"));


////////////////////////////////////////////////////////////////////////
//

LRESULT CAdvancedEditOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   ATLASSERT(!m_sLanguage.IsEmpty());

   CString sKey;
   sKey.Format(_T("editors.%s."), m_sLanguage);
   SET_CHECK(IDC_AUTOCOMPLETE, sKey + _T("autoComplete"));
   SET_CHECK(IDC_CLOSETAGS, sKey + _T("autoClose"));
   SET_CHECK(IDC_AUTOCASE, sKey + _T("autoCase"));
   SET_CHECK(IDC_MATCHBRACES, sKey + _T("matchBraces"));

   return 0;
}

int CAdvancedEditOptionsPage::OnApply()
{
   if( m_pArc->ReadGroupBegin(_T("Editors")) ) 
   {
      while( m_pArc->ReadGroupBegin(_T("Editor")) ) {
         TCHAR szLanguage[32] = { 0 };
         m_pArc->Read(_T("name"), szLanguage, 31);
         if( m_sLanguage == szLanguage ) {
            m_pArc->Delete(_T("Advanced"));
            m_pArc->WriteItem(_T("Advanced"));
            WRITE_CHECKBOX(IDC_AUTOCOMPLETE, _T("autoComplete"));
            WRITE_CHECKBOX(IDC_CLOSETAGS, _T("autoClose"));
            WRITE_CHECKBOX(IDC_AUTOCASE, _T("autoCase"));
            WRITE_CHECKBOX(IDC_MATCHBRACES, _T("matchBraces"));
         }
         m_pArc->ReadGroupEnd();
      }
      m_pArc->ReadGroupEnd();
   }

   // HACK: To clear the iterator cache
   m_pArc->ReadGroupEnd();

   return PSNRET_NOERROR;
}
