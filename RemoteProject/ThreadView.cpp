
#include "StdAfx.h"
#include "resource.h"

#include "ThreadView.h"
#include "Project.h"


/////////////////////////////////////////////////////////////////////////
// Constructor/destructor

CThreadView::CThreadView() :
   m_pProject(NULL)
{
}


/////////////////////////////////////////////////////////////////////////
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
      DeleteAllItems();
      CString sValue = info.GetItem(_T("thread-id"));
      while( !sValue.IsEmpty() ) {
         CString sText;
         sText.Format(IDS_THREAD, sValue, sValue);
         int iItem = InsertItem(GetItemCount(), sText);
         DWORD dwThreadId = (DWORD) _ttol(sText);
         SetItemData(iItem, dwThreadId);
         if( m_dwCurThread == dwThreadId ) {
            SelectItem(iItem);
            m_dwCurThread = dwThreadId;
         }
         sValue = info.FindNext(_T("thread-id"));
      }
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


/////////////////////////////////////////////////////////////////////////
// Implementation

void CThreadView::_SelectThread(int iThreadId)
{
   for( int i = 0; i < GetItemCount(); i++ ) {
      if( (int) GetItemData(i) == iThreadId ) {
         if( GetSelectedIndex() != i ) SelectItem(i);
         m_dwCurThread = iThreadId;
         break;
      }
   }
}

/////////////////////////////////////////////////////////////////////////
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
      LPARAM dwThreadId = GetItemData(iItem);
      CString sCommand;
      sCommand.Format(_T("-thread-select %ld"), dwThreadId);
      m_pProject->DelayedDebugCommand(sCommand);
      m_pProject->DelayedDebugEvent();
   }
   return lRes;
}
