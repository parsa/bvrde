#if !defined(AFX_SERIALIZER_H__20030221_1B1A_011D_26AE_0080AD509054__INCLUDED_)
#define AFX_SERIALIZER_H__20030221_1B1A_011D_26AE_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <msxml2.h>


class CXmlSerializer : public ISerializable
{
public:
   typedef struct CONTEXT
   {
      CComBSTR bstrName;
      CComPtr<IXMLDOMNodeList> spList;
      int iIndex;
      int iLevel;
   };
   CString m_sFilename;
   CSimpleArray<CONTEXT> m_aContexts;
   CComPtr<IXMLDOMDocument> m_spDoc;
   CComPtr<IXMLDOMNode> m_spNode;
   CComPtr<IXMLDOMNamedNodeMap> m_spAttribs;
   int m_iLevel;

   virtual ~CXmlSerializer();

   // Operations

   BOOL Open(LPCTSTR pstrTitle, LPCTSTR pstrFilename);
   BOOL Open(IXMLDOMDocument* pDoc, LPCTSTR pstrTitle, LPCTSTR pstrFilename);
   BOOL Create(LPCTSTR pstrTitle, LPCTSTR pstrFilename);
   BOOL Save();
   void Close();
   BOOL Delete(LPCTSTR pstrName);

   // ISerialiable

   BOOL ReadGroupBegin(LPCTSTR pstrName);
   BOOL ReadGroupEnd();
   BOOL ReadItem(LPCTSTR pstrName);
   BOOL Read(LPCTSTR pstrName, LPTSTR pstrValue, UINT cchMax);
   BOOL Read(LPCTSTR pstrName, SYSTEMTIME& stValue);
   BOOL Read(LPCTSTR pstrName, CString& sValue);
   BOOL Read(LPCTSTR pstrName, long& lValue);
   BOOL Read(LPCTSTR pstrName, BOOL& bValue);

   BOOL WriteGroupBegin(LPCTSTR pstrName);
   BOOL WriteGroupEnd();
   BOOL WriteItem(LPCTSTR pstrName);
   BOOL Write(LPCTSTR pstrName, LPCTSTR pstrValue);
   BOOL Write(LPCTSTR pstrName, SYSTEMTIME stValue);
   BOOL Write(LPCTSTR pstrName, long lValue);
   BOOL Write(LPCTSTR pstrName, BOOL bValue);
   BOOL WriteExternal(LPCTSTR pstrName);
};


#endif // !defined(AFX_SERIALIZER_H__20030221_1B1A_011D_26AE_0080AD509054__INCLUDED_)
