#if !defined(AFX_CHECKINDLG_H__20031225_6EB7_455C_FCAA_0080AD509054__INCLUDED_)
#define AFX_CHECKINDLG_H__20031225_6EB7_455C_FCAA_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MruCombo.h"


class CCheckInDlg : public CDialogImpl<CCheckInDlg>
{
public:
   enum { IDD = IDD_CHECKIN };

   // Operations

   // Message map and handlers

   BEGIN_MSG_MAP(CCheckInDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      return 0;
   }
   LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      bHandled = FALSE;
      return 0;
   }
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
   LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      _UpdateButtons();
      return 0;
   }

   // Implementation

   void _UpdateButtons()
   {
   }
};


#endif // !defined(AFX_CHECKINDLG_H__20031225_6EB7_455C_FCAA_0080AD509054__INCLUDED_)

