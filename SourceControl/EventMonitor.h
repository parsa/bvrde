#if !defined(AFX_EVENTMONITOR_H__20031225_EA56_CA66_2ED3_0080AD509054__INCLUDED_)
#define AFX_EVENTMONITOR_H__20031225_EA56_CA66_2ED3_0080AD509054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RepositoryView.h"

#include "Commands.h"

extern CScCommands _Commands;


class CEventMonitor : public IAppMessageListener, public IIdleListener
{
public:
   CRepositoryView m_viewRepository;

   LRESULT OnAppMessage(HWND /*hWnd*/, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( uMsg == WM_COMMAND ) {
         switch( LOWORD(wParam) ) {
         case ID_VIEW_REPOSITORY:
            {
               _DisplayRepositoryView();
            }
            break;
         case ID_VIEW_CVSDIFF:
            {
               _Commands.ShowDiffView();
            }
            break;
         case ID_SC_CHECKIN:
            {
               CSimpleArray<CString> aFiles;
               if( !_Commands.CollectFiles(aFiles) ) return 0;
               if( !_Commands.CheckIn(aFiles) ) return 0;
            }
            break;
         case ID_SC_CHECKOUT:
            {
               CSimpleArray<CString> aFiles;
               if( !_Commands.CollectFiles(aFiles) ) return 0;
               if( !_Commands.CheckOut(aFiles) ) return 0;
            }
            break;
         case ID_SC_UPDATE:
            {
               CSimpleArray<CString> aFiles;
               if( !_Commands.CollectFiles(aFiles) ) return 0;
               if( !_Commands.Update(aFiles) ) return 0;
            }
            break;
         case ID_SC_ADDFILE:
            {
               CSimpleArray<CString> aFiles;
               if( !_Commands.CollectFiles(aFiles) ) return 0;
               if( !_Commands.AddFile(aFiles) ) return 0;
            }
            break;
         case ID_SC_REMOVEFILE:
            {
               CSimpleArray<CString> aFiles;
               if( !_Commands.CollectFiles(aFiles) ) return 0;
               if( !_Commands.RemoveFile(aFiles) ) return 0;
            }
            break;
         case ID_SC_STATUS:
            {
               CSimpleArray<CString> aFiles;
               if( !_Commands.CollectFiles(aFiles) ) return 0;
               if( !_Commands.StatusFile(aFiles) ) return 0;
            }
            break;
         case ID_SC_DIFFVIEW:
            {
               CSimpleArray<CString> aFiles;
               if( !_Commands.CollectFiles(aFiles) ) return 0;
               if( !_Commands.DiffFile(aFiles) ) return 0;
            }
            break;
         case ID_SC_LOGIN:
            {
               CSimpleArray<CString> aFiles;
               if( !_Commands.CollectFiles(aFiles) ) return 0;
               if( !_Commands.LogIn(aFiles) ) return 0;
            }
            break;
         case ID_SC_LOGOUT:
            {
               CSimpleArray<CString> aFiles;
               if( !_Commands.CollectFiles(aFiles) ) return 0;
               if( !_Commands.LogOut(aFiles) ) return 0;
            }
            break;
         }
      }
      bHandled = FALSE;
      return 0;
   }

   BOOL PreTranslateMessage(MSG* pMsg)
   {
      return FALSE;
   }

   void OnIdle(IUpdateUI* pUIBase)
   {
      pUIBase->UIEnable(ID_SC_UPDATE, !_Commands.sCmdUpdate.IsEmpty());
      pUIBase->UIEnable(ID_SC_CHECKIN, !_Commands.sCmdCheckIn.IsEmpty());
      pUIBase->UIEnable(ID_SC_CHECKOUT, !_Commands.sCmdCheckOut.IsEmpty());
      pUIBase->UIEnable(ID_SC_LOGIN, !_Commands.sCmdLogIn.IsEmpty());
      pUIBase->UIEnable(ID_SC_LOGOUT, !_Commands.sCmdLogOut.IsEmpty());
      pUIBase->UIEnable(ID_SC_DIFFVIEW, !_Commands.sCmdDiff.IsEmpty() && !_Commands.m_bIsFolder);
      pUIBase->UIEnable(ID_SC_STATUS, !_Commands.sCmdStatus.IsEmpty());
      pUIBase->UIEnable(ID_SC_ADDFILE, !_Commands.sCmdAddFile.IsEmpty());
      pUIBase->UIEnable(ID_SC_REMOVEFILE, !_Commands.sCmdRemoveFile.IsEmpty());
      pUIBase->UIEnable(ID_VIEW_REPOSITORY, TRUE);
      pUIBase->UISetCheck(ID_VIEW_REPOSITORY, m_viewRepository.IsWindow());
   }

   void OnGetMenuText(UINT wID, LPTSTR pstrText, int cchMax)
   {
      AtlLoadString(wID, pstrText, cchMax);
   }

   // Implementation

   void _DisplayRepositoryView()
   {
      // Create/remove repository view
      if( !m_viewRepository.IsWindow() ) {
         CWindow wndMain = _pDevEnv->GetHwnd(IDE_HWND_MAIN);
         TCHAR szTitle[128] = { 0 };
         AtlLoadString(IDS_TITLE, szTitle, 127);
         DWORD dwStyle = WS_CHILD | WS_VISIBLE;
         m_viewRepository.Create(wndMain, CWindow::rcDefault, szTitle, dwStyle);
         _pDevEnv->AddAutoHideView(m_viewRepository, IDE_DOCK_LEFT, 2);
         _pDevEnv->ActivateAutoHideView(m_viewRepository);
      }
      else {
         _pDevEnv->RemoveAutoHideView(m_viewRepository);
         m_viewRepository.PostMessage(WM_CLOSE);
      }
   }
};


#endif // !defined(AFX_EVENTMONITOR_H__20031225_EA56_CA66_2ED3_0080AD509054__INCLUDED_)
