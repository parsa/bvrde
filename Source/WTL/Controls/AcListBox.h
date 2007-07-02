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


class CFunctionRtfCtrl : public CRichEditCtrl
{
public:
   HWND m_hWndParent;
   COLORREF m_clrBack;

   HWND Create(HWND hWndParent)
   {
      m_hWndParent = hWndParent;
      COLORREF clrTip = ::GetSysColor(COLOR_INFOBK);
      COLORREF clrWindow = ::GetSysColor(COLOR_WINDOW);
      m_clrBack = BlendRGB(clrTip, clrWindow, 80);
      RECT rcWindow;
      ::GetWindowRect(hWndParent, &rcWindow);
      CRichEditCtrl::Create(hWndParent, rcDefault, NULL, WS_POPUP | ES_MULTILINE | ES_READONLY, WS_EX_TOOLWINDOW);
      ModifyStyleEx(WS_EX_CLIENTEDGE, WS_EX_STATICEDGE);
      SetFont(AtlGetDefaultGuiFont());
      SetTargetDevice(NULL, 0);
      SetEventMask(ENM_REQUESTRESIZE);
      SendMessage(EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN, MAKELPARAM(2, 2));
      SetBackgroundColor(m_clrBack);
      SetUndoLimit(0);
      LimitText(16000);
      HideSelection();
      return m_hWnd;
   }
   void RequestResize(REQRESIZE* prr)
   {
      RECT rcWindow;
      ::GetWindowRect(m_hWndParent, &rcWindow);
      RECT rcSize = prr->rc;
      ::InflateRect(&rcSize, 0, 8);
      rcWindow.bottom = rcWindow.top + (rcSize.bottom - rcSize.top);
      SetWindowPos(HWND_TOPMOST, &rcWindow, SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
   }
   void ShowItem(CRemoteProject* pProject, LPCTSTR pstrType, LPCTSTR pstrItem)
   {
      ATLASSERT(pProject);
      CSimpleArray<CString> aResult;
      pProject->m_TagManager.GetItemInfo(pstrItem, pstrType, TAGINFO_DECLARATION | TAGINFO_COMMENT, aResult);
      if( aResult.GetSize() == 0 ) return;
      CString sDecl;
      CString sComment;
      sDecl = aResult[0].SpanExcluding(_T("|"));
      sComment = aResult[0].Mid(sDecl.GetLength() + 1);
      CString sText;
      sText.Format(_T("%s"), sDecl);
      if( !sComment.IsEmpty() ) {
         sComment = sComment.Left(150);
         if( sComment.GetLength() == 150 ) sComment += _T("...");
         CString sTemp = _T("// ") + sComment;
         sComment = sTemp;
         sText += _T("\n\n   ");
         sText += sComment;
      }
      SetWindowText(sText);
      COLORREF clrText = ::GetSysColor(COLOR_INFOTEXT);
      CHARFORMAT cf = { 0 };     
      cf.dwMask |= CFM_COLOR;
      cf.crTextColor = clrText;
      SetCharFormat(cf, SCF_ALL);
      _ColorText(sText, pstrItem, CFM_BOLD);
      _ColorText(sText, _T("const "), CFM_ITALIC, BlendRGB(clrText, RGB(255,0,128), 20));
      _ColorText(sText, _T("inline "), CFM_ITALIC, BlendRGB(clrText, RGB(255,0,128), 20));
      _ColorText(sText, _T("volatile "), CFM_ITALIC, BlendRGB(clrText, RGB(255,0,128), 20));
      _ColorArgs(sText, 0, BlendRGB(clrText, RGB(0,0,255), 20));
      _ColorText(sText, sComment, 0, RGB(160,160,160));
      CRichEditCtrl::RequestResize();
   }
   void _ColorArgs(CString& sText, BYTE iEffect, COLORREF clrText)
   {
      int iPos = sText.Find('(');
      if( iPos < 0 ) return;
      int cchLen = sText.GetLength();
      CString sWord;
      for( int i = 0; i < cchLen; i++ ) {
         TCHAR c = sText[i];
         switch( c ) {
         case ',':
         case ')':
            _ColorText(sText, sWord, iEffect, clrText);
            break;
         case '\r':
         case '\n':
         case ' ':
            sWord.Empty();
            break;
         default:
            sWord += c;
         }
      }
   }
   void _ColorText(CString& sText, LPCTSTR pstrItem, BYTE iEffect, COLORREF clrText = CLR_NONE)
   {
      int iPos = sText.Find(pstrItem);
      if( iPos < 0 ) return;
      SetSel(iPos, iPos + _tcslen(pstrItem));
      CHARFORMAT cf = { 0 };
      if( clrText != CLR_NONE ) {
         cf.dwMask |= CFM_COLOR;
         cf.crTextColor = clrText;
      }
      if( (iEffect & CFM_BOLD) != 0 ) {
         cf.dwMask |= CFM_BOLD;
         cf.dwEffects |= CFE_BOLD;
      }
      if( (iEffect & CFM_ITALIC) != 0 ) {
         cf.dwMask |= CFM_ITALIC;
         cf.dwEffects |= CFE_ITALIC;
      }
      SetCharFormat(cf, SCF_SELECTION);
   }
};

class CAcListBoxCtrl : public CWindowImpl<CAcListBoxCtrl, CWindow>
{
public:
   CRemoteProject* m_pCppProject;
   CString m_sType;
   CString m_sMember;
   CString m_sList;
   CListBox m_wndList;
   CFunctionRtfCtrl m_wndInfo;
   int m_iCurSel;

   enum { TIMER_ID = 680 };

   void Init(CRemoteProject* pProject, CString& sType, CString& sMember, CString& sList)
   {
      m_pCppProject = pProject;
      m_sType = sType;
      m_sMember = sMember;
      m_sList = sList;
      m_wndList = GetWindow(GW_CHILD);
      m_iCurSel = -1;
      SetTimer(TIMER_ID, 300);
      // Create RichEdit control
      m_wndInfo.Create(m_hWnd);
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
      m_wndInfo.RequestResize((REQRESIZE*) pnmh);
      return 0;
   }

   void _ShowInfo(int iIndex)
   {
      m_wndInfo.ShowWindow(SW_HIDE);
      CString sItem = _GetListItem(iIndex);
      const INT CXWINDOW = 300;
      RECT rcWindow = { 0 };
      GetWindowRect(&rcWindow);
      if( rcWindow.right + CXWINDOW > ::GetSystemMetrics(SM_CXSCREEN) ) return;
      RECT rcInfo = { rcWindow.right + 4, rcWindow.top, rcWindow.right + CXWINDOW, rcWindow.top + 10 };
      m_wndInfo.MoveWindow(&rcInfo);
      m_wndInfo.ShowItem(m_pCppProject, m_sType, sItem);
   }
   CString _GetListItem(int iIndex) const
   {
      int iItem = 0;
      int cchLen = m_sList.GetLength();
      for( int i = 0; i < cchLen; i++ ) {
         if( m_sList[i] == ' ' ) i++, iItem++;
         if( iItem == iIndex ) {
            CString sItem;
            for( ; i < cchLen; i++ ) {
               TCHAR c = m_sList[i];
               switch( c ) {
               case ' ':
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

