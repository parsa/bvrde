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
   public IOutputLineListener,
   public IIdleListener
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_CppEditorView"))

   CScintillaView();

   IProject* m_pProject;            // Reference to the project
   CRemoteProject* m_pCppProject;   // Reference to the Remote CPP project (if attached)
   IView* m_pView;                  // Reference to the view
   CContainedWindowT<CScintillaCtrl> m_ctrlEdit;
   CString m_sFilename;             // Name of file
   CString m_sLanguage;             // Language
   CString m_sOutputToken;          // Match substring for compile view
   CString m_sDwellText;            // Current tip text
   int m_iOutputLine;               // Last matched compile view line
   bool m_bClearSquigglyLines;      // Remove squiggly lines?
   bool m_bAutoIndent;              // Do we need to auto-indent text?
   bool m_bSmartIndent;             // Do we need to smart-indent text?
   bool m_bProtectDebugged;         // Read-Only file when debugging?
   bool m_bAutoComplete;            // Use auto-complete?
   bool m_bMarkErrors;              // Mark errors with squiggly lines?
   bool m_bMouseDwell;              // Awaiting mouse-hover information?
   long m_lDwellPos;                // Position where mouse-hover occoured

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
      MESSAGE_HANDLER(WM_HELP, OnHelp)
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
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnQueryEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSetEditFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnKillEditFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFilePrint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditAutoComplete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugBreakpoint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugRunTo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugSetNext(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnCharAdded(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnMarginClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnDwellStart(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnDwellEnd(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);

   // IOutputLineListener
   
   void OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText);

   // Implementation

   void _AdjustToolTip();
   void _AutoComplete(CHAR ch);
   void _FunctionTip(CHAR ch);
   void _ClearSquigglyLines();
   void _MatchBraces(long lPosition);
   void _SetLineIndentation(int line, int indent);
   void _RegisterListImages();
   bool _HasSelection() const;
   bool _IsRealCppEditPos(long lPos) const;
   int _FindNext(int iFlags, LPCSTR pstrText, bool bWarnings);
   CString _FindBlockType(long lPosition);
   CString _FindTagType(const CString& sName, long lPosition);
   CString _GetSelectedText();
   CString _GetNearText(long iPosition);
   inline bool _iscppchar(CHAR ch) const;
   inline bool _iscppchar(WCHAR ch) const;
   inline int _FunkyStrCmp(LPCTSTR src, LPCTSTR dst) const;
};


#endif // !defined(AFX_SCINTILLAVIEW_H__20030309_A76E_F01E_9DAB_0080AD509054__INCLUDED_)
