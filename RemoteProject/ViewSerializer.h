#if !defined(AFX_VIEWSERIALIZER_H__20030605_A520_3F08_D8EC_0080AD509054__INCLUDED_)
#define AFX_VIEWSERIALIZER_H__20030605_A520_3F08_D8EC_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CViewSerializer : public ISerializable
{
public:
   CSimpleMap<CString, CString> m_aItems;

   void Add(LPCTSTR pstrKey, LPCTSTR pstrValue);

   BOOL ReadGroupBegin(LPCTSTR pstrName);
   BOOL ReadGroupEnd();
   BOOL ReadItem(LPCTSTR pstrName);
   BOOL Read(LPCTSTR pstrName, LPTSTR szValue, UINT cchMax);
   BOOL Read(LPCTSTR pstrName, SYSTEMTIME& stValue);
   BOOL Read(LPCTSTR pstrName, long& lValue);
   BOOL Read(LPCTSTR pstrName, BOOL& bValue);

   BOOL WriteGroupBegin(LPCTSTR pstrName) { return FALSE; };
   BOOL WriteGroupEnd() { return FALSE; };
   BOOL WriteItem(LPCTSTR pstrName) { return FALSE; };
   BOOL Write(LPCTSTR pstrName, LPCTSTR pstrValue) { return FALSE; };
   BOOL Write(LPCTSTR pstrName, SYSTEMTIME stValue) { return FALSE; };
   BOOL Write(LPCTSTR pstrName, long lValue) { return FALSE; };
   BOOL Write(LPCTSTR pstrName, BOOL bValue) { return FALSE; };
   BOOL WriteExternal(LPCTSTR pstrName) { return FALSE; };

   BOOL Delete(LPCTSTR pstrName) { return FALSE; };
};


#endif // !defined(AFX_VIEWSERIALIZER_H__20030605_A520_3F08_D8EC_0080AD509054__INCLUDED_)
