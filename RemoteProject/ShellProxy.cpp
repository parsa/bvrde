
#include "StdAfx.h"
#include "resource.h"

#include "ShellProxy.h"

#include "TelnetProtocol.h"
#include "RloginProtocol.h"
#include "SshProtocol.h"
#include "ComSpecProtocol.h"

#pragma code_seg( "MISC" )


////////////////////////////////////////////////////////
//

static CComAutoCriticalSection g_csLineData;

struct CLockLineDataInit
{
   CLockLineDataInit() { g_csLineData.Lock(); };
   ~CLockLineDataInit() { g_csLineData.Unlock(); };
};


////////////////////////////////////////////////////////
//

CShellManager::CShellManager() :
   m_pProject(NULL),
   m_pProtocol(NULL)
{
}

CShellManager::~CShellManager()
{
   if( m_pProtocol ) delete m_pProtocol;
}

void CShellManager::Init(CRemoteProject* pProject, IShellCallback* pCallback /*= NULL*/)
{
   ATLASSERT(pProject);
   m_pProject = pProject;
   if( m_pProtocol ) m_pProtocol->Init(pProject, this);
}

void CShellManager::Clear()
{
   if( m_pProtocol == NULL ) return;
   m_pProtocol->Clear();
}

bool CShellManager::Load(ISerializable* pArc)
{
   Clear();

   m_sType = _T("telnet");
   if( pArc->ReadItem(_T("Type")) ) {
      TCHAR szType[64] = { 0 };
      TCHAR szServerType[64] = { 0 };
      pArc->Read(_T("method"), szType, 63);
      pArc->Read(_T("serverType"), szServerType, 63);
      m_sType = szType;
      m_sServerType = szServerType;
   }
   if( m_pProtocol ) delete m_pProtocol;
   m_pProtocol = NULL;
   if( m_sType == _T("telnet") )       m_pProtocol = new CTelnetProtocol();
   else if( m_sType == _T("rlogin") )  m_pProtocol = new CRloginProtocol();
   else if( m_sType == _T("ssh") )     m_pProtocol = new CSshProtocol();
   else if( m_sType == _T("comspec") ) m_pProtocol = new CComSpecProtocol();
   else {
      ATLASSERT(false);
      return false;
   }

   m_pProtocol->Init(m_pProject, this);
   if( !m_pProtocol->Load(pArc) ) return false;

   return true;
}

bool CShellManager::Save(ISerializable* pArc)
{
   ATLASSERT(m_pProtocol);

   if( !pArc->WriteItem(_T("Type")) ) return false;
   pArc->Write(_T("method"), m_sType);
   pArc->Write(_T("serverType"), m_sServerType);

   if( !m_pProtocol->Save(pArc) ) return false;

   return true;
}

bool CShellManager::Start()
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return false;
   // Make sure there is a password available
   CString sPassword = GetParam(_T("Password"));
   CString sCertificate = GetParam(_T("Certificate"));
   if( sPassword.IsEmpty() && sCertificate.IsEmpty() ) SecGetPassword();
   // Fire up the connection
   return m_pProtocol->Start();
}

bool CShellManager::Stop()
{
   if( m_pProtocol == NULL ) return true;
   return m_pProtocol->Stop();
}

void CShellManager::SignalStop()
{
   if( m_pProtocol == NULL ) return;
   m_pProtocol->SignalStop();
}

bool CShellManager::IsConnected() const
{
   if( m_pProtocol == NULL ) return false;
   return m_pProtocol->IsConnected();
}

bool CShellManager::IsBusy() const
{
   if( m_pProtocol == NULL ) return false;
   return m_pProtocol->IsBusy();
}

CString CShellManager::GetParam(LPCTSTR pstrName) const
{
   CString sName = pstrName;
   if( sName == _T("Type") ) return m_sType;
   if( sName == _T("ServerType") ) return m_sServerType;
   if( m_pProtocol == NULL ) return _T("");
   return m_pProtocol->GetParam(pstrName);
}

