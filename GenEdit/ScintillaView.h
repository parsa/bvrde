#if !defined(AFX_SCRINTILLAVIEW_H__20030608_E68E_152F_472A_0080AD509054__INCLUDED_)
#define AFX_SCRINTILLAVIEW_H__20030608_E68E_152F_472A_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"


class CScintillaView : 
   public CWindowImpl<CScintillaView, CScintillaCtrl>,
   public CScintillaCommands<CScintillaView>,
   public IIdleListener
{
public:
   DECLARE_WND_SUPERCLASS(_T("BVRDE_EditorView"), CScintillaCtrl::GetWndClassName())

   CScintillaView(IDevEnv* pDevEnv);

   void OnFinalMessage(HWND hWnd);

   typedef struct
   {
      TCHAR szFont[LF_FACESIZE + 1];
      int iHeight;
      COLORREF clrText;
      COLORREF clrBack;
      BOOL bBold;
      BOOL bItalic;
   } SYNTAXCOLOR;

   IDevEnv* m_pDevEnv;
   CContainedWindow m_wndParent;
   CString m_sFilename;
   CString m_sLanguage;
   bool m_bAutoIndent;              // Do we need to auto-indent text?
   bool m_bSmartIndent;             // Do we need to smart-indent text?
   bool m_bMatchBraces;             // Show brach matching?
   bool m_bFolding;                 // Use foldings?
   bool m_bProtectDebugged;         // Read-Only file when debugging?
   bool m_bAutoComplete;            // Use auto-complete?
   bool m_bAutoClose;               // Automatically close HTML/XML tags?
   bool m_bAutoCase;                // Automatically determine case?
   bool m_bAutoSuggest;             // Use auto-suggestion?
   //
   static FINDREPLACEA s_frFind;    // The Find dialog information
   bool m_bAutoCompleteNext;        // AutoComplete displayed at next char added?
   bool m_bAutoTextDisplayed;       // AutoText currently displayed?
   bool m_bSuggestionDisplayed;     // Suggestion word curently displayed
   long lAutoTextPos;               // Text Position for start of AutoText
   int iAutoTextEntry;              // AutoText to activate
   CHAR m_cPrevChar;

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);

   // Operations

   void SetFilename(LPCTSTR pstrFilename);
   void SetLanguage(LPCTSTR pstrLanguage);
   CString GetViewText();
   void SetViewText(LPCTSTR pstrText);

   // Message map and handlers

   BEGIN_MSG_MAP(CScintillaView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
      MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      COMMAND_ID_HANDLER(ID_FILE_PRINT, OnFilePrint)
      COMMAND_ID_HANDLER(ID_EDIT_UPPERCASE, OnUpperCase)
      COMMAND_ID_HANDLER(ID_EDIT_LOWERCASE, OnLowerCase)
      COMMAND_ID_HANDLER(ID_EDIT_GOTO, OnGotoLine)
      COMMAND_ID_HANDLER(ID_EDIT_FIND, OnFind)
      COMMAND_ID_HANDLER(ID_EDIT_REPLACE, OnReplace)
      COMMAND_ID_HANDLER(ID_EDIT_REPEAT, OnRepeat)
      COMMAND_ID_HANDLER(ID_EDIT_INDENT, OnIndent)
      COMMAND_ID_HANDLER(ID_EDIT_UNINDENT, OnIndent)
      COMMAND_ID_HANDLER(ID_EDIT_TABIFY, OnTabify)
      COMMAND_ID_HANDLER(ID_EDIT_UNTABIFY, OnTabify)
      COMMAND_ID_HANDLER(ID_EDIT_COMMENT, OnComment)
      COMMAND_ID_HANDLER(ID_EDIT_UNCOMMENT, OnComment)
      COMMAND_ID_HANDLER(ID_EDIT_ZOOM_IN, OnZoomIn)
      COMMAND_ID_HANDLER(ID_EDIT_ZOOM_OUT, OnZoomOut)
      COMMAND_ID_HANDLER(ID_EDIT_DELLINE, OnDeleteLine)
      COMMAND_ID_HANDLER(ID_EDIT_VIEWWS, OnViewWhiteSpace)
      COMMAND_ID_HANDLER(ID_EDIT_VIEWEOL, OnViewEOL)
      COMMAND_ID_HANDLER(ID_EDIT_VIEWWORDWRAP, OnViewWordWrap)
      COMMAND_ID_HANDLER(ID_BOOKMARK_TOGGLE, OnMarkerToggle)
      COMMAND_ID_HANDLER(ID_BOOKMARK_CLEAR, OnMarkerClear)
      COMMAND_ID_HANDLER(ID_BOOKMARK_PREVIOUS, OnMarkerPrevious)
      COMMAND_ID_HANDLER(ID_BOOKMARK_NEXT, OnMarkerNext)
      COMMAND_ID_HANDLER(ID_MACRO_RECORD, OnMacroStart)
      COMMAND_ID_HANDLER(ID_MACRO_CANCEL, OnMacroCancel)
      COMMAND_RANGE_HANDLER(ID_BOOKMARKS_GOTO1, ID_BOOKMARKS_GOTO8, OnMarkerGoto)
      CHAIN_MSG_MAP_ALT( CScintillaCommands<CScintillaView>, 1 )
   ALT_MSG_MAP(1)
      NOTIFY_CODE_HANDLER(SCN_DWELLEND, OnDwellEnd)
      NOTIFY_CODE_HANDLER(SCN_CHARADDED, OnCharAdded)
      NOTIFY_CODE_HANDLER(SCN_UPDATEUI, OnUpdateUI)
      NOTIFY_CODE_HANDLER(SCN_MARGINCLICK, OnMarginClick)
      NOTIFY_CODE_HANDLER(SCN_MACRORECORD, OnMacroRecord)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnGotoLine(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFind(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnLowerCase(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnUpperCase(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnReplace(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnRepeat(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnIndent(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnTabify(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnComment(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnBoxComment(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnZoomIn(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnZoomOut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDeleteLine(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewWhiteSpace(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewEOL(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewWordWrap(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMarkerToggle(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMarkerClear(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMarkerNext(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMarkerPrevious(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMarkerGoto(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFilePrint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnUpdateUI(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnDwellEnd(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnCharAdded(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnMarginClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnMacroRecord(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnMacroStart(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnMacroCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   
   // Implementation

   void _AutoText(CHAR ch);
   void _AutoSuggest(CHAR ch);
   void _AutoComplete(CHAR ch);
   void _MaintainTags(CHAR ch);
   void _MaintainIndent(CHAR ch);
   void _MatchBraces(long lPos);
   CString _GetProperty(CString sKey) const;
   void _SetLineIndentation(int iLine, int iIndent);
   CString _FindOpenXmlTag(LPCSTR sel, int nSize) const;
   bool _ReplaceOnce();
   int _FindNext(int iFlags, LPCSTR pstrText, bool bWarnings);
   CString _GetNearText(long lPosition);
   int _FunkyStrCmp(LPCTSTR src, LPCTSTR dst);
   bool _iseditchar(char ch) const;
   void _GetSyntaxStyle(LPCTSTR pstrName, SYNTAXCOLOR& syntax);
   bool _AddUnqiue(CSimpleArray<CString>& aList, LPCTSTR pstrText) const;
   void _DefineMarker(int nMarker, int nType, COLORREF clrFore, COLORREF clrBack);
};


#endif // !defined(AFX_SCRINTILLAVIEW_H__20030608_E68E_152F_472A_0080AD509054__INCLUDED_)

