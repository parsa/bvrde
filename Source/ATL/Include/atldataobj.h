#if !defined(AFX_ATLDATAOBJ_H__20040706_4F49_248D_7C5E_0080AD509054__INCLUDED_)
#define AFX_ATLDATAOBJ_H__20040706_4F49_248D_7C5E_0080AD509054__INCLUDED_

#pragma once

/////////////////////////////////////////////////////////////////////////////
// IDataObjet and OLE clipboard wrappers
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2004 Bjarke Viksoe.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//

/////////////////////////////////////////////////////////////////////////
// Misc clipboard functions

#ifdef __ATLCONV_H__

inline BOOL AtlSetClipboardText(HWND hWnd, LPCTSTR pstrText)
{
   USES_CONVERSION;
   HGLOBAL hGlobal = GlobalAlloc(GMEM_FIXED, ::lstrlen(pstrText) + 1);
   if( hGlobal == NULL ) return FALSE;
   CopyMemory(hGlobal, T2CA(pstrText), ::lstrlen(pstrText) + 1);
   if( !::OpenClipboard(hWnd) ) return FALSE;
   ::EmptyClipboard();
   ::SetClipboardData(CF_TEXT, hGlobal);
   ::CloseClipboard();
   return TRUE;
}

#endif // __ATLCONV_H__


/////////////////////////////////////////////////////////////////////////
// CRawDropSource

class CRawDropSource : public IDropSource
{
public:
   // IUnknown

   STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObject)
   {
      if( riid == __uuidof(IDropSource) || riid == IID_IUnknown ) {
         *ppvObject = this;
         return S_OK;
      }
      return E_NOINTERFACE;
   }
   ULONG STDMETHODCALLTYPE AddRef()
   {
      return 1;
   }
   ULONG STDMETHODCALLTYPE Release()
   {
      return 1;
   }
   
   // IDropSource
   
   STDMETHOD(QueryContinueDrag)(BOOL bEsc, DWORD dwKeyState)
   {
      if( bEsc ) return ResultFromScode(DRAGDROP_S_CANCEL);
      if( (dwKeyState & MK_LBUTTON) == 0 ) return ResultFromScode(DRAGDROP_S_DROP);
      return S_OK;
   }
   STDMETHOD(GiveFeedback)(DWORD)
   {
      return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);
   }
};


#ifdef __ATLCOM_H__

//////////////////////////////////////////////////////////////////////////////
// CEnumFORMATETC

class ATL_NO_VTABLE CEnumFORMATETC : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public IEnumFORMATETC
{
public:
   CSimpleValArray<FORMATETC> m_aFmtEtc;
   ULONG m_iCur;

   BEGIN_COM_MAP(CEnumFORMATETC)
      COM_INTERFACE_ENTRY(IEnumFORMATETC)
   END_COM_MAP()

public:
   CEnumFORMATETC() : m_iCur(0)
   {
   }

   void Add(FORMATETC& fmtc)
   {
      m_aFmtEtc.Add(fmtc);
   }

   // IEnumFORMATETC

   STDMETHOD(Next)(ULONG celt, LPFORMATETC lpFormatEtc, ULONG* pceltFetched)
   {
      if( pceltFetched != NULL) *pceltFetched = 0;
      if( lpFormatEtc == NULL ) return E_INVALIDARG;
      if( celt <= 0 || m_iCur >= (ULONG) m_aFmtEtc.GetSize() ) return S_FALSE;
      if( pceltFetched == NULL && celt != 1 ) return S_FALSE;
      ULONG nCount = 0;
      while( m_iCur < (ULONG) m_aFmtEtc.GetSize() && celt > 0 ) {
         *lpFormatEtc++ = m_aFmtEtc[m_iCur++];
         --celt;
         ++nCount;
      }
      if( pceltFetched != NULL ) *pceltFetched = nCount;
      return celt == 0 ? S_OK : S_FALSE;
   }
   STDMETHOD(Skip)(ULONG celt)
   {
      if( m_iCur + celt >= (ULONG) m_aFmtEtc.GetSize() ) return S_FALSE;
      m_iCur += celt;
      return S_OK;
   }
   STDMETHOD(Reset)(void)
   {
      m_iCur = 0;
      return S_OK;
   }
   STDMETHOD(Clone)(IEnumFORMATETC FAR * FAR*)
   {
      ATLTRACENOTIMPL(_T("CEnumFORMATETC::Clone"));
   }
};


//////////////////////////////////////////////////////////////////////////////
// CSimpleDataObj