void CShellManager::SetParam(LPCTSTR pstrName, LPCTSTR pstrValue)
{
   CString sName = pstrName;
   // Changing connection type?
   if( sName == _T("Type") ) {
      if( m_pProtocol ) delete m_pProtocol;
      m_pProtocol = NULL;
      m_sType = pstrValue;
      if( m_sType == _T("telnet") )       m_pProtocol = new CTelnetProtocol();
      else if( m_sType == _T("rlogin") )  m_pProtocol = new CRloginProtocol();
      else if( m_sType == _T("ssh") )     m_pProtocol = new CSshProtocol();
      else if( m_sType == _T("comspec") ) m_pProtocol = new CComSpecProtocol();
      else {
         ATLASSERT(false);
         return;
      }
      m_pProtocol->Init(m_pProject, this);
   }
   if( sName == _T("ServerType") ) m_sServerType = pstrValue;
   // Just pass the parameter on to the actual protocol
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return;
   m_pProtocol->SetParam(pstrName, pstrValue);
}

bool CShellManager::AddLineListener(IOutputLineListener* pListener)
{
   ATLASSERT(pListener);
   CLockLineDataInit lock;
   BOOL bRes = m_LineListeners.Find(pListener) < 0;
   if( bRes ) bRes = m_LineListeners.Add(pListener);
   return bRes == TRUE;
}

bool CShellManager::RemoveLineListener(IOutputLineListener* pListener)
{
   ATLASSERT(pListener);
   CLockLineDataInit lock;
   return m_LineListeners.Remove(pListener) == TRUE;
}

void CShellManager::BroadcastLine(VT100COLOR Color, LPCTSTR pstrText)
{
   ATLASSERT(!::IsBadStringPtr(pstrText,-1));

   // NOTE: To avoid too many dead-locks we make
   //       a copy of the listeners instead of locking the data array
   //       for a considerable time.
   CSimpleValArray<IOutputLineListener*> LineListeners;
   // DATA LOCK
   {
      CLockLineDataInit lock;
      for( int i = 0; i < m_LineListeners.GetSize(); i++ ) LineListeners.Add( m_LineListeners[i] );
   }
   for( int i = LineListeners.GetSize() - 1; i >= 0 ; --i ) {
      LineListeners[i]->OnIncomingLine(Color, pstrText);
      if( m_LineListeners.GetSize() != LineListeners.GetSize() ) break;
   }
}

void CShellManager::PreAuthenticatedLine(LPCTSTR pstrText)
{
   // Try to detect server type...
   if( m_sServerType.IsEmpty() ) {
      CString s = pstrText;
      if( s.Find(_T("Linux")) >= 0 )   m_sServerType = _T("LINUX (Generic)");
      if( s.Find(_T("LINUX")) >= 0 )   m_sServerType = _T("LINUX (Generic)");
      if( s.Find(_T("Windows")) >= 0 ) m_sServerType = _T("Windows");
      if( s.Find(_T("UNIX")) >= 0 )    m_sServerType = _T("UNIX (Generic)");
      if( s.Find(_T("Debian")) >= 0 )  m_sServerType = _T("LINUX (Debian)");
      if( s.Find(_T("Red Hat")) >= 0 ) m_sServerType = _T("LINUX (Red Hat)");
      if( s.Find(_T("Ubuntu")) >= 0 )  m_sServerType = _T("LINUX (Ubuntu)");
      if( s.Find(_T("HP-UX")) >= 0 )   m_sServerType = _T("UNIX (HP-UX)");
   }
}

bool CShellManager::WriteData(LPCTSTR pstrData)
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return false;
   return m_pProtocol->WriteData(pstrData);
}

bool CShellManager::WriteSignal(BYTE bCmd)
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return false;
   return m_pProtocol->WriteSignal(bCmd);
}

bool CShellManager::WriteScreenSize(int w, int h)
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return false;
   return m_pProtocol->WriteScreenSize(w, h);
}

bool CShellManager::WaitForConnection()
{
   ATLASSERT(m_pProtocol);
   if( m_pProtocol == NULL ) return false;
   return m_pProtocol->WaitForConnection();
}

