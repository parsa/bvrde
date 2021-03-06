
#include "StdAfx.h"
#include "resource.h"

#include "ShellProxy.h"

#include "Authen.h"

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
   if( m_pProtocol != NULL ) delete m_pProtocol;
}

void CShellManager::Init(CRemoteProject* pProject, IShellCallback* pCallback /*= NULL*/)
{
   ATLASSERT(pProject);
   m_pProject = pProject;
   if( m_pProtocol != NULL ) m_pProtocol->Init(pProject, this);
}

void CShellManager::Clear()
{
   if( m_pProtocol == NULL ) return;
   m_pProtocol->Clear();
}

bool CShellManager::Load(ISerializable* pArc)
{
   Clear();

   // Book-keep the connection type
   m_sType = _T("telnet");
   if( pArc->ReadItem(_T("Type")) ) {
      TCHAR szType[64] = { 0 };
      TCHAR szServerType[64] = { 0 };
      pArc->Read(_T("method"), szType, 63);
      pArc->Read(_T("serverType"), szServerType, 63);
      m_sType = szType;
      m_sServerType = szServerType;
   }
   // Create the new connection based on the type
   if( m_pProtocol != NULL ) delete m_pProtocol;
   m_pProtocol = NULL;
   if( m_sType == _T("telnet") )   m_pProtocol = new CTelnetProtocol();
   if( m_sType == _T("rlogin") )   m_pProtocol = new CRloginProtocol();
   if( m_sType == _T("ssh") )      m_pProtocol = new CSshProtocol();
   if( m_sType == _T("comspec") )  m_pProtocol = new CComSpecProtocol();
   if( m_pProtocol == NULL ) return false;

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
      if( m_pProtocol != NULL ) delete m_pProtocol;
      m_pProtocol = NULL;
      m_sType = pstrValue;
      if( m_sType == _T("telnet") )   m_pProtocol = new CTelnetProtocol();
      if( m_sType == _T("rlogin") )   m_pProtocol = new CRloginProtocol();
      if( m_sType == _T("ssh") )      m_pProtocol = new CSshProtocol();
      if( m_sType == _T("comspec") )  m_pProtocol = new CComSpecProtocol();
      if( m_pProtocol == NULL ) return;
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

/**
 * Server Detection callback.
 * This function receives shell lines before the username/password
 * has been sent, and its mission is to detect the OS on the remote
 * machine. Matching the OS will allow us to preselect the optimal
 * settings for compiling/debugging during Wizard Project creation.
 */
void CShellManager::PreAuthenticatedLine(LPCTSTR pstrText)
{
   // Try to detect server type...
   if( m_sServerType.IsEmpty() ) 
   {
      static struct {
         LPCTSTR pstrToken; LPCTSTR pstrServerType;
      } aTypes[] = {
         { _T("Windows"),  _T("Windows") },
         { _T("Solaris"),  _T("Solaris") },
         { _T("SunOS"),    _T("Solaris") },
         { _T("Linux"),    _T("LINUX") },
         { _T("LINUX"),    _T("LINUX") },
         { _T("Debian"),   _T("LINUX (Debian)") },
         { _T("Gentoo"),   _T("LINUX (Gentoo)") },
         { _T("Red Hat"),  _T("LINUX (Red Hat)") },
         { _T("Ubuntu"),   _T("LINUX (Ubuntu)") },
         { _T("CentOS"),   _T("LINUX (CentOS)") },
         { _T("UNIX"),     _T("UNIX") },
         { _T("HP-UX"),    _T("UNIX (HP-UX)") },
         { _T("FreeBSD"),  _T("FreeBSD") },
         { _T("OpenBSD"),  _T("OpenBSD") },
      };
      for( int i = 0; i < sizeof(aTypes) / sizeof(aTypes[0]); i++ ) {
         if( _tcsstr(pstrText, aTypes[i].pstrToken) != NULL ) m_sServerType = aTypes[i].pstrServerType;
      }
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

