
#include "StdAfx.h"
#include "resource.h"

#include "RegisterView.h"

#pragma code_seg( "VIEW" )


/////////////////////////////////////////////////////////////////////////
// Constructor/destructor

CRegisterView::CRegisterView() :
   m_pProject(NULL)
{
}


/////////////////////////////////////////////////////////////////////////
// Operations

void CRegisterView::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
}

bool CRegisterView::WantsData()
{
   if( !IsWindow() ) return false;
   if( !IsWindowVisible() ) return false;
   return true;
}

void CRegisterView::SetInfo(LPCTSTR pstrType, CMiInfo& info)
{
   if( _tcscmp(pstrType, _T("register-names")) == 0 ) 
   {
      m_aNames.RemoveAll();
      m_aValues.RemoveAll();
      CString sEmpty = _T("");
      CString sValue = info.GetItem(_T("register-names"));
      while( !sValue.IsEmpty() ) {
         m_aNames.Add(sValue);
         m_aValues.Add(sEmpty);
         sValue = info.FindNext(_T("register-names"));
      }
   }
   else if( _tcscmp(pstrType, _T("register-values")) == 0 ) 
   {
      CString sValue = info.GetItem(_T("number"));
      while( !sValue.IsEmpty() ) {
         long lNumber = _ttol(sValue);
         ATLASSERT(lNumber>=0 && lNumber<m_aNames.GetSize());
         if( lNumber < 0 || lNumber >= m_aNames.GetSize() ) return;
         m_aValues.SetAtIndex(lNumber, info.GetSubItem(_T("value")));
         sValue = info.FindNext(_T("number"));
      }
      // Update display
      ResetContent();
      CString sText;
      int nCount = m_aNames.GetSize();
      for( int i = 0; i < nCount; i++ ) {
         sText.Format(IDS_REGISTER, m_aNames[i], m_aValues[i]);
         AddString(sText);
      }
   }
}

int CRegisterView::GetNameCount() const
{
   return m_aNames.GetSize();
}


/////////////////////////////////////////////////////////////////////////
// Message handlers

LRESULT CRegisterView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
   LRESULT lRes = DefWindowProc();
   SetFont(AtlGetDefaultGuiFont());
   SetColumnWidth(180);
   return lRes;
}
