#if !defined(AFX_FRAME_H__20041113_23C6_DE31_0A29_0080AD509054__INCLUDED_)
#define AFX_FRAME_H__20041113_23C6_DE31_0A29_0080AD509054__INCLUDED_

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
__declspec(selectany) _ATL_FUNC_INFO IWebBrowserEvents2Base::DocumentCompleteInfo = {CC_STDCALL, VT_EMPTY, 2, {VT_DISPATCH,VT_BSTR}};


typedef CWinTraits<WS_POPUPWINDOW 
                   | WS_CLIPCHILDREN 
                   | WS_CLIPSIBLINGS 
                   | WS_CAPTION 
                   | WS_THICKFRAME 
                   | WS_CLIPSIBLINGS 
                   | WS_CLIPCHILDREN
                   | WS_MINIMIZEBOX 
                   | WS_MAXIMIZEBOX, WS_EX_WINDOWEDGE> CPopupWinTraits;

class CFrameWindow : 
   public CWindowImpl<CFrameWindow, CWindow, CPopupWinTraits>,
   public IDispEventSimpleImpl<1, CFrameWindow, &DIID_DWebBrowserEvents2>,
   public IWebBrowserEvents2Base
{
public:
   DECLARE_WND_CLASS_EX(_T("BVRDE_ManPage"), 0, COLOR_BTNFACE)

   typedef struct PAGEINFO
   {
      CComBSTR bstrKeyword;
      CComBSTR bstrLanguage;
      long lPos;
   };

   CToolBarCtrl m_ctrlToolBar;
   CAxWindow m_wndBrowser;
   CComQIPtr<IWebBrowser2> m_spBrowser;
   CComBSTR m_bstrKeyword;
   CComBSTR m_bstrLanguage;
   CSimpleArray<PAGEINFO> m_aStack;
   long m_lMatchPos;
   long m_nMatchCount;

   BOOL SetPage(LPCTSTR pstrKeyword, LPCTSTR pstrLanguage, long lPos);
   BOOL PreTranslateMessage(MSG* pMsg);
   void OnFinalMessage(HWND hWnd);

   BOOL _LoadHtml(IUnknown* pUnk, LPCWSTR pstrHTML);
   void _UpdateButtons();

   // Dispatch handlers

   BEGIN_SINK_MAP(CFrameWindow)
      SINK_ENTRY_INFO(1, DIID_DWebBrowserEvents2, DISPID_BEFORENAVIGATE2, __BeforeNavigate2, &BeforeNavigate2Info)
      SINK_ENTRY_INFO(1, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE, __DocumentComplete, &DocumentCompleteInfo)
   END_SINK_MAP()

   void __stdcall __BeforeNavigate2(/*[in]*/ IDispatch* pDisp, 
      /*[in]*/ VARIANT* URL, 
      /*[in]*/ VARIANT* Flags, 
      /*[in]*/ VARIANT* TargetFrameName, 
      /*[in]*/ VARIANT* PostData, 
      /*[in]*/ VARIANT* Headers, 
      /*[out]*/ VARIANT_BOOL* Cancel);
   void __stdcall __DocumentComplete(/*[in]*/ IDispatch* pDisp, 
      /*[in]*/ BSTR bstrText);

   // Message map and handlers

   BEGIN_MSG_MAP(CFrameWindow)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
      COMMAND_ID_HANDLER(ID_FILE_PRINT, OnFilePrint)
      COMMAND_ID_HANDLER(ID_FILE_CLOSE, OnFileClose)
      COMMAND_ID_HANDLER(ID_VIEW_PREVIOUS, OnViewPrevious)
      COMMAND_ID_HANDLER(ID_VIEW_NEXT, OnViewNext)
      COMMAND_ID_HANDLER(ID_VIEW_BACK, OnViewBack)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnFilePrint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnFileClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewPrevious(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewNext(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
   LRESULT OnViewBack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};


#endif // !defined(AFX_FRAME_H__20041113_23C6_DE31_0A29_0080AD509054__INCLUDED_)

