#if !defined(AFX_GENERATOR_H__20041113_9B8C_BD80_284F_0080AD509054__INCLUDED_)
#define AFX_GENERATOR_H__20041113_9B8C_BD80_284F_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CManPageGenerator : 
   public ILineCallback
{
public:
   CComBSTR m_sResult;

   UINT Generate(HWND hWnd, LPCTSTR pstrKeyword, LPCTSTR pstrLanguage, long& lPos, long& lCount, CComBSTR& bstrResult);
   
   bool _FindFilename(BSTR bstr, long& lPos, long& lCount, LPTSTR pstrFilename) const;
   DWORD _StrReplace(CComBSTR& bstr, LPCWSTR pstrSearchFor, LPCWSTR pstrReplaceWith) const;

   // ILineCallback

   HRESULT STDMETHODCALLTYPE OnIncomingLine(BSTR bstr);

   // IUnknown

   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);
};


#endif // !defined(AFX_GENERATOR_H__20041113_9B8C_BD80_284F_0080AD509054__INCLUDED_)

