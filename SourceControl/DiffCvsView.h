#if !defined(AFX_DIFFCVSVIEW_H__20041113_23C6_DE31_0A29_0080AD509054__INCLUDED_)
#define AFX_DIFFCVSVIEW_H__20041113_23C6_DE31_0A29_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"


class IWebBrowserEvents2Base
{
public:
   static _ATL_FUNC_INFO BeforeNavigate2Info;
   static _ATL_FUNC_INFO DocumentCompleteInfo;
};
__declspec(selectany) _ATL_FUNC_INFO IWebBrowserEvents2Base::BeforeNavigate2Info = { CC_STDCALL, VT_EMPTY, 7, {VT_DISPATCH,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_BOOL} };
__declspec(selectany) _ATL_FUNC_INFO IWebBrowserEvents2Base::DocumentCompleteInfo = { CC_STDCALL, VT_EMPTY, 2, {VT_DISPATCH,VT_BSTR} };


typedef CWinTraits<WS_POPUPWINDOW 
                   | WS_CLIPCHILDREN 
                   | WS_CLIPSIBLINGS 
                   | WS_CAPTION 
                   | WS_THICKFRAME 
                   | WS_CLIPSIBLINGS 
                   | WS_CLIPCHILDREN
                   | WS_MINIMIZEBOX 
                   | WS_MAXIMIZEBOX, WS_EX_WINDOWEDGE> CPopupWinTraits;

class CDiffCvsView : 
   public CWindowImpl<CDiffCvsView, CWindow, CPopupWinTraits>,
   public IDispEventSimpleImpl<1, CDiffCvsView, &DIID_DWebBrowserEvents2>,
   public IWebBrowserEvents2Base,
   public IAppMessageListener
{
public:
   DECLARE_WND_CLASS_EX(_T("BVRDE_DiffCvs"), 0, COLOR_WINDOW)

   enum { TIMERID_LOADHTML = 310 };

   CAxWindow m_wndBrowser;
   CComQIPtr<IWebBrowser2> m_spBrowser;
   CComBSTR m_bstrHTML;

   BOOL GeneratePage(IElement* pElement, CSimpleArray<CString>& aLines);
   void OnFinalMessage(HWND hWnd);

   LRESULT OnAppMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   BOOL PreTranslateMessage(MSG* pMsg);

   BOOL _LoadHtml(IUnknown* pUnk, LPCWSTR pstrHTML);

   BOOL _ParseDiffOriginal(CSimpleArray<CString>& aFile, CSimpleArray<CString>& aLines, CString& sHTML);
   BOOL _ParseDiffContext(CSimpleArray<CString>& aFile, CSimpleArray<CString>& aLines, CString& sHTML);
   BOOL _ParseDiffUnidiff(CSimpleArray<CString>& aFile, CSimpleArray<CString>& aLines, CString& sHTML);

   // Dispatch handlers

   BEGIN_SINK_MAP(CDiffCvsView)
      SINK_ENTRY_INFO(1, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE, __DocumentComplete, &DocumentCompleteInfo)
   END_SINK_MAP()

   void __stdcall __DocumentComplete(/*[in]*/ IDispatch* pDisp, 
      /*[in]*/ BSTR bstrText);

   // Message map and handlers

   BEGIN_MSG_MAP(CDiffCvsView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
      MESSAGE_HANDLER(WM_SETFOCUS, OnTimer)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


#endif // !defined(AFX_DIFFCVSVIEW_H__20041113_23C6_DE31_0A29_0080AD509054__INCLUDED_)

