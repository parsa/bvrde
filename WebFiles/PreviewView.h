#if !defined(AFX_PREVIEWVIEW_H__20030608_47B4_32C1_3B67_0080AD509054__INCLUDED_)
#define AFX_PREVIEWVIEW_H__20030608_47B4_32C1_3B67_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CPreviewView : 
   public CWindowImpl<CPreviewView, CAxWindow>
{
public:
   DECLARE_WND_SUPERCLASS(_T("BVRDE_WebPreviewView"), CAxWindow::GetWndClassName())

   CComQIPtr<IWebBrowser> m_spBrowser;
   CString m_sLanguage;

   // Operations

   HWND Create(HWND hWndParent, RECT& rcPos = CWindow::rcDefault);
   void SetLanguage(CString& sLanguage);
   CString GetViewText();
   void SetViewText(LPCTSTR pstrText);
   void OnIdle(IUpdateUI* pUIBase);

   // Message map and handlers

   BEGIN_MSG_MAP(CPreviewView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      COMMAND_ID_HANDLER(ID_FILE_PRINT, OnFilePrint)
   END_MSG_MAP()
   
   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnFilePrint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   BOOL PreTranslateMessage(MSG* pMsg);

   // Implementation

   void WaitBusy() const;
};


#endif // !defined(AFX_PREVIEWVIEW_H__20030608_47B4_32C1_3B67_0080AD509054__INCLUDED_)

