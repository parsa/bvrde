#if !defined(AFX_WIZARDSHEET_H__20031211_9E59_775D_C3EE_0080AD509054__INCLUDED_)
#define AFX_WIZARDSHEET_H__20031211_9E59_775D_C3EE_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"


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

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
};


#endif // !defined(AFX_WIZARDSHEET_H__20031211_9E59_775D_C3EE_0080AD509054__INCLUDED_)
