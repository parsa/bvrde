#if !defined(AFX_DUMMYELEMENT_H__20040501_62D9_29DD_AC6E_0080AD509054__INCLUDED_)
#define AFX_DUMMYELEMENT_H__20040501_62D9_29DD_AC6E_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CDummyElement : public IElement
{
public:
   CString m_sType;
   CString m_sName;

   CDummyElement(LPCTSTR pstrType, LPCTSTR pstrName) : 
      m_sType(pstrType), m_sName(pstrName)
   {
   }
   BOOL Load(ISerializable* pArchive)
   {
      return FALSE;
   }  
   BOOL Save(ISerializable* pArchive)
   {
      return FALSE;
   }  
   BOOL GetName(LPTSTR pstrName, UINT cchMax) const
   {
      return _tcsncpy(pstrName, m_sName, cchMax) > 0;
   }  
   BOOL GetType(LPTSTR pstrType, UINT cchMax) const
   {
      return _tcsncpy(pstrType, m_sType, cchMax) > 0;
   }  
   IDispatch* GetDispatch()
   {
      return NULL;
   }
};


#endif // !defined(AFX_DUMMYELEMENT_H__20040501_62D9_29DD_AC6E_0080AD509054__INCLUDED_)
