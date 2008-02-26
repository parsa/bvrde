#if !defined(AFX_SCINTILLAVIEW_H__20030309_B76E_F01E_9DAB_0080AD509054__INCLUDED_)
#define AFX_SCINTILLAVIEW_H__20030309_B76E_F01E_9DAB_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../GenEdit/GenEdit.h"

class CSqlProject;
class CView;


class CScintillaView : 
   public CWindowImpl<CScintillaView, CScintillaCtrl>,
   public IIdleListener
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_SqlEditorView"))

   enum { TIMER_ID = 54 };

   typedef enum SQLKEYWORD
   {
      SQL_CONTEXT_UNKNOWN,
      SQL_CONTEXT_START,
      SQL_CONTEXT_IGNORE,
      SQL_CONTEXT_FIELD,
      SQL_CONTEXT_TABLE,
      SQL_CONTEXT_OPERATOR,
      SQL_CONTEXT_EXPRESSION,
   };

   typedef struct 
   {
      CSimpleArray<CString> aTables;
      CSimpleArray<CString> aAlias;
      CString sPrevTable;
      CString sPrevKeyword;
      SQLKEYWORD PrevKeyword;
      bool bIsObjectNext;
   } SQLANALYZE;

   CScintillaView();

   bool m_bAutoCompleteNext;
   CSqlProject* m_pProject;
   CString m_sFilename;
   CView* m_pView;

   bool m_bIgnoreViews;
   bool m_bAutoComplete;
   bool m_bBackgroundLoad;

   // Operations

   void Init(CSqlProject* pProject, CView* pView);

   // Message map and handlers

   BEGIN_MSG_MAP(CScintillaView)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_TIMER, OnTimer)
      MESSAGE_HANDLER(WM_QUERYENDSESSION, OnQueryEndSession)
      MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
      COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
      COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
      COMMAND_ID_HANDLER(ID_EDIT_AUTOCOMPLETE, OnEditAutoComplete)      
      COMMAND_ID_HANDLER(ID_HISTORY_NEW, OnHistoryNew)
      COMMAND_ID_HANDLER(ID_HISTORY_DELETE, OnHistoryDelete)
      COMMAND_ID_HANDLER(ID_HISTORY_LEFT, OnHistoryLeft)
      COMMAND_ID_HANDLER(ID_HISTORY_RIGHT, OnHistoryRight)
      REFLECTED_NOTIFY_CODE_HANDLER(SCN_CHARADDED, OnCharAdded)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnQueryEndSession(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnFileOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditAutoComplete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnCharAdded(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
   LRESULT OnHistoryNew(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnHistoryDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnHistoryLeft(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnHistoryRight(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   // IIdleListener

   void OnIdle(IUpdateUI* pUIBase);
   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax);

   // Implementation

   void _RegisterListImages();
   void _AutoComplete(CHAR ch, int iLenEntered);
   SQLKEYWORD _GetKeyword(CString& sKeyword) const;
   void _AnalyseText(SQLANALYZE& Info, int iLenEntered);
   int _ScintillaCompare(LPCTSTR src, LPCTSTR dst) const;
   int _FindItem(CSimpleArray<CString>& aList, LPCTSTR pstrName) const;
   bool _IsRealSqlEditPos(long lPos, bool bIncludeNonIdentifiers) const;
   inline bool _issqlchar(CHAR ch) const;
};


#endif // !defined(AFX_SCINTILLAVIEW_H__20030309_B76E_F01E_9DAB_0080AD509054__INCLUDED_)