template< class T >
class ATL_NO_VTABLE ISimpleDataObjImpl : public IDataObject
{
public:
   typedef struct DATAOBJ
   {
      FORMATETC FmtEtc;
      STGMEDIUM StgMed;
      BOOL bRelease;
   };
   CSimpleValArray<DATAOBJ> m_aObjects;

   ~ISimpleDataObjImpl()
   {
      for( int i = 0; i < m_aObjects.GetSize(); i++ ) ::ReleaseStgMedium(&m_aObjects[i].StgMed);
   }

   STDMETHOD(GetData)(FORMATETC* pformatetcIn, STGMEDIUM* pmedium)
   {
      ATLTRACE2(atlTraceControls,2,_T("ISimpleDataObjImpl::GetData\n"));
      T* pT = (T*) this;
      return pT->IDataObject_GetData(pformatetcIn, pmedium);
   }
   STDMETHOD(GetDataHere)(FORMATETC* /*pformatetc*/, STGMEDIUM* /*pmedium*/)
   {
      ATLTRACENOTIMPL(_T("ISimpleDataObjImpl::GetDataHere"));
   }
   STDMETHOD(QueryGetData)(FORMATETC* pformatetc)
   {
      ATLASSERT(pformatetc);
      int iIndex = _Find(pformatetc);
      if( iIndex < 0 ) return DV_E_FORMATETC;
      // BUG: Only supports one media pr format!
      if( m_aObjects[iIndex].FmtEtc.lindex != -1 ) return DV_E_LINDEX;
      if( m_aObjects[iIndex].FmtEtc.tymed != pformatetc->tymed ) return DV_E_TYMED;
      return S_OK;
   }
   STDMETHOD(GetCanonicalFormatEtc)(FORMATETC* pformatectIn, FORMATETC* /* pformatetcOut */)
   {
      pformatectIn->ptd = NULL;
      ATLTRACENOTIMPL(_T("ISimpleDataObjImpl::GetCanonicalFormatEtc"));
   }
   STDMETHOD(SetData)(FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease)
   {
      ATLASSERT(pformatetc);
      ATLASSERT(pmedium);
      if( pformatetc == NULL ) return E_POINTER;
      if( pmedium == NULL ) return E_POINTER;
      DATAOBJ obj;
      obj.FmtEtc = *pformatetc;
      obj.StgMed = *pmedium;
      obj.bRelease = fRelease;
      if( !fRelease ) _CopyStgMedium(&obj.StgMed, pmedium, pformatetc);
      int iIndex = _Find(pformatetc);
      if( iIndex < 0 ) m_aObjects.Add(obj); else m_aObjects.SetAtIndex(iIndex, obj);
      return S_OK;
   }
   STDMETHOD(EnumFormatEtc)(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc)
   {
      ATLASSERT(ppenumFormatEtc);
      if( ppenumFormatEtc == NULL ) return E_POINTER;
      *ppenumFormatEtc = NULL;
      if( dwDirection != DATADIR_GET ) return E_NOTIMPL;
      CComObject<CEnumFORMATETC>* pEnum = NULL;
      if( FAILED( CComObject<CEnumFORMATETC>::CreateInstance(&pEnum) ) ) return E_FAIL;
      for( int i = 0; i < m_aObjects.GetSize(); i++ ) pEnum->Add(m_aObjects[i].FmtEtc);
      return pEnum->QueryInterface(ppenumFormatEtc);
   }
   STDMETHOD(DAdvise)(FORMATETC* /*pformatetc*/, DWORD /*advf*/, IAdviseSink* /*pAdvSink*/, DWORD* /*pdwConnection*/)
   {
      ATLTRACENOTIMPL(_T("ISimpleDataObjImpl::DAdvise"));
   }
   STDMETHOD(DUnadvise)(DWORD dwConnection)
   {
      ATLTRACENOTIMPL(_T("ISimpleDataObjImpl::DUnadvise"));
   }
   STDMETHOD(EnumDAdvise)(IEnumSTATDATA** /*ppenumAdvise*/)
   {
      ATLTRACENOTIMPL(_T("ISimpleDataObjImpl::EnumDAdvise"));
   }

   // Helper functions

