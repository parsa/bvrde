
#include "StdAfx.h"
#include "resource.h"

#include "FileProxy.h"

#include "FtpProtocol.h"
#include "SftpProtocol.h"

#pragma code_seg( "MISC" )


////////////////////////////////////////////////////////
//

CFileManager::CFileManager() :
   m_pProtocol(NULL)
{   
}

CFileManager::~CFileManager()
{
   if( m_pProtocol ) delete m_pProtocol;
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

   m_sType = _T("ftp");
   if( pArc->ReadItem(_T("Type")) ) {
      TCHAR szType[64] = { 0 };
      pArc->Read(_T("method"), szType, 63);
      m_sType = szType;
   }
   if( m_pProtocol ) delete m_pProtocol;
   m_pProtocol = NULL;
   if( m_sType == _T("ftp") ) m_pProtocol = new CFtpProtocol();
   else if( m_sType == _T("sftp") ) m_pProtocol = new CSftpProtocol();
   else return false;

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
   if( sName == "Type" ) return m_sType;
   if( m_pProtocol == NULL ) return _T("");
   return m_pProtocol->GetParam(pstrName);
}

void CFileManager::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   // Changing connection type?
   if( sName == "Type" ) {
      if( m_pProtocol ) delete m_pProtocol;
      m_pProtocol = NULL;
      m_sType = pstrValue;
      if( m_sType == _T("ftp") ) m_pProtocol = new CFtpProtocol();
      else if( m_sType == _T("sftp") ) m_pProtocol = new CSftpProtocol();
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
   return m_pProtocol->SaveFile(pstrFilename, bBinary, pData, dwSize);
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

bool CFileManager::EnumFiles(CSimpleArray<WIN32_FIND_DATA>& aFiles)
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return false;
   return m_pProtocol->EnumFiles(aFiles);
}

CString CFileManager::FindFile(LPCTSTR pstrFilename)
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return _T("");
   return m_pProtocol->FindFile(pstrFilename);
}
