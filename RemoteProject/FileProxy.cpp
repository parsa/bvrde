
#include "StdAfx.h"
#include "resource.h"

#include "FileProxy.h"

#include "Authen.h"

#include "FtpProtocol.h"
#include "SftpProtocol.h"
#include "FileProtocol.h"

#pragma code_seg( "MISC" )


////////////////////////////////////////////////////////
//

CFileManager::CFileManager() :
   m_pProtocol(NULL)
{   
}

CFileManager::~CFileManager()
{
   if( m_pProtocol != NULL ) delete m_pProtocol;
}

void CFileManager::Clear()
{
   if( m_pProtocol == NULL ) return;
   m_pProtocol->Clear();
}

bool CFileManager::Load(ISerializable* pArc)
{
   Clear();
   if( !pArc->ReadGroupBegin(_T("FileTransfer")) ) return true;

   // Book-keep the connection type
   m_sType = _T("ftp");
   if( pArc->ReadItem(_T("Type")) ) {
      TCHAR szType[64] = { 0 };
      pArc->Read(_T("method"), szType, 63);
      m_sType = szType;
   }
   // Create the new connection based on the type
   if( m_pProtocol != NULL ) delete m_pProtocol;
   m_pProtocol = NULL;
   if( m_sType == _T("ftp") )      m_pProtocol = new CFtpProtocol();
   if( m_sType == _T("sftp") )     m_pProtocol = new CSftpProtocol();
   if( m_sType == _T("network") )  m_pProtocol = new CFileProtocol();
   if( m_pProtocol == NULL ) return false;

   if( !m_pProtocol->Load(pArc) ) return false;

   if( !pArc->ReadGroupEnd() ) return false;
   return true;
}

bool CFileManager::Save(ISerializable* pArc)
{
   ATLASSERT(m_pProtocol);
   if( !pArc->WriteGroupBegin(_T("FileTransfer")) ) return true;

   if( !pArc->WriteItem(_T("Type")) ) return false;
   pArc->Write(_T("method"), m_sType);

   if( !m_pProtocol->Save(pArc) ) return false;

   if( !pArc->WriteGroupEnd() ) return false;
   return true;
}

bool CFileManager::Start()
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return false;
   // Make sure there is a password available
   CString sPassword = GetParam(_T("Password"));
   CString sCertificate = GetParam(_T("Certificate"));
   if( sPassword.IsEmpty() && sCertificate.IsEmpty() ) SecGetPassword();
   // Fire up connection
   return m_pProtocol->Start();
}

void CFileManager::SignalStop()
{
   if( m_pProtocol == NULL ) return;
   m_pProtocol->SignalStop();
}

bool CFileManager::Stop()
{
   if( m_pProtocol == NULL ) return true;
   return m_pProtocol->Stop();
}

bool CFileManager::IsConnected() const
{
   if( m_pProtocol == NULL ) return false;
   return m_pProtocol->IsConnected();
}

CString CFileManager::GetParam(LPCTSTR pstrName) const
{
   CString sName = pstrName;
   if( sName == _T("Type") ) return m_sType;
   if( m_pProtocol == NULL ) return _T("");
   return m_pProtocol->GetParam(pstrName);
}

void CFileManager::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   // Changing connection type?
   if( sName == _T("Type") ) {
      if( m_pProtocol != NULL ) delete m_pProtocol;
      m_pProtocol = NULL;
      m_sType = pstrValue;
      if( m_sType == _T("ftp") )          m_pProtocol = new CFtpProtocol();
      else if( m_sType == _T("sftp") )    m_pProtocol = new CSftpProtocol();
      else if( m_sType == _T("network") ) m_pProtocol = new CFileProtocol();
   }
   // Just pass the parameter on to the actual protocol
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return;
   m_pProtocol->SetParam(pstrName, pstrValue);
}

bool CFileManager::LoadFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE* ppOut, DWORD* pdwSize /* = NULL*/)
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return false;
   return m_pProtocol->LoadFile(pstrFilename, bBinary, ppOut, pdwSize);
}

bool CFileManager::SaveFile(LPCTSTR pstrFilename, bool bBinary, LPBYTE pData, DWORD dwSize)
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return false;
   // Should we attempt to delete the file first (overwrite read-only file)?
   bool bDeleteFirst = false;
   TCHAR szBuffer[20] = { 0 };
   _pDevEnv->GetProperty(_T("gui.document.protectReadOnly"), szBuffer, 19);
   if( _tcscmp(szBuffer, _T("false")) == 0 ) bDeleteFirst = true;
   if( GetParam(_T("CompatibilityMode")) == _T("true") ) bDeleteFirst = true;
   if( bDeleteFirst ) DeleteFile(pstrFilename);
   // Finally write the file
   return m_pProtocol->SaveFile(pstrFilename, bBinary, pData, dwSize);
}

bool CFileManager::DeleteFile(LPCTSTR pstrFilename)
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return false;
   return m_pProtocol->DeleteFile(pstrFilename);
}

bool CFileManager::SetCurPath(LPCTSTR pstrPath)
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return false;
   return m_pProtocol->SetCurPath(pstrPath);
}

CString CFileManager::GetCurPath()
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return _T("");
   return m_pProtocol->GetCurPath();
}

bool CFileManager::EnumFiles(CSimpleArray<WIN32_FIND_DATA>& aFiles, bool bUseCache)
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return false;
   return m_pProtocol->EnumFiles(aFiles, bUseCache);
}

CString CFileManager::FindFile(LPCTSTR pstrFilename)
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return _T("");
   return m_pProtocol->FindFile(pstrFilename);
}

bool CFileManager::WaitForConnection()
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return FALSE;
   return m_pProtocol->WaitForConnection();
}

