
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
   pArc->Read(_T("searchPath"), m_sSearchPath.GetBufferSetLength(128), 128);
   m_sSearchPath.ReleaseBuffer();

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
   if( sName == _T("Path") ) return m_sPath;
   if( sName == _T("SearchPath") ) return m_sSearchPath;
   if( sName == _T("Separator") ) return _T("\\");
   if( sName == _T("Certificate") ) return _T("Windows Network Drive");
   if( sName == _T("Type") ) return _T("network");
   return "";
}

void CFileProtocol::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   if( sName == _T("Path") ) m_sPath = pstrValue;
   if( sName == _T("SearchPath") ) m_sSearchPath = pstrValue;
}

bool CFileProtocol::LoadFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE* ppOut, DWORD* pdwSize /* = NULL*/)
{
   ATLASSERT(pstrFilename);
   ATLASSERT(ppOut);

   *ppOut = NULL;
   if( pdwSize ) *pdwSize = 0;

   CString sFilename;
   sFilename.Format(_T("%s%s"), m_sPath, pstrFilename);

   CFile f;
   if( !f.Open(sFilename) ) return false;
  
   const DWORD BUFFER_SIZE = 4096;
   LPBYTE pBuffer = (LPBYTE) malloc(BUFFER_SIZE);
   DWORD dwPos = 0;
   DWORD dwSize = BUFFER_SIZE;
   BYTE bBuffer[BUFFER_SIZE];
   while( true ) {
      DWORD dwRead = 0;
      BOOL bRes = f.Read(bBuffer, BUFFER_SIZE, &dwRead);
      if( !bRes ) {
         DWORD dwErr = ::GetLastError();
         f.Close();
         ::SetLastError(dwErr);
         return false;
      }
      if( bRes && dwRead == 0 ) break;
      if( dwPos + dwRead > dwSize ) {
         pBuffer = (LPBYTE) realloc(pBuffer, dwSize + BUFFER_SIZE);
         dwSize += BUFFER_SIZE;
      }
      memcpy(pBuffer + dwPos, bBuffer, dwRead);
      dwPos += dwRead;
   }

   f.Close();

   *ppOut = pBuffer;
   if( pdwSize ) *pdwSize = dwPos;

   return true;
}

bool CFileProtocol::SaveFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE pData, DWORD dwSize)
{
   ATLASSERT(pstrFilename);
   ATLASSERT(pData);

   // Prevent save of an empty file
   if( dwSize == 0 ) {
      ::SetLastError(ERROR_EMPTY);
      return false;
   }

   CString sFilename;
   sFilename.Format(_T("%s%s"), m_sPath, pstrFilename);

   CFile f;
   if( !f.Create(sFilename) ) return false;

   DWORD dwPos = 0;
   while( dwSize > 0 ) {
      DWORD dwWritten = 0;
      BOOL bRes = f.Write(pData + dwPos, dwSize, &dwWritten);
      if( !bRes ) {
         DWORD dwErr = ::GetLastError();
         f.Close();
         ::SetLastError(dwErr);
         return false;
      }
      if( bRes && dwWritten == 0 ) break;
      dwPos += dwWritten;
      dwSize -= dwWritten;
   }
   
   f.Close();

   return true;
}

bool CFileProtocol::SetCurPath(LPCTSTR pstrPath)
{
   ATLASSERT(pstrPath);
   TCHAR szPath[MAX_PATH];
   _tcscpy(szPath, pstrPath);
   ::PathRemoveBackslash(szPath);
   if( ::PathIsRelative(szPath) ) {
      // FIX: Windows doesn't seem to return a good error explanation
      if( ::GetLastError() == 0 ) ::SetLastError(ERROR_CANT_RESOLVE_FILENAME);
      return false;
   }
   if( !::PathIsDirectory(szPath) ) return false;
   ::PathAddBackslash(szPath);
   m_sPath = szPath;
   return true;
}

CString CFileProtocol::GetCurPath()
{
   return m_sPath;
}

bool CFileProtocol::EnumFiles(CSimpleArray<WIN32_FIND_DATA>& aFiles)
{
   WIN32_FIND_DATA fd = { 0 };
   CString sPattern;
   sPattern.Format(_T("%s*.*"), m_sPath);
   HINTERNET hFind = ::FindFirstFile(sPattern, &fd);
   if( hFind == NULL && ::GetLastError() == ERROR_NO_MORE_FILES ) return true;
   if( hFind == NULL ) return false;
   while( true ) {
      aFiles.Add(fd);
      BOOL bRes = ::FindNextFile(hFind, &fd);
      if( !bRes ) break;
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