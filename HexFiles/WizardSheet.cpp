
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

LRESULT CHexEditorOptionsPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   CString sKey = _T("editors.binary.");
   SET_CHECK(IDC_READONLY, sKey + _T("readOnly"));
   return 0;
}

int CHexEditorOptionsPage::OnApply()
{
   if( m_pArc->ReadGroupBegin(_T("Editors")) ) 
   {
      while( m_pArc->ReadGroupBegin(_T("Editor")) ) {
         TCHAR szLanguage[32] = { 0 };
         m_pArc->Read(_T("name"), szLanguage, 31);
         if( _tcscmp(szLanguage, _T("binary")) == 0 ) {
            m_pArc->Delete(_T("Visuals"));
            m_pArc->WriteItem(_T("Visuals"));
            WRITE_CHECKBOX(IDC_READONLY, _T("readOnly"));
         }
         m_pArc->ReadGroupEnd();
      }
      m_pArc->ReadGroupEnd();
   }

   // HACK: To clear the iterator cache
   m_pArc->ReadGroupEnd();

   return PSNRET_NOERROR;
}

