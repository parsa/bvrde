
#include "StdAfx.h"
#include "resource.h"

#include "RegSerializer.h"


/////////////////////////////////////////////////////////////////
// CRegSerializer

BOOL CRegSerializer::Create(LPCTSTR pstrTitle)
{
   Close();
   if( m_reg.Create(HKEY_CURRENT_USER, pstrTitle) != ERROR_SUCCESS ) return FALSE;
   m_sPath = pstrTitle;
   return TRUE;
}

BOOL CRegSerializer::Open(LPCTSTR pstrTitle)
{
   Close();
   if( m_reg.Open(HKEY_CURRENT_USER, pstrTitle, KEY_READ) != ERROR_SUCCESS ) return FALSE;
   m_sPath = pstrTitle;
   return TRUE;
}

void CRegSerializer::Close()
{
   m_reg.Close();
}

// ISerialiable

BOOL CRegSerializer::ReadGroupBegin(LPCTSTR pstrName)
{
   TCHAR szPath[MAX_PATH];
   ::wsprintf(szPath, _T("%s\\%s"), m_sPath, pstrName);
   return Open(szPath);
}

BOOL CRegSerializer::ReadGroupEnd()
{
   TCHAR szPath[MAX_PATH];
   ::lstrcpy(szPath, m_sPath);
   ::PathRemoveFileSpec(szPath);
   return Open(szPath);
}

BOOL CRegSerializer::ReadItem(LPCTSTR /*pstrName*/)
{
   return FALSE;
}

BOOL CRegSerializer::Read(LPCTSTR pstrName, LPTSTR pstrValue, UINT cchMax)
{
   ATLASSERT(m_reg.m_hKey!=NULL);
   DWORD dwCount = cchMax;
   return m_reg.QueryValue(pstrValue, pstrName, &dwCount) == ERROR_SUCCESS;
}

BOOL CRegSerializer::Read(LPCTSTR /*pstrName*/, SYSTEMTIME& /*stValue*/)
{
   return FALSE;
}

BOOL CRegSerializer::Read(LPCTSTR pstrName, CString& sValue)
{
   TCHAR szBuffer[256] = { 0 };
   if( !Read(pstrName, szBuffer, (sizeof(szBuffer)/sizeof(TCHAR))-1) ) return FALSE;
   sValue = szBuffer;
   return TRUE;
}

BOOL CRegSerializer::Read(LPCTSTR pstrName, long& lValue)
{
   TCHAR szBuffer[256] = { 0 };
   if( !Read(pstrName, szBuffer, (sizeof(szBuffer)/sizeof(TCHAR))-1) ) return FALSE;
   lValue = _ttol(szBuffer);
   return TRUE;
}

BOOL CRegSerializer::Read(LPCTSTR pstrName, BOOL& bValue)
{
   TCHAR szBuffer[256] = { 0 };
   if( !Read(pstrName, szBuffer, (sizeof(szBuffer)/sizeof(TCHAR))-1) ) return FALSE;
   bValue = _ttol(szBuffer) != 0;
   return TRUE;
}

BOOL CRegSerializer::WriteGroupBegin(LPCTSTR pstrName)
{
   TCHAR szPath[MAX_PATH];
   ::wsprintf(szPath, _T("%s\\%s"), m_sPath, pstrName);
   return Create(szPath);
}

BOOL CRegSerializer::WriteGroupEnd()
{
   TCHAR szPath[MAX_PATH];
   ::lstrcpy(szPath, m_sPath);
   ::PathRemoveFileSpec(szPath);
   return Create(szPath);
}

BOOL CRegSerializer::WriteItem(LPCTSTR /*pstrName*/)
{
   return FALSE;
}

BOOL CRegSerializer::Write(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   ATLASSERT(m_reg.m_hKey!=NULL);
   return m_reg.SetValue(pstrValue, pstrName) == ERROR_SUCCESS;
}

BOOL CRegSerializer::Write(LPCTSTR /*pstrName*/, SYSTEMTIME /*stValue*/)
{
   return FALSE;
}

BOOL CRegSerializer::Write(LPCTSTR pstrName, long lValue)
{
   TCHAR szBuffer[64];
   ::wsprintf(szBuffer, _T("%ld"), lValue);
   return Write(pstrName, szBuffer);
}

BOOL CRegSerializer::Write(LPCTSTR pstrName, BOOL bValue)
{
   return Write(pstrName, bValue ? _T("1") : _T("0"));
}

BOOL CRegSerializer::WriteExternal(LPCTSTR /*pstrName*/)
{
   return FALSE;
}

BOOL CRegSerializer::Delete(LPCTSTR pstrName)
{
   return m_reg.DeleteSubKey(pstrName) == ERROR_SUCCESS;
}
