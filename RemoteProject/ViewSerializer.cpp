
#include "StdAfx.h"
#include "resource.h"

#include "ViewSerializer.h"

#pragma code_seg( "MISC" )


void CViewSerializer::Add(LPCTSTR pstrKey, LPCTSTR pstrValue)
{
   CString sKey = pstrKey;
   CString sValue = pstrValue;
   m_aItems.Add(sKey, sValue);
}

BOOL CViewSerializer::ReadGroupBegin(LPCTSTR pstrName)
{
   return FALSE;
}

BOOL CViewSerializer::ReadGroupEnd()
{
   return FALSE;
}

BOOL CViewSerializer::ReadItem(LPCTSTR pstrName)
{
   return FALSE;
}

BOOL CViewSerializer::Read(LPCTSTR pstrName, LPTSTR szValue, UINT cchMax)
{
   szValue[0] = '\0';
   CString sKey = pstrName;
   int iIndex = m_aItems.FindKey(sKey);
   if( iIndex < 0 ) return FALSE;
   _tcsncpy(szValue, m_aItems.GetValueAt(iIndex), cchMax);
   return TRUE;
}

BOOL CViewSerializer::Read(LPCTSTR pstrName, SYSTEMTIME& stValue)
{
   return FALSE;
}

BOOL CViewSerializer::Read(LPCTSTR pstrName, long& lValue)
{
   return FALSE;
}

BOOL CViewSerializer::Read(LPCTSTR pstrName, BOOL& bValue)
{
   return FALSE;
}

