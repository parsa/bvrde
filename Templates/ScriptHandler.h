#if !defined(AFX_SCRIPTHANDLER_H__20040730_1F59_ADA4_054D_0080AD509054__INCLUDED_)
#define AFX_SCRIPTHANDLER_H__20040730_1F59_ADA4_054D_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CResponse : public IDispDynImpl<CResponse>
{
public:
   CComBSTR m_bstrResult;

   BEGIN_DISPATCH_MAP(CResponse)
      DISP_METHOD0(Clear, VT_EMPTY)
      DISP_METHOD1(Write, VT_EMPTY, VT_BSTR)
   END_DISPATCH_MAP()

   VOID __stdcall Clear()
   {
      m_bstrResult = L"";
   }
   VOID __stdcall Write(BSTR str)
   {
      m_bstrResult += str;
   }
};

class CGlobals : public IDispDynImpl<CGlobals>
{
public:
   CSimpleMap<CComBSTR, CComBSTR> m_map;

   BEGIN_DISPATCH_MAP(CGlobals)
      DISP_PROPGET(Count, VT_I4)
      //DISP_PROP_ID(Item, DISPID_VALUE, VT_BSTR, VT_VARIANT)
      { OLESTR("Item"), DISPID_VALUE, DISPATCH_PROPERTYGET, VT_BSTR, 1, { VT_BSTR }, (void (__stdcall _atl_disp_classtype::*)())get_Item }, \
      { OLESTR("Item"), DISPID_PROPERTYPUT, DISPATCH_PROPERTYPUT, VT_EMPTY, 2, { VT_BSTR, VT_BSTR }, (void (__stdcall _atl_disp_classtype::*)())put_Item },
   END_DISPATCH_MAP()

   LONG __stdcall get_Count()
   {
      return (LONG) m_map.GetSize();
   }
   BSTR __stdcall get_Item(BSTR Index)
   {
      CComBSTR bstrKey = Index;
      int iIndex = m_map.FindKey(bstrKey);
      if( iIndex < 0 ) return CComBSTR().Detach();
      CComBSTR bstrValue = m_map.GetValueAt(iIndex);
      return bstrValue.Detach();
   }
   VOID __stdcall put_Item(BSTR Index, BSTR Value)
   {
      CComBSTR bstrKey = Index;
      CComBSTR bstrValue = Value;
      if( !m_map.SetAt(bstrKey, bstrValue) ) m_map.Add(bstrKey, bstrValue);
   }
};

class CContainer : public IDispDynImpl<CContainer>
{
public:
   CComPtr<IDispatch> m_spDevEnv;
   CComPtr<IDispatch> m_spSolution;
   CComPtr<IDispatch> m_spProject;
   CComPtr<IDispatch> m_spView;
   CResponse m_Response;
   CGlobals m_Globals;

   void Init(IDispatch* pDevEnv, IDispatch* pSolution, IDispatch* pProject, IDispatch* pView)
   {
      m_spDevEnv = pDevEnv;
      m_spSolution = pSolution;
      m_spProject = pProject;
      m_spView = pView;
      m_Response.Clear();
   }

   BEGIN_DISPATCH_MAP(CContainer)
      DISP_PROPGET(App, VT_DISPATCH)
      DISP_PROPGET(Response, VT_DISPATCH)
      DISP_PROPGET(Globals, VT_DISPATCH)
      DISP_PROPGET(Solution, VT_DISPATCH)
      DISP_PROPGET(Project, VT_DISPATCH)
      DISP_PROPGET(File, VT_DISPATCH)
   END_DISPATCH_MAP()

   IDispatch* __stdcall get_App() { return m_spDevEnv; };
   IDispatch* __stdcall get_Response() { return &m_Response; };
   IDispatch* __stdcall get_Globals() { return &m_Globals; };
   IDispatch* __stdcall get_Solution() { return m_spSolution; };
   IDispatch* __stdcall get_Project() { return m_spProject; };
   IDispatch* __stdcall get_File() { return m_spView; };
};



