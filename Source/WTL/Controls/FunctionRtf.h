#if !defined(AFX_FUNCTIONRTF_H__20051126_335B_FC59_68F7_0080AD509054__INCLUDED_)
#define AFX_FUNCTIONRTF_H__20051126_335B_FC59_68F7_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// FunctionRtf - RTF Function syntax-coloring window
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2005 - 2007 Bjarke Viksoe.
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

/**
 * Label with colors.
 * This is a RichEdit extension which can color C++ declarations
 * using the coloring ability of the RichEdit.
 */
class CFunctionRtfCtrl : public CRichEditCtrl
{
public:
   HWND m_hWndParent;

   HWND Create(HWND hWndParent, COLORREF clrBack)
   {
      m_hWndParent = hWndParent;
      CRichEditCtrl::Create(hWndParent, rcDefault, NULL, WS_POPUP | ES_MULTILINE | ES_READONLY, WS_EX_TOOLWINDOW);
      ModifyStyleEx(WS_EX_CLIENTEDGE, WS_EX_STATICEDGE);
      SetFont(AtlGetDefaultGuiFont());
      SetTargetDevice(NULL, 0);
      SetEventMask(ENM_REQUESTRESIZE);
      SendMessage(EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN, MAKELPARAM(2, 2));
      SetBackgroundColor(clrBack);
      SetUndoLimit(0);
      LimitText(8000);
      HideSelection();
      return m_hWnd;
   }
   void SetResize(REQRESIZE* pRR)
   {
      ATLASSERT(IsWindow());
      if( !IsWindow() ) return;
      RECT rcWindow = { 0 };
      GetWindowRect(&rcWindow);
      RECT rcSize = pRR->rc;
      ::InflateRect(&rcSize, 0, 8);
      rcWindow.bottom = rcWindow.top + (rcSize.bottom - rcSize.top);
      SetWindowPos(HWND_TOPMOST, &rcWindow, SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
   }
   void ShowItem(LPCTSTR pstrName, LPCTSTR pstrDeclaration, LPCTSTR pstrComment)
   {
      // Format the member declaration.
      // Here follow a range of strange replacements. They will basically turn...
      //    int Foo(int a, int b);
      // into
      //    int Foo( int<nbsp>a, int<nbsp>b<nbsp>);
      // where <nbsp> is a non-breaking space. Looks good in formatting/word-
      // wrapping.
      CString sText = pstrDeclaration;
      sText.Replace(_T("  "), _T(" "));
      sText.Replace(' ', 0xA0);
      sText.Replace(_T(",\xA0"), _T(", "));
      sText.Replace(_T("("), _T("( "));    sText.Replace(_T("( \xA0"), _T("( "));
      sText.Replace(_T(")"), _T("\xA0)")); sText.Replace(_T("\xA0\xA0)"), _T("\xA0)"));
      int cchDeclLen = sText.GetLength();
      // Ah, we can also have a comment for this declaration. Format
      // it below. We'll color it gray in a little while too.
      CString sComment = pstrComment;
      if( !sComment.IsEmpty() ) {
         sComment = sComment.Left(150);
         if( sComment.GetLength() == 150 ) sComment += _T("...");
         CString sTemp;
         sTemp.Format(_T("\n\n  // %s"), sComment);
         sText += sTemp;
      }
      SetWindowText(sText);
      // Finally we can do coloring of the text. Since the text is techincally
      // identical to our plan-text string, we can use the positions from the string
      // data to select/format/unselect the richedit.
      COLORREF clrText = ::GetSysColor(COLOR_INFOTEXT);
      CHARFORMAT cf = { 0 };     
      cf.dwMask |= CFM_COLOR;
      cf.crTextColor = clrText;
      SetCharFormat(cf, SCF_ALL);
      _ColorText(sText, pstrName,       CFM_BOLD);
      _ColorText(sText, _T("const"),    0, BlendRGB(clrText, RGB(255,0,128), 20));
      _ColorText(sText, _T("inline"),   CFM_ITALIC, BlendRGB(clrText, RGB(255,0,128), 20));
      _ColorText(sText, _T("extern"),   CFM_ITALIC, BlendRGB(clrText, RGB(255,0,128), 20));
      _ColorText(sText, _T("static"),   CFM_ITALIC, BlendRGB(clrText, RGB(255,0,128), 20));
      _ColorText(sText, _T("volatile"), CFM_ITALIC, BlendRGB(clrText, RGB(255,0,128), 20));
      _ColorText(sText, _T("virtual"),  0, BlendRGB(clrText, RGB(255,0,128), 20));
      _ColorArgs(sText, 0, BlendRGB(clrText, RGB(0,0,255), 20));
      _ColorText(sText, sComment, 0, RGB(130,130,130));
      // Now we will indent the declaration if it spans muliple lines.
      SetSel(0, cchDeclLen);
      PARAFORMAT2 pf;
      pf.cbSize = sizeof(pf);
      pf.dwMask = PFM_OFFSET;
      pf.dxOffset = 150;
      SetParaFormat(pf);
      // Finally we can ask the RichEdit it resize itself
      // to its optimal size. This will also show the window
      // once again.
      SetSel(-1, -1);
      CRichEditCtrl::RequestResize();
   }

   void _ColorArgs(CString& sText, BYTE iEffect, COLORREF clrText)
   {
      int iPos = sText.Find('(');
      if( iPos < 0 ) return;
      int cchLen = sText.GetLength();
      CString sWord;
      CString sTemp;
      for( int i = iPos; i < cchLen; i++ ) {
         TCHAR c = sText[i];
         switch( c ) {
         case ',':
         case ')':
            _ColorText(sText, sWord, iEffect, clrText);
            break;
         case '\r':
         case '\n':
         case 0xA0:
            if( sText[i + 1] != ')' ) sWord.Empty();
            break;
         case ' ':
            break;
         case '/':
            i = cchLen;
            break;
         default:
            sWord += c;
         }
      }
   }
   void _ColorText(CString& sText, LPCTSTR pstrItem, BYTE iEffect, COLORREF clrText = CLR_NONE)
   {
      int cchText = sText.GetLength();
      int cchItem = (int) _tcslen(pstrItem);
      int iStartPos = 0;
      for( int iPos = 0; (iPos = sText.Find(pstrItem, iStartPos)) >= 0; iStartPos = iPos + 1 ) {
         if( iPos > 0 && _istalnum(sText[iPos - 1]) ) continue;
         if( iPos + cchItem < cchText && _istalnum(sText[iPos + cchItem]) ) continue;
         SetSel(iPos, iPos + cchItem);
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
   }
};


#endif // !defined(AFX_FUNCTIONRTF_H__20051126_335B_FC59_68F7_0080AD509054__INCLUDED_)
