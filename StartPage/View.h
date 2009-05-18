#if !defined(AFX_VIEW_H__20030621_A3D1_4439_D58E_0080AD509054__INCLUDED_)
#define AFX_VIEW_H__20030621_A3D1_4439_D58E_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//////////////////////////////////////////////////////////////////////////
//

class IWebBrowserEvents2Base
{
public:
   static _ATL_FUNC_INFO BeforeNavigate2Info;
   static _ATL_FUNC_INFO DocumentCompleteInfo;
};
__declspec(selectany) _ATL_FUNC_INFO IWebBrowserEvents2Base::BeforeNavigate2Info = { CC_STDCALL, VT_EMPTY, 7, {VT_DISPATCH,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_BOOL} };
__declspec(selectany) _ATL_FUNC_INFO IWebBrowserEvents2Base::DocumentCompleteInfo = { CC_STDCALL, VT_EMPTY, 2, {VT_DISPATCH,VT_BSTR} };


class CStartPageView : 
   public CWindowImpl<CStartPageView, CAxWindow>,
   public IDispEventSimpleImpl<1, CStartPageView, &DIID_DWebBrowserEvents2>,
   public IWebBrowserEvents2Base,
   public IView
{
public:
   DECLARE_WND_SUPERCLASS(_T("BVRDE_StartPage"), CAxWindow::GetWndClassName())

   CComQIPtr<IWebBrowser2> m_spBrowser;
   CWindow m_wndFrame;

   // Operations

   BOOL PreTranslateMessage(MSG* pMsg);
   void _AttachRecentProjectList();

   // IView

   BOOL GetName(LPTSTR pstrName, UINT cchMax) const;
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const;
   IDispatch* GetDispatch();
   BOOL IsDirty() const;
   BOOL Load(ISerializable* pArc);
   BOOL Save(ISerializable* pArc);
   void ActivateUI();
   void DeactivateUI();
   void EnableModeless(BOOL bEnable);
   IElement* GetParent() const;
   BOOL SetName(LPCTSTR pstrName);
   BOOL Save();
   BOOL Reload();
   BOOL OpenView(long lLineNum);
   void CloseView();
   BOOL GetText(BSTR* pbstrText);
   BOOL GetFileName(LPTSTR pstrName, UINT cchMax) const;
   LRESULT PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
   LRESULT SendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);

   // Message map and handlers

   BEGIN_MSG_MAP(CStartPageView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
   END_MSG_MAP()
   
   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

   // Dispatch map and handlers

   BEGIN_SINK_MAP(CStartPageView)
      SINK_ENTRY_INFO(1, DIID_DWebBrowserEvents2, DISPID_BEFORENAVIGATE2, &__BeforeNavigate2, &BeforeNavigate2Info)
      SINK_ENTRY_INFO(1, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE, &__DocumentComplete, &DocumentCompleteInfo)
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
};


#endif // !defined(AFX_VIEW_H__20030621_A3D1_4439_D58E_0080AD509054__INCLUDED_)

