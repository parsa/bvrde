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

   typedef enum MEMBERMATCHMODE
   {
      MATCH_NORMAL = 0,
      MATCH_ALLOW_PARTIAL,
      MATCH_LHS,
   };

   enum { MAX_CONTEXT_ENTRIES = 10 };

   typedef struct {
      CString sName;
      DWORD dwTimestamp;
   } CONTEXTENTRY;

   typedef struct 
   {
      int iLineNum;
      long lStartPos;
      long lEndPos;
      CString sText;
   } ERRORMARKINFO;

   typedef struct 
   {
      CString sName;
      CString sType;
      CString sScope;
      CSimpleArray<CString> aDecl;
   } MEMBERINFO;

   typedef struct
   {
      long lPos;
      long lCurTip;
      bool bExpand;
      CString sMemberName;
      CString sMemberType;
      CString sMemberScope;
      CString sMemberNamespace;
      COLORREF clrBack;
      COLORREF clrText;
      CString sText;
      CSimpleArray<CString> aDecl;
   } TOOLTIPINFO;

   CContainedWindowT<CScintillaCtrl> m_ctrlEdit;     // The Scintilla edit control
   IProject* m_pProject;                             // Reference to the project
   CRemoteProject* m_pCppProject;                    // Reference to the Remote CPP project (if attached)
   IView* m_pView;                                   // Reference to the view
   CString m_sFilename;                              // Name of file
   CString m_sLanguage;                              // Language
   CString m_aOutputToken[4];                        // Match substring for compile view
   bool m_bAutoIndent;                               // Do we need to auto-indent text?
   bool m_bSmartIndent;                              // Do we need to smart-indent text?
   bool m_bProtectDebugged;                          // Read-Only file when debugging?
   bool m_bAutoComplete;                             // Use auto-complete?
   bool m_bMarkErrors;                               // Mark errors with squiggly lines?
   bool m_bDelayedHoverData;                         // Awaiting mouse-hover information?
   bool m_bDwellEnds;                                // Accept tooltip timeout period?
   bool m_bSuggestionDisplayed;                      // Context-suggestion displayed?
   TOOLTIPINFO m_TipInfo;                            // Information about displayed tooltip
   CONTEXTENTRY m_aContexts[MAX_CONTEXT_ENTRIES];    // Recent Context entries
   CSimpleArray<ERRORMARKINFO> m_aErrorInfo;         // Remove squiggly lines?

   // Operations

   void OnFinalMessage(HWND hWnd);

   BOOL Init(CRemoteProject* pCppProject, IProject* pProject, IView* pView, LPCTSTR pstrFilename, LPCTSTR pstrLanguage);
   BOOL GetText(LPSTR& pstrText);
   BOOL SetText(LPCSTR pstrText);
   int FindText(UINT iFlags, LPCTSTR pstrText, bool bShowWarnings);

   // Message map and handlers

   BEGIN_MSG_MAP(CScintillaView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_QUERYENDSESSION, OnQueryEndSession)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
      MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
      MESSAGE_HANDLER(WM_HELP, OnHelp)
      COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
      COMMAND_ID_HANDLER(ID_EDIT_OPENINCLUDE, OnEditOpenInclude)
      COMMAND_ID_HANDLER(ID_EDIT_OPENDECLARATION, OnEditOpenDeclaration)
      COMMAND_ID_HANDLER(ID_EDIT_OPENIMPLEMENTATION, OnEditOpenImplementation)
      COMMAND_ID_HANDLER(ID_EDIT_AUTOCOMPLETE, OnEditAutoComplete)
      COMMAND_ID_HANDLER(ID_DEBUG_BREAKPOINT, OnDebugBreakpoint)
      COMMAND_ID_HANDLER(ID_DEBUG_STEP_RUN, OnDebugRunTo)
      COMMAND_ID_HANDLER(ID_DEBUG_STEP_SET, OnDebugSetNext)   
      COMMAND_ID_HANDLER(ID_DEBUG_EDIT_LINK, OnDebugLink)
      NOTIFY_CODE_HANDLER(SCN_CHARADDED, OnCharAdded)
      NOTIFY_CODE_HANDLER(SCN_MODIFIED, OnModified)
      NOTIFY_CODE_HANDLER(SCN_MARGINCLICK, OnMarginClick)
      NOTIFY_CODE_HANDLER(SCN_CALLTIPCLICK, OnCallTipClick)
      NOTIFY_CODE_HANDLER(SCN_DWELLSTART, OnDwellStart)
      NOTIFY_CODE_HANDLER(SCN_DWELLEND, OnDwellEnd)
      NOTIFY_CODE_HANDLER(SCN_AUTOCSELECTION, OnAutoExpand)
      CHAIN_COMMANDS_HWND( m_ctrlEdit )
   ALT_MSG_MAP(1) // CScintillCtrl
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetEditFocus)
      MESSAGE_HANDLER(WM_KILLFOCUS, OnKillEditFocus)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnQueryEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSetEditFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnKillEditFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFilePrint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditAutoComplete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditOpenInclude(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditOpenDeclaration(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditOpenImplementation(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugBreakpoint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugRunTo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugSetNext(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnDebugLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnCharAdded(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnModified(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnMarginClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnCallTipClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnDwellStart(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnDwellEnd(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnAutoExpand(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);

   // IOutputLineListener
   
   void OnIncomingLine(VT100COLOR nColor, LPCTSTR pstrText);

   // Implementation

   void _AutoComplete(int ch);
   void _AutoSuggest(int ch);
   void _FunctionTip(int ch);
   void _ClearAllSquigglyLines();
   void _ClearSquigglyLine(long lPos);
   void _MatchBraces(long lPos);
   void _SetLineIndentation(int line, int indent);
   void _RegisterListImages();
   bool _HasSelection() const;
   int _CountCommas(LPCTSTR pstrText) const;
   bool _IsRealCppEditPos(long lPos, bool bIncludeNonIdentifiers) const;
   void _ShowToolTip(long lPos, CString sText, bool bAdjustPos, bool bAcceptTimeout, COLORREF clrText, COLORREF clrBack);
   void _ShowMemberToolTip(long lPos, CTagDetails* pInfo, long lCurTip, bool bFilterMembers, bool bExpand, bool bAdjustPos, bool bAcceptTimeout, COLORREF clrBack, COLORREF clrText);
   bool _GetMemberInfo(long lPos, CTagDetails& Info, DWORD dwTimeout, MEMBERMATCHMODE Mode);
   bool _GetContextList(long lPos, DWORD dwTimeout, CSimpleValArray<TAGINFO*>& aList);
   bool _AddContextEntry(LPCTSTR pstrEntry);
   CString _FindBlockType(long lPos);
   CString _FindIncludeUnderCursor(long lPos);
   bool _FindLocalVariableType(const CString& sName, long lPos, CTagDetails& Info);
   CString _UndecorateType(CString sType);
   CString _GetSelectedText();
   CString _GetNearText(long lPosition, bool bExcludeKeywords = true);
   bool _iswhitechar(int ch) const;
   bool _issepchar(WCHAR ch) const;
   bool _iscppchar(int ch) const;
   bool _iscppcharw(WCHAR ch) const;
};


#endif // !defined(AFX_SCINTILLAVIEW_H__20030309_A76E_F01E_9DAB_0080AD509054__INCLUDED_)
