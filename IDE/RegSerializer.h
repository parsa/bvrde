#if !defined(AFX_REGSERIALIZER_H__20030506_9F7F_8BEB_3E44_0080AD509054__INCLUDED_)
#define AFX_REGSERIALIZER_H__20030506_9F7F_8BEB_3E44_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CRegSerializer : public ISerializable
{
public:
   CRegKey m_reg;
   CString m_sPath;

   BOOL Create(LPCTSTR pstrTitle);
   BOOL Open(LPCTSTR pstrTitle);
   void Close();

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

   BOOL Delete(LPCTSTR pstrName);
};


#endif // !defined(AFX_REGSERIALIZER_H__20030506_9F7F_8BEB_3E44_0080AD509054__INCLUDED_)
