
#include "StdAfx.h"
#include "resource.h"

#include "Project.h"

#include "FileProtocol.h"

#pragma code_seg( "PROTOCOLS" )


////////////////////////////////////////////////////////
//

CFileProtocol::CFileProtocol() :
   m_bCancel(false),
   m_bConnected(false)
{
   Clear();
}

CFileProtocol::~CFileProtocol()
{
   Stop();
}

void CFileProtocol::Clear()
{
   m_sPath.Empty();
   m_sSearchPath.Empty();
}

bool CFileProtocol::Load(ISerializable* pArc)
{
   Clear();

   if( !pArc->ReadItem(_T("MappedDrive")) ) return false;
   pArc->Read(_T("path"), m_sPath.GetBufferSetLength(MAX_PATH), MAX_PATH);
   m_sPath.ReleaseBuffer();
   pArc->Read(_T("searchPath"), m_sSearchPath.GetBufferSetLength(200), 200);
   m_sSearchPath.ReleaseBuffer();

   m_sPath.TrimRight(_T("\\"));

   return true;
}

bool CFileProtocol::Save(ISerializable* pArc)
{
   if( !pArc->WriteItem(_T("MappedDrive")) ) return false;
   pArc->Write(_T("path"), m_sPath);
   pArc->Write(_T("searchPath"), m_sSearchPath);
   return true;
}

bool CFileProtocol::Start()
{
   Stop();
   m_sCurDir = m_sPath;
   m_bConnected = true;
   return true;
}

void CFileProtocol::SignalStop()
{
   m_bCancel = true;
}

bool CFileProtocol::Stop()
{
   m_bCancel = false;
   m_bConnected = false;
   return true;
}

bool CFileProtocol::IsConnected() const
{
   return m_bConnected;
}

CString CFileProtocol::GetParam(LPCTSTR pstrName) const
{
   CString sName = pstrName;
   if( sName == _T("Type") ) return _T("network");
   if( sName == _T("Path") ) return m_sPath;
   if( sName == _T("SearchPath") ) return m_sSearchPath;
   if( sName == _T("Separator") ) return _T("\\");
   if( sName == _T("Certificate") ) return _T("Windows Network Drive");
   return _T("");
}

void CFileProtocol::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   if( sName == _T("Path") ) m_sPath = pstrValue;
   if( sName == _T("SearchPath") ) m_sSearchPath = pstrValue;
   m_sPath.TrimRight(_T("\\"));
}

bool CFileProtocol::LoadFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE* ppOut, DWORD* pdwSize /* = NULL*/)
{
   ATLASSERT(pstrFilename);
   ATLASSERT(ppOut);

   *ppOut = NULL;
   if( pdwSize != NULL ) *pdwSize = 0;

   CString sFilename = pstrFilename;
   if( ::PathIsRelative(pstrFilename) && m_sCurDir.GetLength() > 1 ) sFilename.Format(_T("%s\\%s"), m_sCurDir, pstrFilename);

   CFile f;
   if( !f.Open(sFilename) ) return false; 
   DWORD dwSize = f.GetSize();
   *ppOut = (LPBYTE) malloc(dwSize);
   if( *ppOut == NULL ) {
      f.Close();
      ::SetLastError(ERROR_OUTOFMEMORY);
      return false;
   }
   DWORD dwBytesRead = 0;
   BOOL bRes = f.Read(*ppOut, dwSize, &dwBytesRead);
   if( !bRes || dwBytesRead != dwSize ) {
      DWORD dwErr = ::GetLastError();
      free(*ppOut); *ppOut = NULL;
      f.Close();
      ::SetLastError(dwErr);
      return false;
   }
   f.Close();

   if( pdwSize != NULL ) *pdwSize = dwSize;

   return true;
}

bool CFileProtocol::SaveFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE pData, DWORD dwSize)
{
   ATLASSERT(pstrFilename);
   ATLASSERT(pData);

   CString sFilename = pstrFilename;
   if( ::PathIsRelative(pstrFilename) && m_sCurDir.GetLength() > 1 ) sFilename.Format(_T("%s\\%s"), m_sCurDir, pstrFilename);

   CFile f;
   if( !f.Create(sFilename) ) return false;
   DWORD dwBytesWritten = 0;
   if( !f.Write(pData, dwSize, &dwBytesWritten) ) return false;
   f.Close();

   return dwBytesWritten == dwSize;
}

bool CFileProtocol::DeleteFile(LPCTSTR pstrFilename)
{
   ATLASSERT(pstrFilename);
   CString sFilename = pstrFilename;
   if( ::PathIsRelative(pstrFilename) && m_sCurDir.GetLength() > 1 ) sFilename.Format(_T("%s\\%s"), m_sCurDir, pstrFilename);
   return CFile::Delete(sFilename) == TRUE;
}

bool CFileProtocol::SetCurPath(LPCTSTR pstrPath)
{
   ATLASSERT(pstrPath);
   TCHAR szPath[MAX_PATH];
   _tcscpy(szPath, pstrPath);
   ::PathRemoveBackslash(szPath);
   if( ::PathIsRelative(szPath) ) {
      if( ::GetLastError() == 0 ) ::SetLastError(ERROR_CANT_RESOLVE_FILENAME);
      return false;
   }
   if( !::PathIsDirectory(szPath) ) {
      if( ::GetLastError() == 0 ) ::SetLastError(ERROR_PATH_NOT_FOUND);
      return false;
   }
   m_sCurDir = szPath;
   return true;
}

CString CFileProtocol::GetCurPath()
{
   return m_sCurDir;
}

bool CFileProtocol::EnumFiles(CSimpleArray<WIN32_FIND_DATA>& aFiles, bool /*bUseCache*/)
{
   WIN32_FIND_DATA fd = { 0 };
   CString sPattern;
   sPattern.Format(_T("%s\\*.*"), m_sCurDir);
   HINTERNET hFind = ::FindFirstFile(sPattern, &fd);
   if( hFind == NULL && ::GetLastError() == ERROR_NO_MORE_FILES ) return true;
   if( hFind == NULL ) return false;
   while( true ) {
      if( _tcscmp(fd.cFileName, _T(".")) != 0 
          && _tcscmp(fd.cFileName, _T("..")) != 0 )
      {
         aFiles.Add(fd);
      }
      if( !::FindNextFile(hFind, &fd) ) break;
   }   
   ::FindClose(hFind);
   return true;
}

CString CFileProtocol::FindFile(LPCTSTR pstrFilename)
{
   // Get file information
   WIN32_FIND_DATA fd = { 0 };
   if( !::PathIsRelative(pstrFilename) ) {
      if( ::PathFileExists(pstrFilename) ) return pstrFilename;
   }
   else {
      CString sPath = m_sSearchPath;
      while( !sPath.IsEmpty() ) {
         CString sSubPath = sPath.SpanExcluding(_T(";"));
         CString sFilename = sSubPath;
         if( sFilename.Right(1) != _T("\\") ) sFilename += _T("\\");
         sFilename += pstrFilename;
         if( ::PathFileExists(sFilename) ) return sFilename;
         sPath = sPath.Mid(sSubPath.GetLength() + 1);
      }
   }
   return _T("");
}

bool CFileProtocol::WaitForConnection()
{
   return true;
}

