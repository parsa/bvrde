
#include "StdAfx.h"
#include "resource.h"

#include "Macro.h"
#include "MainFrm.h"

#pragma code_seg( "DIALOGS" )

#define SCRIPT_PROGID  L"VBScript"


// Operations

void CMacro::Init(CMainFrame* pFrame, CApplicationOM* pApp)
{
   m_pMainFrame = pFrame;
   m_Globals.Init(pApp);
   m_hWnd = m_pMainFrame->m_hWnd;
}

bool CMacro::RunMacroFromFile(LPCTSTR pstrFilename, LPCTSTR pstrFunction)
{
   ATLASSERT(m_pMainFrame);
   ATLASSERT(::IsWindow(m_hWnd));
   ATLASSERT(!::IsBadStringPtr(pstrFilename,-1));

   // Load the file (into stack buffer)
   CFile f;
   if( !f.Open(pstrFilename) ) {
      m_pMainFrame->_ShowMessageBox(m_hWnd, IDS_ERR_FILEOPEN, IDS_CAPTION_ERROR, MB_ICONEXCLAMATION);
      return false;
   }
   DWORD dwSize = f.GetSize();
   LPSTR pstr = new CHAR[dwSize + 1];
   f.Read(pstr, dwSize);
   f.Close();
   pstr[dwSize] = '\0';

   bool bRes = RunMacroFromScript(pstr, pstrFunction);
   
   delete [] pstr;
   return bRes;
}

bool CMacro::RunMacroFromScript(CComBSTR bstrData, 
                                CComBSTR bstrFunction, 
                                DWORD dwFlags /*= 0*/, 
                                VARIANT* pvarResult /*= NULL*/, 
                                EXCEPINFO* pExcepInfo /*= NULL*/)
{
   // Create the Active Scripting Engine.
   CComPtr<IActiveScript> spScript;
   HRESULT Hr;
   Hr = spScript.CoCreateInstance(SCRIPT_PROGID);
   if( FAILED(Hr) ) {
      // No VB Script?
      m_pMainFrame->_ShowMessageBox(m_hWnd, IDS_ERR_VBSCRIPT, IDS_CAPTION_ERROR, MB_ICONERROR);
      return false; 
   }

   // Remember exception information if we handle
   // them locally.
   m_pExcepInfo = pExcepInfo;

   CComQIPtr<IActiveScriptSite> spPass = this;
   if( spPass == NULL ) return false;

   Hr = spScript->SetScriptSite(spPass);
   if( FAILED(Hr) ) return false;

   CComQIPtr<IActiveScriptParse> spParse = spScript;
   if( spParse == NULL ) return false;

   spParse->InitNew();

   // Add the custom methods...
   // They are initialized in the GetItemInfo() method (see below).
   Hr = spScript->AddNamedItem(OLESTR("Globals"), SCRIPTITEM_ISVISIBLE | SCRIPTITEM_GLOBALMEMBERS);
   if( FAILED(Hr) ) return false;
   Hr = spScript->AddNamedItem(OLESTR("App"), SCRIPTITEM_ISVISIBLE);
   if( FAILED(Hr) ) return false;

   // Start the scripting engine...
   m_spIActiveScript = spScript;
   Hr = spScript->SetScriptState(SCRIPTSTATE_STARTED);
   if( FAILED(Hr) ) return false;

   // Run the script...
   Hr = spParse->ParseScriptText(bstrData,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 0,
                                 dwFlags,
                                 pvarResult,
                                 pExcepInfo);

   // Now, invoke our function! The is almost magically easy
   // due to ATL helper classes.
   if( bstrFunction.Length() > 0 ) {
      CComDispatchDriver spDisp;
      Hr = spScript->GetScriptDispatch(NULL, &spDisp);
      if( FAILED(Hr) ) return false;
      Hr = spDisp.Invoke0(bstrFunction, NULL);
      if( FAILED(Hr) ) return false;
   }

   spScript->SetScriptState(SCRIPTSTATE_CLOSED);

   return true;
}

// IActiveScriptSite

STDMETHODIMP CMacro::GetItemInfo(LPCOLESTR pstrName,
                                 DWORD dwReturnMask,
                                 IUnknown** ppUnk,
                                 ITypeInfo** ppti)
{
   ATLASSERT(pstrName);
   if( (dwReturnMask & SCRIPTINFO_ITYPEINFO) != 0 ) {
      *ppti = NULL;
      return E_FAIL;
   }
   if( (dwReturnMask & SCRIPTINFO_IUNKNOWN) == 0 ) return E_FAIL;
   if( ppUnk == NULL ) return E_POINTER;
   *ppUnk = NULL;
   if( wcsicmp( pstrName, L"Globals" ) == 0 ) *ppUnk = &m_Globals;
   if( wcsicmp( pstrName, L"App" ) == 0 ) *ppUnk = g_pDevEnv->GetDispatch();
   return *ppUnk == NULL ? E_FAIL : S_OK;
}

STDMETHODIMP CMacro::GetDocVersionString(BSTR* pbstrVersion)
{
   if( pbstrVersion == NULL ) return E_POINTER;
   CComBSTR bstr;
   bstr.LoadString(IDR_MAINFRAME);
   *pbstrVersion = bstr.Detach();
   return S_OK;
}

STDMETHODIMP CMacro::OnScriptError(IActiveScriptError* pScriptError)
{
   ATLASSERT(pScriptError);
   ATLASSERT(m_pMainFrame);
   EXCEPINFO e = { 0 };
   pScriptError->GetExceptionInfo(&e);
   if( m_pExcepInfo) {
      *m_pExcepInfo = e;
   }
   else {
      DWORD dwContext = 0;
      ULONG ulLine = 0;
      LONG lPos = 0;
      pScriptError->GetSourcePosition(&dwContext, &ulLine, &lPos);
      CString sMsg;
      sMsg.Format(IDS_ERR_SCRIPT, 
         e.bstrSource,
         e.scode,
         e.bstrDescription,
         ulLine + 1);
      m_pMainFrame->ShowMessageBox(m_hWnd, sMsg, CString(MAKEINTRESOURCE(IDS_CAPTION_ERROR)), MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
   }
   if( m_spIActiveScript ) m_spIActiveScript->SetScriptState(SCRIPTSTATE_DISCONNECTED);
   m_bNoFailures = false;
   return S_OK;
}

