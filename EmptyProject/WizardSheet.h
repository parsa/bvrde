#if !defined(AFX_WIZARDSHEET_H__20050118_53A3_FF7E_3388_0080AD509054__INCLUDED_)
#define AFX_WIZARDSHEET_H__20050118_53A3_FF7E_3388_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CMessagePage : public CPropertyPageImpl<CMessagePage>
{
public:
   enum { IDD = IDD_WIZARD_MESSAGE };

   // Operations

   int OnSetActive()
   {
      SetWizardButtons(GetPropertySheet().GetActiveIndex() == 0 ? PSWIZB_NEXT : PSWIZB_BACK | PSWIZB_NEXT);
      return 0;
   }

   // Message map and handlers

   BEGIN_MSG_MAP(CMessagePage)
      CHAIN_MSG_MAP( CPropertyPageImpl<CMessagePage> )
   END_MSG_MAP()
};


#endif // !defined(AFX_WIZARDSHEET_H__20050118_53A3_FF7E_3388_0080AD509054__INCLUDED_)
