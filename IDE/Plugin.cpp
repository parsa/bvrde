
#include "StdAfx.h"
#include "resource.h"
 

void CPlugin::Init(LPCTSTR pstrFilename, BOOL bActive)
{
   ATLASSERT(!::IsBadStringPtr(pstrFilename,-1));
   m_sFilename = pstrFilename;
   m_lType = 0;
   m_bActive = bActive;
   m_bLoaded = FALSE;
   m_pSetMenu = NULL;
   m_pSetPopupMenu = NULL;
}

BOOL CPlugin::LoadPackage(IDevEnv* pDevEnv)
{
   // Load the plugin DLL
   m_Lib.Load(m_sFilename);
   if( !m_Lib.IsLoaded() ) return FALSE;

   // There's a number of callbacks that a plugin must support.
   // So bind the function-members now.
   typedef VOID (CALLBACK* LPFNGETNAME)(LPTSTR,UINT);
   typedef LONG (CALLBACK* LPFNGETTYPE)();
   typedef BOOL (CALLBACK* LPFNINITIALIZE)(IDevEnv*);
   LPFNGETNAME pGetName = (LPFNGETNAME) m_Lib.GetProcAddress("Plugin_GetName");
   LPFNGETTYPE pGetType = (LPFNGETTYPE) m_Lib.GetProcAddress("Plugin_GetType");
   LPFNINITIALIZE pInitialize = (LPFNINITIALIZE) m_Lib.GetProcAddress("Plugin_Initialize");
   ATLASSERT(pGetName);
   ATLASSERT(pGetType);
   ATLASSERT(pInitialize);
   if( pGetName == NULL ) return FALSE;
   if( pGetType == NULL ) return FALSE;
   if( pInitialize == NULL ) return FALSE;

   // Bind some more callbacks members
   m_pSetMenu = (LPFNSETMENU) m_Lib.GetProcAddress("Plugin_SetMenu");
   m_pSetPopupMenu = (LPFNSETPOPUPMENU) m_Lib.GetProcAddress("Plugin_SetPopupMenu");

   // Get the plugin name and type
   pGetName(m_sName.GetBufferSetLength(128), 128);
   m_sName.ReleaseBuffer();
   m_lType = pGetType();

   // An active plugin never allow the DLL code to initialize
   if( !m_bActive ) return TRUE;

   // Call initialize to begin startup
   BOOL bRes = FALSE;
   ATLTRY( bRes = pInitialize(pDevEnv) );
   m_bLoaded = bRes;

   return bRes;
}

CString CPlugin::GetFilename() const
{
   return m_sFilename;
}

CString CPlugin::GetName() const
{
   return m_sName;
}

LONG CPlugin::GetType() const
{
   return m_lType;
}

BOOL CPlugin::IsLoaded() const
{
   return m_bLoaded;
}

BOOL CPlugin::IsMarkedActive() const
{
   return m_bActive;
}

void CPlugin::ChangeActiveState(BOOL bActive)
{
   // NOTE: We won't load/unload a plugin because of this; we'll just wait
   //       for the next reboot.
   m_bActive = bActive;
}

CString CPlugin::GetDescription() const
{
   typedef VOID (CALLBACK* LPFNGETDESCRIPTION)(LPTSTR,UINT);
   LPFNGETDESCRIPTION pGetDescription = (LPFNGETDESCRIPTION) m_Lib.GetProcAddress("Plugin_GetDescription");
   CString sDescription;
   if( pGetDescription == NULL ) return sDescription;
   pGetDescription(sDescription.GetBufferSetLength(300), 300);
   sDescription.ReleaseBuffer();
   return sDescription;
}

IProject* CPlugin::CreateProject()
{
   typedef IProject* (CALLBACK* LPFNCREATEPROJECT)();
   LPFNCREATEPROJECT pCreateProject = (LPFNCREATEPROJECT) m_Lib.GetProcAddress("Plugin_CreateProject");
   ATLASSERT(pCreateProject);
   if( pCreateProject == NULL ) return NULL;
   return pCreateProject();
}

BOOL CPlugin::DestroyProject(IProject* pProject)
{
   typedef BOOL (CALLBACK* LPFNDESTROYPROJECT)(IProject*);
   LPFNDESTROYPROJECT pDestroyProject = (LPFNDESTROYPROJECT) m_Lib.GetProcAddress("Plugin_DestroyProject");
   ATLASSERT(pDestroyProject);
   if( pDestroyProject == NULL ) return FALSE;
   return pDestroyProject(pProject);
}

UINT CPlugin::QueryAcceptFile(LPCTSTR pstrFilename) const
{
   if( !m_bLoaded ) return 0;
   typedef UINT (CALLBACK* LPFNACCEPTFILE)(LPCTSTR);
   LPFNACCEPTFILE pAcceptFile = (LPFNACCEPTFILE) m_Lib.GetProcAddress("Plugin_QueryAcceptFile");
   ATLASSERT(pAcceptFile);
   if( pAcceptFile == NULL ) return FALSE;
   return pAcceptFile(pstrFilename);
}

IView* CPlugin::CreateView(LPCTSTR pstrFilename, IProject* pProject, IElement* pParent) const
{
   typedef IView* (CALLBACK* LPFNCREATEVIEW)(LPCTSTR, IProject*, IElement*);
   LPFNCREATEVIEW pCreateView = (LPFNCREATEVIEW) m_Lib.GetProcAddress("Plugin_CreateView");
   ATLASSERT(pCreateView);
   if( pCreateView == NULL ) return FALSE;
   return pCreateView(pstrFilename, pProject, pParent);
}

void CPlugin::SetMenu(HMENU hMenu)
{
   if( !m_bLoaded ) return;
   if( m_pSetMenu == NULL ) return;
   ATLASSERT(::IsMenu(hMenu));
   m_pSetMenu(hMenu);
}

void CPlugin::SetPopupMenu(IElement* pElement, HMENU hMenu)
{
   if( !m_bLoaded ) return;
   if( m_pSetPopupMenu == NULL ) return;
   ATLASSERT(::IsMenu(hMenu));
   m_pSetPopupMenu(pElement, hMenu);
}
