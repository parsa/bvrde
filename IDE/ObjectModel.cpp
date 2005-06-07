
#include "StdAfx.h"
#include "resource.h"

#include "MainFrm.h"
#include "Solution.h"


/////////////////////////////////////////////////////////////////////////////
// CGlobalsOM

void CGlobalsOM::Init(CApplicationOM* pApp)
{
   m_pOwner = pApp;
}

IDispatch* CGlobalsOM::get_App()
{
   return m_pOwner;
}

IDispatch* CGlobalsOM::get_Solution()
{
   return m_pOwner->get_Solution();
}

IDispatch* CGlobalsOM::get_ActiveWindow()
{
   return m_pOwner->get_ActiveWindow();
}

IDispatch* CGlobalsOM::get_ActiveProject()
{
   return m_pOwner->get_ActiveProject();
}

IDispatch* CGlobalsOM::CreateObject(BSTR bstrProgId)
{
   CComPtr<IDispatch> spDisp;
   if( FAILED(spDisp.CoCreateInstance(bstrProgId)) ) return NULL;
   return spDisp.Detach();
}

VOID CGlobalsOM::Sleep(LONG lPeriod)
{
   if( lPeriod < 0 ) lPeriod = 0;
   ::Sleep(lPeriod);
}


/////////////////////////////////////////////////////////////////////////////
// CApplicationOM

CApplicationOM::CApplicationOM(CMainFrame* pFrame) :
   m_pOwner(pFrame)
{
}

BSTR CApplicationOM::get_Version()
{
   DWORD dwVersion = m_pOwner->GetVersion();
   WCHAR wszVersion[32];
   ::wsprintfW(wszVersion, _T("%ld.%ld"), (long)(LOWORD(dwVersion)), (long)(HIWORD(dwVersion)));
   BSTR bstrVersion = ::SysAllocString(wszVersion);
   return bstrVersion;
}

IDispatch* CApplicationOM::get_Solution()
{
   if( g_pSolution == NULL ) return NULL;
   return g_pSolution->GetDispatch();
}

IDispatch* CApplicationOM::get_ActiveWindow()
{
   HWND hWnd = m_pOwner->MDIGetActive();
   if( hWnd == NULL ) return NULL;
   CWinProp prop = hWnd;
   IView* pView = NULL;
   prop.GetProperty(_T("View"), pView);
   if( pView == NULL ) return NULL;
   IDispatch* pDisp = pView->GetDispatch();
   if( pDisp == NULL ) return new CFileOM(pView);
   return pDisp;
}

IDispatch* CApplicationOM::get_ActiveProject()
{
   IProject* pProject = g_pSolution->GetActiveProject();
   if( pProject == NULL ) return NULL;
   IDispatch* pDisp = pProject->GetDispatch();
   if( pDisp == NULL ) return new CProjectOM(pProject);
   return pDisp;
}

IDispatch* CApplicationOM::get_RecentProjects()
{
   return new CRecentProjectsOM(m_pOwner);
}

VOID __stdcall CApplicationOM::SendRawMessage(long uMsg, long wParam, long lParam)
{
   m_pOwner->SendMessage((UINT) uMsg, (WPARAM) wParam, (LPARAM) lParam);
}

VOID CApplicationOM::Quit()
{
   m_pOwner->PostMessage(WM_CLOSE);
}


/////////////////////////////////////////////////////////////////////////////
// CRecentProjectsOM

CRecentProjectsOM::CRecentProjectsOM(CMainFrame* pFrame) :
   m_pOwner(pFrame)
{
}

LONG CRecentProjectsOM::get_Count()
{
   return m_pOwner->m_mru.m_arrDocs.GetSize();
}

BSTR CRecentProjectsOM::Item(LONG iIndex)
{
   if( iIndex < 1 || iIndex > m_pOwner->m_mru.m_arrDocs.GetSize() ) return NULL;
   return ::SysAllocString(m_pOwner->m_mru.m_arrDocs[get_Count() - iIndex].szDocName);
}


/////////////////////////////////////////////////////////////////////////////
// CSolutionOM

CSolutionOM::CSolutionOM(ISolution* pSolution) :
   m_pOwner(pSolution)
{
}

BSTR CSolutionOM::get_Name()
{
   TCHAR szName[128] = { 0 };
   m_pOwner->GetName(szName, 127);
   BSTR bstrName = ::SysAllocString(szName);
   return bstrName;
}

BSTR CSolutionOM::get_Type()
{
   TCHAR szType[64] = { 0 };
   m_pOwner->GetType(szType, 63);
   BSTR bstrType = ::SysAllocString(szType);
   return bstrType;
}