   int _Find(FORMATETC* pformatetc) const
   {
      for( int i = 0; i < m_aObjects.GetSize(); i++ ) {
         if( m_aObjects[i].FmtEtc.cfFormat == pformatetc->cfFormat ) return i;
      }
      return -1;
   }
   HRESULT _CopyStgMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc)
   {
      switch( pMedSrc->tymed ) {
      case TYMED_GDI:
      case TYMED_FILE:
      case TYMED_ENHMF:
      case TYMED_MFPICT:
      case TYMED_HGLOBAL:
         pMedDest->hGlobal = (HGLOBAL) ::OleDuplicateData(pMedSrc->hGlobal, pFmtSrc->cfFormat, NULL);
         break;
      case TYMED_ISTREAM:
         {
            pMedDest->pstm = NULL;
            if( FAILED( ::CreateStreamOnHGlobal(NULL, TRUE, &pMedDest->pstm) ) ) return E_OUTOFMEMORY;
            ULARGE_INTEGER alot = { 0, INT_MAX };
            if( FAILED( pMedSrc->pstm->CopyTo(pMedDest->pstm, alot, NULL, NULL) ) ) return E_FAIL;
         }
         break;
      case TYMED_ISTORAGE:
         {
            pMedDest->pstg = NULL;
            if( FAILED( ::StgCreateDocfile(NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE | STGM_CREATE, 0, &pMedDest->pstg) ) ) return E_OUTOFMEMORY;
            if( FAILED( pMedSrc->pstg->CopyTo(0, NULL, NULL, pMedDest->pstg) ) ) return E_FAIL;
         }
         break;
      default:
         ATLASSERT(false);
         return DV_E_TYMED;
      }
      pMedDest->tymed = pMedSrc->tymed;
      pMedDest->pUnkForRelease = pMedSrc->pUnkForRelease;
      return S_OK;
   }
};

class ATL_NO_VTABLE CSimpleDataObj : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public ISimpleDataObjImpl<CSimpleDataObj>
{
public:
   BEGIN_COM_MAP(CSimpleDataObj)
      COM_INTERFACE_ENTRY(IDataObject)
   END_COM_MAP()

   HRESULT IDataObject_GetData(FORMATETC* pformatetc, STGMEDIUM* pmedium)
   {
      ATLASSERT(pmedium);
      ATLASSERT(pformatetc);
      int iIndex = _Find(pformatetc);
      if( iIndex < 0 ) return DV_E_FORMATETC;
      if( pformatetc->lindex != -1 ) return DV_E_LINDEX;
      if( (m_aObjects[iIndex].FmtEtc.tymed & pformatetc->tymed) == 0 ) return DV_E_LINDEX;
      return _CopyStgMedium(pmedium, &m_aObjects[iIndex].StgMed, &m_aObjects[iIndex].FmtEtc);
   }

   // Operations

   HRESULT SetGlobalData(CLIPFORMAT cf, LPCVOID pData, DWORD dwSize)
   {
      FORMATETC fmtetc = { cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
      STGMEDIUM stgmed = { TYMED_HGLOBAL, { 0 }, 0 };
      stgmed.hGlobal = ::GlobalAlloc(GMEM_FIXED, (SIZE_T) dwSize);
      if( stgmed.hGlobal == NULL ) return E_OUTOFMEMORY;
      memcpy(stgmed.hGlobal, pData, (size_t) dwSize);
      return SetData(&fmtetc, &stgmed, TRUE);
   }
   HRESULT SetTextData(LPCSTR pstrData)
   {
      return SetGlobalData(CF_TEXT, pstrData, ::lstrlenA(pstrData) + 1);
   }
   HRESULT SetUnicodeTextData(LPCWSTR pstrData)
   {
      return SetGlobalData(CF_UNICODETEXT, pstrData, (::lstrlenW(pstrData) + 1) * sizeof(WCHAR));
   }
   HRESULT SetHDropData(LPCSTR pstrFilename)
   {
      FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
      STGMEDIUM stgmed = { TYMED_HGLOBAL, { 0 }, 0 };
      DWORD dwSize = sizeof(DROPFILES) + ::lstrlenA(pstrFilename) + 2;
      stgmed.hGlobal = ::GlobalAlloc(GMEM_SHARE | GHND | GMEM_ZEROINIT, dwSize);
      if( stgmed.hGlobal == NULL ) return E_OUTOFMEMORY;
      DROPFILES* pDest = (DROPFILES*) GlobalLock(stgmed.hGlobal);
      pDest->pFiles = sizeof(DROPFILES);
      pDest->fWide = FALSE;
      ::lstrcpyA( ((LPSTR) pDest) + sizeof(DROPFILES), pstrFilename );
      GlobalUnlock(stgmed.hGlobal);
      return SetData(&fmtetc, &stgmed, TRUE);
   }
};

#endif // __ATLCOM_H__


#endif // !defined(AFX_ATLDATAOBJ_H__20040706_4F49_248D_7C5E_0080AD509054__INCLUDED_)

