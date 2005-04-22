#if !defined(AFX_WIZARDSHEET_H__20040420_901C_6B58_9C31_0080AD509054__INCLUDED_)
#define AFX_WIZARDSHEET_H__20040420_901C_6B58_9C31_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlframe.h>      // for COwnerDraw
#include <atldlgs.h>       // for CPropertyPageImpl

#include "PropertyList.h"


///////////////////////////////////////////////////////////////
//

class COptionsPage : public CPropertyPageImpl<COptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_COMMANDS };

   enum
   {
      SC_SYSTEM_NONE = 0,
      SC_SYSTEM_CVS,
      SC_SYSTEM_SUBVERSION,
      SC_SYSTEM_CUSTOM,
   };

   CComboBox m_ctrlType;
   CPropertyListCtrl m_ctrlList;
   ISerializable* m_pArc;

   // Overloads

   int OnSetActive();
   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(COptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_HANDLER(IDC_TYPE, CBN_SELCHANGE, OnTypeChange)
      CHAIN_MSG_MAP( CPropertyPageImpl<COptionsPage> )
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnTypeChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   // Implementation

   void _PopulateList();
};


#endif // !defined(AFX_WIZARDSHEET_H__20040420_901C_6B58_9C31_0080AD509054__INCLUDED_)
