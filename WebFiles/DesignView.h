#if !defined(AFX_DESIGNVIEW_H__20030608_61E7_9670_1982_0080AD509054__INCLUDED_)
#define AFX_DESIGNVIEW_H__20030608_61E7_9670_1982_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CDesignView : 
   public CWindowImpl<CDesignView, CAxWindow>
{
public:
   DECLARE_WND_SUPERCLASS(_T("BVRDE_WebDesignView"), CAxWindow::GetWndClassName())

   CComQIPtr<IWebBrowser2> m_spBrowser;
   CString m_sLanguage;

   // Operations

   HWND Create(HWND hWndParent, RECT& rcPos = CWindow::rcDefault);
   BOOL PreTranslateMessage(MSG* pMsg);
   void SetLanguage(CString& sLanguage);
   void OnIdle(IUpdateUI* pUIBase);
   CString GetViewText();
   void SetViewText(LPCTSTR pstrText);

   // Message map and handlers

   BEGIN_MSG_MAP(CDesignView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      COMMAND_ID_HANDLER(ID_FILE_PRINT, OnFilePrint)
      COMMAND_ID_HANDLER(ID_EDIT_UNDO, OnEditUndo)
      COMMAND_ID_HANDLER(ID_EDIT_REDO, OnEditRedo)
      COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
      COMMAND_ID_HANDLER(ID_EDIT_CUT, OnEditCut)
      COMMAND_ID_HANDLER(ID_EDIT_PASTE, OnEditPaste)
      COMMAND_ID_HANDLER(ID_EDIT_FIND, OnEditFind)
      COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, OnEditSelectAll)
      COMMAND_ID_HANDLER(ID_EDIT_INDENT, OnEditIndent)
      COMMAND_ID_HANDLER(ID_EDIT_UNINDENT, OnEditUnindent)
      COMMAND_ID_HANDLER(ID_EDIT_BOLD, OnEditBold)
      COMMAND_ID_HANDLER(ID_EDIT_ITALIC, OnEditItalic)
      COMMAND_ID_HANDLER(ID_EDIT_UNDERLINE, OnEditUnderline)
      COMMAND_ID_HANDLER(ID_EDIT_ALIGN_LEFT, OnEditAlignLeft)
      COMMAND_ID_HANDLER(ID_EDIT_ALIGN_MIDDLE, OnEditAlignMiddle)
      COMMAND_ID_HANDLER(ID_EDIT_ALIGN_RIGHT, OnEditAlignRight)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnFilePrint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditUndo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditRedo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditCut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditPaste(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditFind(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditSelectAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditIndent(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditUnindent(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditBold(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditItalic(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditUnderline(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditAlignLeft(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditAlignMiddle(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnEditAlignRight(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

   // Implementation

   void WaitBusy() const;
};


#endif // !defined(AFX_DESIGNVIEW_H__20030608_61E7_9670_1982_0080AD509054__INCLUDED_)

