
#include "StdAfx.h"
#include "resource.h"

#include "MemoryView.h"
#include "Project.h"


/////////////////////////////////////////////////////////////////////////
// Constructor/destructor

CMemoryView::CMemoryView() :
   m_pProject(NULL)
{
}


/////////////////////////////////////////////////////////////////////////
// Operations

#pragma code_seg( "VIEW" )

void CMemoryView::Init(CRemoteProject* pProject)
{
   m_pProject = pProject;
}

bool CMemoryView::WantsData() 
{
   if( !IsWindow() ) return false;
   if( !IsWindowVisible() ) return false;
   return true;
}

void CMemoryView::SetInfo(LPCTSTR pstrType, CMiInfo& info)
{
   if( _tcscmp(pstrType, _T("addr")) == 0 ) 
   {
      CString sAddress;
      CString sMemory;
      CString sAscii;
      CString sText;
      int iStep = 0;
      CString sValue = info.GetItem(_T("addr"), _T("memory"));
      while( !sValue.IsEmpty() ) {
         if( sAddress.IsEmpty() ) sAddress = sValue;
         sMemory += info.FindNext(_T("data")) + _T(" ");
         sAscii += info.FindNext(_T("ascii"));
         if( (++iStep % 4) == 0 ) {
            sText += sAddress;
            sText += _T("  ");
            sText += sMemory;
            sText += _T("   ");
            sText += sAscii;
            sText += _T("\r\n");
            sAddress = sMemory = sAscii = _T("");
         }
         sValue = info.FindNext(_T("addr"), _T("memory"));
      }
      m_ctrlMemory.SetWindowText(sText);
   }
}


/////////////////////////////////////////////////////////////////////////
// Message handlers

LRESULT CMemoryView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
   m_ctrlAddress.Create(this, 1, m_hWnd, &rcDefault, NULL, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
   m_ctrlAddress.SetFont(AtlGetDefaultGuiFont());
   m_ctrlMemory.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY, WS_EX_CLIENTEDGE);
   m_ctrlMemory.SetFont(AtlGetStockFont(ANSI_FIXED_FONT));
   m_ctrlAddress.SetFocus();
   return 0;
}

LRESULT CMemoryView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
   CClientRect rcClient = m_hWnd;
   RECT rcAddress = { 0, 0, rcClient.right - rcClient.left, ::GetSystemMetrics(SM_CYCAPTION) };
   RECT rcMemory = { 0, rcAddress.bottom, rcAddress.right, rcClient.bottom - rcClient.top };
   m_ctrlAddress.MoveWindow(&rcAddress);
   m_ctrlMemory.MoveWindow(&rcMemory);
   return 0;
}

LRESULT CMemoryView::OnEditChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == '\n' ) return 0;
   bHandled = FALSE;
   return 0;
}

LRESULT CMemoryView::OnEditKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
   if( wParam == VK_RETURN ) {
      CString sAddress = CWindowText(m_ctrlAddress);
      CString sFormat = _T("x");
      int iPos = sAddress.Find(',');
      if( iPos > 0 ) {
         sFormat = sAddress.Mid(iPos + 1);
         sAddress = sAddress.Left(iPos);
         sFormat.TrimLeft();
         sAddress.TrimRight();
      }
      if( sAddress.IsEmpty() ) return 0;
      CString sCommand;
      sCommand.Format(_T("-data-read-memory \"%s\" %s 4 64 1 ."), 
         sAddress, 
         sFormat);
      m_pProject->DelayedDebugCommand(sCommand);
      return 0;
   }
   bHandled = FALSE;
   return 0;
}

