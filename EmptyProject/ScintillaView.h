#if !defined(AFX_SCINTILLAVIEW_H__20030309_A76E_F01E_9DAB_0080AD509054__INCLUDED_)
#define AFX_SCINTILLAVIEW_H__20030309_A76E_F01E_9DAB_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Forward declare
class CEmptyProject;

#include "../GenEdit/GenEdit.h"


class CScintillaView : 
   public CWindowImpl<CScintillaView>,
   public IIdleListener
{
public:
   DECLARE_WND_CLASS(_T("BVRDE_StdEditorView"))

   CScintillaView();

   IProject* m_pProject;
   IView* m_pView;
   CContainedWindowT<CScintillaCtrl> m_ctrlEdit;
   CString m_sFilename;
   CString m_sLanguage;

   // Operations

   void OnFinalMessage(HWND hWnd);

   BOOL Init(IProject* pProject, IView* pView, LPCTSTR pstrFilename, LPCTSTR pstrLanguage);
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
      CHAIN_COMMANDS_HWND( m_ctrlEdit )
   ALT_MSG_MAP(1) // CScintillCtrl
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetEditFocus)
      MESSAGE_HANDLER(WM_KILLFOCUS, OnKillEditFocus)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnQueryEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnSetEditFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnKillEditFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
   LRESULT OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
   LRESULT OnFilePrint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

   // IIdleListener

   void OnIdle(IUpdateUI* /*pUIBase*/);
};


#endif // !defined(AFX_SCINTILLAVIEW_H__20030309_A76E_F01E_9DAB_0080AD509054__INCLUDED_)
