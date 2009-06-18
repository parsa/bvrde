#if !defined(AFX_ACLISTBOX_H__20051126_335B_FC59_68F7_0080AD509054__INCLUDED_)
#define AFX_ACLISTBOX_H__20051126_335B_FC59_68F7_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// AutoCompleteComboBox - Scintilla extension
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

#include "FunctionRtf.h"


/**
 * AutoComplete control.
 * This is an extension for the ListBox control used by the Scintilla
 * Editor control. It will manage a popup window on the right side of the
 * control, displaying the C++ declaration of the item selected in 
 * the actual ListBox.
 */
class CAcListBoxCtrl : public CWindowImpl<CAcListBoxCtrl, CWindow>
{
public:
   CRemoteProject* m_pProject;
   CListBox m_wndList;
   CFunctionRtfCtrl m_wndInfo;

   CString m_sType;
   CString m_sList;
   int m_iCurSel;

   enum { TIMER_ID = 680 };

   void Init(CRemoteProject* pProject, const CString sType, const CString sList)
   {
      m_pProject = pProject;
      m_sType = sType;
      m_sList = sList;
      m_wndList = GetWindow(GW_CHILD);
      m_iCurSel = -1;
      // Ignite the timer which checks for changes in selection
      SetTimer(TIMER_ID, 300);
      // Create RichEdit control
      COLORREF clrTip = ::GetSysColor(COLOR_INFOBK);
      COLORREF clrWindow = ::GetSysColor(COLOR_WINDOW);
      m_wndInfo.Create(m_hWnd, BlendRGB(clrTip, clrWindow, 70));
   }

   void OnFinalMessage()
   {
      delete this;
   }

   BEGIN_MSG_MAP(CAcListBoxCtrl)
      MESSAGE_HANDLER(WM_TIMER, OnTimer)
      NOTIFY_CODE_HANDLER(EN_REQUESTRESIZE, OnRequestResize);
   END_MSG_MAP()

   LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( wParam != TIMER_ID ) return 0;
      if( !m_wndList.IsWindow() ) return 0;
      int iCurSel = m_wndList.GetCurSel();
      if( iCurSel != m_iCurSel ) _ShowInfo(iCurSel);
      m_iCurSel = iCurSel;
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnRequestResize(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      REQRESIZE* pRR = (REQRESIZE*) pnmh;
      m_wndInfo.SetResize(pRR);
      return 0;
   }

   void _ShowInfo(int iIndex)
   {
      ATLASSERT(m_pProject);
      CString sItem = _GetListItem(iIndex);
      // Reposition window. Now the RichEdit control will also do its own magic and 
      // try to optimally resize the control (because it has the REQUESTRESIZE 
      // style), so we need to be carefull here...
      m_wndInfo.ShowWindow(SW_HIDE);
      const INT CXWINDOW = 300;
      RECT rcWindow = { 0 };
      GetWindowRect(&rcWindow);
      RECT rcInfo = { rcWindow.right + 4, rcWindow.top, rcWindow.right + CXWINDOW, rcWindow.top + 10 };
      if( rcInfo.right > ::GetSystemMetrics(SM_CXSCREEN) ) return;
      m_wndInfo.MoveWindow(&rcInfo, FALSE);
      // Look up information about this member.
      // While we'll allow deep-search, we'll only allow for a very short search period.
      CSimpleValArray<TAGINFO*> aList;
      m_pProject->m_TagManager.FindItem(sItem, m_sType, 99, ::GetTickCount() + 200, aList);
      if( aList.GetSize() == 0 ) return;
      CTagDetails Info;
      m_pProject->m_TagManager.GetItemInfo(aList[0], Info);
      m_wndInfo.ShowItem(sItem, Info);
   }

   CString _GetListItem(int iIndex) const
   {
      int iItem = 0;
      int cchLen = m_sList.GetLength();
      for( int i = 0; i < cchLen; i++ ) {
         // The | (pipe) char is the item separator
         // The ? (question) is followed by the image number
         if( m_sList[i] == '|' ) {
            i++, 
            iItem++;
         }
         if( iItem == iIndex ) {
            CString sItem;
            for( ; i < cchLen; i++ ) {
               TCHAR c = m_sList[i];
               switch( c ) {
               case '|':
               case '?':
                  return sItem;
               default:
                  sItem += c;
               }
            }
            return sItem;
         }
      }
      return _T("");
   }
};


#endif // !defined(AFX_ACLISTBOX_H__20051126_335B_FC59_68F7_0080AD509054__INCLUDED_)