class ATL_NO_VTABLE CScriptHandler :
   public CComObjectRootEx<CComSingleThreadModel>,
   public IActiveScriptSiteImpl<CScriptHandler>,
   public IActiveScriptSiteWindowImpl<CScriptHandler>
{
public:
   HWND m_hWnd;
   IDispatch* m_pDevEnv;
   CContainer m_Container;

   void Init(HWND hWnd, IDispatch* pDevEnv, IDispatch* pSolution, IDispatch* pProject, IDispatch* pView)
   {
      m_hWnd = hWnd;
      m_pDevEnv = pDevEnv;
      m_Container.Init(pDevEnv, pSolution, pProject, pView);
   }

   bool ShowDialog(LPCTSTR pstrName, LPCTSTR pstrHtmlFilename)
   {
      USES_CONVERSION;

      if( ::lstrlen(pstrName) == 0 ) return true;

      // Dynamically load MSHTML for getting ShowHTMLDialog function
      HINSTANCE hInst = ::LoadLibrary(_T("MSHTML.DLL"));
      if( hInst == NULL ) return FALSE;
      // The SHOWHTMLDIALOGFN declare is from the IE SDK.
      SHOWHTMLDIALOGFN *pfnShowHTMLDialog = (SHOWHTMLDIALOGFN*) ::GetProcAddress(hInst, "ShowHTMLDialog");
      if( pfnShowHTMLDialog == NULL ) return FALSE;

      CComBSTR bstrFilename = L"file:///";
      bstrFilename += T2CW(pstrHtmlFilename);

      CComVariant vIn = (IDispatch*) &m_Container;
      CComVariant vOut;
      CComBSTR bstrOptions = L"";

      CComPtr<IMoniker> spMoniker;
      HRESULT Hr = ::CreateURLMoniker(NULL, bstrFilename, &spMoniker);
      if( SUCCEEDED(Hr) ) {
         Hr = (*pfnShowHTMLDialog)(m_hWnd, spMoniker.p, &vIn, OLE2W(bstrOptions), &vOut);
         spMoniker.Release();
      }
      ::FreeLibrary(hInst);
      return Hr == S_OK;
   }

   bool Execute(LPCWSTR pstrScript)
   {
      if( ::lstrlenW(pstrScript) == 0 ) return true;

      CComPtr<IActiveScript> spScript;
      HRESULT Hr = spScript.CoCreateInstance(L"VBScript");
      if( FAILED(Hr) ) {
         // No VB Script?
         ::MessageBox(m_hWnd, _T("Missing VB Script Interpreter?"), _T("Script Parse Error"), MB_ICONERROR);
         return false; 
      }

      CComQIPtr<IActiveScriptSite> spPass = this;
      if( spPass == NULL ) return false;

      Hr = spScript->SetScriptSite(spPass);
      if( FAILED(Hr) ) return false;

      CComQIPtr<IActiveScriptParse> spParse = spScript;
      if( spParse == NULL ) return false;

      spParse->InitNew();

      // Add the custom methods...
      if( FAILED( spScript->AddNamedItem(OLESTR("Container"), SCRIPTITEM_ISVISIBLE | SCRIPTITEM_GLOBALMEMBERS) ) ) return false;

      // Start the scripting engine...
      m_spIActiveScript = spScript;
      Hr = spScript->SetScriptState(SCRIPTSTATE_STARTED);
      if( FAILED(Hr) ) return false;

      // Run the script...
      CComBSTR bstrScript = pstrScript;
      Hr = spParse->ParseScriptText(bstrScript,
                                    NULL,
                                    NULL,
                                    NULL,
                                    0,
                                    0,
                                    0,
                                    NULL,
                                    NULL);

      spScript->SetScriptState(SCRIPTSTATE_CLOSED);

      return true;
   }

   CComBSTR GetResult() const
   {
      return m_Container.m_Response.m_bstrResult;
   }
  
   BEGIN_COM_MAP(CScriptHandler)
      COM_INTERFACE_ENTRY(IActiveScriptSite)
      COM_INTERFACE_ENTRY(IActiveScriptSiteWindow)
   END_COM_MAP()

   STDMETHOD(GetItemInfo)(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown** ppUnk, ITypeInfo** ppti)
   {
      ATLASSERT(pstrName);
      if( (dwReturnMask & SCRIPTINFO_ITYPEINFO) != 0 ) {
         *ppti = NULL;
         return E_FAIL;
      }
      if( (dwReturnMask & SCRIPTINFO_IUNKNOWN) == 0 ) return E_FAIL;
      if( ppUnk == NULL ) return E_POINTER;
      *ppUnk = NULL;
      if( wcsicmp( pstrName, L"Container" ) == 0 ) *ppUnk = &m_Container;
      return *ppUnk == NULL ? E_FAIL : S_OK;
   }

   STDMETHOD(GetDocVersionString)(BSTR* pbstrVersion)
   {
      if( pbstrVersion == NULL ) return E_POINTER;
      CComBSTR bstr = L"NewFileScript";
      *pbstrVersion = bstr.Detach();
      return S_OK;
   }

   STDMETHOD(OnScriptError)(IActiveScriptError* pScriptError)
   {
      ATLASSERT(pScriptError);
      EXCEPINFO e = { 0 };
      pScriptError->GetExceptionInfo(&e);
      DWORD dwContext = 0;
      ULONG ulLine = 0;
      LONG lPos = 0;
      pScriptError->GetSourcePosition(&dwContext, &ulLine, &lPos);
      TCHAR szMsg[500] = { 0 };
      ::wsprintf(szMsg, _T("Script Error:\r\nSource: %s\r\nCode: %08X\r\nDescription: %s\r\nLine: %ld"),
         e.bstrSource,
         e.scode,
         e.bstrDescription,
         ulLine + 1);
      ::MessageBox(m_hWnd, szMsg, _T("Script Error"), MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
      if( m_spIActiveScript ) m_spIActiveScript->SetScriptState(SCRIPTSTATE_DISCONNECTED);
      return S_OK;
   }
};


#endif // !defined(AFX_SCRIPTHANDLER_H__20040730_1F59_ADA4_054D_0080AD509054__INCLUDED_)