BSTR CSolutionOM::get_Filename()
{
   TCHAR szFilename[MAX_PATH] = { 0 };
   m_pOwner->GetFileName(szFilename, MAX_PATH);
   BSTR bstrType = ::SysAllocString(szFilename);
   return bstrType;
}

IDispatch* CSolutionOM::get_Projects()
{
   return new CProjectsOM(m_pOwner);
}

VARIANT_BOOL CSolutionOM::Open(BSTR Filename)
{
   USES_CONVERSION;
   return m_pOwner->LoadSolution(OLE2CT(Filename)) ? VARIANT_TRUE : VARIANT_FALSE;
}

VOID CSolutionOM::Close()
{
   m_pOwner->Close();
}

VARIANT_BOOL CSolutionOM::Save(BSTR Filename)
{
   CString sFilename = Filename;
   return m_pOwner->SaveSolution(sFilename) ? VARIANT_TRUE : VARIANT_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CProjectsOM

CProjectsOM::CProjectsOM(ISolution* pSolution) :
   m_pOwner(pSolution)
{
}

LONG CProjectsOM::get_Count()
{
   return m_pOwner->GetItemCount();
}

IDispatch* CProjectsOM::Item(LONG iIndex)
{
   IProject* pProject = m_pOwner->GetItem(iIndex - 1);
   if( pProject == NULL ) return NULL;
   IDispatch* pDisp = pProject->GetDispatch();
   if( pDisp == NULL ) return new CProjectOM(pProject);
   return pDisp;
}


/////////////////////////////////////////////////////////////////////////////
// CProjectOM

CProjectOM::CProjectOM(IProject* pProject) :
   m_pOwner(pProject)
{
}

BSTR CProjectOM::get_Name()
{
   TCHAR szName[128] = { 0 };
   m_pOwner->GetName(szName, 127);
   BSTR bstrName = ::SysAllocString(szName);
   return bstrName;
}

BSTR CProjectOM::get_Type()
{
   TCHAR szType[64] = { 0 };
   m_pOwner->GetType(szType, 63);
   BSTR bstrType = ::SysAllocString(szType);
   return bstrType;
}

BSTR CProjectOM::get_Class()
{
   TCHAR szClass[64] = { 0 };
   m_pOwner->GetClass(szClass, 63);
   BSTR bstrClass = ::SysAllocString(szClass);
   return bstrClass;
}

IDispatch* CProjectOM::get_Files()
{
   return new CFilesOM(m_pOwner);
}


/////////////////////////////////////////////////////////////////////////////
// CFilesOM

CFilesOM::CFilesOM(IProject* pProject) :
   m_pOwner(pProject)
{
}

LONG CFilesOM::get_Count()
{
   return m_pOwner->GetItemCount();
}

IDispatch* CFilesOM::Item(LONG iIndex)
{
   IView* pView = m_pOwner->GetItem(iIndex - 1);
   if( pView == NULL ) return NULL;
   IDispatch* pDisp = pView->GetDispatch();
   if( pDisp == NULL ) return new CFileOM(pView);
   return pDisp;
}


/////////////////////////////////////////////////////////////////////////////
// CFileOM

CFileOM::CFileOM(IView* pView) :
   m_pOwner(pView)
{
}

BSTR CFileOM::get_Name()
{
   TCHAR szName[128] = { 0 };
   m_pOwner->GetName(szName, 127);
   BSTR bstrName = ::SysAllocString(szName);
   return bstrName;
}

BSTR CFileOM::get_Type()
{
   TCHAR szType[64] = { 0 };
   m_pOwner->GetType(szType, 63);
   BSTR bstrType = ::SysAllocString(szType);
   return bstrType;
}

BSTR CFileOM::get_Filename()
{
   TCHAR szFilename[MAX_PATH] = { 0 };
   m_pOwner->GetFileName(szFilename, MAX_PATH);
   BSTR bstrType = ::SysAllocString(szFilename);
   return bstrType;
}

BSTR CFileOM::get_Text()
{
   CComBSTR bstrText;
   m_pOwner->GetText(&bstrText);
   return bstrText.Detach();
}

VARIANT_BOOL CFileOM::Open()
{
   return m_pOwner->OpenView(0) ? VARIANT_TRUE : VARIANT_FALSE;
}

VOID CFileOM::Close()
{
   m_pOwner->CloseView();
}

VARIANT_BOOL CFileOM::Save()
{
   return m_pOwner->Save() ? VARIANT_TRUE : VARIANT_FALSE;
}

