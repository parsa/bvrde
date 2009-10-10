
#include "StdAfx.h"
#include "resource.h"

#include "ThreadView.h"
#include "Project.h"


/////////////////////////////////////////////////////////////////////////
// CThreadView

CThreadView::CThreadView() :
   m_pProject(NULL)
{
}

CThreadView::~CThreadView()
{
   if( IsWindow() ) /* scary */
      DestroyWindow();
}

// Operations

#pragma code_seg( "VIEW" )

void CThreadView::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
   m_dwCurThread = 1;
   m_dblDbgVersion = 0.0;
}

bool CThreadView::WantsData() 
{
   if( !IsWindow() ) return false;
   if( !IsWindowVisible() ) return false;
   return true;
}

void CThreadView::SetInfo(LPCTSTR pstrType, CMiInfo& info)
{
   if( _tcscmp(pstrType, _T("bvrde_init")) == 0 ) 
   {
      LPTSTR pstrTemp = NULL;
      m_dblDbgVersion = _tcstod(m_pProject->m_DebugManager.GetParam(_T("DebuggerVersion")), &pstrTemp);
   }
   else if( _tcscmp(pstrType, _T("thread-ids")) == 0 ) 
   {
      SetRedraw(FALSE);
      DeleteAllItems();
      CString sThreadId = info.GetItem(_T("thread-id"));
      CString sInfo;
      while( !sThreadId.IsEmpty() ) {
         DWORD dwThreadId = (DWORD) _ttol(sThreadId);
         sInfo.Format(IDS_THREAD, sThreadId, sThreadId);
         int iItem = InsertItem(GetItemCount(), sInfo);
         SetItemData(iItem, (LPARAM) dwThreadId);
         if( m_dwCurThread == dwThreadId ) SelectItem(iItem);
         sThreadId = info.FindNext(_T("thread-id"));
      }
      if( GetSelectedIndex() < 0 ) SelectItem(0);
      SetRedraw(TRUE);
      Invalidate();
   }
   else if( _tcscmp(pstrType, _T("threads")) == 0 )
   {
      SetRedraw(FALSE);
      DeleteAllItems();
      CString sCurrentId = info.GetItem(_T("current-thread-id"));
      if( !sCurrentId.IsEmpty() ) m_dwCurThread = (DWORD) _ttol(sCurrentId);
      CString sThreadId = info.GetItem(_T("id"), _T("threads"));
      CString sInfo, sTarget, sText;
      while( !sThreadId.IsEmpty() ) {
         DWORD dwThreadId = (DWORD) _ttol(sThreadId);
         sTarget = info.GetSubItem(_T("target-id"));
         sText = info.GetSubItem(_T("info"));
         sInfo.Format(_T("%ld - %s - %s"), dwThreadId, sTarget.IsEmpty() ? sThreadId : sTarget, sText);
         sInfo.TrimRight(_T(" -"));
         int iItem = InsertItem(GetItemCount(), sInfo);
         SetItemData(iItem, (LPARAM) dwThreadId);
         if( m_dwCurThread == dwThreadId ) SelectItem(iItem);
         sThreadId = info.FindNext(_T("id"), _T("threads"));
      }
      if( GetSelectedIndex() < 0 ) SelectItem(0);
      SetRedraw(TRUE);
      Invalidate();
   }
   else if( _tcscmp(pstrType, _T("stopped")) == 0 ) 
   {
      CString sValue = info.GetItem(_T("thread-id"));
      if( !sValue.IsEmpty() ) _SelectThreadId(_ttol(sValue));
   }
   else if( _tcscmp(pstrType, _T("new-thread-id")) == 0 ) 
   {
      CString sValue = info.GetItem(_T("new-thread-id"));
      if( !sValue.IsEmpty() ) _SelectThreadId(_ttol(sValue));
   }
}

void CThreadView::EvaluateView(CSimpleArray<CString>& aDbgCmd)
{
   if( m_dblDbgVersion >= 6.9 - EPSILON ) {
      //aDbgCmd.Add(CString(_T("-thread-info")));
      aDbgCmd.Add(CString(_T("-thread-list-ids")));
   }
   else {
      aDbgCmd.Add(CString(_T("-thread-list-ids")));
   }
}

// Implementation

void CThreadView::_SelectThreadId(long lThreadId)
{
   for( int i = 0; i < GetItemCount(); i++ ) {
      if( (long) GetItemData(i) == lThreadId ) {
         if( GetSelectedIndex() != i ) SelectItem(i);
         break;
      }
   }
   m_dwCurThread = (DWORD) lThreadId;
}

// Message handlers

LRESULT CThreadView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   LRESULT lRes = DefWindowProc();
   SetFont(AtlGetDefaultGuiFont());
   return lRes;
}

LRESULT CThreadView::OnDblClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   ATLASSERT(m_pProject);
   LRESULT lRes = DefWindowProc();
   int iItem = GetSelectedIndex();
   if( iItem >= 0 ) {
      DWORD dwThreadId = (DWORD) GetItemData(iItem);
      CString sCommand;
      sCommand.Format(_T("-thread-select %ld"), dwThreadId);
      m_pProject->DelayedDebugCommand(sCommand);
      m_pProject->DelayedDebugEvent();
   }
   return lRes;
}

