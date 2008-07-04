
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
}

bool CThreadView::WantsData() 
{
   if( !IsWindow() ) return false;
   if( !IsWindowVisible() ) return false;
   return true;
}

void CThreadView::SetInfo(LPCTSTR pstrType, CMiInfo& info)
{
   if( _tcscmp(pstrType, _T("thread-ids")) == 0 ) 
   {
      SetRedraw(FALSE);
      DeleteAllItems();
      CString sValue = info.GetItem(_T("thread-id"));
      while( !sValue.IsEmpty() ) {
         CString sText;
         sText.Format(IDS_THREAD, sValue, sValue);
         int iItem = InsertItem(GetItemCount(), sText);
         DWORD dwThreadId = (DWORD) _ttol(sText);
         SetItemData(iItem, dwThreadId);
         if( m_dwCurThread == dwThreadId ) SelectItem(iItem);
         sValue = info.FindNext(_T("thread-id"));
      }
      if( GetSelectedIndex() < 0 ) SelectItem(0);
      SetRedraw(TRUE);
      Invalidate();
   }
   else if( _tcscmp(pstrType, _T("stopped")) == 0 ) 
   {
      CString sValue = info.GetItem(_T("thread-id"));
      if( !sValue.IsEmpty() ) _SelectThread(_ttol(sValue));
   }
   else if( _tcscmp(pstrType, _T("new-thread-id")) == 0 ) 
   {
      CString sValue = info.GetItem(_T("new-thread-id"));
      if( !sValue.IsEmpty() ) _SelectThread(_ttol(sValue));
   }
}

void CThreadView::EvaluateView(CSimpleArray<CString>& aDbgCmd)
{
   aDbgCmd.Add(CString(_T("-thread-list-ids")));
}

// Implementation

void CThreadView::_SelectThread(long lThreadId)
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

