
#include "StdAfx.h"
#include "resource.h"

#include "Generator.h"


UINT CManPageGenerator::Generate(HWND hWnd, LPCTSTR pstrKeyword, LPCTSTR pstrLanguage, long& lPos, long& lCount, CComBSTR& bstrResult)
{
   ATLASSERT(!::IsBadStringPtr(pstrKeyword,-1));

   ISolution* pSolution = _pDevEnv->GetSolution();
   if( pSolution == NULL ) return IDS_ERR_MEMORY;

   IProject* pProject = pSolution->GetActiveProject();
   if( pProject == NULL ) return IDS_ERR_MEMORY;

   TCHAR szProjectType[64] = { 0 };
   pProject->GetClass(szProjectType, 63);
   if( _tcscmp(szProjectType, _T("Remote C++")) != 0 ) return IDS_ERR_WRONGPROJECT;

   CComDispatchDriver dd = pProject->GetDispatch();
   
   CComVariant vConnected;
   if( FAILED( dd.GetPropertyByName(L"IsConnected", &vConnected) ) ) return IDS_ERR_WRONGPROJECT;
   if( vConnected.vt != VT_BOOL || vConnected.boolVal == VARIANT_FALSE ) return IDS_ERR_NOTCONNECTED;

   // First we try to get the 'man' utilty to print the actual filename(s).
   // We need this so we can send one to the 'man2html' utility...
   TCHAR szCommand[300] = { 0 };
   ::wsprintf(szCommand, _T("man -wa %s"), pstrKeyword);

   CComBSTR bstrCommand = szCommand;
   m_sResult = L"";

   CComVariant aParams[3];
   aParams[2] = bstrCommand;
   aParams[1] = (IUnknown*) this;
   aParams[0] = 3000L;
   if( FAILED( dd.InvokeN(OLESTR("ExecCommand"), aParams, 3) ) ) return IDS_ERR_INVOKE;
   if( m_sResult.Length() == 0 ) return IDS_ERR_BADANSWER;
   if( wcsstr(m_sResult, L"command not found") != NULL ) return IDS_ERR_NOTSUPPORTED;
   if( wcsstr(m_sResult, L"unknown option") != NULL ) return IDS_ERR_BADVERSION;

   TCHAR szFilename[MAX_PATH] = { 0 };
   if( !_FindFilename(m_sResult, lPos, lCount, szFilename) ) return IDS_ERR_NOTFOUND;

   // Now we can generate the html document...
   if( _tcsstr(szFilename, L".gz") != NULL ) {
      ::wsprintf(szCommand, _T("gzip -cd %s | man2html"), szFilename);
   }
   else {
      ::wsprintf(szCommand, _T("man2html %s"), szFilename);
   }

   bstrCommand = szCommand;
   m_sResult = L"";

   aParams[2] = bstrCommand;
   aParams[1] = (IUnknown*) this;
   aParams[0] = 6000L;
   if( FAILED( dd.InvokeN(OLESTR("ExecCommand"), aParams, 3) ) ) return IDS_ERR_INVOKE;
   if( m_sResult.Length() == 0 ) return IDS_ERR_BADANSWER;
   if( wcsstr(m_sResult, L"command not found") != NULL ) return IDS_ERR_NOTSUPPORTED;

   if( wcsstr(m_sResult, L"<HTML>") == NULL ) return IDS_ERR_NOTFOUND;
   bstrResult = wcsstr(m_sResult, L"<HTML>");

   // We should shine up the HTML a bit to look presentable.
   // This is after all a Windows machine, and not a crummy UNIX box.
   // A lot can be done just by adding a style-sheet, so that's what we're doing here...
   _StrReplace(bstrResult, L"<HTML>", L"\xFEFF<HTML>");
   _StrReplace(bstrResult, L"</HEAD>", L"<LINK href=\"res://manpages.dll/style.css\" type=\"text/css\" rel=\"stylesheet\"></HEAD>");

   return 0;
}

bool CManPageGenerator::_FindFilename(BSTR bstr, long& lPos, long& nCount, LPTSTR pstrFilename) const
{
   USES_CONVERSION;

   // If user didn't specify which item to display, we'll choose
   // the first one found.
   // TODO: Try to match the best one (maybe using language??)
   if( lPos < 0 ) lPos = 0;

   nCount = 0;
   pstrFilename[0] = '\0';

   while( true ) {
      // Looks like a path to you?
      if( *bstr == '/' ) {
         // Grab the line
         WCHAR szLine[200] = { 0 };
         LPCWSTR pstrEnd = wcschr(bstr, '\n');
         if( pstrEnd == NULL ) wcsncpy(szLine, bstr, 199); else wcsncpy(szLine, bstr, min(199, pstrEnd - bstr));
         // Did the UNIX box return a link?
         // Apparently man2html spits out file-links like this:
         //   /usr/man/cat1/man2html.1.gz (<-- /usr/man/man1/man2html.1.gz) 
         if( wcsstr(szLine, L"(<--") != NULL ) {
            while( wcschr(szLine, ')') != NULL ) *(wcschr(szLine, ')')) = '\0';
            bstr = wcsstr(szLine, L"(<--") + 3;
         }
         // Return filename
         if( lPos == nCount++ ) _tcsncpy(pstrFilename, OLE2CT(szLine), MAX_PATH);
      }
      bstr = wcschr(bstr, '\n');
      if( bstr == NULL ) break;
      bstr++;
   }

   return pstrFilename[0] != '\0';
}

DWORD CManPageGenerator::_StrReplace(CComBSTR& bstr, LPCWSTR pstrSearchFor, LPCWSTR pstrReplaceWith) const
{
   LPWSTR p = NULL;
   int nMatch = -1;
   DWORD   dwCount = 0;
   size_t cchSearch = wcslen(pstrSearchFor);
   size_t cchReplace = wcslen(pstrReplaceWith);
   while( (p = wcsstr(bstr.m_str + nMatch + 1, pstrSearchFor)) != NULL )
   {
      nMatch = p - bstr.m_str;
      size_t cchText = bstr.Length();
      CComBSTR bstrResult( cchText + (cchReplace - cchSearch) );
      wcsncpy(bstrResult.m_str, bstr, nMatch);
      wcsncpy(bstrResult.m_str + nMatch, pstrReplaceWith, cchReplace);
      wcscpy(bstrResult.m_str + nMatch + cchReplace, bstr.m_str + nMatch + cchSearch);
      bstr = bstrResult;
      dwCount++;
      nMatch += cchReplace;
   }
   return dwCount;
}


// ILineCallback

HRESULT CManPageGenerator::OnIncomingLine(BSTR bstr)
{
   m_sResult += bstr;
   m_sResult += _T("\n");
   return S_OK;
}

// IUnknown

HRESULT CManPageGenerator::QueryInterface(REFIID riid, void** ppvObject)
{
   if( riid == __uuidof(ILineCallback) || riid == IID_IUnknown ) {
      *ppvObject = (ILineCallback*) this;
      return S_OK;
   }
   return E_NOINTERFACE;
}

ULONG CManPageGenerator::AddRef(void)
{
   return 1;
}

ULONG CManPageGenerator::Release(void)
{
   return 1;
}

