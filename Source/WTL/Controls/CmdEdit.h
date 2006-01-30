#if !defined(AFX_CMDEDIT_H__20051127_C3B0_2358_6BFD_0080AD509054__INCLUDED_)
#define AFX_CMDEDIT_H__20051127_C3B0_2358_6BFD_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CmdEdit - Edit that send WM_COMMAND
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2005 Bjarke Viksoe.
//
// This code may be used in compiled form in any way you desire. This
// source file may be redistributed by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//


class CCmdEditCtrl : public CWindowImpl<CCmdEditCtrl, CEdit>
{
public:
   DECLARE_WND_SUPERCLASS(_T("WTL_CmdEditCtrl"), CEdit::GetWndClassName())

   UINT m_uCmdID;

   void SetCommand(UINT uCmdID)
   {
      m_uCmdID = uCmdID;
   }

   void OnFinalMessage()
   {
      delete this;
   }

   BEGIN_MSG_MAP(CCmdEditCtrl)
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
   END_MSG_MAP()

   LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( wParam == VK_RETURN ) {
         CWindow wndTop = GetTopLevelWindow();
         wndTop.PostMessage(WM_COMMAND, MAKEWPARAM(m_uCmdID, 0), (LPARAM) (HWND) GetParent());
         return 0;
      }
      bHandled = FALSE;
      return 0;
   }
   LRESULT OnGetDlgCode(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      return DefWindowProc() | DLGC_WANTALLKEYS;
   }
};


#endif // !defined(AFX_CMDEDIT_H__20051127_C3B0_2358_6BFD_0080AD509054__INCLUDED_)

