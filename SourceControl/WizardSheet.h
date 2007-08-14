#if !defined(AFX_WIZARDSHEET_H__20040420_901C_6B58_9C31_0080AD509054__INCLUDED_)
#define AFX_WIZARDSHEET_H__20040420_901C_6B58_9C31_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlframe.h>      // for COwnerDraw
#include <atldlgs.h>       // for CPropertyPageImpl

#include "FontCombo.h"
#include "ColorCombo.h"
#include "PropertyList.h"


///////////////////////////////////////////////////////////////
//

class CMainOptionsPage : public CPropertyPageImpl<CMainOptionsPage>
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

   ISerializable* m_pArc;

   CComboBox m_ctrlType;
   CPropertyListCtrl m_ctrlList;

   // Overloads

   int OnSetActive();
   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CMainOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_HANDLER(IDC_TYPE, CBN_SELCHANGE, OnTypeChange)
      CHAIN_MSG_MAP( CPropertyPageImpl<CMainOptionsPage> )
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnTypeChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   // Implementation

   void _PopulateList();
};

///////////////////////////////////////////////////////////////
//

class CDiffOptionsPage : public CPropertyPageImpl<CDiffOptionsPage>
{
public:
   enum { IDD = IDD_OPTIONS_DIFF };

   ISerializable* m_pArc;

   CFontPickerComboCtrl m_ctrlFace;
   CComboBox m_ctrlFontSize;
   CButton m_ctrlWordWrap;
   CButton m_ctrlListUnchanged;

   // Overloads

   int OnSetActive();
   int OnApply();

   // Message map and handlers

   BEGIN_MSG_MAP(CDiffOptionsPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_HANDLER(IDC_FONT, CBN_SELCHANGE, OnFontChange)
      CHAIN_MSG_MAP( CPropertyPageImpl<CDiffOptionsPage> )
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnFontChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};


#endif // !defined(AFX_WIZARDSHEET_H__20040420_901C_6B58_9C31_0080AD509054__INCLUDED_)
