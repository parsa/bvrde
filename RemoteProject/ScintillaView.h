#if !defined(AFX_SCINTILLAVIEW_H__20030309_A76E_F01E_9DAB_0080AD509054__INCLUDED_)
#define AFX_SCINTILLAVIEW_H__20030309_A76E_F01E_9DAB_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Forward declare
class CRemoteProject;

#include "../GenEdit/GenEdit.h"


class CScintillaView : 
   public CWindowImpl<CScintillaView>,
   public IIdleListener
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_CppEditorView"))

   CScintillaView();

   IProject* m_pProject;
   CRemoteProject* m_pCppProject;
   IView* m_pView;
   CContainedWindowT<CScintillaCtrl> m_ctrlEdit;
   CString m_sFilename;
   CString m_sLanguage;

   bool m_bAutoIndent;              // Do we need to auto-indent text?
   bool m_bSmartIndent;             // Do we need to smart-indent text?
   bool m_bProtectDebugged;         // Read-Only file when debugging?
   bool m_bAutoComplete;            // Use auto-complete?
   bool m_bAutoSuggest;             // Use auto-suggestion?
   bool m_bMouseDwell;              // Awaiting mouse-hover information
   long m_lDwellPos;                // Position where mouse-hover occoured
   bool m_bSuggestionDisplayed;     // Suggestion word curently displayed

   // Operations

   void OnFinalMessage(HWND hWnd);

   BOOL Init(CRemoteProject* pCppProject, IProject* pProject, IView* pView, LPCTSTR pstrFilename, LPCTSTR pstrLanguage);
   BOOL GetText(LPSTR& pstrText);
   BOOL SetText(LPCSTR pstrText);

   // Message map and handlers

   BEGIN_MSG_MAP(CScintillaView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_QUERYENDSESSION, OnQueryEndSession)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
      MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
      COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
      COMMAND_ID_HANDLER(ID_EDIT_AUTOCOMPLETE, OnEditAutoComplete)
      COMMAND_ID_HANDLER(ID_DEBUG_BREAKPOINT, OnDebugBreakpoint)
      COMMAND_ID_HANDLER(ID_DEBUG_STEP_RUN, OnDebugRunTo)
      COMMAND_ID_HANDLER(ID_DEBUG_STEP_SET, OnDebugSetNext)   
      COMMAND_ID_HANDLER(ID_DEBUG_EDIT_LINK, OnDebugLink)
      NOTIFY_CODE_HANDLER(SCN_CHARADDED, OnCharAdded)
      NOTIFY_CODE_HANDLER(SCN_MARGINCLICK, OnMarginClick)
      NOTIFY_CODE_HANDLER(SCN_DWELLSTART, OnDwellStart)
      NOTIFY_CODE_HANDLER(SCN_DWELLEND, OnDwellEnd)
      CHAIN_COMMANDS_HWND( m_ctrlEdit )
   ALT_MSG_MAP(1) // CScintillCtrl
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetEditFocus)
      MESSAGE_HANDLER(WM_KILLFOCUS, OnKillEditFocus)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnQueryEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnSetEditFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnKillEditFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
   LRESULT OnFilePrint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
   LRESULT OnEditAutoComplete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
   LRESULT OnDebugBreakpoint(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
   LRESULT OnDebugRunTo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
   LRESULT OnDebugSetNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
   LRESULT OnDebugLink(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
   LRESULT OnCharAdded(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
   LRESULT OnMarginClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
   LRESULT OnDwellStart(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
   LRESULT OnDwellEnd(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

   // IIdleListener

   void OnIdle(IUpdateUI* /*pUIBase*/);

   // Implementation

   void _AutoSuggest(CHAR ch);
   void _AutoComplete(CHAR ch);
   void _FunctionTip(CHAR ch);
   void _MatchBraces(long lPosition);
   void _SetLineIndentation(int line, int indent);
   bool _HasSelection() const;
   void _RegisterListImages();
   int _FindNext(int iFlags, LPCSTR pstrText, bool bWarnings);
   CString _FindTagType(CString& sName, long lPosition);
   CString _GetSelectedText();
   CString _GetNearText(long iPosition);
   inline bool _iscppchar(CHAR ch) const;
   inline bool _iscppchar(WCHAR ch) const;
   inline int _FunkyStrCmp(LPCTSTR src, LPCTSTR dst) const;
};


#endif // !defined(AFX_SCINTILLAVIEW_H__20030309_A76E_F01E_9DAB_0080AD509054__INCLUDED_)
